if(DEFINED ZEPHYR_BASE)
    message(STATUS "Building as a Zephyr module")
    include(zephyr/zephyr.cmake)

    return()
endif()

cmake_minimum_required(VERSION 3.13)
project(firmups-device-sdk VERSION 1.0)

include(cmake/warn_all.cmake)
include(cmake/config.cmake)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    message(STATUS "CMAKE_BUILD_TYPE is not set. Defaulting to Debug.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
endif()

# Enable compile commands export
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check if build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add sanitizer flags for Debug builds
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fsanitize-undefined-trap-on-error")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined -fsanitize-undefined-trap-on-error")
endif()


# Add tinycbor dependency
include(cmake/tinycbor.cmake)

# Add ascon-c dependency
include(cmake/ascon.cmake)

# Add library
add_library(firmups-device-sdk-obj OBJECT
    src/sdk.c
    src/codec/cbor_helper.c
    src/codec/cose.c
    src/codec/crypto.c
    src/operation/device_info.c
    src/operation/firmware.c
    src/operation/parameter.c
    src/operation/error.c)
#target_compile_options(firmups-device-sdk PRIVATE -fvisibility=hidden)
set_target_properties(tinycbor PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(firmups-device-sdk-obj PRIVATE tinycbor)
if(NOT DEFINED FIRMUPS_USE_AES AND NOT DEFINED FIRMUPS_USE_CRYPTO_CALLBACKS)
    set_target_properties(crypto_aead_asconaead128_${FIRMUPS_ASCON_IMPL} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(firmups-device-sdk-obj PRIVATE crypto_aead_asconaead128_${FIRMUPS_ASCON_IMPL})
endif()

warn_all(firmups-device-sdk-obj)
configure_firmups_sdk(firmups-device-sdk-obj)

if(NOT DEFINED FIRMUPS_LOG_LEVEL AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(firmups-device-sdk-obj PRIVATE FIRMUPS_LOG_LEVEL_DEBUG)
endif()

target_include_directories(firmups-device-sdk-obj
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# Test
if(DEFINED FIRMUPS_TESTING)
    ## Add Unity
    add_subdirectory(dependencies/unity)
    target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)

    ## Add test executable
    add_executable(firmups-device-sdk-tests test/sdk_test.c)
    #target_link_options(firmups-device-sdk-tests PRIVATE "-static")
    target_link_libraries(firmups-device-sdk-tests firmups-device-sdk-obj unity)
    configure_firmups_sdk(firmups-device-sdk-tests)

    # # Optionally add a test target
    enable_testing()
    add_test(NAME firmups-device-sdk-test COMMAND firmups-device-sdk-tests)
endif()

# Shared library (.so)
add_library(firmups-device-sdk SHARED $<TARGET_OBJECTS:firmups-device-sdk-obj>)

# Static library (.a)
add_library(firmups-device-sdk-static STATIC $<TARGET_OBJECTS:firmups-device-sdk-obj>)

# Optional: make the static library install name also be libfirmups-device-sdk.a
set_target_properties(firmups-device-sdk-static PROPERTIES OUTPUT_NAME firmups-device-sdk)
