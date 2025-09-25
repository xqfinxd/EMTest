#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>

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
	void Initialize();
	const glm::ivec4* QueryIcon(const char* name) const;

private:
	JsonAsset m_Json;
	std::unordered_map<std::string_view, glm::ivec4> m_Icons;
};

struct MapLocation {
	std::string name;
	glm::ivec2 pos;
	std::map<int, std::string> diff;
};

class MapThumbnail {
public:
	void LoadMap(const char* mapName);
	const std::vector<MapLocation>& GetSpawnPoints() const {
		return m_SpawnPoints;
	}
	const std::vector<MapLocation>& GetAllPoints() const {
		return m_AllPoints;
	}

private:
	std::vector<MapLocation> m_SpawnPoints;
	std::vector<MapLocation> m_AllPoints;
};
