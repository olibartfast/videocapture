#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

#include "Frame.hpp"
#include "VideoWriterConfig.hpp"
#include "VideoWriterInterface.hpp"

// Moves encoding off the caller's thread. Frames are validated on the caller's
// thread, then handed through a bounded queue to one encoder thread, which is
// the only thread calling the wrapped backend while a destination is open.
//
// Overload policy is to wait: when the queue is full, writeFrame() blocks until
// the encoder takes a frame, so no frame is dropped. A queue slot is reserved
// before a frame is copied, so the writer holds at most capacity frames in the
// queue plus the one being encoded, whichever overload is used. An encoder
// failure is sticky for the destination: queued frames are discarded, blocked
// producers wake, and every later writeFrame() and the closing release() return
// false.
//
// release() and the destructor join the encoder thread, so they wait for a
// backend call in progress however long it takes; there is no cancellation.
class AsyncVideoWriter final : public VideoWriterInterface {
public:
    // A longer queue cannot raise the encoder's sustained rate. It absorbs
    // timing jitter between producer and encoder, which can improve achieved
    // throughput, at the cost of memory and latency.
    static constexpr std::size_t kDefaultCapacity = 4;

    // Throws std::invalid_argument when encoder is null.
    explicit AsyncVideoWriter(std::unique_ptr<VideoWriterInterface> encoder,
                              std::size_t capacity = kDefaultCapacity);
    ~AsyncVideoWriter() override;

    AsyncVideoWriter(const AsyncVideoWriter&) = delete;
    AsyncVideoWriter& operator=(const AsyncVideoWriter&) = delete;

    bool initialize(const std::string& destination,
                    const videocapture::VideoWriterConfig& config) override;
    bool writeFrame(const videocapture::Frame& frame) override;
    bool writeFrame(videocapture::Frame&& frame) override;
    [[nodiscard]] bool isOpen() const override;
    bool release() override;

private:
    [[nodiscard]] bool accepts(const videocapture::Frame& frame) const;
    // Waits for room in the queue. Returns false, holding no slot, once the
    // destination has failed.
    bool reserveSlot();
    void cancelSlot();
    // Queues the frame into the reserved slot.
    bool commitSlot(videocapture::Frame&& frame);
    void encodeLoop(std::stop_token stop);

    std::unique_ptr<VideoWriterInterface> encoder_;
    const std::size_t capacity_;

    // Touched only by the thread driving the writer.
    videocapture::VideoWriterConfig config_{};
    bool open_ = false;

    // Shared with the encoder thread.
    mutable std::mutex mutex_;
    std::condition_variable_any frameQueued_;
    std::condition_variable spaceAvailable_;
    std::deque<videocapture::Frame> queue_;
    std::size_t reservedSlots_ = 0;
    bool failed_ = false;

    // Destroyed first, so its RAII join precedes destruction of shared state.
    std::jthread encoderThread_;
};
