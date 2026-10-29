# Coldpac dependency-baseline integration.
#
# Fetches https://github.com/olibartfast/coldpac pinned to release tag
# v0.1.1 and validates deps.yaml at configure time (hermetic, schema-only).
# The informational `coldpac-status` target compares wanted vs installed
# vs candidate per deps.lock and exits 0 clean / 2 updates / 1 drift.
#
# Offline/air-gapped override:
#   cmake -DFETCHCONTENT_SOURCE_DIR_COLDPAC=/path/to/coldpac ..
# Disable entirely:
#   cmake -DVIDEOCAPTURE_USE_COLDPAC=OFF ..

option(VIDEOCAPTURE_USE_COLDPAC "Fetch coldpac and validate deps.yaml" ON)

if(VIDEOCAPTURE_USE_COLDPAC)
    include(FetchContent)
    FetchContent_Declare(
        coldpac
        GIT_REPOSITORY https://github.com/olibartfast/coldpac.git
        GIT_TAG v0.1.2
    )
    FetchContent_MakeAvailable(coldpac)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(NOT Python3_FOUND)
        message(WARNING "Python3 not found; skipping coldpac deps.yaml validation")
    else()
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${coldpac_SOURCE_DIR}/coldpac"
                --manifest "${CMAKE_CURRENT_SOURCE_DIR}/deps.yaml"
                --lock "${CMAKE_CURRENT_SOURCE_DIR}/deps.lock"
                validate
            RESULT_VARIABLE _coldpac_validate_result
            OUTPUT_VARIABLE _coldpac_validate_out
            ERROR_VARIABLE _coldpac_validate_err
        )
        if(NOT _coldpac_validate_result EQUAL 0)
            message(FATAL_ERROR "coldpac validation failed:\n${_coldpac_validate_err}")
        endif()
        message(STATUS "coldpac: ${_coldpac_validate_out}")

        add_custom_target(coldpac-status
            COMMAND "${Python3_EXECUTABLE}" "${coldpac_SOURCE_DIR}/coldpac"
                --manifest "${CMAKE_CURRENT_SOURCE_DIR}/deps.yaml"
                --lock "${CMAKE_CURRENT_SOURCE_DIR}/deps.lock"
                status
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "coldpac: wanted vs installed vs candidate"
            VERBATIM)
    endif()
endif()
