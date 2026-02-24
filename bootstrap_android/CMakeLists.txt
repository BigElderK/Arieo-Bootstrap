cmake_minimum_required(VERSION 3.31)

project(Arieo-Bootstrap-Android LANGUAGES NONE)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Bootstrap Android is built via Gradle.
    # This CMakeLists.txt creates custom targets to invoke Gradle build commands.

    message(STATUS "Arieo-Bootstrap-Android: Configuring Gradle build targets")

    # Determine build type - check multi-config generators first, then single-config
    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(IS_MULTI_CONFIG)
        # Multi-config generator (Visual Studio, Ninja Multi-Config, Xcode)
        set(ANDROID_BUILD_TYPE "$<IF:$<CONFIG:Debug>,Debug,Release>")
        set(GRADLE_BUILD_TASK "$<IF:$<CONFIG:Debug>,assembleDebug,assembleRelease>")
        message(STATUS "  Multi-config generator detected, build type determined at build time")
    else()
        # Single-config generator (Ninja, Unix Makefiles)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
            set(ANDROID_BUILD_TYPE "Debug")
            set(GRADLE_BUILD_TASK "assembleDebug")
        else()
            set(ANDROID_BUILD_TYPE "Release")
            set(GRADLE_BUILD_TASK "assembleRelease")
        endif()
        message(STATUS "  Single-config generator, CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -> ${ANDROID_BUILD_TYPE}")
    endif()

    # Output directories
    set(APK_OUTPUT_DIR "$ENV{ARIEO_PACKAGES_BUILD_OUTPUT_DIR}/${ARIEO_PACKAGE_CATEGORY}/${ARIEO_PACKAGE_NAME}/${ARIEO_BUILD_CONFIGURE_PRESET}/apk")
    file(MAKE_DIRECTORY ${APK_OUTPUT_DIR})
    message(STATUS "  APK output directory: ${APK_OUTPUT_DIR}")
    
    # Determine gradlew executable based on platform
    if(WIN32)
        set(GRADLEW_CMD "${CMAKE_CURRENT_LIST_DIR}/gradlew.bat")
    else()
        set(GRADLEW_CMD "${CMAKE_CURRENT_LIST_DIR}/gradlew")
    endif()

    set(APK_DEBUG_SRC "${CMAKE_CURRENT_LIST_DIR}/app/build/outputs/apk/debug/app-debug.apk")
    set(APK_RELEASE_SRC "${CMAKE_CURRENT_LIST_DIR}/app/build/outputs/apk/release/app-release-unsigned.apk")

    set(APK_DEBUG_OUTPUT "${APK_OUTPUT_DIR}/debug/app-debug.apk")
    set(APK_RELEASE_OUTPUT "${APK_OUTPUT_DIR}/release/app-release-unsigned.apk")

    # Main build target - builds based on current configuration
    # Use --no-daemon to prevent Gradle daemon from keeping the subprocess alive after build completes
    # Use --no-build-cache to ensure Java source changes (e.g. MainActivity.java) are always recompiled
    add_custom_target(
        arieo_bootstrap_android
        COMMAND ${GRADLEW_CMD} ${GRADLE_BUILD_TASK} --no-daemon --no-build-cache
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<IF:$<CONFIG:Debug>,${APK_DEBUG_SRC},${APK_RELEASE_SRC}>
            $<IF:$<CONFIG:Debug>,${APK_DEBUG_OUTPUT},${APK_RELEASE_OUTPUT}>
        BYPRODUCTS
            ${APK_DEBUG_SRC}
            ${APK_RELEASE_SRC}
            ${APK_DEBUG_OUTPUT}
            ${APK_RELEASE_OUTPUT}
        DEPENDS ${GRADLEW_CMD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Building Arieo Bootstrap Android (${ANDROID_BUILD_TYPE})"
        VERBATIM
    )

    # Install APK to install prefix based on build type
    set(install_apk_dir "${CMAKE_INSTALL_PREFIX}/${ARIEO_PACKAGE_CATEGORY}/${ARIEO_PACKAGE_NAME}/${ARIEO_BUILD_CONFIGURE_PRESET}/apk")
    if(ANDROID_BUILD_TYPE STREQUAL "Debug")
        install(FILES ${APK_DEBUG_OUTPUT}
            DESTINATION ${install_apk_dir}/${ANDROID_BUILD_TYPE}
            OPTIONAL
        )
    else()
        install(FILES ${APK_RELEASE_OUTPUT}
            DESTINATION ${install_apk_dir}/${ANDROID_BUILD_TYPE}
            OPTIONAL
        )
    endif()
else()
    # Non-Android: create a no-op target so COMPONENTS reference is always satisfied
    add_custom_target(arieo_bootstrap_android
        COMMENT "Arieo-Bootstrap-Android: skipped (not an Android build)"
    )
endif()

