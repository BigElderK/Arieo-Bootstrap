#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"
#include "bootstrap_engine.h"

using namespace Arieo;

typedef int (*MAIN_ENTRY_FUN)(void*);
std::filesystem::path g_main_module_path;

int prepareEngine(std::string manifest_path)
{
    Core::Logger::setDefaultLogger("bootstrap");
    Core::Logger::info("Preparing engine with manifest path: {}", manifest_path);
    std::filesystem::path manifest_file_path = std::filesystem::absolute(std::filesystem::path(manifest_path));
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_PATH", manifest_file_path.string());
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_DIR", manifest_file_path.parent_path().string());
    
    Core::Logger::info("Loading application manifest from path: {}", manifest_file_path.string());

    Core::Manifest manifest;
    manifest.loadFromFile(manifest_file_path);
    manifest.applyPresetEnvironments();

    Core::Logger::info("Application manifest loaded and preset environments applied.");

    // Use default engine path (relative to executable)
    std::filesystem::path engine_install_path = manifest.getEnginePath();
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("ARIEO_ENGINE_PATH", engine_install_path.string());

    Core::Logger::info("Engine path set to: {}", engine_install_path.string());

    if(g_main_module_path.empty() == false)
    {
        Core::Logger::fatal("Main entry module is already loaded. Bootstrap engine should only be called once.");
        return -1;
    }

    // Load main entry module from manifest-specified path
    g_main_module_path = manifest.getEngineModulePath("main_module");
    Core::Logger::info("Main entry module path set to: {}", g_main_module_path.string());
    
    return 0;
}

std::string getMainModulePath()
{
    if(g_main_module_path.empty())
    {
        Core::Logger::fatal("Main module path is empty. Please call prepareEngine first to set up the main module path.");
        return "";
    }
    
    return g_main_module_path.string();
}

int bootstrapEngine()
{
    if(g_main_module_path.empty())
    {
        Core::Logger::fatal("Main module path is empty. Please call prepareEngine first to set up the main module path.");
        return -1;
    }
    
    Core::Logger::info("Loading main entry module from path: {}", g_main_module_path.string());

    Arieo::Core::SystemUtility::Lib::LIBTYPE main_module_lib = Arieo::Core::SystemUtility::Lib::loadLibrary(
        g_main_module_path
    );

    if(main_module_lib == nullptr)
    {
        Core::Logger::fatal("Failed to load main entry module from path: {}", g_main_module_path.string());
        return -1;
    }

    Core::Logger::info("Main entry module loaded successfully.");

    MAIN_ENTRY_FUN main_entry_fun = (MAIN_ENTRY_FUN)Arieo::Core::SystemUtility::Lib::getProcAddress(
        main_module_lib, 
        "MainEntry"
    );

    return main_entry_fun(nullptr);
}
