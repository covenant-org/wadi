#!/bin/bash

set -e
BUILD_TYPE=${1:-Debug}

cmake -B build -H. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
if [ "$BUILD_TYPE" == "Debug" ]; then
  if [ -f compile_commands.json ]; then
      cp compile_commands.json compile_commands.json.bak
  fi
  bear -- cmake --build build
  if [ -f compile_commands.json.bak ]; then
      old_commands_count=$(cat compile_commands.json.bak | jq length)
      new_commands_count=$(cat compile_commands.json | jq length)
      if [ $new_commands_count -gt $old_commands_count ]; then
          rm compile_commands.json.bak
      else
          mv compile_commands.json.bak compile_commands.json
      fi
  fi
else
  cmake --build build
fi
