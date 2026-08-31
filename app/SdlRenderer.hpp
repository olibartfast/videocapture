#pragma once

#include "Renderer.hpp"

#include <memory>
#include <string>

namespace videocapture::app {

class SdlRenderer final : public PollingRenderer {
public:
    explicit SdlRenderer(std::string title);
    ~SdlRenderer() override;

private:
    bool present(const Frame& frame) override;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace videocapture::app
