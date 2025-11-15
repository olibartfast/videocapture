#include <gtest/gtest.h>
#include "VideoCaptureFactory.hpp"
#include "VideoCaptureInterface.hpp"
#include <memory>

class FactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(FactoryTest, CreateVideoInterface) {
    auto capture = createVideoInterface();
    ASSERT_NE(capture, nullptr);
}

TEST_F(FactoryTest, CreateAndInitialize) {
    auto capture = createVideoInterface();
    ASSERT_NE(capture, nullptr);
    
    // Test with invalid source should fail
    EXPECT_FALSE(capture->initialize("/nonexistent/video.mp4"));
}

TEST_F(FactoryTest, MultipleInstances) {
    auto capture1 = createVideoInterface();
    auto capture2 = createVideoInterface();
    auto capture3 = createVideoInterface();
    
    ASSERT_NE(capture1, nullptr);
    ASSERT_NE(capture2, nullptr);
    ASSERT_NE(capture3, nullptr);
}

TEST_F(FactoryTest, ReleaseAfterCreate) {
    auto capture = createVideoInterface();
    ASSERT_NE(capture, nullptr);
    
    // Should not crash
    EXPECT_NO_THROW(capture->release());
}

