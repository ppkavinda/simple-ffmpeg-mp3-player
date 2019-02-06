cmake_minimum_required(VERSION 3.13)
project(v2)

set(CMAKE_CXX_STANDARD 14)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_library(AVCODEC_LIBRARY NAMES avcodec)
find_library(AVFORMAT_LIBRARY NAMES avformat)
find_library(SWSCALE_LIBRARY NAMES swscale)
find_library(AVUTIL_LIBRARY NAMES avutil)
find_library(SWR_LIBRARY NAMES swresample)

find_package(SDL2 REQUIRED)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/res/audio/sampleAudio.wav"
        "${CMAKE_CURRENT_BINARY_DIR}/sampleAudio.wav"
        COPYONLY
)
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/res/video/sampleVideo.MKV"
        "${CMAKE_CURRENT_BINARY_DIR}/sampleVideo.MKV"
        COPYONLY
)

add_executable(v2 main.cpp)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

target_link_libraries(v2 PRIVATE
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWR_LIBRARY}
        ${SDL2_LIBRARY}
        )