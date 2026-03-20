#include "GStreamerOpenCV.hpp"

#include <gst/video/video.h>

#include <stdexcept>

std::mutex GStreamerOpenCV::frameMutex_;
std::condition_variable GStreamerOpenCV::frameAvailable_;
cv::Mat GStreamerOpenCV::frame_;
bool GStreamerOpenCV::end_of_stream_ = false;
bool GStreamerOpenCV::isFrameReady_ = false;

GStreamerOpenCV::GStreamerOpenCV() : error_(nullptr), pipeline_(nullptr), sink_(nullptr), bus_(nullptr) {}

GStreamerOpenCV::~GStreamerOpenCV() {
    if (pipeline_) {
        gst_element_set_state(GST_ELEMENT(pipeline_), GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline_));
    }
}

void GStreamerOpenCV::initGstLibrary(int argc, char* argv[]) {
    gst_init(&argc, &argv);
    setEndOfStream(false);
    isFrameReady_ = false;
}

void GStreamerOpenCV::runPipeline(const std::string& link) {
    const std::string pipelineCmd = getPipelineCommand(link);
    if (pipelineCmd.empty()) {
        throw std::runtime_error("Unable to translate the input source into a GStreamer pipeline");
    }

    gchar* descr = g_strdup(pipelineCmd.c_str());
    pipeline_ = gst_parse_launch(descr, &error_);
    g_free(descr);
    checkError();
}

void GStreamerOpenCV::checkError() {
    if (error_ != nullptr) {
        g_printerr("Could not construct pipeline: %s\n", error_->message);
        g_error_free(error_);
        error_ = nullptr;
        throw std::runtime_error("Pipeline construction failed");
    }
}

std::string GStreamerOpenCV::getPipelineCommand(const std::string& link) const {
    gchar* uri = nullptr;

    if (link.find("://") != std::string::npos) {
        uri = g_strdup(link.c_str());
    } else {
        uri = g_filename_to_uri(link.c_str(), nullptr, nullptr);
    }

    if (uri == nullptr) {
        return {};
    }

    std::string pipeline = "uridecodebin uri=\"";
    pipeline += uri;
    pipeline += "\" ! videoconvert ! video/x-raw,format=BGR ! appsink name=autovideosink";
    g_free(uri);
    return pipeline;
}

GstFlowReturn GStreamerOpenCV::newPreroll(GstAppSink* appsink, gpointer data) {
    (void)appsink;
    (void)data;
    return GST_FLOW_OK;
}

GstFlowReturn GStreamerOpenCV::newSample(GstAppSink* appsink, gpointer data) {
    (void)data;

    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (sample == nullptr) {
        return GST_FLOW_ERROR;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (caps == nullptr || buffer == nullptr) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstVideoInfo videoInfo;
    if (!gst_video_info_from_caps(&videoInfo, caps)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstVideoFrame videoFrame;
    if (!gst_video_frame_map(&videoFrame, &videoInfo, buffer, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const int width = GST_VIDEO_INFO_WIDTH(&videoInfo);
    const int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);
    cv::Mat bgrFrame(
        height,
        width,
        CV_8UC3,
        GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 0),
        GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 0));

    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        bgrFrame.copyTo(GStreamerOpenCV::frame_);
        isFrameReady_ = true;
        frameAvailable_.notify_one();
    }

    gst_video_frame_unmap(&videoFrame);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

gboolean GStreamerOpenCV::myBusCallback(GstBus* bus, GstMessage* message, gpointer data) {
    (void)bus;
    (void)data;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug;
            gst_message_parse_error(message, &err, &debug);
            g_printerr("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS:
            g_message("End of stream");
            setEndOfStream(true);
            break;
        default:
            break;
    }
    return TRUE;
}

void GStreamerOpenCV::getSink() {
    sink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "autovideosink");
    gst_app_sink_set_emit_signals(GST_APP_SINK(sink_), true);
    gst_app_sink_set_drop(GST_APP_SINK(sink_), true);
    gst_app_sink_set_max_buffers(GST_APP_SINK(sink_), 1);
    GstAppSinkCallbacks callbacks{};
    callbacks.new_preroll = newPreroll;
    callbacks.new_sample = newSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink_), &callbacks, nullptr, nullptr);
}

void GStreamerOpenCV::setBus() {
    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    gst_bus_add_watch(bus_, myBusCallback, nullptr);
    gst_object_unref(bus_);
}

void GStreamerOpenCV::setState(GstState state) {
    gst_element_set_state(GST_ELEMENT(pipeline_), state);
}

void GStreamerOpenCV::setMainLoopEvent(bool event) {
    g_main_iteration(event);
}

void GStreamerOpenCV::setEndOfStream(bool value) {
    end_of_stream_ = value;
    if (value) {
        frameAvailable_.notify_all();
    }
}

void GStreamerOpenCV::setFrame(const cv::Mat& frame) {
    frame_ = frame.clone();
}

bool GStreamerOpenCV::isEndOfStream() {
    return end_of_stream_;
}

cv::Mat GStreamerOpenCV::getFrame() const {
    return frame_;
}
