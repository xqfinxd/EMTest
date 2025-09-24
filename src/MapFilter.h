#pragma once

#include "MapViewer.h"

class MapFilter {
public:
	MapFilter() = default;
	~MapFilter() = default;

	void RenderImGui(MapViewer& mapViewer);

private:
	int m_MapType = -1;
	int m_SpawnPoint = -1;
};
