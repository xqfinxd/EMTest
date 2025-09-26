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

bool IsLandHere(
    const rapidjson::Value& value,
    const std::string& landing) {
    auto itr = value.FindMember("Spawn Point");
    if (itr == value.MemberEnd())
        return false;
    if (landing.compare(itr->value.GetString()) != 0)
        return false;

    return true;
}

std::string_view GetCampType(
    const rapidjson::Value& value,
    const char* morm, // Minor Base or Major Base
    const std::string& landing) {
    std::string_view result;
    do {
        auto itr = value.FindMember(morm);
        if (itr == value.MemberEnd())
            break;
        auto campItr = itr->value.FindMember(landing.c_str());
        if (campItr == itr->value.MemberEnd())
            break;
        if (!campItr->value.IsString())
            break;
        result = campItr->value.GetString();
    } while (false);
    
    return result;
}

template<class T>
using Stringify = std::function<std::string(const T&)>;

template<class T>
static bool RenderCombo(const char* label,
    const T& container, int& idx,
    Stringify<typename T::value_type>&& tostr) {
    std::string text;
    if (idx >= 0 && idx < container.size()) {
        auto itr = std::cbegin(container);
        std::advance(itr, idx);
        text = tostr(*itr);
    }

    bool changed = false;
    if (ImGui::BeginCombo(label, text.c_str())) {
        for (auto itr = std::cbegin(container);
            itr != std::cend(container); ++itr) {
            int i = std::distance(std::cbegin(container), itr);
            bool selected = idx == i;

            std::string item = tostr(*itr);
            item = item + "##" + label + std::to_string(i);
            if (ImGui::Selectable(item.c_str(), selected)) {
                if (idx != i) changed = true;
                idx = i;
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
    auto& terrain = m_Variables.GetTerrains();
    m_Terrains.assign(terrain.begin(), terrain.end());
}

void MapFilter::RenderImGui(MapViewer& mapViewer) {
    using namespace std;
    
    do {
        if (FilterTerrain()) {
            const char* terrain = m_Terrains[m_TerrainIndex].data();
            mapViewer.ReloadMap(terrain);
            m_Thumbnail.LoadMap(terrain);

            // reset
            set<string_view> tmp;
            m_Thumbnail.Foreach([&tmp](const rapidjson::Value& value) {
                auto itr = value.FindMember("Spawn Point");
                if (itr == value.MemberEnd())
                    return;
                tmp.insert(itr->value.GetString());
            });
            m_Landings.assign(tmp.begin(), tmp.end());
            m_LandingIndex = -1;
            m_SmallCampTypes.clear();
            m_SmallCampTypeIndex = -1;
            m_CampTypes.clear();
            m_CampTypeIndex = -1;
            m_MapIdx = -1;

            // map icon
            auto& icons = mapViewer.MakeIcons();
            icons.clear();
            for (const auto& landing : m_Landings) {
                if (auto pos = m_Thumbnail.MinorLoc(landing.data()))
                    icons.push_back({ *pos, Icons_::SPAWN_POINT });
            }
        }
        
        if (m_TerrainIndex < 0 || m_TerrainIndex >= m_Terrains.size())
            break;

        if (FilterLanding()) {
            set<string_view> tmp;
            m_Thumbnail.Foreach([&tmp, this](const rapidjson::Value& value) {
                auto& landing = m_Landings[m_LandingIndex];
                if (!IsLandHere(value, landing))
                    return;
                auto campType = GetCampType(value, "Minor Base", landing);
                
                tmp.insert(campType);
            });

            // reset
            m_SmallCampTypes.assign(tmp.begin(), tmp.end());;
            m_SmallCampTypeIndex = -1;
            m_CampTypes.clear();
            m_CampTypeIndex = -1;
            m_MapIdx = -1;

            // map icon
            auto& icons = mapViewer.MakeIcons();
            icons.clear();
            auto& landing = m_Landings[m_LandingIndex];
            if (auto pos = m_Thumbnail.MinorLoc(landing.data()))
                icons.push_back({ *pos, Icons_::SPAWN_POINT });
        }

        if (m_LandingIndex < 0 || m_LandingIndex >= m_Landings.size())
            break;

        if (FilterSmallCampType()) {
            auto& landcamp = m_Landings[m_LandingIndex];
            m_NearCamp = m_Thumbnail.Near(landcamp.data());

            m_CampTypes.clear();
            m_Thumbnail.Foreach([this](const rapidjson::Value& value) {
                auto& landing = m_Landings[m_LandingIndex];
                if (!IsLandHere(value, landing))
                    return;

                auto idxItr = value.FindMember("index");
                if (idxItr == value.MemberEnd())
                    return;
                int mapIdx = value["index"].GetInt();

                std::string campType = GetCampType(value, "Major Base", m_NearCamp).data();
                m_CampTypes[mapIdx] = campType;
            });
            m_CampTypeIndex = -1;
            m_MapIdx = -1;
        }

        if (m_SmallCampTypeIndex < 0 || m_SmallCampTypeIndex >= m_SmallCampTypes.size())
            break;

        if (FilterNearCamp()) {
            auto itr = m_CampTypes.begin();
            std::advance(itr, m_CampTypeIndex);
            m_MapIdx = itr->first;
        }

        if (m_CampTypeIndex < 0 || m_CampTypeIndex >= m_CampTypes.size())
            break;

        ImGui::Text("MapIndex: %d", m_MapIdx);
    } while (false);
}

bool MapFilter::FilterTerrain() {
    // 选择地形
    bool changed = RenderCombo("地形", m_Terrains, m_TerrainIndex,
        [](const std::string_view& view) {
            return std::string(view);
        }
    );

    return changed && m_TerrainIndex >= 0 && m_TerrainIndex < m_Terrains.size();
}

bool MapFilter::FilterLanding() {
    // 选择落地点
    bool changed = RenderCombo("落地点", m_Landings, m_LandingIndex,
        [this](const std::string_view& loc) {
            if (auto pos = m_Thumbnail.MinorLoc(loc.data()))
                return ivec2tostr(*pos);
            return std::string("--------");
        }
    );

    return changed && m_LandingIndex >= 0 && m_LandingIndex < m_Landings.size();
}

bool MapFilter::FilterSmallCampType() {
    // 选择落地营地
    bool changed = RenderCombo("落地营地", m_SmallCampTypes, m_SmallCampTypeIndex,
        [](const std::string_view& camp) {
            return std::string(camp);
        }
    );

    return changed;
}

bool MapFilter::FilterNearCamp() {
    using CampType = decltype(m_CampTypes)::value_type;
    bool changed = RenderCombo("附近地点", m_CampTypes, m_CampTypeIndex,
        [](const CampType& camp) {
            return camp.second;
        }
    );

    return changed;
}
