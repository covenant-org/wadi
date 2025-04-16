cmake_minimum_required(VERSION 3.5)
include(CMakePrintHelpers)

project(wadi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(LIBWEBRTC_PATH "${CMAKE_SOURCE_DIR}/libs/webrtc/ubuntu-22.04-amd64/libwebrtc.a")
set(TARGET_LIBS "")
set(TARGET_INCLUDE_DIRS include
	include/webrtc
	include/webrtc/third_party/abseil-cpp)
file(GLOB_RECURSE CPP_SOURCE_FILES src/*.cpp)

execute_process(
	COMMAND bash -c "uname -m"
	OUTPUT_VARIABLE SYSTEM_ARCH)

if(${SYSTEM_ARCH} MATCHES ^aarch64)
	set(SYSTEM_AARCH64 TRUE)
endif()

if(SYSTEM_AARCH64)
	set(LIBWEBRTC_PATH "${CMAKE_SOURCE_DIR}/libs/webrtc/ubuntu-tegra-aarch64/libwebrtc.a")
	add_definitions(-DWEBRTC_POSIX=1)
	add_definitions(-D__STDC_CONSTANT_MACROS=1)
	list(APPEND TARGET_LIBS "X11" "Xrandr")

	if(EXISTS "/usr/src/jetson_multimedia_api/")
		set(JETSON_MULTIMEDIA_API TRUE)
		set(MULTIMEDIA_API_TOP_DIR "/usr/src/jetson_multimedia_api")

		if(EXISTS "/dev/nvhost-msenc")
			set(HW_ENCODING_SUPPORT TRUE)
			target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC HW_ENCODING_SUPPORT=1)

			# Location of the CUDA Toolkit
			set(CUDA_PATH "/usr/local/cuda")

			set(MULTIMEDIA_CLASS_DIR "${MULTIMEDIA_API_TOP_DIR}/samples/common/classes")
			set(ALGO_CUDA_DIR "${MULTIMEDIA_API_TOP_DIR}/samples/common/algorithm/cuda")
			set(ALGO_TRT_DIR "${MULTIMEDIA_API_TOP_DIR}/samples/common/algorithm/trt")

			list(APPEND TARGET_LIBS 
				pthread
				nvv4l2
				EGL
				GLESv2
				nvbufsurface
				nvbufsurftransform
				nvjpeg
				nvosd
				drm
				cuda
				cudart
			)


			link_directories(
				"${CUDA_PATH}/lib64"
				"/usr/lib/${TEGRA_ARMABI}/nvidia"
				"/usr/lib/${TEGRA_ARMABI}/nvidia/tegra")

			list(APPEND TARGET_INCLUDE_DIRS
				"${MULTIMEDIA_API_TOP_DIR}/include"
				"${MULTIMEDIA_API_TOP_DIR}/include/libjpeg-8b"
				${ALGO_CUDA_DIR}
				${ALGO_TRT_DIR}
				"${CUDA_PATH}/include"
				"/usr/include/${TEGRA_ARMABI}" 
				"/usr/include/libdrm" 
				"/usr/include/opencv4"
				)

			file(GLOB MULTIMEDIA_CLASS_SOURCE_FILES "${MULTIMEDIA_CLASS_DIR}/*.cpp")
			list(APPEND CPP_SOURCE_FILES ${MULTIMEDIA_CLASS_SOURCE_FILES})
		else()
			message(WARNING "Could not find hardware video encoding /dev/nvhost-msenc")
			list(FILTER CPP_SOURCE_FILES EXCLUDE REGEX jetson_encoder.cpp$)
		endif()
	endif()

endif()

list(PREPEND TARGET_LIBS ${LIBWEBRTC_PATH})

cmake_print_variables(LIBWEBRTC_PATH)
cmake_print_variables(TARGET_LIBS)
cmake_print_variables(TARGET_INCLUDE_DIRS)
cmake_print_variables(CPP_SOURCE_FILES)

add_executable(${CMAKE_PROJECT_NAME} ${CPP_SOURCE_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${TARGET_LIBS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${TARGET_INCLUDE_DIRS})
