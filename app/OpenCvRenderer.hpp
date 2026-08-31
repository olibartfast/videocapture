#pragma once

#include "Renderer.hpp"

#include <string>

namespace videocapture::app {

class OpenCvRenderer final : public PollingRenderer {
public:
    explicit OpenCvRenderer(std::string title);

private:
    bool present(const Frame& frame) override;

    std::string title_;
    bool warned_ = false;
};

}  // namespace videocapture::app
