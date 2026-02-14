cmake_minimum_required (VERSION 3.31)

include($ENV{ARIEO_BUILDENV_PACKAGE_INSTALL_FOLDER}/cmake/build_environment.cmake)

project(ArieoBootstrap)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootstrap_android)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootstrap_execute)