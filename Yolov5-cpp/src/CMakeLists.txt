# Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.

# CMake lowest version requirement
cmake_minimum_required(VERSION 3.5.1)

# project information
project(BA9_CPLUS_APP)

# Compile options
add_compile_options(-std=c++11)

add_definitions(-DENABLE_DVPP_INTERFACE)

# Specify target generation path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "../../../out")
set(CMAKE_CXX_FLAGS_DEBUG "-fPIC -O0 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -O2 -Wall")
set(CMAKE_BUILD_TYPE Release)

if (NOT DEFINED ENV{INSTALL_DIR})
    MESSAGE(FATAL_ERROR "Not Defined INSTALL_DIR")
endif()

include_directories(
	$ENV{INSTALL_DIR}/runtime/include/
        /home/zmd/Prj_310/ba9/cplus/yolov5cpp/inc/
        /home/zmd/Prj_310/usr_inc/
	)

# add host lib path
link_directories(
        /home/zmd/Prj_310/ba9/cplus/yolov5cpp/lib/
	$ENV{INSTALL_DIR}/runtime/lib64/stub
        /usr/lib/aarch64-linux-gnu/
        
	)

add_executable(main
        main.cpp
        inference.cpp
        model_process.cpp
        udpTransmit.cpp
        )

target_link_libraries(main
         ascendcl acl_cblas acl_dvpp stdc++ opencv_core opencv_imgproc opencv_imgcodecs pthread devmmap drvdsmi ascend_hal slog c_sec)
#        ascendcl acl_cblas acl_dvpp stdc++ opencv_core opencv_imgproc opencv_imgcodecs pthread drvdsmi ascend_hal slog c_sec devmmap)

install(TARGETS main DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
