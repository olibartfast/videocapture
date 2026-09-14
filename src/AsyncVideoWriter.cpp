#include "AsyncVideoWriter.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "WriterSupport.hpp"

namespace {

constexpr const char* kWriterName = "Video writer";

}  // namespace

AsyncVideoWriter::AsyncVideoWriter(std::unique_ptr<VideoWriterInterface> encoder,
                                   std::size_t capacity)
    : encoder_(std::move(encoder)), capacity_(capacity > 0 ? capacity : 1) {
    if (!encoder_) {
        throw std::invalid_argument("AsyncVideoWriter requires an encoder");
    }
}

AsyncVideoWriter::~AsyncVideoWriter() {
    release();
}

bool AsyncVideoWriter::initialize(const std::string& destination,
                                  const videocapture::VideoWriterConfig& config) {
    release();

    // The encoder thread is not running, so the backend is still only touched
    // from this thread.
    if (!encoder_->initialize(destination, config)) {
        return false;
    }

    config_ = config;
    open_ = true;
    try {
        encoderThread_ = std::thread(&AsyncVideoWriter::encodeLoop, this);
    } catch (...) {
        open_ = false;
        config_ = {};
        encoder_->release();
        throw;
    }
    return true;
}

bool AsyncVideoWriter::writeFrame(const videocapture::Frame& frame) {
    if (!accepts(frame) || !reserveSlot()) {
        return false;
    }
    // Copy only once there is room, so a producer waiting on a full queue does
    // not hold an extra frame.
    videocapture::Frame copy;
    try {
        copy = frame;
    } catch (...) {
        cancelSlot();
        throw;
    }
    return commitSlot(std::move(copy));
}

bool AsyncVideoWriter::writeFrame(videocapture::Frame&& frame) {
    if (!accepts(frame) || !reserveSlot()) {
        return false;
    }
    return commitSlot(std::move(frame));
}

bool AsyncVideoWriter::isOpen() const {
    if (!open_) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(mutex_);
    return !failed_;
}

bool AsyncVideoWriter::release() {
    if (!open_) {
        return true;
    }

    {
        const std::lock_guard<std::mutex> lock(mutex_);
        closing_ = true;
    }
    frameQueued_.notify_one();
    encoderThread_.join();

    bool succeeded = false;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        succeeded = !failed_;
        queue_.clear();
        closing_ = false;
        failed_ = false;
    }

    // The thread is joined, so the writer is closed from here on whatever the
    // backend does; a later release() or the destructor must not join again.
    open_ = false;
    config_ = {};

    // Finalize even after a failure so that the backend releases its resources
    // and whatever was encoded is still closed properly.
    bool finalized = false;
    try {
        finalized = encoder_->release();
    } catch (const std::exception& error) {
        std::cerr << kWriterName << ": encoder failed while finalizing: " << error.what()
                  << std::endl;
    } catch (...) {
        std::cerr << kWriterName << ": encoder failed while finalizing" << std::endl;
    }
    return finalized && succeeded;
}

bool AsyncVideoWriter::accepts(const videocapture::Frame& frame) const {
    if (!open_) {
        std::cerr << kWriterName << ": writeFrame() called before initialize()" << std::endl;
        return false;
    }
    return videocapture::writer::validateFrame(frame, config_, kWriterName);
}

bool AsyncVideoWriter::reserveSlot() {
    std::unique_lock<std::mutex> lock(mutex_);
    spaceAvailable_.wait(lock,
                         [this] { return failed_ || queue_.size() + reservedSlots_ < capacity_; });
    if (failed_) {
        return false;
    }
    ++reservedSlots_;
    return true;
}

void AsyncVideoWriter::cancelSlot() {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        --reservedSlots_;
    }
    spaceAvailable_.notify_one();
}

bool AsyncVideoWriter::commitSlot(videocapture::Frame&& frame) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        --reservedSlots_;
        if (failed_) {
            return false;
        }
        try {
            queue_.push_back(std::move(frame));
        } catch (...) {
            // The slot is free again although nothing was queued, so a producer
            // waiting for room must re-check.
            lock.unlock();
            spaceAvailable_.notify_one();
            throw;
        }
    }
    frameQueued_.notify_one();
    return true;
}

void AsyncVideoWriter::encodeLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
        frameQueued_.wait(lock, [this] { return closing_ || !queue_.empty(); });
        if (queue_.empty()) {
            // Closing with nothing left to encode.
            return;
        }

        videocapture::Frame frame = std::move(queue_.front());
        queue_.pop_front();
        lock.unlock();
        spaceAvailable_.notify_one();

        bool written = false;
        try {
            written = encoder_->writeFrame(frame);
        } catch (const std::exception& error) {
            std::cerr << kWriterName << ": encoder failed while writing a frame: " << error.what()
                      << std::endl;
        } catch (...) {
            std::cerr << kWriterName << ": encoder failed while writing a frame" << std::endl;
        }

        lock.lock();
        if (!written) {
            // Frames behind a failed one cannot make the destination whole, so
            // they are discarded rather than encoded.
            failed_ = true;
            queue_.clear();
            lock.unlock();
            spaceAvailable_.notify_all();
            return;
        }
    }
}
