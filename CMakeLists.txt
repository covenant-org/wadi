cmake_minimum_required(VERSION 3.5)

project(wadi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_DEFINITIONS(-DWEBRTC_POSIX=1)
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS=1)

include_directories(include)
include_directories(include/webrtc)
include_directories(include/webrtc/third_party/abseil-cpp)

add_executable(wadi src/main.cpp src/v4l.cpp src/whip.cpp)

target_link_libraries(wadi
	#  ${CMAKE_SOURCE_DIR}/libs/webrtc/ubuntu-22.04-amd64/libwebrtc.a
       ${CMAKE_SOURCE_DIR}/libs/libwebrtc.a
       X11
       Xrandr
)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lX11 -lXrandr")
