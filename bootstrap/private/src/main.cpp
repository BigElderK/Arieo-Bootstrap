#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"

#include "bootstrap_engine.h"

#if defined(ARIEO_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
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
        // Download manifest from the HTTP server into the virtual filesystem.
        // emscripten_wget is available because the binary is built with -sASYNCIFY.
        const char* manifest_url = "/04_applications/Arieo-Application-RenderTest/application.unified/arieo_render_test_app/app.manifest.yaml";
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





