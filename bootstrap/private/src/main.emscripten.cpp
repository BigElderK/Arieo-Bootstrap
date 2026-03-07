#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"

#include "bootstrap_engine.h"

#if defined(ARIEO_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/em_js.h>

EM_JS(int, js_get_manifest_url_param, (char* buf, int buf_size), {
    var params = new URLSearchParams(window.location.search);
    var val = params.get('manifest');
    if (!val) return 0;
    var len = Math.min(val.length, buf_size - 1);
    stringToUTF8(val.substring(0, len), buf, buf_size);
    return 1;
});

using namespace Arieo;
int main(int argc, char *argv[])
{    
    Core::Logger::setDefaultLogger("bootstrap");

    // Read ?manifest=<url> from the page query string via a linked JS function.
    char url_param[2048] = {};
    int has_url_param = js_get_manifest_url_param(url_param, (int)sizeof(url_param));

    // Fall back to a hard-coded default if no query param was given.
    const char* manifest_url = has_url_param
        ? url_param
        : nullptr; // No default URL, require it to be provided via query string.

    if (manifest_url == nullptr)
    {
        Core::Logger::fatal("Usage: {} --manifest=<manifest-path>", argv[0]);
        return -1;
    }

    // Append a cache-busting timestamp so the browser never serves a cached copy.
    std::string manifest_url_nocache_str = Base::StringUtility::format("{}?t={}", manifest_url, long(emscripten_get_now()));
    {
        const char* manifest_vfs = "/app/app.manifest.yaml";
        std::filesystem::create_directories(std::filesystem::path(manifest_vfs).parent_path());
        emscripten_wget(manifest_url_nocache_str.c_str(), manifest_vfs);

        // Verify the file landed in the VFS (emscripten_wget is void — no return code).
        if (FILE* f = fopen(manifest_vfs, "r")) { fclose(f); }
        else
        {
            Core::Logger::fatal("emscripten_wget failed: could not open {} in VFS (URL: {})",
                manifest_vfs, manifest_url_nocache_str);
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

        Core::Logger::info("Preloading module from URL: {} to {}", module_url_no_cache, formalized_module_path);
        std::filesystem::create_directories(formalized_module_path.parent_path());
        emscripten_wget(module_url_no_cache.c_str(), formalized_module_path.string().c_str());

        if (FILE* f = fopen(formalized_module_path.string().c_str(), "r")) { fclose(f); }
        else
        {
            Core::Logger::fatal("emscripten_wget failed: could not open {} in VFS (URL: {})",
                formalized_module_path.string(), module_url_no_cache);
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