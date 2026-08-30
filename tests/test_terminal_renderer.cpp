#include <gtest/gtest.h>

#include "TerminalRenderer.hpp"

namespace {

std::size_t countOccurrences(const std::string& value, const std::string& needle) {
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = value.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

}  // namespace

TEST(TerminalRendererTest, ConvertsBGRRowsToTrueColorHalfBlocks) {
    videocapture::Frame frame(1, 2, videocapture::PixelFormat::BGR8);
    frame.data()[0] = 0;
    frame.data()[1] = 0;
    frame.data()[2] = 255;
    frame.data()[3] = 255;
    frame.data()[4] = 0;
    frame.data()[5] = 0;

    const std::string output = videocapture::app::TerminalRenderer::renderFrame(frame, 1, 1);

    EXPECT_NE(output.find("\x1b[38;2;255;0;0m"), std::string::npos);
    EXPECT_NE(output.find("\x1b[48;2;0;0;255m"), std::string::npos);
    EXPECT_NE(output.find("\xE2\x96\x80"), std::string::npos);
}

TEST(TerminalRendererTest, DownscalesWithoutExceedingTerminalBounds) {
    videocapture::Frame frame(8, 8, videocapture::PixelFormat::BGR8);

    const std::string output = videocapture::app::TerminalRenderer::renderFrame(frame, 2, 1);

    EXPECT_EQ(countOccurrences(output, "\xE2\x96\x80"), 2U);
    EXPECT_EQ(countOccurrences(output, "\n"), 1U);
}

TEST(TerminalRendererTest, RejectsUnsupportedLayoutsAndInvalidBounds) {
    const videocapture::Frame rgb(2, 2, videocapture::PixelFormat::RGB8);
    const videocapture::Frame bgr(2, 2, videocapture::PixelFormat::BGR8);

    EXPECT_TRUE(videocapture::app::TerminalRenderer::renderFrame(rgb, 80, 24).empty());
    EXPECT_TRUE(videocapture::app::TerminalRenderer::renderFrame(bgr, 0, 24).empty());
    EXPECT_TRUE(videocapture::app::TerminalRenderer::renderFrame(bgr, 80, 0).empty());
}
