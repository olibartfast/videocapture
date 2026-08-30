#include <gtest/gtest.h>

#include "SdlRenderer.hpp"

TEST(SdlRendererTest, AcceptsPackedBGRFrames) {
    const videocapture::Frame frame(2, 2, videocapture::PixelFormat::BGR8);

    EXPECT_TRUE(videocapture::app::SdlRenderer::supports(frame));
}

TEST(SdlRendererTest, RejectsEmptyAndUnsupportedFrames) {
    const videocapture::Frame empty;
    const videocapture::Frame rgb(2, 2, videocapture::PixelFormat::RGB8);
    const videocapture::Frame nv12(2, 2, videocapture::PixelFormat::NV12);

    EXPECT_FALSE(videocapture::app::SdlRenderer::supports(empty));
    EXPECT_FALSE(videocapture::app::SdlRenderer::supports(rgb));
    EXPECT_FALSE(videocapture::app::SdlRenderer::supports(nv12));
}
