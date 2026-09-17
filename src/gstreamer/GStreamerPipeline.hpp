#pragma once

#include "Frame.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

// One caller drives each pipeline. Streaming callbacks use that instance's
// userdata and mutex; independent pipelines may be driven concurrently.
class GStreamerPipeline {
public:
    GStreamerPipeline() = default;
    ~GStreamerPipeline();
    GStreamerPipeline(const GStreamerPipeline&) = delete;
    GStreamerPipeline& operator=(const GStreamerPipeline&) = delete;

    void initGstLibrary(int argc, char* argv[]);
    void runPipeline(const std::string& link);
    void checkError();
    void getSink();
    void setBus();
    void setState(GstState state);
    void reset();
    bool readFrame(videocapture::Frame& frame);
    [[nodiscard]] bool isEndOfStream() const;

private:
    static void endOfStream(GstAppSink* appsink, gpointer data);
    static GstFlowReturn newPreroll(GstAppSink* appsink, gpointer data);
    static GstFlowReturn newSample(GstAppSink* appsink, gpointer data);
    GstFlowReturn receiveSample(GstAppSink* appsink);
    void pollBus();
    void markEndOfStream();
    [[nodiscard]] std::string getPipelineCommand(const std::string& link) const;

    GError* error_ = nullptr;
    GstElement* pipeline_ = nullptr;
    GstElement* sink_ = nullptr;
    GstBus* bus_ = nullptr;

    mutable std::mutex frameMutex_;
    std::condition_variable frameAvailable_;
    videocapture::Frame frame_;
    bool isFrameReady_ = false;
    bool endOfStream_ = false;
    std::uint64_t nextSequence_ = 0;
};
