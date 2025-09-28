#include "AssetUtils.h"
#include <algorithm>
#include <fstream>

#include <SDL_log.h>
#include <SDL_assert.h>
#include <set>

using JsonMember = std::decay_t<decltype(*(rapidjson::Value{}.MemberBegin()))>;
using JsonValue = rapidjson::Value;

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

std::unique_ptr<rapidjson::Document> LoadJson(const char* fname) {
    std::unique_ptr<Document> doc(new Document);
    if (!LoadJson(fname, *doc))
        return nullptr;

    return doc;
}

bool toivec2(glm::ivec2& result, const char* str) {
    if (str == nullptr || strlen(str) == 0)
        return false;
    char* newstr = nullptr;
    result.x = std::strtol(str, &newstr, 10);
    result.y = std::strtol(newstr + 1, &newstr, 10);
    return true;
}

const char* SubString(const JsonValue* value, const char* key) {
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

const JsonValue* SubObject(const JsonValue* value, const char* key) {
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

void Variables::Initialize() {
    m_Json = LoadJson("defines.json");
    SDL_assert(m_Json);

    auto mapsItr = m_Json->FindMember("Maps");
    if (mapsItr != m_Json->MemberEnd() && mapsItr->value.IsArray()) {
        for (const auto& map : mapsItr->value.GetArray()) {
            m_Terrains.emplace_back(map.GetString());
        }
    }

    auto lordsItr = m_Json->FindMember("Nightloads");
    if (lordsItr != m_Json->MemberEnd() && lordsItr->value.IsArray()) {
        for (const auto& lord : lordsItr->value.GetArray()) {
            m_Nightlords.emplace_back(lord.GetString());
        }
    }
}

void IconAtlas::Initialize() {
    m_Json = LoadJson("icons.json");
    SDL_assert(m_Json);

    for (auto itr = m_Json->MemberBegin();
        itr != m_Json->MemberEnd(); ++itr) {
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

        m_IconMap[name] = glm::ivec4(offset, size);
    }
}

const glm::ivec4* IconAtlas::QueryRect(string_view name) const {
    auto iter = m_IconMap.find(name);
    if (iter == m_IconMap.end())
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
    m_MapList = LoadJson(MAP_PATH(mapName).c_str());
    SDL_assert(m_MapList && m_MapList->IsArray());
    m_Locations.clear();
    LoadLocation1(m_Locations[eCircle], "Circle", "Night 1 Circle");
    LoadLocation1(m_Locations[eCircle], "Circle", "Night 2 Circle");
    LoadLocation1(m_Locations[eRotBlessing], "Rot Blessing", "Rot Blessing");
    LoadLocation1(m_Locations[eFrenzyTower], "Frenzy Tower", "Frenzy Tower");
    LoadLocation1(m_Locations[eDemonMerchant], "Demon Merchant", "Scale-Bearing Merchant");

    LoadLocation2(m_Locations[eMinorBase], "Minor Base", "Minor Base");
    LoadLocation2(m_Locations[eMajorBase], "Major Base", "Major Base");
    LoadLocation2(m_Locations[eEvergaol], "Evergaol", "Evergaol");
    LoadLocation2(m_Locations[eFieldBoss], "Field Boss", "Field Boss");
    LoadLocation2(m_Locations[eRottedWoods], "Rotted Woods", "Rotted Woods");
}

void MapThumbnail::Foreach(MapFilter&& filter) {
    auto arr = m_MapList->GetArray();
    std::for_each(arr.Begin(), arr.End(), filter);
}

const rapidjson::Value* MapThumbnail::Find(MapFinder&& finder) {
    auto arr = m_MapList->GetArray();
    return std::find_if(arr.Begin(), arr.End(), finder);
}

bool MapThumbnail::LoadDetail(int mapIdx, MapDetail& detail) {
    detail.index = mapIdx;

    auto itr = std::find_if(m_MapList->Begin(), m_MapList->End(),
        [mapIdx](const rapidjson::Value& value) {
            auto itr = value.FindMember("index");
            if (itr == value.MemberEnd())
                return false;
            if (!itr->value.IsInt())
                return false;
            return itr->value.GetInt() == mapIdx;
        }
    );
    if (itr == m_MapList->End())
        return false;

    detail.nightlord = SubString(itr, "Nightlord");
    if (auto pos = Query(eMinorBase, SubString(itr, "Spawn Point"))) {
        detail.spawn_point = *pos;
    }
    detail.special_event = SubString(itr, "Special Event");
    detail.night_1_boss = SubString(itr, "Night 1 Boss");
    detail.night_2_boss = SubString(itr, "Night 2 Boss");
    detail.extra_boss = SubString(itr, "Extra Night Boss");

    if (auto pos = Query(eCircle, SubString(itr, "Night 1 Circle"))) {
        detail.day_1_circle = *pos;
    }
    if (auto pos = Query(eCircle, SubString(itr, "Night 2 Circle"))) {
        detail.day_2_circle = *pos;
    }
    if (auto sub = SubObject(itr, "Castle")) {
        detail.castle_type = SubString(sub, "Castle");
    }
    if (auto sub = SubObject(itr, "Minor Base")) {
        LoadMapLocations(*sub, detail.minor, eMinorBase);
    }
    if (auto sub = SubObject(itr, "Major Base")) {
        LoadMapLocations(*sub, detail.major, eMajorBase);
    }
    if (auto sub = SubObject(itr, "Evergaol")) {
        LoadMapLocations(*sub, detail.evergaol, eEvergaol);
    }
    if (auto sub = SubObject(itr, "Field Boss")) {
        LoadMapLocations(*sub, detail.field, eFieldBoss);
    }
    if (auto sub = SubObject(itr, "Rotted Woods")) {
        LoadMapLocations(*sub, detail.rotted_woods, eRottedWoods);
    }

    if (auto sub = SubObject(itr, "Arena Boss")) {
        detail.castle_basement = SubString(sub, "Castle Basement");
    }
    if (auto sub = SubObject(itr, "Field Boss")) {
        detail.castle_rooftop = SubString(sub, "Castle Rooftop");
    }
    if (auto pos = Query(eRotBlessing, SubString(itr, "Rot Blessing"))) {
        detail.rot_blessing = *pos;
    }
    if (auto pos = Query(eFrenzyTower, SubString(itr, "Frenzy Tower"))) {
        detail.frenzy_tower = *pos;
    }
    if (auto pos = Query(eDemonMerchant, SubString(itr, "Scale-Bearing Merchant"))) {
        detail.demon_merchant = *pos;
    }

    return true;
}

string_view MapThumbnail::Near(const char* locName) const {
    string_view majorCamp;

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

void MapThumbnail::LoadLocation1(Locations& loc, const char* source, const char* key) {
    auto doc = LoadJson(LOC_PATH(source).c_str());
    Foreach(
        [&loc, &doc, key](const rapidjson::Value& member) {
            auto itr = member.FindMember(key);
            if (itr == member.MemberEnd())
                return;
            if (!itr->value.IsString())
                return;
            string_view subKey = itr->value.GetString();
            glm::ivec2 pos;
            if (toivec2(pos, SubString(doc.get(), subKey.data())))
                loc[subKey] = pos;
        }
    );
}

void MapThumbnail::LoadLocation2(Locations& loc, const char* source, const char* key) {
    auto doc = LoadJson(LOC_PATH(source).c_str());
    Foreach(
        [&loc, &doc, key](const rapidjson::Value& member) {
            auto itr = member.FindMember(key);
            if (itr == member.MemberEnd())
                return;
            if (!itr->value.IsObject())
                return;

            for (auto subItr = itr->value.MemberBegin();
                subItr != itr->value.MemberEnd(); ++subItr) {
                string_view subKey = subItr->name.GetString();
                glm::ivec2 pos;
                if (toivec2(pos, SubString(doc.get(), subKey.data())))
                    loc[subKey] = pos;
            }
        }
    );
}

void MapThumbnail::LoadMapLocations(const rapidjson::Value& subObj,
    MapLocations& target, LocationType locType) {
    for (auto itr = subObj.MemberBegin();
        itr != subObj.MemberEnd(); ++itr) {
        if (!itr->name.IsString() || !itr->value.IsString())
            continue;
        auto pos = Query(locType, itr->name.GetString());
        if (!pos) continue;

        target[glm::vec2(*pos)] = itr->value.GetString();
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

BossDefines::BossDefines() {
    m_Json = LoadJson("boss.json");
}

int BossDefines::BossType(string_view bossName) const {
    auto itr = m_Json->FindMember(bossName.data());
    if (itr == m_Json->MemberEnd())
        return 0;
    if (!itr->value.IsInt())
        return 0;
    return itr->value.GetInt();
}
