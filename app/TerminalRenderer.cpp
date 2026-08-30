#include "TerminalRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace videocapture::app {
namespace {

bool enableTerminalOutput() noexcept {
#ifdef _WIN32
    if (_isatty(_fileno(stdout)) == 0) {
        return false;
    }

    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    return output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode) != 0 &&
           SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

void appendColor(std::string& output, const std::uint8_t* top, const std::uint8_t* bottom) {
    output += "\x1b[38;2;";
    output += std::to_string(top[2]);
    output += ';';
    output += std::to_string(top[1]);
    output += ';';
    output += std::to_string(top[0]);
    output += "m\x1b[48;2;";
    output += std::to_string(bottom[2]);
    output += ';';
    output += std::to_string(bottom[1]);
    output += ';';
    output += std::to_string(bottom[0]);
    output += "m\xE2\x96\x80";  // Upper half block.
}

}  // namespace

TerminalRenderer::TerminalRenderer() : available_(enableTerminalOutput()) {}

TerminalRenderer::~TerminalRenderer() {
    if (available_) {
        std::cout << "\x1b[0m\n";
    }
}

bool TerminalRenderer::render(const Frame& frame) {
    if (!available_) {
        return false;
    }

    const auto [columns, rows] = terminalSize();
    const std::string preview = renderFrame(frame, columns, rows > 1 ? rows - 1 : rows);
    if (preview.empty()) {
        return false;
    }

    if (firstFrame_) {
        std::cout << "\x1b[2J";
        firstFrame_ = false;
    }
    std::cout << "\x1b[H" << preview << std::flush;
    return true;
}

std::string TerminalRenderer::renderFrame(const Frame& frame, std::size_t maxColumns,
                                          std::size_t maxRows) {
    if (frame.empty() || frame.format() != PixelFormat::BGR8 || frame.planeCount() != 1 ||
        maxColumns == 0 || maxRows == 0) {
        return {};
    }

    const double horizontalScale =
        static_cast<double>(maxColumns) / static_cast<double>(frame.width());
    const double verticalScale =
        (static_cast<double>(maxRows) * 2.0) / static_cast<double>(frame.height());
    const double scale = std::min({1.0, horizontalScale, verticalScale});
    const int outputWidth =
        std::max(1, static_cast<int>(std::floor(static_cast<double>(frame.width()) * scale)));
    const int outputPixelHeight =
        std::max(1, static_cast<int>(std::floor(static_cast<double>(frame.height()) * scale)));
    const int outputRows = (outputPixelHeight + 1) / 2;

    std::string output;
    output.reserve(static_cast<std::size_t>(outputWidth) * static_cast<std::size_t>(outputRows) *
                   32);

    for (int outputRow = 0; outputRow < outputRows; ++outputRow) {
        const int topOutputY = outputRow * 2;
        const int bottomOutputY = std::min(topOutputY + 1, outputPixelHeight - 1);
        const int topSourceY = static_cast<int>(static_cast<std::int64_t>(topOutputY) *
                                                frame.height() / outputPixelHeight);
        const int bottomSourceY = static_cast<int>(static_cast<std::int64_t>(bottomOutputY) *
                                                   frame.height() / outputPixelHeight);
        const std::uint8_t* topRow =
            frame.data() + static_cast<std::size_t>(topSourceY) * frame.rowStride();
        const std::uint8_t* bottomRow =
            frame.data() + static_cast<std::size_t>(bottomSourceY) * frame.rowStride();

        for (int outputColumn = 0; outputColumn < outputWidth; ++outputColumn) {
            const int sourceX = static_cast<int>(static_cast<std::int64_t>(outputColumn) *
                                                 frame.width() / outputWidth);
            appendColor(output, topRow + sourceX * 3, bottomRow + sourceX * 3);
        }
        output += "\x1b[0m\n";
    }

    return output;
}

std::pair<std::size_t, std::size_t> TerminalRenderer::terminalSize() noexcept {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(output, &info) != 0) {
        return {static_cast<std::size_t>(info.srWindow.Right - info.srWindow.Left + 1),
                static_cast<std::size_t>(info.srWindow.Bottom - info.srWindow.Top + 1)};
    }
#else
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 && size.ws_row > 0) {
        return {size.ws_col, size.ws_row};
    }
#endif
    return {80, 24};
}

}  // namespace videocapture::app
