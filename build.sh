#!/bin/bash

set -e
BUILD_TYPE=${1:-Debug}

cmake -B build -H. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
if [ "$BUILD_TYPE" == "Debug" ]; then
  bear -- cmake --build build
else
  cmake --build build
fi
