#pragma once

#include "MapViewer.h"

class MapFilter {
public:
	MapFilter() = default;
	~MapFilter() = default;

	void Initialize();
	void RenderImGui(MapViewer& mapViewer);

private:
	MapThumbnail m_Thumbnail;
	Variables m_Variables;

	std::vector<std::string_view> m_Terrains;
	int m_TerrainIndex = -1;

	std::vector<MapLocation> m_SpawnPoints;
	int m_SpawnPointIndex = -1;
	int m_CampIndex = -1;
	glm::ivec2 m_NearPoint;
	std::string m_NearPointName;
	std::string m_NearPointDesc;
};
