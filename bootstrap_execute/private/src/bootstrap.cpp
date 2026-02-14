#include "base/prerequisites.h"
#include "core/core.h"
#include "core/config/config.h"
#include "stdio.h"
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

    std::filesystem::path main_entry_lib_path = Base::StringUtility::format(
        "{}/{}",
        Arieo::Core::SystemUtility::FileSystem::getFormalizedPath("${EXE_DIR}/modules"), 
        Core::SystemUtility::Lib::getDymLibFileName("arieo_main_module")
    );

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