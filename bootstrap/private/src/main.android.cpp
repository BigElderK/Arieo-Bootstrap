#include "base/prerequisites.h"

#if defined(ARIEO_PLATFORM_ANDROID)
#include <android/log.h>
#include <jni.h>
#include <cstdlib>

#include "bootstrap_engine.h"

#define LOG_TAG "ArieoEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// JNI function to set real system environment variables
extern "C" JNIEXPORT jboolean JNICALL
Java_com_arieo_bootstrap_MainActivity_nativeSetEnvironmentVariable(JNIEnv *env, jobject /* this */, jstring name, jstring value) {
    const char *env_name = env->GetStringUTFChars(name, nullptr);
    const char *env_value = env->GetStringUTFChars(value, nullptr);

    if (env_name && env_value) {
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

// JNI function to set real system environment variables
extern "C" JNIEXPORT jstring JNICALL
Java_com_arieo_bootstrap_MainActivity_nativePrepareEngine(JNIEnv *env, jobject /* this */, jstring manifest_file_path) 
{
    const char *manifest_path = env->GetStringUTFChars(manifest_file_path, nullptr);

    if (manifest_path) {
        std::string manifest_path_string(manifest_path);
        
        LOGI("Received manifest file path from Java: %s", manifest_path_string.c_str());

        int result = prepareEngine(manifest_path_string);
        if(result == 0)
        {
            LOGI("Engine prepared successfully with manifest: %s", manifest_path_string.c_str());
        }
        else
        {
            LOGI("Failed to prepare engine with manifest: %s", manifest_path_string.c_str());
        }

        std::string main_module_name = getMainModulePath();

        env->ReleaseStringUTFChars(manifest_file_path, manifest_path);
        return (result == 0) ? env->NewStringUTF(main_module_name.c_str()) : nullptr;
    }

    return nullptr;
}
#endif
