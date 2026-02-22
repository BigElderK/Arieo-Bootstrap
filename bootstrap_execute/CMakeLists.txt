cmake_minimum_required(VERSION 3.31)


if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    ARIEO_ENGINE_PROJECT(
        arieo_bootstrap
        PROJECT_TYPE bootstrap

        DEPENDENCIES
            ARIEO_PACKAGES
                Arieo-Core
                Arieo-Interface-RHI
                Arieo-Interface-Archive
                Arieo-Interface-FileLoader
            THIRDPARTY_PACKAGES
                gtest
                yaml-cpp
            PRIVATE_LIBS
                Arieo-Interface-RHI::arieo_rhi_interface
                Arieo-Interface-Archive::arieo_archive_interface
                Arieo-Interface-FileLoader::arieo_file_loader_interface
                Arieo-Core::arieo_core
                yaml-cpp::yaml-cpp

        SOURCES
            CXX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    ARIEO_ENGINE_PROJECT(
        arieo_bootstrap
        PROJECT_TYPE bootstrap

        DEPENDENCIES
            ARIEO_PACKAGES
                Arieo-Core
                Arieo-Interface-RHI
                Arieo-Interface-Archive
                Arieo-Interface-FileLoader
            THIRDPARTY_PACKAGES
                gtest
                yaml-cpp
            PRIVATE_LIBS
                Arieo-Interface-RHI::arieo_rhi_interface
                Arieo-Interface-Archive::arieo_archive_interface
                Arieo-Interface-FileLoader::arieo_file_loader_interface
                Arieo-Core::arieo_core
                yaml-cpp::yaml-cpp
            EXTERNAL_LIBS
                $ENV{DEVCRAFT_WORKSPACE}/llvm/llvm-21/${TARGET_TRIPLE}/lib/${CMAKE_SYSTEM_PROCESSOR}-unknown-linux-gnu/*

        SOURCES
            CXX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    ARIEO_ENGINE_PROJECT(
        arieo_bootstrap
        PROJECT_TYPE bootstrap

        DEPENDENCIES
            ARIEO_PACKAGES
                Arieo-Core
                Arieo-Interface-RHI
                Arieo-Interface-Archive
                Arieo-Interface-FileLoader
            THIRDPARTY_PACKAGES
                gtest
                yaml-cpp
            PRIVATE_LIBS
                Arieo-Interface-RHI::arieo_rhi_interface
                Arieo-Interface-Archive::arieo_archive_interface
                Arieo-Interface-FileLoader::arieo_file_loader_interface
                Arieo-Core::arieo_core
                yaml-cpp::yaml-cpp
            EXTERNAL_LIBS
                ${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-android/*.so

        SOURCES
            CXX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    ARIEO_ENGINE_PROJECT(
        arieo_bootstrap
        PROJECT_TYPE bootstrap

        DEPENDENCIES
            ARIEO_PACKAGES
                Arieo-Core
                Arieo-Interface-RHI
                Arieo-Interface-Archive
                Arieo-Interface-FileLoader
            THIRDPARTY_PACKAGES
                gtest
                yaml-cpp
            PRIVATE_LIBS
                Arieo-Interface-RHI::arieo_rhi_interface
                Arieo-Interface-Archive::arieo_archive_interface
                Arieo-Interface-FileLoader::arieo_file_loader_interface
                Arieo-Core::arieo_core
                yaml-cpp::yaml-cpp

        SOURCES
            CXX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*.cpp
    )
endif()

# Set RPATH to find libc++.so in libs folder
# Use readelf -d /path/to/arieo_bootstrap to verify
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set_target_properties(arieo_bootstrap PROPERTIES
  BUILD_RPATH "\$ORIGIN/libs"
  INSTALL_RPATH "\$ORIGIN/libs"
)

# Copy libc++ to output dir for android builds
# if(ARIEO_HOST_OS STREQUAL "android")
#     # Copy libc++ to output dir, make build failed if not present
#     set(libcxx_src_file "${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-android/libc++_shared.so")
#     set(libcxx_output_file "${ARIEO_LIBS_OUTPUT_DIRECTORY}/libc++_shared.so")
#     add_custom_command(
#         OUTPUT ${libcxx_output_file}
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             ${libcxx_src_file}
#             ${libcxx_output_file}
#         DEPENDS ${libcxx_src_file}
#     )
#     add_custom_target(
#         CopyLibCXXShared
#         DEPENDS ${libcxx_output_file}
#     )
#     add_dependencies(
#         arieo_bootstrap
#         CopyLibCXXShared
#     )
# elseif(ARIEO_HOST_OS STREQUAL "ubuntu" OR ARIEO_HOST_OS STREQUAL "raspberry")
#     # Copy libc++ to output dir, make build failed if not present
#     set(libcxx_src_file "$ENV{DEVCRAFT_WORKSPACE}/llvm/llvm-21/${TARGET_TRIPLE}/lib/${CMAKE_SYSTEM_PROCESSOR}-unknown-linux-gnu/libc++.so")
#     set(libcxx_output_file "${ARIEO_LIBS_OUTPUT_DIRECTORY}/libc++.so")
#     add_custom_command(
#         OUTPUT ${libcxx_output_file}
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             ${libcxx_src_file}
#             ${libcxx_output_file}
#         DEPENDS ${libcxx_src_file}
#     )
#     add_custom_target(
#         CopyLibCXXShared
#         DEPENDS ${libcxx_output_file}
#     )
#     add_dependencies(
#         arieo_bootstrap
#         CopyLibCXXShared
#     )
# endif()


