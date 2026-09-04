#include "SokolRenderer.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "SokolFramebuffer.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

namespace videocapture::app {

struct SokolRenderer::Impl {
    explicit Impl(std::string windowTitle) : title(std::move(windowTitle)) {}

    std::size_t run(FrameReader frameReader) {
        readFrame = std::move(frameReader);

        sapp_desc description{};
        description.user_data = this;
        description.init_userdata_cb = initialize;
        description.frame_userdata_cb = renderFrame;
        description.cleanup_userdata_cb = cleanup;
        description.event_userdata_cb = handleEvent;
        description.width = 640;
        description.height = 480;
        description.sample_count = 1;
        description.high_dpi = true;
        description.window_title = title.c_str();
        description.logger.func = slog_func;
        sapp_run(description);
        return frameCount;
    }

    static void initialize(void* userData) {
        auto& self = *static_cast<Impl*>(userData);

        sg_desc graphicsDescription{};
        graphicsDescription.environment = sglue_environment();
        graphicsDescription.logger.func = slog_func;
        sg_setup(graphicsDescription);

        self.initialized = sg_isvalid();
        if (self.initialized) {
            videocapture_sokol_framebuffer_setup();
        } else {
            std::cerr << "warning: Sokol preview initialization failed\n";
        }
    }

    static void renderFrame(void* userData) {
        auto& self = *static_cast<Impl*>(userData);
        if (!self.readFrame(self.frame) || self.frame.empty()) {
            sapp_request_quit();
            return;
        }
        ++self.frameCount;

        if (!self.initialized) {
            return;
        }

        if (Renderer::supports(self.frame)) {
            Renderer::copyBgrToRgba(self.frame, self.pixels);
            if (!videocapture_sokol_framebuffer_update(self.pixels.data(), self.pixels.size(),
                                                       self.frame.width(), self.frame.height()) &&
                !self.warned) {
                self.warned = true;
                std::cerr << "warning: Sokol framebuffer update failed, preview disabled\n";
            }
        } else if (!Renderer::supports(self.frame) && !self.warned) {
            self.warned = true;
            std::cerr << "warning: Sokol preview requires a packed BGR8 frame, preview disabled\n";
        }

        sg_pass pass{};
        pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass.action.colors[0].clear_value = {0.0F, 0.0F, 0.0F, 1.0F};
        pass.swapchain = sglue_swapchain();
        sg_begin_pass(pass);
        videocapture_sokol_framebuffer_render();
        sg_end_pass();
        sg_commit();
    }

    static void cleanup(void* userData) {
        auto& self = *static_cast<Impl*>(userData);
        if (self.initialized) {
            videocapture_sokol_framebuffer_shutdown();
        }
        if (sg_isvalid()) {
            sg_shutdown();
        }
        self.initialized = false;
    }

    static void handleEvent(const sapp_event* event, void*) {
        if (event->type == SAPP_EVENTTYPE_KEY_DOWN &&
            (event->key_code == SAPP_KEYCODE_ESCAPE || event->key_code == SAPP_KEYCODE_Q)) {
            sapp_request_quit();
        }
    }

    std::string title;
    FrameReader readFrame;
    Frame frame;
    std::vector<std::uint8_t> pixels;
    std::size_t frameCount = 0;
    bool initialized = false;
    bool warned = false;
};

SokolRenderer::SokolRenderer(std::string title) : impl_(std::make_unique<Impl>(std::move(title))) {}

SokolRenderer::~SokolRenderer() = default;

std::size_t SokolRenderer::run(FrameReader readFrame) {
    return impl_->run(std::move(readFrame));
}

}  // namespace videocapture::app
