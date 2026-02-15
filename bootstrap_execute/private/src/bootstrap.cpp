#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "stdio.h"
#include <fstream>
#include <sstream>
using namespace Arieo;

typedef int (*MAIN_ENTRY_FUN)(void*);

int main(int argc, char *argv[])
{
    // load manifest from first argument
    if (argc < 2)
    {
        printf("Usage: %s <manifest_path>\n", argv[0]);
        return -1;
    }

    std::filesystem::path manifest_file_path = std::filesystem::absolute(std::filesystem::path(argv[1]));
    Arieo::Core::SystemUtility::Environment::setEnvironmentValue("APP_MANIFEST_PATH", manifest_file_path.string());

    // Load main entry module from manifest-specified path
    std::string main_module_path;
    {
        // Read manifest file content
        std::ifstream manifest_file(manifest_file_path);
        if (!manifest_file.is_open())
        {
            printf("Cannot open manifest file: %s\r\n", manifest_file_path.string().c_str());
            return -1;
        }
        std::stringstream buffer;
        buffer << manifest_file.rdbuf();
        std::string manifest_content = buffer.str();

        // Parse YAML
        Core::ConfigNode config_node = Core::ConfigFile::Load(manifest_content);
        if (config_node.IsNull())
        {
            printf("Failed to parse manifest file: %s\r\n", manifest_file_path.string().c_str());
            return -1;
        }

        // Get host OS name and navigate to the appropriate node
        std::string host_os_name = Core::SystemUtility::getHostOSName();
        Core::ConfigNode system_node = config_node["app"]["host_os"][host_os_name];
        if (!system_node.IsDefined())
        {
            printf("Cannot find 'app.host_os.%s' in manifest: %s\r\n", host_os_name.c_str(), manifest_file_path.string().c_str());
            return -1;
        }

        // Set environments from manifest before loading any libs
        Core::ConfigNode env_node = system_node["environments"];
        if (env_node.IsDefined())
        {
            for (auto env_iter = env_node.begin(); env_iter != env_node.end(); ++env_iter)
            {
                std::string env_name = env_iter->first.as<std::string>();
                if (env_iter->second.IsSequence())
                {
                    // Prepend to existing environment
                    for (auto value_iter = env_iter->second.begin(); value_iter != env_iter->second.end(); ++value_iter)
                    {
                        std::string append_value = value_iter->as<std::string>();
                        Core::SystemUtility::Environment::prependEnvironmentValue(
                            env_name,
                            Core::SystemUtility::FileSystem::getFormalizedPath(append_value));
                    }
                }
                else
                {
                    // Set environment value
                    std::string env_value = env_iter->second.as<std::string>();
                    Core::SystemUtility::Environment::setEnvironmentValue(
                        env_name,
                        Core::SystemUtility::FileSystem::getFormalizedPath(env_value));
                }
            }
        }

        // Get main_module path from manifest
        Core::ConfigNode main_module_node = system_node["main_module"];
        if (!main_module_node.IsDefined())
        {
            printf("Cannot find 'app.host_os.%s.main_module' in manifest: %s\r\n", host_os_name.c_str(), manifest_file_path.string().c_str());
            return -1;
        }
        main_module_path = main_module_node.as<std::string>();
    }

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