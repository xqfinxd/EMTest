#include "AssetUtils.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <set>
#include <locale>
#include <codecvt>
#include <SDL_log.h>
#include <SDL_assert.h>

std::unique_ptr<Translator> g_Translator{ nullptr };
bool g_EnableChinese = true;
std::wstring_convert<std::codecvt_utf8<wchar_t>> g_Converter;

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

string_view TR(string_view ostr) {
    if (g_EnableChinese) {
        if (!g_Translator)
            g_Translator.reset(new Translator);
        return g_Translator->Find(ostr);
    }
    return ostr;
}

std::wstring ConvertFrom(string_view str) {
    return g_Converter.from_bytes(str.data());
}

bool LoadJson(const char* fname, rapidjson::Document& doc) {
    std::string path = DATA_DIR(fname);
    std::ifstream ss(path);
    if (!ss.is_open()) {
        SDL_Log("Could not open the file %s", path.c_str());
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

const glm::ivec2* AsVec2(const char* str) {
    if (str == nullptr || strlen(str) == 0)
        return nullptr;
    static glm::ivec2 result;
    char* newstr = nullptr;
    result.x = std::strtol(str, &newstr, 10);
    if (!newstr || str == newstr)
        return nullptr;
    result.y = std::strtol(newstr + 1, &newstr, 10);

    return &result;
}

bool toivec2(glm::ivec2& result, const char* str) {
    if (str == nullptr || strlen(str) == 0)
        return false;
    char* newstr = nullptr;
    result.x = std::strtol(str, &newstr, 10);
    result.y = std::strtol(newstr + 1, &newstr, 10);
    return true;
}

const JsonValue* SubValue(const JsonValue* value, const char* key) {
    const rapidjson::Value* nulObject = nullptr;
    if (!value)
        return nulObject;
    auto itr = value->FindMember(key);
    if (itr == value->MemberEnd())
        return nulObject;
    return &itr->value;
}

void Variables::Initialize() {
    m_Json = LoadJson("defines.json");
    SDL_assert(m_Json);

    auto mapsValue = SubValue(m_Json.get(), "Maps");
    if (mapsValue && mapsValue->IsArray()) {
        for (const auto& map : mapsValue->GetArray()) {
            m_Terrains.emplace_back(map.GetString());
        }
    }

    auto lordsValue = SubValue(m_Json.get(), "Nightloads");
    if (lordsValue && lordsValue->IsArray()) {
        for (const auto& lord : lordsValue->GetArray()) {
            m_Nightlords.emplace_back(lord.GetString());
        }
    }
}

void IconAtlas::Initialize() {
    m_Json = LoadJson("icons.json");
    SDL_assert(m_Json);

    auto SubVec2 = [](const JsonValue* value_, glm::ivec2& result) {
        if (!value_ || !value_->IsString())
            return false;
        auto tmp_ = AsVec2(value_->GetString());
        if (!tmp_) return false;
        result = *tmp_;
        return true;
    };

    for (auto itr = m_Json->MemberBegin();
        itr != m_Json->MemberEnd(); ++itr) {
        std::string_view name{ itr->name.GetString() };

        glm::ivec2 offset;
        if (!SubVec2(SubValue(&itr->value, "offset"), offset))
            continue;
        
        glm::ivec2 size;
        if (!SubVec2(SubValue(&itr->value, "size"), size))
            continue;

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
            auto idxValue = SubValue(&value, "index");
            if (!idxValue || !idxValue->IsInt())
                return false;
            return idxValue->GetInt() == mapIdx;
        }
    );
    if (itr == m_MapList->End()) return false;

    auto SubString = [](const JsonValue* value, const char* key) {
        auto sub = SubValue(value, key);
        if (!sub || !sub->IsString())
            return "";
        return sub->GetString();
    };

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
    if (auto sub = SubValue(itr, "Castle")) {
        detail.castle_type = SubString(sub, "Castle");
    }
    if (auto sub = SubValue(itr, "Minor Base")) {
        LoadMapLocations(*sub, detail.minor, eMinorBase);
    }
    if (auto sub = SubValue(itr, "Major Base")) {
        LoadMapLocations(*sub, detail.major, eMajorBase);
    }
    if (auto sub = SubValue(itr, "Evergaol")) {
        LoadMapLocations(*sub, detail.evergaol, eEvergaol);
    }
    if (auto sub = SubValue(itr, "Field Boss")) {
        LoadMapLocations(*sub, detail.field, eFieldBoss);
    }
    if (auto sub = SubValue(itr, "Rotted Woods")) {
        LoadMapLocations(*sub, detail.rotted_woods, eRottedWoods);
    }

    if (auto sub = SubValue(itr, "Arena Boss")) {
        detail.castle_basement = SubString(sub, "Castle Basement");
    }
    if (auto sub = SubValue(itr, "Field Boss")) {
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

const glm::ivec2* MapThumbnail::Query(LocationType loc, string_view locName) const {
    if (locName.empty())
        return nullptr;
    auto typeItr = m_Locations.find(loc);
    if (typeItr == m_Locations.end())
        return nullptr;
    auto itr = typeItr->second.find(locName);
    if (itr == typeItr->second.end())
        return nullptr;
    return &itr->second;
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
            auto value_ = SubValue(&member, key);
            if (!value_ || !value_->IsString())
                return;
            string_view subKey = value_->GetString();
            auto locvalue_ = SubValue(doc.get(), subKey.data());
            if (!locvalue_ || !locvalue_->IsString())
                return;

            glm::ivec2 pos;
            if (toivec2(pos, locvalue_->GetString()))
                loc[subKey] = pos;
        }
    );
}

void MapThumbnail::LoadLocation2(Locations& loc, const char* source, const char* key) {
    auto doc = LoadJson(LOC_PATH(source).c_str());
    Foreach(
        [&loc, &doc, key](const rapidjson::Value& member) {
            auto value_ = SubValue(&member, key);
            if (!value_ || !value_->IsObject())
                return;
            
            for (auto subItr = value_->MemberBegin();
                subItr != value_->MemberEnd(); ++subItr) {
                string_view subKey = subItr->name.GetString();
                auto locvalue_ = SubValue(doc.get(), subKey.data());
                if (!locvalue_ || !locvalue_->IsString())
                    continue;

                glm::ivec2 pos;
                if (toivec2(pos, locvalue_->GetString()))
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
    auto typeValue = SubValue(m_Json.get(), bossName.data());
    if (!typeValue || !typeValue->IsInt())
        return 0;
    return typeValue->GetInt();
}

Translator::Translator() {
    Load("chs main.json");
    Load("chs special event.json");
    Load("chs ui.json");
}

string_view Translator::Find(string_view word) const {
    std::hash<std::string_view> hashFunc{};
    auto itr = m_Table.find(hashFunc(word));
    if (itr == m_Table.end())
        return word;
    return itr->second;
}

std::string Translator::Collect() const {
    auto totalSize = std::accumulate(m_Table.begin(), m_Table.end(), 0,
        [](size_t old, const Table::value_type& e) {
            return old + e.second.length();
        }
    );
    std::string ret;
    ret.reserve(totalSize + 256);
    ret.append(127, '\0');
    std::iota(ret.begin(), ret.end(), 1);
    std::for_each(m_Table.begin(), m_Table.end(),
        [&ret](const Table::value_type& e) {
            ret.insert(ret.end(), e.second.begin(), e.second.end());
        }
    );
    ret.push_back('\0');
    return ret;
}

void Translator::Load(const char* fname) {
    std::hash<std::string_view> hashFunc{};
    auto doc = LoadJson(fname);
    std::for_each(doc->MemberBegin(), doc->MemberEnd(),
        [this,&hashFunc](const JsonMember& member) {
            if (!member.name.IsString() || !member.value.IsString())
                return;
            size_t hash = hashFunc(member.name.GetString());
            m_Table[hash] = member.value.GetString();
        }
    );
}
