#include "AssetUtils.h"
#include <algorithm>
#include <fstream>

#include <SDL_log.h>
#include <SDL_assert.h>
#include <set>

using JsonMember = std::decay_t<decltype(*(rapidjson::Value{}.MemberBegin()))>;

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
    m_Locations.clear();
    LoadLocation(eMinorBase, "Minor Base", "Minor Base");
    LoadLocation(eMajorBase, "Major Base", "Major Base");
    LoadLocation(eEvergaol, "Evergaol", "Evergaol");
    LoadLocation(eFieldBoss, "Field Boss", "Field Boss");
    LoadLocation(eCircle, "Circle", "Night 1 Circle");
    LoadLocation(eCircle, "Circle", "Night 2 Circle");
    LoadLocation(eRottedWoods, "Rotted Woods", "Rotted Woods");
    LoadLocation(eRotBlessing, "Rot Blessing", "Rot Blessing");
    LoadLocation(eFrenzyTower, "Frenzy Tower", "Frenzy Tower");
    LoadLocation(eDemonMerchant, "Demon Merchant", "Scale-Bearing Merchant");
}

void MapThumbnail::Foreach(MapFilter&& filter) {
    auto mapList = m_Json.GetDoc().GetArray();
    std::for_each(mapList.Begin(), mapList.End(), filter);
}

const rapidjson::Value* MapThumbnail::Find(MapFinder&& finder) {
    auto mapList = m_Json.GetDoc().GetArray();
    return std::find_if(mapList.Begin(), mapList.End(), finder);
}

std::string_view MapThumbnail::Near(const char* locName) const {
    std::string_view majorCamp;

    auto minorItr = m_Locations.find(LocationType::eMinorBase);
    auto majorItr = m_Locations.find(LocationType::eMajorBase);
    if (minorItr == m_Locations.end() || majorItr == m_Locations.end())
        return majorCamp;

    auto itr = minorItr->second.find(locName);
    if (itr == minorItr->second.end())
        return majorCamp;

    using ValueType = decltype(*majorItr->second.begin());
    auto& pos = itr->second;
    float dis = FLT_MAX;
    std::for_each(majorItr->second.begin(), majorItr->second.end(),
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

void MapThumbnail::LoadLocation(LocationType loc, const char* source, const char* key) {
    auto& target = m_Locations[loc];

    std::unordered_set<std::string_view> exist;
    Foreach([&exist, key](const rapidjson::Value& member) {
        auto itr = member.FindMember(key);
        if (itr == member.MemberEnd())
            return;
        auto& value = itr->value;
        if (value.IsNull())
            return;
        if (value.IsString()) {
            exist.insert(value.GetString());
        }
        else if(value.IsObject()) {
            for (auto subItr = value.MemberBegin();
                subItr != value.MemberEnd(); ++subItr) {
                if (!subItr->name.IsString())
                    continue;
                exist.insert(subItr->name.GetString());
            }
        }
    });

    JsonAsset mlocJson;
    mlocJson.Load(LOC_PATH(source).c_str());

    auto& mlocDoc = mlocJson.GetDoc();
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

namespace VisitJson
{
const char* getstr(const rapidjson::Value* value, const char* key) {
    const char* nulString = "";
    if (!value)
        return nulString;
    auto itr = value->FindMember(key);
    if (itr == value->MemberEnd())
        return nulString;
    if (!itr->value.IsString())
        return nulString;
    return itr->value.GetString();
}

const rapidjson::Value* getobj(const rapidjson::Value* value, const char* key) {
    const rapidjson::Value* nulObject = nullptr;
    if (!value)
        return nulObject;
    auto itr = value->FindMember(key);
    if (itr == value->MemberEnd())
        return nulObject;
    if (!itr->value.IsObject())
        return nulObject;
    return &itr->value;
}

}

void MapDetail::Load(const rapidjson::Value& value, const MapThumbnail& thumbnail) {
    using namespace VisitJson;

    auto visit = [&value, &thumbnail](const char* key,
        LocationType locType, MapLocations& target) {
        auto sub = getobj(&value, key);
        if (!sub) return;
        for (auto itr = sub->MemberBegin();
            itr != sub->MemberEnd(); ++itr) {
            auto& mem = *itr;
            if (!mem.name.IsString() || !mem.value.IsString())
                continue;
            auto pos = thumbnail.Query(locType, mem.name.GetString());
            if (!pos)
                continue;
            target[glm::vec2(*pos)] = mem.value.GetString();
        }
    };

    nightlord = getstr(&value, "Nightlord");
    if (auto pos = thumbnail.Query(eMinorBase, getstr(&value, "Spawn Point"))) {
        spawn_point = *pos;
    }
    special_event = getstr(&value, "Special Event");
    night_1_boss = getstr(&value, "Night 1 Boss");
    night_2_boss = getstr(&value, "Night 2 Boss");
    extra_boss = getstr(&value, "Extra Night Boss");
    if (auto pos = thumbnail.Query(eCircle, getstr(&value, "Night 1 Circle"))) {
        day_1_circle = *pos;
    }
    if (auto pos = thumbnail.Query(eCircle, getstr(&value, "Night 2 Circle"))) {
        day_2_circle = *pos;
    }
    if (auto sub = getobj(&value, "Castle")) {
        castle_type = getstr(sub, "Castle");
    }
    visit("Minor Base", eMinorBase, minor);
    visit("Major Base", eMajorBase, major);
    visit("Evergaol", eEvergaol, evergaol);
    if (auto sub = getobj(&value, "Arena Boss")) {
        castle_basement = getstr(sub, "Castle Basement");
    }
    if (auto sub = getobj(&value, "Field Boss")) {
        castle_rooftop = getstr(sub, "Castle Rooftop");
    }
    visit("Field Boss", eFieldBoss, field);
    visit("Rotted Woods", eRottedWoods, rotted_woods);
    if (auto pos = thumbnail.Query(eRotBlessing, getstr(&value, "Rot Blessing"))) {
        rot_blessing = *pos;
    }
    if (auto pos = thumbnail.Query(eFrenzyTower, getstr(&value, "Frenzy Tower"))) {
        frenzy_tower = *pos;
    }
    if (auto pos = thumbnail.Query(eDemonMerchant, getstr(&value, "Scale-Bearing Merchant"))) {
        demon_merchant = *pos;
    }
}

void MapDetail::Reset() {
    index = -1;
    nightlord.clear();
    spawn_point = { FLT_MAX,FLT_MAX };
    special_event.clear();
    night_1_boss.clear();
    night_2_boss.clear();
    extra_boss.clear();
    day_1_circle = { FLT_MAX,FLT_MAX };
    day_2_circle = { FLT_MAX,FLT_MAX };
    castle_type.clear();
    major.clear();
    minor.clear();
    evergaol.clear();
    field.clear();
    castle_basement.clear();
    castle_rooftop.clear();
    rotted_woods.clear();
    rot_blessing = { FLT_MAX,FLT_MAX };
    frenzy_tower = { FLT_MAX,FLT_MAX };
    demon_merchant = { FLT_MAX,FLT_MAX };
}
