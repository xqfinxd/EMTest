#pragma once

#include "MapViewer.h"

class MapFilter {
public:
	MapFilter() = default;
	~MapFilter() = default;

	void Initialize(MapViewer* view);
	void RenderImGui();

private:
	bool FilterTerrain();
	void OnFilterTerrain();
	bool FilterLanding();
	void OnFilterLanding();
	bool FilterSmallCampType();
	void OnFilterSmallCampType();
	bool FilterNearCamp();
	void OnFilterNearCamp();

private:
	MapViewer* m_Viewer = nullptr;

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

	MapDetail m_MapDetail;
};
