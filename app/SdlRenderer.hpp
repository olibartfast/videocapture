#pragma once

#include "Frame.hpp"

#include <memory>
#include <string>

namespace videocapture::app {

class SdlRenderer {
public:
    explicit SdlRenderer(std::string title);
    ~SdlRenderer();

    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;
    SdlRenderer(SdlRenderer&&) noexcept;
    SdlRenderer& operator=(SdlRenderer&&) noexcept;

    // Returns false only when the user requests that playback stop. Display
    // failures disable the preview without interrupting frame decoding.
    bool render(const Frame& frame);

    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] static bool supports(const Frame& frame) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace videocapture::app
