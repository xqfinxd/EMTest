#include "AssetUtils.h"
#include <algorithm>
#include <fstream>

#include <SDL_log.h>
#include <SDL_assert.h>

std::string TEX_DIR(const std::string& fname_) {
    std::string name(fname_);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return std::string("assets/textures/") + name;
}

std::string DATA_DIR(const std::string& fname_) {
    std::string name(fname_);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return std::string("assets/datas/") + name;
}

bool LoadJson(const char* fname, rapidjson::Document& doc) {
    std::string path = DATA_DIR(fname);
    std::ifstream ss(path);
    if (!ss.is_open()) {
        SDL_Log("Could not open the file %s\n", path.c_str());
        return false;
    }
    using isb_iter = std::istreambuf_iterator<char>;
    std::string content{ isb_iter(ss), isb_iter() };
    doc.Parse(content.c_str());
    return !doc.HasParseError();
}

bool toivec2(glm::ivec2& result, const std::string& str) {
    auto pos = str.find_first_of(',', 0);
    if (pos == str.npos)
        return false;

    result.x = std::stoi(str.substr(0, pos));
    result.y = std::stoi(str.substr(pos + 1));
    return true;
}

JsonAsset::~JsonAsset() {
    Cleanup();
}

void JsonAsset::Cleanup() {
    rapidjson::Document().Swap(m_Document);
}

bool JsonAsset::Load(const char* fname) {
    return ::LoadJson(fname, m_Document);
}

void Variables::Initialize() {
    m_Json.Load("defines.json");
    auto& doc = m_Json.GetDoc();

    auto mapsItr = doc.FindMember("Maps");
    if (mapsItr != doc.MemberEnd() && mapsItr->value.IsArray()) {
        for (const auto& map : mapsItr->value.GetArray()) {
            m_Terrains.emplace_back(map.GetString());
        }
    }

    auto lordsItr = doc.FindMember("Nightloads");
    if (lordsItr != doc.MemberEnd() && lordsItr->value.IsArray()) {
        for (const auto& lord : lordsItr->value.GetArray()) {
            m_Nightlords.emplace_back(lord.GetString());
        }
    }
}

void IconAtlas::Initialize() {
    m_Json.Load("icons.json");
    auto& doc = m_Json.GetDoc();

    for (auto itr = doc.MemberBegin();
        itr != doc.MemberEnd(); ++itr) {
        std::string_view name{ itr->name.GetString() };

        glm::ivec2 offset, size;
        auto offsetItr = itr->value.FindMember("offset");
        if (offsetItr != itr->value.MemberEnd()) {
            toivec2(offset, offsetItr->value.GetString());
        }
        auto sizeItr = itr->value.FindMember("size");
        if (sizeItr != itr->value.MemberEnd()) {
            toivec2(size, sizeItr->value.GetString());
        }

        m_Icons[name] = glm::ivec4(offset, size);
    }
}

const glm::ivec4* IconAtlas::QueryIcon(const char* name) const {
    auto iter = m_Icons.find(name);
    if (iter == m_Icons.end())
        return nullptr;
    return &iter->second;
}

static std::string locpath(const char* name) {
    return std::string("loc ") + name + ".json";
}

void MapThumbnail::LoadMap(const char* mapName) {
    using namespace std;
    m_SpawnPoints.clear();
    m_AllPoints.clear();

    string jsonFn = "map ";
    jsonFn.append(mapName);
    jsonFn.append(".json");

    JsonAsset mJson;
    SDL_assert(mJson.Load(jsonFn.c_str()));
    auto& mDoc = mJson.GetDoc();

    auto getbase_ = [](const rapidjson::Document& doc, const char* member) {
        map<string_view, MapLocation> result;

        for (const auto& elem : doc.GetArray()) {
            auto idxItr = elem.FindMember("index");
            if (idxItr == elem.MemberEnd())
                continue;
            int mapIdx = elem["index"].GetInt();

            auto mItr = elem.FindMember(member);
            if (mItr == elem.MemberEnd())
                continue;

            for (auto campItr = mItr->value.MemberBegin();
                campItr != mItr->value.MemberEnd(); ++campItr) {
                string_view name = campItr->name.GetString();

                result[name].diff[mapIdx] = campItr->value.GetString();
            }
        }

        JsonAsset mlocJson;
        bool locLoaded = mlocJson.Load(locpath(member).c_str());
        auto& mlocDoc = mlocJson.GetDoc();
        for (auto& p : result) {
            p.second.name = p.first;
            if (!locLoaded)
                continue;

            auto locItr = mlocDoc.FindMember(p.first.data());
            if (locItr != mlocDoc.MemberEnd()) {
                toivec2(p.second.pos, locItr->value.GetString());
            }
        }
        
        return result;
    };

    auto minorBase = getbase_(mDoc, "Minor Base");
    auto majorBase = getbase_(mDoc, "Major Base");
    map<string_view, MapLocation> spBase;

    for (const auto& elem : mDoc.GetArray()) {
        auto idxItr = elem.FindMember("index");
        if (idxItr == elem.MemberEnd())
            continue;
        int mapIdx = elem["index"].GetInt();

        auto spItr = elem.FindMember("Spawn Point");
        if (spItr == elem.MemberEnd())
            continue;
        string_view name = spItr->value.GetString();
        SDL_assert(minorBase.count(name));
        spBase[name].diff[mapIdx] = minorBase[name].diff[mapIdx];
    }
    
    for (auto& p : spBase) {
        p.second.name = p.first;
        p.second.pos = minorBase[p.first].pos;
    }

    for (const auto& p : spBase) {
        m_SpawnPoints.push_back(p.second);
    }
    for (const auto& p : minorBase) {
        m_AllPoints.push_back(p.second);
    }
    for (const auto& p : majorBase) {
        m_AllPoints.push_back(p.second);
    }
}
