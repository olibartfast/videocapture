#pragma once

#include "Frame.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace videocapture::app {

class Renderer {
public:
    using FrameReader = std::function<bool(Frame&)>;

    virtual ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    virtual std::size_t run(FrameReader readFrame) = 0;

    [[nodiscard]] static bool supports(const Frame& frame) noexcept;

protected:
    Renderer() = default;

    static void copyBgrToRgba(const Frame& frame, std::vector<std::uint8_t>& pixels);
};

class PollingRenderer : public Renderer {
public:
    std::size_t run(FrameReader readFrame) final;

private:
    virtual bool present(const Frame& frame) = 0;
};

std::unique_ptr<Renderer> createRenderer(std::string title);

}  // namespace videocapture::app
