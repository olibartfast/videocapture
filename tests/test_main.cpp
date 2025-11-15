#include <gtest/gtest.h>

// Google Test main is provided by GTest::Main
// This file can be used for global test setup if needed

class VideoTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Global test setup
    }

    void TearDown() override {
        // Global test cleanup
    }
};

// Register the environment
::testing::Environment* const test_env =
    ::testing::AddGlobalTestEnvironment(new VideoTestEnvironment);
