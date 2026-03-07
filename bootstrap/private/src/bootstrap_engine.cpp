#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "core/manifest/manifest.h"
#include "bootstrap_engine.h"

using namespace Arieo;

typedef int (*MAIN_ENTRY_FUN)(void*, const char*);
Core::Manifest g_manifest;

Core::Manifest& prepareEngine(std::string manifest_path)
{
    Core::Logger::info("Preparing engine with manifest path: {}", manifest_path);
    std::filesystem::path manifest_file_path = std::filesystem::absolute(std::filesystem::path(manifest_path));
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_PATH", manifest_file_path.string());
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_DIR", manifest_file_path.parent_path().string());
    
    Core::Logger::info("Loading application manifest from path: {}", manifest_file_path.string());

    g_manifest.loadFromFile(manifest_file_path);
    g_manifest.applyPresetEnvironments();

    Core::Logger::info("Application manifest loaded and preset environments applied.");

    // Use default engine path (relative to executable)
    std::filesystem::path engine_install_path = g_manifest.getEnginePath();
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("ARIEO_ENGINE_PATH", engine_install_path.string());

    Core::Logger::info("Engine path set to: {}", engine_install_path.string());

    std::filesystem::path main_module_path = g_manifest.getEngineModulePath("main_module");
    if(main_module_path.empty())
    {
        Core::Logger::fatal("Main entry module path is empty in manifest. Please specify the main entry module path in manifest under engine_modules with key 'main_module'.");
        return g_manifest;
    }

    // Load main entry module from manifest-specified path
    main_module_path = main_module_path.is_absolute() ? main_module_path : (engine_install_path / main_module_path);
    Core::Logger::info("Main entry module path set to: {}", main_module_path.string());
    
    return g_manifest;
}

std::string getMainModulePath()
{
    return g_manifest.getEngineModulePath("main_module").string();
}

Core::Manifest& getManifest()
{
    return g_manifest;
}

int bootstrapEngine()
{
    std::string main_module_path = g_manifest.getEngineModulePath("main_module").string();
    if(main_module_path.empty())
    {
        Core::Logger::fatal("Main module path is empty. Please call prepareEngine first to set up the main module path.");
        return -1;
    }
    
    Core::Logger::info("1 Loading main entry module from path: {}", main_module_path);

    Arieo::Core::SystemUtility::Lib::LIBTYPE main_module_lib = Arieo::Core::SystemUtility::Lib::loadLibrary(
        main_module_path
    );

    Core::Logger::info("1 End Loading main entry module from path: {}", main_module_path);

    if(main_module_lib == nullptr)
    {
        const char* dl_error = Arieo::Core::SystemUtility::Lib::getLastError();
        Core::Logger::info("Failed to load main entry module from path: {} | dlerror: {}",
            main_module_path,
            dl_error != nullptr ? dl_error : "(no error message)");
        return -1;
    }

    Core::Logger::info("Main entry module loaded successfully.");

    MAIN_ENTRY_FUN main_entry_fun = (MAIN_ENTRY_FUN)Arieo::Core::SystemUtility::Lib::getProcAddress(
        main_module_lib, 
        "MainEntry"
    );

    return main_entry_fun(nullptr, g_manifest.getManifestContext().c_str());
}





