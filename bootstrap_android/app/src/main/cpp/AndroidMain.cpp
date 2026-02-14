// Copyright 2022 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <dlfcn.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <jni.h>
#include <unistd.h>

#define LOG_TAG "ArieoEngine"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// Process the next main command
void handle_cmd(android_app *app, int32_t cmd)
{
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        // InitVulkan(app);
        LOGI("APP_CMD_INIT_WINDOW");
        break;
    case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        // DeleteVulkan();
        break;
    default:
        __android_log_print(ANDROID_LOG_INFO, "Arieo",
                            "event not handled: %d", cmd);
    }
}

typedef int (*MAIN_ENTRY_INIT_FUN)(void*);
typedef bool (*MAIN_ENTRY_TICK_FUN)();
typedef void (*MAIN_ENTRY_DEINIT_FUN)();

// JNI function to set real system environment variables
extern "C" JNIEXPORT jboolean JNICALL
Java_com_arieo_bootstrap_MainActivity_nativeSetEnvironmentVariable(JNIEnv *env, jobject /* this */, jstring name, jstring value) {
    const char *env_name = env->GetStringUTFChars(name, nullptr);
    const char *env_value = env->GetStringUTFChars(value, nullptr);
    
    if (env_name && env_value) {
        // Set the environment variable using setenv
        int result = setenv(env_name, env_value, 1); // 1 means overwrite if exists
        
        LOGI("Setting environment variable %s = %s", env_name, env_value);
        
        env->ReleaseStringUTFChars(name, env_name);
        env->ReleaseStringUTFChars(value, env_value);
        
        return (result == 0) ? JNI_TRUE : JNI_FALSE;
    }
    
    if (env_name) env->ReleaseStringUTFChars(name, env_name);
    if (env_value) env->ReleaseStringUTFChars(value, env_value);
    
    return JNI_FALSE;
}

void android_main(android_app *app)
{
    int events;
    android_poll_source *source;
    
    LOGI("Enter android_main");
    LOGI("Waiting for app window to be initialized");
    {
        static bool app_window_initialized = false;
        // Captureless lambda that uses userData
        app->onAppCmd = [](android_app* app, int32_t cmd) 
        {    
            switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                LOGI("APP_CMD_INIT_WINDOW");
                app_window_initialized = true;
                break;
            case APP_CMD_TERM_WINDOW:
                break;
            default:
                break;
            }
        };

        // Wait for the window to be initialized
        while (true)
        {
            if (ALooper_pollOnce(1, nullptr,
                                &events, (void **)&source) >= 0)
            {
                if (source != NULL)
                    source->process(app, source);
            }

            if(app->window != nullptr && app_window_initialized == true)
            {
                break;
            }

            if(app->destroyRequested != 0)
            {
                return;
            }
        }

        // check if app->window is valid
        if (app->window == nullptr || app_window_initialized == false)
        {
            LOGE("App window is null on start");
            return;
        }
    }

    // The window is being shown, get it ready.
    LOGI("App window initialized");

    LOGI("Waiting for manifest_path to be set by Java");
    const char* app_manifest_path = nullptr;
    {
        while (true)
        {
            if (ALooper_pollOnce(1, nullptr,
                                &events, (void **)&source) >= 0)
            {
                if (source != NULL)
                    source->process(app, source);
            }

            // Check if APP_MANIFEST_PATH environment variable is set (should be set by Java JNI call)
            app_manifest_path = getenv("APP_MANIFEST_PATH");
            if (app_manifest_path != nullptr) 
            {
                LOGI("APP_MANIFEST_PATH environment variable is set to: %s", app_manifest_path);
                break;
            } 
            else 
            {
                LOGW("APP_MANIFEST_PATH environment variable is not set - waiting for Java to set it");
            }

            if(app->destroyRequested != 0)
            {
                return;
            }
        }
    }
    
    LOGI("Got manifest_path: %s", app_manifest_path);
    LOGI("Loading main entry library");
    MAIN_ENTRY_INIT_FUN main_entry_init_fun = nullptr;
    MAIN_ENTRY_TICK_FUN main_entry_tick_fun = nullptr;
    MAIN_ENTRY_DEINIT_FUN main_entry_uninit_fun = nullptr;
    {
        void *main_entry_lib = nullptr;
        {
            main_entry_lib = dlopen("libarieo_main_module.so", RTLD_LAZY | RTLD_GLOBAL);
            if (main_entry_lib == nullptr)
            {
                LOGE("Cannot load libarieo_main_module.so: %s", dlerror());
                return;
            }
        }

        main_entry_init_fun = (MAIN_ENTRY_INIT_FUN)dlsym(
            main_entry_lib,
            "MainEntry_Init");

        if (main_entry_init_fun == nullptr)
        {
            LOGE("Cannot load MAIN_ENTRY_INIT_FUN from arieo_main_entry.dll");
            return;
        }

        main_entry_tick_fun = (MAIN_ENTRY_TICK_FUN)dlsym(
            main_entry_lib,
            "MainEntry_Tick");

        if (main_entry_tick_fun == nullptr)
        {
            LOGE("Cannot load MAIN_ENTRY_TICK_FUN from arieo_main_entry.dll");
            return;
        }

        main_entry_uninit_fun = (MAIN_ENTRY_DEINIT_FUN)dlsym(
            main_entry_lib,
            "MainEntry_Deinit");

        if (main_entry_uninit_fun == nullptr)
        {
            LOGE("Cannot load MAIN_ENTRY_DEINIT_FUN from arieo_main_entry.dll");
            return;
        }
    }

    LOGI("Start running engine");
    main_entry_init_fun(app);
    while (true)
    {
        if (ALooper_pollOnce(1, nullptr,
                            &events, (void **)&source) >= 0)
        {
            if (source != NULL)
                source->process(app, source);
        }

        if(main_entry_tick_fun() == false)
        {
            break;
        }

        if(app->destroyRequested != 0)
        {
            break;
        }
    }
    main_entry_uninit_fun();
}
