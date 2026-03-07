#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string_view>
#include "base/prerequisites.h"
#include "core/core.h"
#include "core/manifest/manifest.h"

Arieo::Core::Manifest& prepareEngine(std::string manifest_path);
Arieo::Core::Manifest& getManifest();
std::string getMainModulePath();
int bootstrapEngine();




