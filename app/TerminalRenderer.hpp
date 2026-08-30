#pragma once

#include "Frame.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace videocapture::app {

class TerminalRenderer {
public:
    TerminalRenderer();
    ~TerminalRenderer();

    [[nodiscard]] bool available() const noexcept { return available_; }
    bool render(const Frame& frame);

    // Builds a true-color ANSI preview without writing to the terminal. This
    // pure operation also makes the scaling and BGR conversion testable.
    [[nodiscard]] static std::string renderFrame(const Frame& frame, std::size_t maxColumns,
                                                 std::size_t maxRows);

private:
    [[nodiscard]] static std::pair<std::size_t, std::size_t> terminalSize() noexcept;

    bool available_ = false;
    bool firstFrame_ = true;
};

}  // namespace videocapture::app
