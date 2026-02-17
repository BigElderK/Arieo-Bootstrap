#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
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
    std::string engine_path;

    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (arg.starts_with("--manifest="))
        {
            manifest_path = arg.substr(11);
        }
        else if (arg.starts_with("--engine-path="))
        {
            engine_path = arg.substr(14);
        }
    }

    if (manifest_path.empty())
    {
        printf("Usage: %s --manifest=<manifest-path> [--engine-path=<engine-path>]\n", argv[0]);
        return -1;
    }

    std::filesystem::path manifest_file_path = std::filesystem::absolute(std::filesystem::path(manifest_path));
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_PATH", manifest_file_path.string());

    if (engine_path.empty() == false)
    {
        std::filesystem::path engine_file_path = std::filesystem::absolute(std::filesystem::path(engine_path));
        Arieo::Core::SystemUtility::Environment::setEnvironmentValue("ARIEO_ENGINE_PATH", engine_file_path.string());
    }
    else
    {
        // Use default engine path (same directory as executable)
        std::filesystem::path exe_path = Arieo::Core::SystemUtility::FileSystem::getFormalizedPath(
            "${EXE_DIR}../../../../../02_engine"
        );
    }

    // Load main entry module from manifest-specified path
    std::string main_module_path = Arieo::Core::SystemUtility::FileSystem::getFormalizedPath(
        "$ENV{ARIEO_ENGINE_PATH}/Arieo-Module-Main/${PLATFORM}/lib/Release/arieo_main_module" + std::string(Arieo::Core::SystemUtility::Lib::getDymLibFileExtension())
    );

    std::filesystem::path main_entry_lib_path = Core::SystemUtility::FileSystem::getFormalizedPath(main_module_path);

    Arieo::Core::SystemUtility::Lib::LIBTYPE main_entry_lib = Arieo::Core::SystemUtility::Lib::loadLibrary(
        main_entry_lib_path
    );

    if(main_entry_lib == nullptr)
    {
        printf("Cannot load %s\r\n", main_entry_lib_path.string().c_str());
        return -1;
    }

    MAIN_ENTRY_FUN main_entry_tick_fun = (MAIN_ENTRY_FUN)Arieo::Core::SystemUtility::Lib::getProcAddress(
        main_entry_lib, 
        "MainEntry"
    );

    return main_entry_tick_fun(nullptr);
}