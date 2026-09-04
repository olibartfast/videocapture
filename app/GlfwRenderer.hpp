#pragma once

#include "Renderer.hpp"

#include <memory>
#include <string>

namespace videocapture::app {

class GlfwRenderer final : public PollingRenderer {
public:
    explicit GlfwRenderer(std::string title);
    ~GlfwRenderer() override;

private:
    bool present(const Frame& frame) override;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace videocapture::app
