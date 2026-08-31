#include <gtest/gtest.h>

#include "Renderer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

class TestRenderer final : public videocapture::app::PollingRenderer {
public:
    using Renderer::copyBgrToRgba;

    explicit TestRenderer(std::size_t stopAfter = 0) : stopAfter_(stopAfter) {}

    [[nodiscard]] std::size_t presentedFrames() const noexcept { return presentedFrames_; }

private:
    bool present(const videocapture::Frame&) override {
        ++presentedFrames_;
        return stopAfter_ == 0 || presentedFrames_ < stopAfter_;
    }

    std::size_t stopAfter_;
    std::size_t presentedFrames_ = 0;
};

}  // namespace

TEST(RendererTest, AcceptsPackedBGRFrames) {
    const videocapture::Frame frame(2, 2, videocapture::PixelFormat::BGR8);

    EXPECT_TRUE(videocapture::app::Renderer::supports(frame));
}

TEST(RendererTest, RejectsEmptyAndUnsupportedFrames) {
    const videocapture::Frame empty;
    const videocapture::Frame rgb(2, 2, videocapture::PixelFormat::RGB8);
    const videocapture::Frame nv12(2, 2, videocapture::PixelFormat::NV12);

    EXPECT_FALSE(videocapture::app::Renderer::supports(empty));
    EXPECT_FALSE(videocapture::app::Renderer::supports(rgb));
    EXPECT_FALSE(videocapture::app::Renderer::supports(nv12));
}

TEST(RendererTest, PollingRendererCountsFramesAndHonorsStopRequest) {
    TestRenderer renderer(2);
    std::size_t suppliedFrames = 0;

    const std::size_t decodedFrames = renderer.run([&suppliedFrames](videocapture::Frame& frame) {
        if (suppliedFrames == 4) {
            return false;
        }
        frame.resize(2, 2);
        ++suppliedFrames;
        return true;
    });

    EXPECT_EQ(decodedFrames, 2U);
    EXPECT_EQ(renderer.presentedFrames(), 2U);
    EXPECT_EQ(suppliedFrames, 2U);
}

TEST(RendererTest, ConvertsBGRPixelsToRGBA) {
    videocapture::Frame frame(2, 1);
    const std::uint8_t source[] = {1, 2, 3, 10, 20, 30};
    std::copy(std::begin(source), std::end(source), frame.data());
    std::vector<std::uint8_t> pixels;

    TestRenderer::copyBgrToRgba(frame, pixels);

    const std::vector<std::uint8_t> expected = {3, 2, 1, 255, 30, 20, 10, 255};
    EXPECT_EQ(pixels, expected);
}
