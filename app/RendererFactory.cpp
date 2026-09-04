#include "Renderer.hpp"

#if defined(VIDEOCAPTURE_APP_RENDERER_OPENCV)
#include "OpenCvRenderer.hpp"
#elif defined(VIDEOCAPTURE_APP_RENDERER_SDL2)
#include "SdlRenderer.hpp"
#elif defined(VIDEOCAPTURE_APP_RENDERER_GLFW)
#include "GlfwRenderer.hpp"
#elif defined(VIDEOCAPTURE_APP_RENDERER_SOKOL)
#include "SokolRenderer.hpp"
#else
#error "No sample app renderer was selected"
#endif

#include <memory>
#include <utility>

namespace videocapture::app {

std::unique_ptr<Renderer> createRenderer(std::string title) {
#if defined(VIDEOCAPTURE_APP_RENDERER_OPENCV)
    return std::make_unique<OpenCvRenderer>(std::move(title));
#elif defined(VIDEOCAPTURE_APP_RENDERER_SDL2)
    return std::make_unique<SdlRenderer>(std::move(title));
#elif defined(VIDEOCAPTURE_APP_RENDERER_GLFW)
    return std::make_unique<GlfwRenderer>(std::move(title));
#elif defined(VIDEOCAPTURE_APP_RENDERER_SOKOL)
    return std::make_unique<SokolRenderer>(std::move(title));
#endif
}

}  // namespace videocapture::app
