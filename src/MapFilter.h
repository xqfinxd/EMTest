#pragma once

#include "MapViewer.h"

class MapFilter {
public:
	MapFilter() = default;
	~MapFilter() = default;

	void Initialize();
	void RenderImGui(MapViewer& mapViewer);

private:
	bool FilterTerrain();
	bool FilterLanding();
	bool FilterSmallCampType();
	bool FilterNearCamp();

private:
	MapThumbnail m_Thumbnail;
	Variables m_Variables;

	std::vector<std::string> m_Terrains;
	int m_TerrainIndex = -1;

	std::vector<std::string> m_Landings;
	int m_LandingIndex = -1;

	std::vector<std::string> m_SmallCampTypes;
	int m_SmallCampTypeIndex = -1;

	std::string m_NearCamp;
	std::map<int, std::string> m_CampTypes;
	int m_CampTypeIndex;

	int m_MapIdx = -1;
};
