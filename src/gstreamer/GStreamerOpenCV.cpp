#include "GStreamerOpenCV.hpp"
#include <opencv2/imgproc.hpp>

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
}

void GStreamerOpenCV::runPipeline(const std::string& link) {
    const std::string pipelineCmd = getPipelineCommand(link);
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
    // If the input contains '!' it's a complete GStreamer pipeline, use it as-is
    if (link.find('!') != std::string::npos) {
        return link;
    }
    // Otherwise, treat as a source location and build the pipeline
    else if (link.find("rtsp") != std::string::npos) {
        return "rtspsrc location=" + link + " ! decodebin ! videoconvert ! appsink name=autovideosink";
    }
    else {
        return "filesrc location=" + link + " ! decodebin ! videoconvert ! appsink name=autovideosink";
    }
}

GstFlowReturn GStreamerOpenCV::newPreroll(GstAppSink* appsink, gpointer data) {
    g_print("Got preroll!\n");
    return GST_FLOW_OK;
}

GstFlowReturn GStreamerOpenCV::newSample(GstAppSink* appsink, gpointer data) {
    static int framecount = 0;
    framecount++;

    GstSample* sample = gst_app_sink_pull_sample(appsink);
    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    const GstStructure* s = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);

    cv::Mat mYUV(height + height / 2, width, CV_8UC1, (char*)map.data);
    cv::Mat mBGR(height, width, CV_8UC3);
    cv::cvtColor(mYUV, mBGR, cv::COLOR_YUV2BGR_NV12);
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        mBGR.copyTo(GStreamerOpenCV::frame_);
        isFrameReady_ = true;
        frameAvailable_.notify_one();
    }

    gst_buffer_unmap(buffer, &map);

    if (framecount == 1) {
        g_print("Caps: %s\n", gst_caps_to_string(caps));
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

gboolean GStreamerOpenCV::myBusCallback(GstBus* bus, GstMessage* message, gpointer data) {
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
            end_of_stream_ = true;
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
    GstAppSinkCallbacks callbacks = { nullptr, newPreroll, newSample };
    gst_app_sink_set_callbacks(GST_APP_SINK(sink_), &callbacks, nullptr, nullptr);
}

void GStreamerOpenCV::setBus() {
    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    gst_bus_add_watch(bus_, myBusCallback, nullptr);
    gst_object_unref(bus_);
}

void GStreamerOpenCV::setState(GstState state) {
    if (!pipeline_) {
        // Pipeline doesn't exist, nothing to do
        return;
    }
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT(pipeline_), state);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("Failed to change pipeline state");
    }
}

void GStreamerOpenCV::setMainLoopEvent(bool event) {
    g_main_iteration(event);
}

bool GStreamerOpenCV::isEndOfStream() {
    return end_of_stream_;
}

cv::Mat GStreamerOpenCV::getFrame() const {
    return frame_;
}