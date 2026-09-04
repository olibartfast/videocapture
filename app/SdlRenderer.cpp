#include "SdlRenderer.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>

#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace videocapture::app {
namespace {

void warnOnce(bool& warned, const std::string& message) {
    if (!warned) {
        warned = true;
        std::cerr << "warning: " << message << '\n';
    }
}

}  // namespace

struct SdlRenderer::Impl {
    explicit Impl(std::string windowTitle) : title(std::move(windowTitle)) {}

    ~Impl() {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
        if (sdlInitialized) {
            SDL_Quit();
        }
    }

    bool initialize(int frameWidth, int frameHeight) {
        initializationAttempted = true;

        SDL_SetMainReady();
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            warnOnce(warned,
                     std::string("SDL_Init failed (") + SDL_GetError() + "), preview disabled");
            return false;
        }
        sdlInitialized = true;

        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  frameWidth, frameHeight,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (window == nullptr) {
            warnOnce(warned, std::string("SDL_CreateWindow failed (") + SDL_GetError() +
                                 "), preview disabled");
            return false;
        }

        renderer =
            SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer == nullptr) {
            renderer = SDL_CreateRenderer(window, -1, 0);
        }
        if (renderer == nullptr) {
            warnOnce(warned, std::string("SDL_CreateRenderer failed (") + SDL_GetError() +
                                 "), preview disabled");
            return false;
        }

        return createTexture(frameWidth, frameHeight);
    }

    bool createTexture(int frameWidth, int frameHeight) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING,
                                    frameWidth, frameHeight);
        if (texture == nullptr) {
            warnOnce(warned, std::string("SDL_CreateTexture failed (") + SDL_GetError() +
                                 "), preview disabled");
            return false;
        }

        width = frameWidth;
        height = frameHeight;
        SDL_SetWindowSize(window, width, height);
        ok = true;
        return true;
    }

    bool show(const Frame& frame) {
        if (quit) {
            return false;
        }
        if (!Renderer::supports(frame)) {
            warnOnce(warned, "SDL preview requires a packed BGR8 frame, preview disabled");
            return true;
        }
        if (!initializationAttempted && !initialize(frame.width(), frame.height())) {
            return true;
        }
        if (!ok) {
            return true;
        }
        if ((frame.width() != width || frame.height() != height) &&
            !createTexture(frame.width(), frame.height())) {
            ok = false;
            return true;
        }

        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
                return false;
            }
            if (event.type == SDL_KEYDOWN) {
                const SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE || key == SDLK_q) {
                    quit = true;
                    return false;
                }
            }
        }

        const int pitch = static_cast<int>(frame.rowStride());
        if (SDL_UpdateTexture(texture, nullptr, frame.data(), pitch) != 0 ||
            SDL_RenderClear(renderer) != 0 ||
            SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
            warnOnce(warned, std::string("SDL frame rendering failed (") + SDL_GetError() +
                                 "), preview disabled");
            ok = false;
            return true;
        }

        SDL_RenderPresent(renderer);
        return true;
    }

    std::string title;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    bool initializationAttempted = false;
    bool sdlInitialized = false;
    bool ok = false;
    bool quit = false;
    bool warned = false;
};

SdlRenderer::SdlRenderer(std::string title) : impl_(std::make_unique<Impl>(std::move(title))) {}

SdlRenderer::~SdlRenderer() = default;

bool SdlRenderer::present(const Frame& frame) {
    return impl_->show(frame);
}

}  // namespace videocapture::app
