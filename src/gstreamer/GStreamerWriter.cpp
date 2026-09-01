#include "GStreamerWriter.hpp"

#include <cstring>
#include <iostream>
#include <string>

#include <gst/video/video.h>

#include "WriterSupport.hpp"

namespace {

// Names used to find our own elements in a pipeline we built. A caller-supplied
// pipeline only has to provide the appsrc.
constexpr const char* kSourceName = "videocapture_src";
constexpr const char* kFileSinkName = "videocapture_sink";

// How long release() waits for the muxer to finish writing its header or index
// after end of stream before giving up on a clean file.
constexpr GstClockTime kEndOfStreamTimeout = 10 * GST_SECOND;

GstVideoFormat toGstVideoFormat(videocapture::PixelFormat format) {
    switch (format) {
        case videocapture::PixelFormat::Gray8:
            return GST_VIDEO_FORMAT_GRAY8;
        case videocapture::PixelFormat::RGB8:
            return GST_VIDEO_FORMAT_RGB;
        case videocapture::PixelFormat::BGR8:
            return GST_VIDEO_FORMAT_BGR;
        case videocapture::PixelFormat::RGBA8:
            return GST_VIDEO_FORMAT_RGBA;
        case videocapture::PixelFormat::BGRA8:
            return GST_VIDEO_FORMAT_BGRA;
        case videocapture::PixelFormat::NV12:
        case videocapture::PixelFormat::YUV420P:
            break;
    }
    return GST_VIDEO_FORMAT_UNKNOWN;
}

// Muxer for a container, chosen from the destination's extension. Returns an
// empty string when the extension is not one we can map.
std::string muxerForExtension(const std::string& extension) {
    if (extension == ".mp4" || extension == ".m4v" || extension == ".mov") {
        return "qtmux";
    }
    if (extension == ".mkv") {
        return "matroskamux";
    }
    if (extension == ".avi") {
        return "avimux";
    }
    return {};
}

}  // namespace

GStreamerWriter::~GStreamerWriter() {
    release();
}

bool GStreamerWriter::buildPipelineDescription(const std::string& destination,
                                               std::string& description) const {
    const std::string extension = videocapture::writer::destinationExtension(destination);
    const std::string muxer = muxerForExtension(extension);
    if (muxer.empty()) {
        std::cerr << "GStreamer writer: cannot choose a container for '" << destination
                  << "'; use a .mp4, .mov, .m4v, .mkv or .avi destination, or pass a complete "
                     "pipeline containing an appsrc"
                  << std::endl;
        return false;
    }

    // Codec intent becomes an encoder and, for the parsed formats, the parser
    // the muxers expect ahead of them. Auto keeps AVI on MJPEG, which every
    // installation can write, and uses H.264 elsewhere.
    std::string encoder;
    switch (config_.codec) {
        case videocapture::VideoCodec::H264:
            encoder = "x264enc";
            break;
        case videocapture::VideoCodec::HEVC:
            encoder = "x265enc";
            break;
        case videocapture::VideoCodec::MJPEG:
            encoder = "jpegenc";
            break;
        case videocapture::VideoCodec::Auto:
            encoder = extension == ".avi" ? "jpegenc" : "x264enc";
            break;
    }

    if (config_.bitRateBitsPerSecond > 0) {
        if (encoder == "jpegenc") {
            std::cerr << "GStreamer writer: jpegenc has no bit rate control; "
                         "using the encoder default"
                      << std::endl;
        } else {
            // x264enc and x265enc both take kbit/s.
            const std::int64_t kilobits = config_.bitRateBitsPerSecond / 1000;
            encoder += " bitrate=" + std::to_string(kilobits > 0 ? kilobits : 1);
        }
    }

    std::string parser;
    if (encoder.rfind("x264enc", 0) == 0) {
        parser = " ! h264parse";
    } else if (encoder.rfind("x265enc", 0) == 0) {
        parser = " ! h265parse";
    }

    // The file name is set on the element afterwards rather than embedded in
    // the description, so paths containing spaces or quotes need no escaping.
    description = std::string("appsrc name=") + kSourceName + " ! videoconvert ! " + encoder +
                  parser + " ! " + muxer + " ! filesink name=" + kFileSinkName;
    return true;
}

bool GStreamerWriter::bindElements(bool ownsFileSink, const std::string& destination) {
    source_ = gst_bin_get_by_name(GST_BIN(pipeline_), kSourceName);
    if (!source_) {
        // A caller-supplied pipeline may name its appsrc anything, so fall back
        // to the first one in the pipeline.
        GstIterator* iterator = gst_bin_iterate_sources(GST_BIN(pipeline_));
        GValue item = G_VALUE_INIT;
        while (gst_iterator_next(iterator, &item) == GST_ITERATOR_OK) {
            auto* candidate = GST_ELEMENT(g_value_get_object(&item));
            if (GST_IS_APP_SRC(candidate)) {
                source_ = GST_ELEMENT(gst_object_ref(candidate));
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
    if (!source_ || !GST_IS_APP_SRC(source_)) {
        std::cerr << "GStreamer writer: the pipeline must contain an appsrc" << std::endl;
        return false;
    }

    if (ownsFileSink) {
        GstElement* fileSink = gst_bin_get_by_name(GST_BIN(pipeline_), kFileSinkName);
        if (!fileSink) {
            std::cerr << "GStreamer writer: could not find the output filesink" << std::endl;
            return false;
        }
        g_object_set(fileSink, "location", destination.c_str(), nullptr);
        gst_object_unref(fileSink);
    }

    // Timestamps come from the frames we push, and a blocking push keeps memory
    // bounded when the encoder is slower than the caller.
    gst_app_src_set_stream_type(GST_APP_SRC(source_), GST_APP_STREAM_TYPE_STREAM);
    g_object_set(source_, "format", GST_FORMAT_TIME, "is-live", FALSE, "block", TRUE, nullptr);
    return true;
}

bool GStreamerWriter::initialize(const std::string& destination,
                                 const videocapture::VideoWriterConfig& config) {
    // Reinitialization finalizes the previous destination before replacing it.
    release();

    if (destination.empty()) {
        std::cerr << "GStreamer writer: destination is empty" << std::endl;
        return false;
    }
    if (!config.valid()) {
        std::cerr << "GStreamer writer: invalid configuration (" << config.width << "x"
                  << config.height << " @ " << config.frameRate << " fps)" << std::endl;
        return false;
    }
    config_ = config;

    gst_init(nullptr, nullptr);

    const bool callerSuppliedPipeline = destination.find('!') != std::string::npos;
    std::string description = destination;
    if (!callerSuppliedPipeline && !buildPipelineDescription(destination, description)) {
        cleanup();
        return false;
    }

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(description.c_str(), &error);
    if (error) {
        std::cerr << "GStreamer writer: pipeline construction failed: " << error->message
                  << std::endl;
        g_error_free(error);
        cleanup();
        return false;
    }
    if (!pipeline_) {
        std::cerr << "GStreamer writer: pipeline construction returned no pipeline" << std::endl;
        cleanup();
        return false;
    }

    if (!bindElements(!callerSuppliedPipeline, destination)) {
        cleanup();
        return false;
    }

    // The pipeline stays in READY until the first frame arrives, because the
    // appsrc caps depend on that frame's pixel layout.
    if (gst_element_set_state(pipeline_, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "GStreamer writer: could not prepare the pipeline for " << destination
                  << std::endl;
        cleanup();
        return false;
    }

    nextFrameIndex_ = 0;
    initialized_ = true;
    return true;
}

bool GStreamerWriter::startStream(videocapture::PixelFormat format) {
    const GstVideoFormat videoFormat = toGstVideoFormat(format);
    if (videoFormat == GST_VIDEO_FORMAT_UNKNOWN) {
        std::cerr << "GStreamer writer: unsupported source pixel format" << std::endl;
        return false;
    }

    gint frameRateNumerator = 0;
    gint frameRateDenominator = 1;
    gst_util_double_to_fraction(config_.frameRate, &frameRateNumerator, &frameRateDenominator);

    GstCaps* caps = gst_caps_new_simple(
        "video/x-raw", "format", G_TYPE_STRING, gst_video_format_to_string(videoFormat), "width",
        G_TYPE_INT, config_.width, "height", G_TYPE_INT, config_.height, "framerate",
        GST_TYPE_FRACTION, frameRateNumerator, frameRateDenominator, nullptr);
    gst_app_src_set_caps(GST_APP_SRC(source_), caps);
    gst_caps_unref(caps);

    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "GStreamer writer: could not start the pipeline" << std::endl;
        return false;
    }

    streamFormat_ = format;
    streaming_ = true;
    return true;
}

bool GStreamerWriter::writeFrame(const videocapture::Frame& frame) {
    if (!initialized_) {
        std::cerr << "GStreamer writer: writeFrame() called before initialize()" << std::endl;
        return false;
    }
    if (!videocapture::writer::validateFrame(frame, config_, "GStreamer writer")) {
        return false;
    }
    if (!streaming_) {
        if (!startStream(frame.format())) {
            return false;
        }
    } else if (frame.format() != streamFormat_) {
        std::cerr << "GStreamer writer: the pixel format changed after the stream was negotiated"
                  << std::endl;
        return false;
    }

    // GStreamer's implicit stride for packed video rounds rows up to four
    // bytes, while Frame packs them tightly, so rows are copied individually
    // and the real stride is attached as video meta for downstream elements.
    const std::size_t rowBytes =
        static_cast<std::size_t>(frame.width()) *
        static_cast<std::size_t>(videocapture::writer::packedBytesPerPixel(frame.format()));
    const std::size_t stride = GST_ROUND_UP_4(rowBytes);
    GstBuffer* buffer = gst_buffer_new_allocate(
        nullptr, stride * static_cast<std::size_t>(frame.height()), nullptr);
    if (!buffer) {
        std::cerr << "GStreamer writer: could not allocate an output buffer" << std::endl;
        return false;
    }

    GstMapInfo mapping;
    if (!gst_buffer_map(buffer, &mapping, GST_MAP_WRITE)) {
        std::cerr << "GStreamer writer: could not map the output buffer" << std::endl;
        gst_buffer_unref(buffer);
        return false;
    }
    for (int row = 0; row < frame.height(); ++row) {
        std::memcpy(mapping.data + static_cast<std::size_t>(row) * stride,
                    frame.data() + static_cast<std::size_t>(row) * frame.rowStride(), rowBytes);
    }
    gst_buffer_unmap(buffer, &mapping);

    const gsize planeOffsets[GST_VIDEO_MAX_PLANES] = {0, 0, 0, 0};
    const gint planeStrides[GST_VIDEO_MAX_PLANES] = {static_cast<gint>(stride), 0, 0, 0};
    gst_buffer_add_video_meta_full(
        buffer, GST_VIDEO_FRAME_FLAG_NONE, toGstVideoFormat(frame.format()),
        static_cast<guint>(frame.width()), static_cast<guint>(frame.height()), 1, planeOffsets,
        planeStrides);

    // Output timing is the constant rate the writer was opened with. Deriving
    // both edges from the frame index keeps the timeline free of rounding drift.
    const auto timestampOf = [this](std::uint64_t index) {
        return static_cast<GstClockTime>(static_cast<double>(index) * GST_SECOND /
                                         config_.frameRate);
    };
    const GstClockTime presentationTimestamp = timestampOf(nextFrameIndex_);
    GST_BUFFER_PTS(buffer) = presentationTimestamp;
    GST_BUFFER_DTS(buffer) = presentationTimestamp;
    GST_BUFFER_DURATION(buffer) = timestampOf(nextFrameIndex_ + 1) - presentationTimestamp;
    ++nextFrameIndex_;

    // push_buffer takes ownership of the buffer whatever it returns.
    const GstFlowReturn flow = gst_app_src_push_buffer(GST_APP_SRC(source_), buffer);
    if (flow != GST_FLOW_OK) {
        std::cerr << "GStreamer writer: the pipeline rejected a frame (" << gst_flow_get_name(flow)
                  << ")" << std::endl;
        return false;
    }
    return true;
}

bool GStreamerWriter::isOpen() const {
    return initialized_;
}

void GStreamerWriter::release() {
    if (initialized_ && streaming_) {
        gst_app_src_end_of_stream(GST_APP_SRC(source_));

        // Muxers write their header or index only once end of stream reaches
        // the sink, so tearing the pipeline down before then truncates the file.
        GstBus* bus = gst_element_get_bus(pipeline_);
        if (bus) {
            GstMessage* message = gst_bus_timed_pop_filtered(
                bus, kEndOfStreamTimeout,
                static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
            if (!message) {
                std::cerr << "GStreamer writer: timed out finalizing the destination; "
                             "the output may be incomplete"
                          << std::endl;
            } else {
                if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
                    GError* error = nullptr;
                    gchar* debug = nullptr;
                    gst_message_parse_error(message, &error, &debug);
                    std::cerr << "GStreamer writer: could not finalize the destination: "
                              << error->message << std::endl;
                    g_error_free(error);
                    g_free(debug);
                }
                gst_message_unref(message);
            }
            gst_object_unref(bus);
        }
    }
    cleanup();
}

void GStreamerWriter::cleanup() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
    if (source_) {
        gst_object_unref(source_);
        source_ = nullptr;
    }
    if (pipeline_) {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    config_ = {};
    streamFormat_ = videocapture::PixelFormat::BGR8;
    nextFrameIndex_ = 0;
    streaming_ = false;
    initialized_ = false;
}
