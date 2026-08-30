#pragma once

#include "VideoCaptureInterface.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

class GStreamerPipeline {
public:
    GStreamerPipeline();
    ~GStreamerPipeline();

    void initGstLibrary(int argc, char* argv[]);
    void runPipeline(const std::string& link);
    void checkError();
    void getSink();
    void setBus();
    void setState(GstState state);
    void setMainLoopEvent(bool event);
    videocapture::Frame takeFrame();

    static bool isEndOfStream();

    static std::mutex frameMutex_;
    static std::condition_variable frameAvailable_;
    static bool isFrameReady_;

private:
    static void endOfStream(GstAppSink* appsink, gpointer data);
    static GstFlowReturn newPreroll(GstAppSink* appsink, gpointer data);
    static GstFlowReturn newSample(GstAppSink* appsink, gpointer data);
    static gboolean myBusCallback(GstBus* bus, GstMessage* message, gpointer data);

    std::string getPipelineCommand(const std::string& link) const;
    void removeBusWatch();

    GError* error_ = nullptr;
    GstElement* pipeline_ = nullptr;
    GstElement* sink_ = nullptr;
    guint busWatchId_ = 0;
    static videocapture::Frame frame_;
    static std::atomic_bool endOfStream_;
    static std::atomic_uint64_t nextSequence_;
};
