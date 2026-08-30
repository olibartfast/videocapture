#include "GStreamerPipeline.hpp"

#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

#include <gst/video/video.h>

std::mutex GStreamerPipeline::frameMutex_;
std::condition_variable GStreamerPipeline::frameAvailable_;
videocapture::Frame GStreamerPipeline::frame_;
std::atomic_bool GStreamerPipeline::endOfStream_ = false;
std::atomic_uint64_t GStreamerPipeline::nextSequence_ = 0;
bool GStreamerPipeline::isFrameReady_ = false;

GStreamerPipeline::GStreamerPipeline() = default;

GStreamerPipeline::~GStreamerPipeline() {
    removeBusWatch();
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
    if (sink_) {
        gst_object_unref(sink_);
    }
    if (pipeline_) {
        gst_object_unref(pipeline_);
    }
    if (error_) {
        g_error_free(error_);
    }
}

void GStreamerPipeline::initGstLibrary(int argc, char* argv[]) {
    gst_init(&argc, &argv);
}

void GStreamerPipeline::runPipeline(const std::string& link) {
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

void GStreamerPipeline::endOfStream(GstAppSink*, gpointer) {
    // The appsink reports end of stream on the streaming thread, so consumers
    // learn about it without depending on anyone iterating the main context.
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        endOfStream_.store(true);
    }
    frameAvailable_.notify_all();
}

GstFlowReturn GStreamerPipeline::newPreroll(GstAppSink*, gpointer) {
    return GST_FLOW_OK;
}

GstFlowReturn GStreamerPipeline::newSample(GstAppSink* appsink, gpointer) {
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_EOS;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstVideoInfo videoInfo;
    if (!caps || !buffer || !gst_video_info_from_caps(&videoInfo, caps) ||
        GST_VIDEO_INFO_FORMAT(&videoInfo) != GST_VIDEO_FORMAT_BGR) {
        gst_sample_unref(sample);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    GstVideoFrame mappedFrame;
    if (!gst_video_frame_map(&mappedFrame, &videoInfo, buffer, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

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
        nextFrame.setSequence(nextSequence_.fetch_add(1));
        const GstClockTime presentationTimestamp = GST_BUFFER_PTS(buffer);
        if (GST_CLOCK_TIME_IS_VALID(presentationTimestamp) &&
            presentationTimestamp <=
                static_cast<GstClockTime>(std::numeric_limits<std::int64_t>::max())) {
            nextFrame.setTimestamp(
                std::chrono::nanoseconds(static_cast<std::int64_t>(presentationTimestamp)));
        }

        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            frame_ = std::move(nextFrame);
            isFrameReady_ = true;
        }
        frameAvailable_.notify_one();
    }

    gst_video_frame_unmap(&mappedFrame);
    gst_sample_unref(sample);
    return result;
}

gboolean GStreamerPipeline::myBusCallback(GstBus*, GstMessage* message, gpointer) {
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* error = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &error, &debug);
            g_printerr("GStreamer error: %s\n", error->message);
            g_error_free(error);
            g_free(debug);
            {
                std::lock_guard<std::mutex> lock(frameMutex_);
                endOfStream_.store(true);
            }
            frameAvailable_.notify_all();
            break;
        }
        case GST_MESSAGE_EOS: {
            {
                std::lock_guard<std::mutex> lock(frameMutex_);
                endOfStream_.store(true);
            }
            frameAvailable_.notify_all();
            break;
        }
        default:
            break;
    }
    return TRUE;
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
    GstAppSinkCallbacks callbacks = {endOfStream, newPreroll, newSample};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink_), &callbacks, nullptr, nullptr);
}

void GStreamerPipeline::setBus() {
    removeBusWatch();
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    busWatchId_ = gst_bus_add_watch(bus, myBusCallback, nullptr);
    gst_object_unref(bus);
}

void GStreamerPipeline::removeBusWatch() {
    if (busWatchId_ != 0) {
        g_source_remove(busWatchId_);
        busWatchId_ = 0;
    }
}

void GStreamerPipeline::setState(GstState state) {
    if (!pipeline_) {
        return;
    }
    if (state == GST_STATE_PLAYING) {
        std::lock_guard<std::mutex> lock(frameMutex_);
        endOfStream_.store(false);
        nextSequence_.store(0);
        isFrameReady_ = false;
        frame_.clear();
    }
    if (gst_element_set_state(pipeline_, state) == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("Failed to change GStreamer pipeline state");
    }
}

void GStreamerPipeline::setMainLoopEvent(bool event) {
    g_main_context_iteration(nullptr, event);
}

bool GStreamerPipeline::isEndOfStream() {
    return endOfStream_.load();
}

videocapture::Frame GStreamerPipeline::takeFrame() {
    videocapture::Frame result = std::move(frame_);
    frame_.clear();
    return result;
}
