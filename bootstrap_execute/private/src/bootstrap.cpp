#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"
#include "stdio.h"
#include <fstream>
#include <sstream>
#include <string_view>
using namespace Arieo;

typedef int (*MAIN_ENTRY_FUN)(void*);

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

    std::filesystem::path manifest_file_path = std::filesystem::absolute(std::filesystem::path(manifest_path));
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_PATH", manifest_file_path.string());
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_DIR", manifest_file_path.parent_path().string());
    
    Core::Manifest manifest;
    manifest.loadFromFile(manifest_file_path);
    manifest.applyPresetEnvironments();

    // Use default engine path (relative to executable)
    std::filesystem::path engine_install_path = manifest.getEnginePath();
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("ARIEO_ENGINE_PATH", engine_install_path.string());

    // Load main entry module from manifest-specified path
    std::filesystem::path main_module_path = manifest.getEngineModulePath("main_module");
    Arieo::Core::SystemUtility::Lib::LIBTYPE main_entry_lib = Arieo::Core::SystemUtility::Lib::loadLibrary(
        main_module_path
    );

    if(main_entry_lib == nullptr)
    {
        Core::Logger::fatal("Failed to load main entry module from path: {}", main_module_path.string());
        return -1;
    }

    MAIN_ENTRY_FUN main_entry_fun = (MAIN_ENTRY_FUN)Arieo::Core::SystemUtility::Lib::getProcAddress(
        main_entry_lib, 
        "MainEntry"
    );

    return main_entry_fun(nullptr);
}