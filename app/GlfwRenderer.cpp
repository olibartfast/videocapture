#include "GlfwRenderer.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <GLFW/glfw3.h>

namespace videocapture::app {
namespace {

void warnOnce(bool& warned, const std::string& message) {
    if (!warned) {
        warned = true;
        std::cerr << "warning: " << message << '\n';
    }
}

}  // namespace

struct GlfwRenderer::Impl {
    explicit Impl(std::string windowTitle) : title(std::move(windowTitle)) {}

    ~Impl() {
        if (window != nullptr) {
            glfwMakeContextCurrent(window);
            if (texture != 0U) {
                glDeleteTextures(1, &texture);
            }
            glfwDestroyWindow(window);
        }
        if (glfwInitialized) {
            glfwTerminate();
        }
    }

    bool initialize(int frameWidth, int frameHeight) {
        initializationAttempted = true;
        if (glfwInit() != GLFW_TRUE) {
            const char* description = nullptr;
            glfwGetError(&description);
            warnOnce(warned, std::string("glfwInit failed") +
                                 (description != nullptr ? ": " + std::string(description) : "") +
                                 ", preview disabled");
            return false;
        }
        glfwInitialized = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(frameWidth, frameHeight, title.c_str(), nullptr, nullptr);
        if (window == nullptr) {
            const char* description = nullptr;
            glfwGetError(&description);
            warnOnce(warned, std::string("glfwCreateWindow failed") +
                                 (description != nullptr ? ": " + std::string(description) : "") +
                                 ", preview disabled");
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return resizeTexture(frameWidth, frameHeight);
    }

    bool resizeTexture(int frameWidth, int frameHeight) {
        glfwMakeContextCurrent(window);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameWidth, frameHeight, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        if (glGetError() != GL_NO_ERROR) {
            warnOnce(warned, "GLFW OpenGL texture creation failed, preview disabled");
            return false;
        }

        width = frameWidth;
        height = frameHeight;
        glfwSetWindowSize(window, width, height);
        ok = true;
        return true;
    }

    bool show(const Frame& frame) {
        if (!Renderer::supports(frame)) {
            warnOnce(warned, "GLFW preview requires a packed BGR8 frame, preview disabled");
            return true;
        }
        if (!initializationAttempted && !initialize(frame.width(), frame.height())) {
            return true;
        }
        if (!ok) {
            return true;
        }
        if ((frame.width() != width || frame.height() != height) &&
            !resizeTexture(frame.width(), frame.height())) {
            ok = false;
            return true;
        }

        glfwPollEvents();
        if (glfwWindowShouldClose(window) == GLFW_TRUE ||
            glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            return false;
        }

        Renderer::copyBgrToRgba(frame, pixels);
        glfwMakeContextCurrent(window);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                        pixels.data());

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0F, 1.0F);
        glVertex2f(-1.0F, -1.0F);
        glTexCoord2f(0.0F, 0.0F);
        glVertex2f(-1.0F, 1.0F);
        glTexCoord2f(1.0F, 1.0F);
        glVertex2f(1.0F, -1.0F);
        glTexCoord2f(1.0F, 0.0F);
        glVertex2f(1.0F, 1.0F);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glfwSwapBuffers(window);
        return true;
    }

    std::string title;
    GLFWwindow* window = nullptr;
    GLuint texture = 0;
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    bool initializationAttempted = false;
    bool glfwInitialized = false;
    bool ok = false;
    bool warned = false;
};

GlfwRenderer::GlfwRenderer(std::string title) : impl_(std::make_unique<Impl>(std::move(title))) {}

GlfwRenderer::~GlfwRenderer() = default;

bool GlfwRenderer::present(const Frame& frame) {
    return impl_->show(frame);
}

}  // namespace videocapture::app
