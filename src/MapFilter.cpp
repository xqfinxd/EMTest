#include "MapFilter.h"
#include "MapDefines.h"
#include "imgui.h"

void MapFilter::RenderImGui(MapViewer& mapViewer) {
    int mapCount = sizeof(Maps_::LIST) / sizeof(Maps_::LIST[0]);
    if (ImGui::Combo("选择地形", &m_MapType, Maps_::LIST, mapCount)) {
        std::string mapName = Maps_::LIST[m_MapType];
        mapViewer.ReloadMap(mapName.c_str());
        m_SpawnPoint = -1;
    }

    if (ImGui::BeginCombo("选择出生", "")) {
        ImGui::EndCombo();
    }
}
