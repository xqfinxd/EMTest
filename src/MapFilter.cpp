#include "MapFilter.h"
#include <set>
#include <functional>

#include <SDL_Log.h>
#include <imgui.h>

namespace Icons_ {
    constexpr static const char* SPAWN_POINT = "Spawn Point";
    constexpr static const char* BURIED_TREASURE = "Buried Treasure";
    constexpr static const char* ROT_BLESSING = "Rot Blessing";
    constexpr static const char* BOSS = "BOSS";
    constexpr static const char* SCRARAB = "Scarab";
    constexpr static const char* GRACE_OF_SITE = "Grace of Site";
    constexpr static const char* EVERGOAL = "Evergaol";
    constexpr static const char* CAMP = "Camp";
    constexpr static const char* GREAT_CHURCH = "Great Church";
    constexpr static const char* TOWNSHIP = "Township";
    constexpr static const char* SORCERERS_RISE = "Sorcerer's Rise";
    constexpr static const char* RUINS = "Ruins";
    constexpr static const char* FORT = "Fort";
    constexpr static const char* CHURCH = "Church";
}

static std::string ivec2tostr(const glm::ivec2& pos) {
    std::string str;
    str.append(std::to_string(pos.x));
    str.append(",");
    str.append(std::to_string(pos.y));
    return str;
}

template<class T>
static bool RenderCombo(const char* label, const T& container, int& idx,
    std::function<std::string(const typename T::value_type&)>&& tostr) {
    std::string previewText;
    if (idx >= 0 && idx < container.size()) {
        auto itr = std::cbegin(container);
        std::advance(itr, idx);
        previewText = tostr(*itr);
    }
    bool changed = false;
    if (ImGui::BeginCombo(label, previewText.c_str())) {
        for (auto itr = std::cbegin(container); itr != std::cend(container); ++itr) {
            int i = std::distance(std::cbegin(container), itr);
            bool selected = idx == i;

            if (ImGui::Selectable(tostr(*itr).c_str(), selected)) {
                idx = i;
                changed = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

void MapFilter::Initialize() {
    m_Variables.Initialize();
    m_Terrains = m_Variables.GetTerrains();
}

void MapFilter::RenderImGui(MapViewer& mapViewer) {
    using namespace std;
    do {
        // 选择地形
        bool terrainChanged = RenderCombo("地形", m_Terrains, m_TerrainIndex,
            [](const string_view& view) {
                return string(view);
            }
        );

        if (terrainChanged && m_TerrainIndex >= 0 && m_TerrainIndex < m_Terrains.size()) {
            const char* terrainName = m_Terrains[m_TerrainIndex].data();
            mapViewer.ReloadMap(terrainName);
            m_Thumbnail.LoadMap(terrainName);
            m_SpawnPoints = m_Thumbnail.GetSpawnPoints();
        }
        
        if (m_TerrainIndex < 0) break;

        // 选择出生点
        bool spawnChanged = RenderCombo("落地点", m_SpawnPoints, m_SpawnPointIndex,
            [](const MapLocation& loc) {
                return ivec2tostr(loc.pos);
            }
        );
        auto& icons = mapViewer.MakeIcons();
        icons.clear();
        if (m_SpawnPointIndex >= 0 && m_SpawnPointIndex < m_SpawnPoints.size()) {
            glm::ivec2 pos = m_SpawnPoints[m_SpawnPointIndex].pos;
            icons.push_back({ pos, Icons_::SPAWN_POINT });
        }
        else {
            for (const auto& sp : m_SpawnPoints) {
                icons.push_back({ sp.pos, Icons_::SPAWN_POINT });
            }
        }

        if (m_SpawnPointIndex < 0 || m_SpawnPointIndex >= m_SpawnPoints.size())
            break;

        // 选择出生营地
        const auto& campLoc = m_SpawnPoints[m_SpawnPointIndex];
        bool campChanged = RenderCombo("落地营地", campLoc.diff, m_CampIndex,
            [](const std::pair<int, string>& camp) {
                return std::to_string(camp.first) + ". " + camp.second;
            }
        );
        
        if (campChanged) {
            auto curItr = std::begin(campLoc.diff);
            std::advance(curItr, m_CampIndex);
            int curMapIdx = curItr->first;

            float dis = FLT_MAX;
            auto& allCamps = m_Thumbnail.GetAllPoints();
            auto closestItr = allCamps.end();
            for (auto dstItr = allCamps.begin(); dstItr != allCamps.end(); dstItr++) {
                float newdis = glm::distance(glm::vec2(campLoc.pos), glm::vec2(dstItr->pos));
                if (newdis < 1)
                    continue;

                if (newdis < dis) {
                    closestItr = dstItr;
                    dis = newdis;
                }
            }

            if (closestItr != allCamps.end()) {
                m_NearPoint = closestItr->pos;
                m_NearPointName = closestItr->name;
                auto dstCampItr = closestItr->diff.find(curMapIdx);
                if (dstCampItr != closestItr->diff.end()) {
                    m_NearPointDesc = dstCampItr->second;
                }
            }
        }

        if (m_CampIndex >= 0 && m_CampIndex < campLoc.diff.size()) {
            ImGui::Text("落地点附近 (%d,%d) 处是\n %s",
                m_NearPoint.x, m_NearPoint.y, m_NearPointDesc.c_str());
            icons.push_back({ m_NearPoint, Icons_::CAMP });
        }
    } while (false);
}
