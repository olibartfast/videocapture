#pragma once

#include "Renderer.hpp"

#include <memory>
#include <string>

namespace videocapture::app {

class SokolRenderer final : public Renderer {
public:
    explicit SokolRenderer(std::string title);
    ~SokolRenderer() override;

    std::size_t run(FrameReader readFrame) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace videocapture::app
