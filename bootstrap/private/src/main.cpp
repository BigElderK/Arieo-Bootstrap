#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"

#include "bootstrap_engine.h"

using namespace Arieo;
int main(int argc, char *argv[])
{
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

    if (manifest_path.empty())
    {
        Core::Logger::fatal("Usage: {} --manifest=<manifest-path>", argv[0]);
        return -1;
    }

    prepareEngine(manifest_path);
    return bootstrapEngine();
}
