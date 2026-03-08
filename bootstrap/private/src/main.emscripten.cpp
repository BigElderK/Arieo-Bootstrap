#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"

#include "bootstrap_engine.h"

#if defined(ARIEO_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/em_js.h>
#include <emscripten/fetch.h>

EM_JS(int, js_get_manifest_url_param, (char* buf, int buf_size), {
    // With -sPROXY_TO_PTHREAD=1, main() runs on a Web Worker where window is
    // not defined. Guard here so this falls through to the argv path instead.
    if (typeof window === 'undefined') return 0;
    var params = new URLSearchParams(window.location.search);
    var val = params.get('manifest');
    if (!val) return 0;
    var len = Math.min(val.length, buf_size - 1);
    stringToUTF8(val.substring(0, len), buf, buf_size);
    return 1;
});

// Synchronously fetch a URL and write it into the Emscripten virtual file system.
// Uses EMSCRIPTEN_FETCH_SYNCHRONOUS which blocks the calling thread via Atomics.wait.
// Must be called from a pthread worker thread, NOT the main browser thread.
static bool sync_fetch_to_vfs(const char* url, const char* vfs_path)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
    emscripten_fetch_t* fetch = emscripten_fetch(&attr, url);
    const bool ok = (fetch->status == 200);
    if (ok)
    {
        std::filesystem::create_directories(std::filesystem::path(vfs_path).parent_path());
        if (FILE* f = fopen(vfs_path, "wb"))
        {
            fwrite(fetch->data, 1, (size_t)fetch->numBytes, f);
            fclose(f);
        }
    }
    emscripten_fetch_close(fetch);
    return ok;
}

using namespace Arieo;
int main(int argc, char *argv[])
{    
    Core::Logger::setDefaultLogger("bootstrap");

    // -sPROXY_TO_PTHREAD replaces main() with a trampoline on the main browser
    // thread that creates a pthread and calls the original main(argc, argv) on it.
    // The trampoline copies Module['arguments'] into argv, so pre.js can pass
    // '--manifest=<url>' from window.location.search (only readable on main thread).
    // Check argv first; fall back to EM_JS for non-PROXY_TO_PTHREAD builds.
    const char* manifest_url = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--manifest=", 11) == 0)
        {
            manifest_url = argv[i] + 11;
            break;
        }
    }

    char url_param[2048] = {};
    if (!manifest_url && js_get_manifest_url_param(url_param, (int)sizeof(url_param)))
    {
        manifest_url = url_param;
    }

    if (manifest_url == nullptr)
    {
        Core::Logger::fatal("Usage: {} --manifest=<manifest-path>", argv[0]);
        return -1;
    }

    // Append a cache-busting timestamp so the browser never serves a cached copy.
    std::string manifest_url_nocache_str = Base::StringUtility::format("{}?t={}", manifest_url, long(emscripten_get_now()));
    {
        const char* manifest_vfs = "/app/app.manifest.yaml";
        if (!sync_fetch_to_vfs(manifest_url_nocache_str.c_str(), manifest_vfs))
        {
            Core::Logger::fatal("Failed to download manifest from: {}", manifest_url_nocache_str);
            return -1;
        }

        prepareEngine(std::string(manifest_vfs));
    }

    Core::Manifest& manifest = getManifest();

    manifest.applyPresetEnvironments();
    // set web root environment variable for engine to locate resources
    Core::SystemUtility::Environment::setEnvironmentValue("WEB_ROOT", "");
    
    // Preload module paths to ensure they are cached by the browser before main entry module tries to load them.
    for(std::filesystem::path module_path : manifest.getAllEngineModulePaths())
    {
        std::filesystem::path formalized_module_path = Core::SystemUtility::FileSystem::getFormalizedPath(module_path.string());
        std::string module_url_no_cache = Base::StringUtility::format(
            "{}?t={}", 
            formalized_module_path.string(), 
            long(emscripten_get_now())
        );

        Core::Logger::info("Preloading module: {} -> {}", module_url_no_cache, formalized_module_path.string());
        if (!sync_fetch_to_vfs(module_url_no_cache.c_str(), formalized_module_path.string().c_str()))
        {
            Core::Logger::fatal("Failed to download module from: {}", module_url_no_cache);
            return -1;
        }
    }

    // Print manifest file content
    // if (FILE* f = fopen(manifest_vfs, "r"))
    // {
    //     fseek(f, 0, SEEK_END);
    //     long size = ftell(f);
    //     rewind(f);
    //     std::string content(size, '\0');
    //     fread(content.data(), 1, static_cast<size_t>(size), f);
    //     fclose(f);
    //     Core::Logger::info("Manifest content:\n{}", content);
    // }
    // else
    // {
    //     Core::Logger::error("Failed to open manifest file: {}", manifest_vfs);
    // }

    
    return bootstrapEngine();

    // const char* manifest_vfs = "/app/app.manifest.yaml";
    // Core::Logger::info("Downloading manifest from {}", manifest_url);
    // emscripten_wget(manifest_url, manifest_vfs);
    // manifest_path = manifest_vfs;


}

#endif