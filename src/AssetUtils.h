#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <memory>
#include <functional>

#include <glm/glm.hpp>
#include <rapidjson/document.h>

std::string TEX_DIR(const std::string& fname);
std::string DATA_DIR(const std::string& fname);

using Document = rapidjson::Document;
using string_view = std::string_view;
using JsonValue = rapidjson::Value;

string_view TR(string_view ostr);

bool LoadJson(const char* fname, Document& doc);
std::unique_ptr<Document> LoadJson(const char* fname);
const JsonValue* SubValue(const JsonValue* value, const char* key);

class Variables {
public:
	using NameList = std::vector<std::string_view>;
	void Initialize();
	const NameList& GetTerrains() const {
		return m_Terrains;
	}
	const NameList& GetNightlords() const {
		return m_Nightlords;
	}

private:
	std::unique_ptr<Document> m_Json;
    NameList m_Terrains;
    NameList m_Nightlords;
};

class Translator {
public:
	using Table = std::unordered_map<size_t, std::string>;
	Translator();
	string_view Find(string_view word) const;

private:
	void Load(const char* fname);

private:
	Table m_Table;
};

class BossDefines {
public:
	BossDefines();
	int BossType(string_view bossName) const;

private:
	std::unique_ptr<Document> m_Json;
};

class IconAtlas {
public:
	using Rects = std::unordered_map<string_view, glm::ivec4>;
	void Initialize();
	const glm::ivec4* QueryRect(string_view name) const;

private:
	std::unique_ptr<Document> m_Json;
	Rects m_IconMap;
};

struct Vec2Comp {
	bool operator()(const glm::vec2& l, const glm::vec2& r) const {
		glm::vec2 zp{ 0,0 };
		return glm::distance(zp, l) < glm::distance(zp, r);
	}
};

using MapLocations = std::map<glm::vec2, std::string, Vec2Comp>;

struct MapDetail {
	int index = -1;
	std::string nightlord;
	glm::vec2 spawn_point{};
	std::string special_event;
	std::string night_1_boss;
	std::string night_2_boss;
	std::string extra_boss;
	glm::vec2 day_1_circle{};
	glm::vec2 day_2_circle{};
	std::string castle_type;
	MapLocations major;
	MapLocations minor;
	MapLocations evergaol;
	MapLocations field;
	std::string castle_basement;
	std::string castle_rooftop;
	MapLocations rotted_woods;
	glm::vec2 rot_blessing{};
	glm::vec2 frenzy_tower{};
	glm::vec2 demon_merchant{};

	void Reset();
};

enum LocationType {
    eMinorBase,
    eMajorBase,
    eEvergaol,
    eFieldBoss,
    eCircle,
    eRottedWoods,
    eRotBlessing,
    eFrenzyTower,
    eDemonMerchant,
};

class MapThumbnail {
public:
	using Locations = std::unordered_map<string_view, glm::ivec2>;
	using Places = std::vector<string_view>;
	using MapFilter = std::function<void(const rapidjson::Value&)>;
	using MapFinder = std::function<bool(const rapidjson::Value&)>;

	void LoadMap(const char* mapName);
	
	void Foreach(MapFilter&& filter);
	const rapidjson::Value* Find(MapFinder&& finder);
	bool LoadDetail(int mapIdx, MapDetail& detail);

	const glm::ivec2* Query(LocationType loc, string_view locName) const;
	string_view Near(const char* locName) const;

private:
	void LoadLocation1(Locations& loc, const char* source, const char* key);
	void LoadLocation2(Locations& loc, const char* source, const char* key);
	void LoadMapLocations(const rapidjson::Value& subObj,
		MapLocations& target, LocationType locType);
	
private:
	std::unique_ptr<Document> m_MapList;
	std::map<LocationType, Locations> m_Locations;
};
