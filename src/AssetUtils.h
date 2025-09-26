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

struct MapLocation {
	std::string name;
	glm::ivec2 pos;
	std::map<int, std::string> diff;
};

class MapThumbnail {
public:
	using Locations = std::unordered_map<std::string_view, glm::ivec2>;
	using Places = std::vector<std::string_view>;
	using MapFilter = std::function<void(const rapidjson::Value&)>;

	void LoadMap(const char* mapName);
	
	void Foreach(MapFilter&& filter);

	const glm::ivec2* MinorLoc(const char* locName) const {
		return QueryLocation(m_Minor, locName);
	}
	const glm::ivec2* MajorLoc(const char* locName) const {
		return QueryLocation(m_Major, locName);
	}
	std::string_view Near(const char* locName) const;

private:
	void LoadLocation(Locations& target, const char* source,
		const std::unordered_set<std::string_view>& exist);
	const glm::ivec2* QueryLocation(const Locations& locs, const char* locName) const {
		auto itr = m_Minor.find(locName);
		if (itr != m_Minor.end())
			return &itr->second;
		return nullptr;
	}

private:
	JsonAsset m_Json;
	Locations m_Minor;
	Locations m_Major;
};
