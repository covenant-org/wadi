cmake_minimum_required(VERSION 3.5)

project(wadi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

include_directories(include)

add_executable(wadi src/main.cpp src/v4l.cpp)
