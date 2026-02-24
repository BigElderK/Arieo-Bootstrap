cmake_minimum_required (VERSION 3.31)

include($ENV{ARIEO_BUILD_ENV_PACKAGES_OUTPUT_DIR}/cmake/build_environment.cmake)

ARIEO_PACKAGE(
    Arieo-Bootstrap
    CATEGORY 03_bootstrap
    DEPENDS
        https://github.com/BigElderK/Arieo-Core.git@main
        https://github.com/BigElderK/Arieo-Interface-RHI.git@main
        https://github.com/BigElderK/Arieo-Interface-Archive.git@main
        https://github.com/BigElderK/Arieo-Interface-FileLoader.git@main
    COMPONENTS
        arieo_bootstrap_android
)

project(ArieoBootstrap)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootstrap_android)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootstrap)
