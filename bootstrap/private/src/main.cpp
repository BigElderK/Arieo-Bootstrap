#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"

#include "bootstrap_engine.h"

#if defined(ARIEO_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/em_asm.h>
#endif

using namespace Arieo;
int main(int argc, char *argv[])
{    
    Core::Logger::setDefaultLogger("bootstrap");
    // Parse command line arguments
    std::string manifest_path;

    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (arg.starts_with("--manifest="))
        {
            manifest_path = arg.substr(11);
        }
    }

    // while(true)
    // {
    //     printf("Hello world from printf\n");
    //     Core::Logger::info("Hello world from Arieo logger");
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }

    if (manifest_path.empty())
    {
#if defined(ARIEO_PLATFORM_EMSCRIPTEN)
        // Read ?manifest=<url> from the page query string if provided.
        char url_param[2048] = {};
        int has_url_param = EM_ASM_INT({
            var params = new URLSearchParams(window.location.search);
            var val = params.get('manifest');
            if (!val) return 0;
            var len = Math.min(val.length, $1 - 1);
            for (var i = 0; i < len; i++)
                HEAPU8[$0 + i] = val.charCodeAt(i);
            HEAPU8[$0 + len] = 0;
            return 1;
        }, url_param, (int)sizeof(url_param));

        // Fall back to a hard-coded default if no query param was given.
        const char* manifest_url = has_url_param
            ? url_param
            : "/04_applications/Arieo-Application-RenderTest/application.unified/arieo_render_test_app/app.manifest.yaml";
        const char* manifest_vfs = "/app/app.manifest.yaml";
        Core::Logger::info("Downloading manifest from {}", manifest_url);
        emscripten_wget(manifest_url, manifest_vfs);
        manifest_path = manifest_vfs;

        // Print manifest file content
        if (FILE* f = fopen(manifest_vfs, "r"))
        {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            rewind(f);
            std::string content(size, '\0');
            fread(content.data(), 1, static_cast<size_t>(size), f);
            fclose(f);
            Core::Logger::info("Manifest content:\n{}", content);
        }
        else
        {
            Core::Logger::error("Failed to open manifest file: {}", manifest_vfs);
        }
#else
        Core::Logger::fatal("Usage: {} --manifest=<manifest-path>", argv[0]);
        return -1;
#endif
    }
    
    prepareEngine(manifest_path);
    return bootstrapEngine();
}





