#include "AssetUtils.h"
#include <algorithm>
#include <fstream>
#include <SDL_log.h>

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

static bool toivec2(glm::ivec2& result, const std::string& str, char delim = ',') {
    auto pos = str.find_first_of(delim, 0);
    if (pos == str.npos)
        return false;
    
    result.x = std::stoi(str.substr(0, pos));
    result.y = std::stoi(str.substr(pos + 1));
    return true;
}

bool AtlasMgr::LoadImpl(const char* jsonFn, AtlasMgr* ptr) {
    std::string path = GetDataPath(jsonFn);
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open()) {
        SDL_Log("Could not open the file %s\n", path.c_str());
        return false;
    }

    nlohmann::json config;
    jsonFile >> config;

    if (!ptr->LoadJson(config))
        return false;

    return true;
}

const glm::ivec4* IconMgr::GetRect(const char* name) const {
    auto iter = m_Map.find(name);
    if (iter == m_Map.end())
        return nullptr;
    return &iter->second;
}

bool IconMgr::LoadJson(const nlohmann::json& data) {
    if (!data.is_array())
        return false;

    for (size_t i = 0; i < data.size(); i++) {
        auto name = data[i].at("type").get<std::string>();
        glm::ivec2 offset, size;
        toivec2(offset, data[i].at("offset").get<std::string>());
        toivec2(size, data[i].at("size").get<std::string>());
        m_Map[name] = glm::ivec4(offset, size);
    }

    return true;
}
