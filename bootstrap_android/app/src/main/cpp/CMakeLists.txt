#[[
Copyright 2022 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
]]
cmake_minimum_required(VERSION 3.4.1)

project(arieo_bootstrap)

# Set path to your prebuilt static libraries
set(EXTRA_LIB_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a)

# Integrate GameActivity, refer to
#     https://d.android.com/games/agdk/integrate-game-activity
# for the detailed instructions.
find_package(game-activity REQUIRED CONFIG)

# build vulkan app

add_library(${CMAKE_PROJECT_NAME} SHARED
    AndroidMain.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
# message(FATAL_ERROR "WOKAO CXX: ${CMAKE_SHARED_LINKER_FLAGS}\r\n links: ${CMAKE_CXX_FLAGS}")
# set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Enable verbose output from Makefile builds" FORCE)

# No longer need minizip since we use Java ZIP APIs via JNI

target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    game-activity::game-activity_static
    log
    android
)

set_target_properties(arieo_bootstrap PROPERTIES LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")