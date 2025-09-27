#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <functional>

#include <glm/glm.hpp>
#include <rapidjson/document.h>

std::string TEX_DIR(const std::string& fname);
std::string DATA_DIR(const std::string& fname);

bool LoadJson(const char* fname, rapidjson::Document& doc);

class JsonAsset {
public:
	~JsonAsset();
	void Cleanup();
	bool Load(const char* fname);
	const rapidjson::Document& GetDoc() const {
		return m_Document;
	}

private:
	rapidjson::Document m_Document;
};

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
	JsonAsset m_Json;
    NameList m_Terrains;
    NameList m_Nightlords;
};

class IconAtlas {
public:
	using Rects = std::unordered_map<std::string_view, glm::ivec4>;
	void Initialize();
	const glm::ivec4* QueryIcon(const char* name) const;

private:
	JsonAsset m_Json;
	Rects m_Icons;
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
	using Locations = std::unordered_map<std::string_view, glm::ivec2>;
	using Places = std::vector<std::string_view>;
	using MapFilter = std::function<void(const rapidjson::Value&)>;
	using MapFinder = std::function<bool(const rapidjson::Value&)>;

	void LoadMap(const char* mapName);
	
	void Foreach(MapFilter&& filter);
	const rapidjson::Value* Find(MapFinder&& finder);

	const glm::ivec2* Query(LocationType loc, const char* locName) const {
		if (!locName || strlen(locName) == 0)
			return nullptr;
		auto typeItr = m_Locations.find(loc);
		if (typeItr == m_Locations.end())
			return nullptr;
		auto itr = typeItr->second.find(locName);
		if (itr == typeItr->second.end())
			return nullptr;
		return &itr->second;
	}
	std::string_view Near(const char* locName) const;

private:
	void LoadLocation(LocationType loc, const char* source, const char* key);
	
private:
	JsonAsset m_Json;
	std::map<LocationType, Locations> m_Locations;
};

struct PosComp {
	bool operator()(const glm::vec2& l, const glm::vec2& r) const {
		glm::vec2 zp{ 0,0 };
		return glm::distance(zp, l) < glm::distance(zp, r);
	}
};

using MapLocations = std::map<glm::vec2, std::string, PosComp>;

struct MapDetail {
	int index = 0;
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

	void Load(const rapidjson::Value& value, const MapThumbnail& thumbnail);
	void Reset();
};