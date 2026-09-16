#include "GStreamerPipeline.hpp"

#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include <gst/video/video.h>

GStreamerPipeline::~GStreamerPipeline() {
    reset();
}

void GStreamerPipeline::reset() {
    // NULL stops and joins streaming tasks before callback userdata or frame
    // state can be destroyed. No bus watch is registered on a shared context.
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
    if (sink_) {
        GstAppSinkCallbacks callbacks{};
        if (GST_IS_APP_SINK(sink_)) {
            gst_app_sink_set_callbacks(GST_APP_SINK(sink_), &callbacks, nullptr, nullptr);
        }
        gst_object_unref(std::exchange(sink_, nullptr));
    }
    if (bus_) {
        gst_object_unref(std::exchange(bus_, nullptr));
    }
    if (pipeline_) {
        gst_object_unref(std::exchange(pipeline_, nullptr));
    }
    if (error_) {
        g_error_free(std::exchange(error_, nullptr));
    }
    const std::lock_guard lock(frameMutex_);
    frame_.clear();
    isFrameReady_ = false;
    endOfStream_ = false;
    nextSequence_ = 0;
}

void GStreamerPipeline::initGstLibrary(int argc, char* argv[]) {
    gst_init(&argc, &argv);
}

void GStreamerPipeline::runPipeline(const std::string& link) {
    reset();
    const std::string pipelineCommand = getPipelineCommand(link);
    pipeline_ = gst_parse_launch(pipelineCommand.c_str(), &error_);
    checkError();
    if (!pipeline_) {
        throw std::runtime_error("GStreamer pipeline construction returned no pipeline");
    }
}

void GStreamerPipeline::checkError() {
    if (error_) {
        const std::string message = error_->message;
        g_error_free(error_);
        error_ = nullptr;
        throw std::runtime_error("Pipeline construction failed: " + message);
    }
}

std::string GStreamerPipeline::getPipelineCommand(const std::string& link) const {
    if (link.find('!') != std::string::npos) {
        return link;
    }
    if (link.find("rtsp") != std::string::npos) {
        return "rtspsrc location=" + link +
               " ! decodebin ! videoconvert ! video/x-raw,format=BGR"
               " ! appsink name=videocapture_sink";
    }
    return "filesrc location=" + link +
           " ! decodebin ! videoconvert ! video/x-raw,format=BGR"
           " ! appsink name=videocapture_sink";
}

void GStreamerPipeline::markEndOfStream() {
    {
        const std::lock_guard lock(frameMutex_);
        endOfStream_ = true;
    }
    frameAvailable_.notify_all();
}

void GStreamerPipeline::endOfStream(GstAppSink*, gpointer data) {
    static_cast<GStreamerPipeline*>(data)->markEndOfStream();
}

GstFlowReturn GStreamerPipeline::newPreroll(GstAppSink*, gpointer) {
    return GST_FLOW_OK;
}

GstFlowReturn GStreamerPipeline::newSample(GstAppSink* appsink, gpointer data) {
    auto& self = *static_cast<GStreamerPipeline*>(data);
    try {
        return self.receiveSample(appsink);
    } catch (...) {
        // Never unwind a C callback. Allocation failure ends only this stream.
        self.markEndOfStream();
        return GST_FLOW_ERROR;
    }
}

GstFlowReturn GStreamerPipeline::receiveSample(GstAppSink* appsink) {
    const std::unique_ptr<GstSample, decltype(&gst_sample_unref)> sample(
        gst_app_sink_pull_sample(appsink), gst_sample_unref);
    if (!sample) {
        return GST_FLOW_EOS;
    }

    GstCaps* caps = gst_sample_get_caps(sample.get());
    GstBuffer* buffer = gst_sample_get_buffer(sample.get());
    GstVideoInfo videoInfo;
    if (!caps || !buffer || !gst_video_info_from_caps(&videoInfo, caps) ||
        GST_VIDEO_INFO_FORMAT(&videoInfo) != GST_VIDEO_FORMAT_BGR) {
        return GST_FLOW_NOT_NEGOTIATED;
    }

    GstVideoFrame mappedFrame;
    if (!gst_video_frame_map(&mappedFrame, &videoInfo, buffer, GST_MAP_READ)) {
        return GST_FLOW_ERROR;
    }

    const auto unmap = [](GstVideoFrame* mapped) { gst_video_frame_unmap(mapped); };
    const std::unique_ptr<GstVideoFrame, decltype(unmap)> mapping(&mappedFrame, unmap);

    const int width = static_cast<int>(GST_VIDEO_INFO_WIDTH(&videoInfo));
    const int height = static_cast<int>(GST_VIDEO_INFO_HEIGHT(&videoInfo));
    const int sourceStride = GST_VIDEO_FRAME_PLANE_STRIDE(&mappedFrame, 0);
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    const auto* source =
        static_cast<const std::uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&mappedFrame, 0));

    GstFlowReturn result = GST_FLOW_OK;
    if (sourceStride < 0 || static_cast<std::size_t>(sourceStride) < rowBytes) {
        result = GST_FLOW_ERROR;
    } else {
        videocapture::Frame nextFrame;
        nextFrame.resize(width, height, videocapture::PixelFormat::BGR8);
        for (int row = 0; row < height; ++row) {
            std::memcpy(nextFrame.data() + static_cast<std::size_t>(row) * nextFrame.rowStride(),
                        source + static_cast<std::size_t>(row) * sourceStride, rowBytes);
        }
        const GstClockTime presentationTimestamp = GST_BUFFER_PTS(buffer);
        if (GST_CLOCK_TIME_IS_VALID(presentationTimestamp) &&
            std::in_range<std::int64_t>(presentationTimestamp)) {
            nextFrame.setTimestamp(
                std::chrono::nanoseconds(static_cast<std::int64_t>(presentationTimestamp)));
        }

        {
            const std::lock_guard lock(frameMutex_);
            nextFrame.setSequence(nextSequence_++);
            frame_ = std::move(nextFrame);
            isFrameReady_ = true;
        }
        frameAvailable_.notify_one();
    }

    return result;
}

void GStreamerPipeline::pollBus() {
    // Pop only our bus; unlike a default-context watch, this never dispatches
    // callbacks belonging to another capture or to the embedding application.
    while (GstMessage* message = gst_bus_pop_filtered(
               bus_, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS))) {
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            GError* error = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &error, &debug);
            g_printerr("GStreamer error: %s\n", error->message);
            g_error_free(error);
            g_free(debug);
        }
        markEndOfStream();
        gst_message_unref(message);
    }
}

void GStreamerPipeline::getSink() {
    sink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "videocapture_sink");
    if (!sink_) {
        GstIterator* iterator = gst_bin_iterate_sinks(GST_BIN(pipeline_));
        GValue item = G_VALUE_INIT;
        while (gst_iterator_next(iterator, &item) == GST_ITERATOR_OK) {
            auto* candidate = GST_ELEMENT(g_value_get_object(&item));
            if (GST_IS_APP_SINK(candidate)) {
                sink_ = GST_ELEMENT(gst_object_ref(candidate));
                g_value_reset(&item);
                break;
            }
            g_value_reset(&item);
        }
        if (G_VALUE_TYPE(&item) != 0) {
            g_value_unset(&item);
        }
        gst_iterator_free(iterator);
    }
    if (!sink_ || !GST_IS_APP_SINK(sink_)) {
        throw std::runtime_error("GStreamer pipeline must contain an appsink");
    }

    GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", nullptr);
    gst_app_sink_set_caps(GST_APP_SINK(sink_), caps);
    gst_caps_unref(caps);
    gst_app_sink_set_emit_signals(GST_APP_SINK(sink_), true);
    gst_app_sink_set_drop(GST_APP_SINK(sink_), true);
    gst_app_sink_set_max_buffers(GST_APP_SINK(sink_), 1);
    GstAppSinkCallbacks callbacks{
        .eos = endOfStream, .new_preroll = newPreroll, .new_sample = newSample};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink_), &callbacks, this, nullptr);
}

void GStreamerPipeline::setBus() {
    if (bus_) {
        gst_object_unref(std::exchange(bus_, nullptr));
    }
    bus_ = gst_element_get_bus(pipeline_);
    if (!bus_) {
        throw std::runtime_error("GStreamer pipeline has no bus");
    }
}

void GStreamerPipeline::setState(GstState state) {
    if (!pipeline_) {
        return;
    }
    if (state == GST_STATE_PLAYING) {
        const std::lock_guard lock(frameMutex_);
        endOfStream_ = false;
        nextSequence_ = 0;
        isFrameReady_ = false;
        frame_.clear();
    }
    if (gst_element_set_state(pipeline_, state) == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("Failed to change GStreamer pipeline state");
    }
}

bool GStreamerPipeline::isEndOfStream() const {
    const std::lock_guard lock(frameMutex_);
    return endOfStream_;
}

bool GStreamerPipeline::readFrame(videocapture::Frame& frame) {
    if (!bus_) {
        frame.clear();
        return false;
    }
    pollBus();
    constexpr auto busPollInterval = std::chrono::milliseconds(5);
    std::unique_lock lock(frameMutex_);
    while (!isFrameReady_ && !endOfStream_) {
        lock.unlock();
        pollBus();
        lock.lock();
        frameAvailable_.wait_for(lock, busPollInterval,
                                 [this] { return isFrameReady_ || endOfStream_; });
    }
    if (!isFrameReady_) {
        frame.clear();
        return false;
    }
    frame = std::exchange(frame_, {});
    isFrameReady_ = false;
    return !frame.empty();
}
