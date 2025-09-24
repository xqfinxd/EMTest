#include "AssetUtils.h"
#include <algorithm>

std::string GetTexPath(const char* name_) {
    std::string name(name_);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return std::string("assets/textures/") + name + ".png";
}

std::string GetDataPath(const char* name_) {
    std::string name(name_);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return std::string("assets/datas/") + name;
}
