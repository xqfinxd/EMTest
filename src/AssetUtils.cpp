#include "AssetUtils.h"
#include <algorithm>
#include <fstream>

#include <SDL_log.h>
#include <SDL_assert.h>
#include <set>

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

static std::string LOC_PATH(const char* name) {
    return std::string("loc ") + name + ".json";
}

static std::string MAP_PATH(const char* name) {
    return std::string("map ") + name + ".json";
}

void MapThumbnail::LoadMap(const char* mapName) {
    m_Json.Load(MAP_PATH(mapName).c_str());

    {
        const char* name = "Minor Base";
        std::unordered_set<std::string_view> exist;
        Foreach([&exist,name](const rapidjson::Value& member) {
            auto itr = member.FindMember(name);
            if (itr == member.MemberEnd())
                return;
            auto& bases = itr->value;
            for (auto subItr = bases.MemberBegin();
                subItr != bases.MemberEnd(); ++subItr) {
                if (subItr->value.IsString())
                    exist.insert(subItr->name.GetString());
            }
        });
        LoadLocation(m_Minor, name, exist);
    }

    {
        const char* name = "Major Base";
        std::unordered_set<std::string_view> exist;
        Foreach([&exist, name](const rapidjson::Value& member) {
            auto itr = member.FindMember(name);
            if (itr == member.MemberEnd())
                return;
            auto& bases = itr->value;
            for (auto subItr = bases.MemberBegin();
                subItr != bases.MemberEnd(); ++subItr) {
                if (subItr->value.IsString())
                    exist.insert(subItr->name.GetString());
            }
        });
        LoadLocation(m_Major, name, exist);
    }
}

void MapThumbnail::Foreach(MapFilter&& filter) {
    auto mapList = m_Json.GetDoc().GetArray();
    std::for_each(mapList.Begin(), mapList.End(), filter);
}

std::string_view MapThumbnail::Near(const char* locName) const {
    std::string_view majorCamp;
    auto itr = m_Minor.find(locName);
    if (itr == m_Minor.end())
        return majorCamp;

    using ValueType = decltype(*m_Major.begin());
    auto& pos = itr->second;
    float dis = FLT_MAX;
    std::for_each(m_Major.begin(), m_Major.end(),
        [&pos,&dis,&majorCamp](ValueType& value) {
            float newdis = glm::distance(
                glm::vec2(pos),
                glm::vec2(value.second));
            if (newdis < 1)
                return;

            if (newdis < dis) {
                majorCamp = value.first;
                dis = newdis;
            }
        }
    );

    return majorCamp;
}

void MapThumbnail::LoadLocation(Locations& target, const char* source,
    const std::unordered_set<std::string_view>& exist) {
    target.clear();

    JsonAsset mlocJson;
    mlocJson.Load(LOC_PATH(source).c_str());

    auto& mlocDoc = mlocJson.GetDoc();
    using JsonMember = decltype(*(mlocDoc.MemberBegin()));
    std::for_each(mlocDoc.MemberBegin(), mlocDoc.MemberEnd(),
        [&target,&exist](const JsonMember& member) {
            if (!member.value.IsString())
                return;
            auto itr = exist.find(member.name.GetString());
            if (itr == exist.end())
                return;
            std::string_view key = *itr;
            toivec2(target[key], member.value.GetString());
        }
    );
}
