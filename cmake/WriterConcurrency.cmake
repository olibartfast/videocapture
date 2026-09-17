# Check the actual standard library, not the compiler's C++20 language flag.
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

function(validate_writer_concurrency)
    find_package(Threads REQUIRED)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    # Recheck when the selected SDK, deployment target, or flags change.
    unset(VIDEOCAPTURE_HAS_WRITER_CONCURRENCY CACHE)
    check_cxx_source_compiles([=[
        #include <condition_variable>
        #include <mutex>
        #include <sstream>
        #include <stop_token>
        #include <syncstream>
        #include <thread>
        int main() {
            std::mutex mutex;
            std::condition_variable_any ready;
            std::ostringstream output;
            std::jthread worker([&](std::stop_token stop) {
                std::unique_lock<std::mutex> lock(mutex);
                ready.wait(lock, stop, [] { return false; });
                std::osyncstream(output) << "stopped";
            });
            worker.request_stop();
            worker.join();
        }
    ]=] VIDEOCAPTURE_HAS_WRITER_CONCURRENCY)
    cmake_pop_check_state()
    if(NOT VIDEOCAPTURE_HAS_WRITER_CONCURRENCY)
        message(FATAL_ERROR
            "USE_VIDEOWRITER requires a C++20 standard library with std::jthread, "
            "std::stop_token, stop-aware condition_variable_any::wait, and std::osyncstream. "
            "The selected compiler/SDK cannot compile and link these features. "
            "Select a supporting toolchain (check libc++ feature availability on macOS), "
            "or configure with -DUSE_VIDEOWRITER=OFF for capture only.")
    endif()
endfunction()
