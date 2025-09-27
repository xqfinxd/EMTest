#include "MapFilter.h"
#include <set>
#include <functional>

#include <SDL_Log.h>
#include <imgui.h>

namespace Icons_ {
    constexpr static const char* SPAWN_POINT = "Padding 1";
    constexpr static const char* MAJOR_BASE = "Padding 2";
    constexpr static const char* ROT_BLESSING = "Rot Blessing";
    constexpr static const char* BOSS = "Boss";
    constexpr static const char* RED_BOSS = "Red Boss";
    constexpr static const char* EVERGOAL = "Evergaol";
    constexpr static const char* CAMP = "Camp";
    constexpr static const char* SMALL_CAMP = "Small Camp";
    constexpr static const char* CHURCH = "Church";
    constexpr static const char* GREAT_CHURCH = "Great Church";
    constexpr static const char* TOWNSHIP = "Township";
    constexpr static const char* SORCERERS_RISE = "Sorcerer's Rise";
    constexpr static const char* RUINS = "Ruins";
    constexpr static const char* FORT = "Fort";
    constexpr static const char* CIRCLE = "Circle";
    constexpr static const char* CART = "Cart";
    constexpr static const char* DEMON_MERCHANT = "Demon Merchant";
    
    constexpr static float SPAWN_POINT_SCALE_1 = 0.4f;
    constexpr static float SPAWN_POINT_SCALE_2 = 0.8f;
    constexpr static float MAJOR_BASE_SCALE = 0.5f;
    constexpr static float EVERGOAL_SCALE = 0.6f;
    constexpr static float BOSS_SCALE = 0.6f;
    constexpr static float ROT_BLESSING_SCALE = 0.4f;
    constexpr static float DEMON_MERCHANT_SCALE = 0.6f;

    const char* From(const std::string& name_, float& scale) {
        std::string name = name_;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        scale = 1;
        if (name.npos != name.find("ruins")) {
            scale = 0.6f;
            return RUINS;
        }
        else if (name.npos != name.find("fort")) {
            scale = 0.6f;
            return FORT;
        }
        else if (name.npos != name.find("sorcerer's rise")) {
            scale = 0.6f;
            return SORCERERS_RISE;
        }
        else if (name.npos != name.find("camp")) {
            if (name.npos != name.find("small camp")) {
                if (name.npos != name.find("caravans")) {
                    scale = 0.4f;
                    return CART;
                }
                else {
                    scale = 0.3f;
                    return SMALL_CAMP;
                }
            }
            else {
                scale = 0.6f;
                return CAMP;
            }
        }
        else if (name.npos != name.find("church")) {
            if (name.npos != name.find("great church")) {
                scale = 0.6f;
                return GREAT_CHURCH;
            }
            else {
                scale = 0.7f;
                return CHURCH;
            }
        }
        else if (name.npos != name.find("township")) {
            scale = 0.6f;
            return TOWNSHIP;
        }
        return "";
    }
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

void MapFilter::Initialize(MapViewer* view) {
    m_Viewer = view;
    m_Variables.Initialize();
    auto& terrain = m_Variables.GetTerrains();
    m_Terrains.assign(terrain.begin(), terrain.end());
}

static int GetFlags(std::set<int>&& values) {
    int flags = 0;
    for (int v : values) {
        flags = flags | (1 << v);
    }
    return flags;
}

void MapFilter::RenderImGui() {
    using namespace std;
    
    do {
        if (FilterTerrain()) {
            OnFilterTerrain();
        }
        
        if (m_TerrainIndex < 0 || m_TerrainIndex >= m_Terrains.size())
            break;

        if (FilterLanding()) {
            OnFilterLanding();
        }

        if (m_LandingIndex < 0 || m_LandingIndex >= m_Landings.size())
            break;

        if (FilterSmallCampType()) {
            OnFilterSmallCampType();
        }

        if (m_SmallCampTypeIndex < 0 || m_SmallCampTypeIndex >= m_SmallCampTypes.size())
            break;

        if (FilterNearCamp()) {
            OnFilterNearCamp();
        }

        if (m_CampTypeIndex < 0 || m_CampTypeIndex >= m_CampTypes.size())
            break;

        const auto& detail = m_MapDetail;
        ImGui::Text("地图索引: %d", detail.index);
        ImGui::Text("夜王: %s", detail.nightlord.c_str());
        ImGui::Text("第一夜BOSS: %s", detail.night_1_boss.c_str());
        ImGui::Text("第一夜BOSS: %s", detail.night_2_boss.c_str());
        if (detail.special_event.empty()) {
            ImGui::Text("特殊事件: %s", detail.nightlord.c_str());
            if (!detail.extra_boss.empty()) {
                ImGui::Text("额外夜晚BOSS: %s", detail.extra_boss.c_str());
            }
        }
        if (!detail.castle_type.empty()) {
            ImGui::Text("主城类型: %s", detail.castle_type.c_str());
            ImGui::Text("主城地下: %s", detail.castle_basement.c_str());
            ImGui::Text("主城楼顶: %s", detail.castle_rooftop.c_str());
        }
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

void MapFilter::OnFilterTerrain() {
    const char* terrain = m_Terrains[m_TerrainIndex].data();
    m_Viewer->ReloadMap(terrain);
    m_Thumbnail.LoadMap(terrain);

    // reset
    std::set<std::string_view> tmp;
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
    m_MapDetail.Reset();

    // map icon
    m_Viewer->RemoveAllButtons(1);
    for (int i = 0; i < m_Landings.size(); i++) {
        auto& landing = m_Landings[i];
        auto pos = m_Thumbnail.Query(eMinorBase, landing.data());
        if (!pos) continue;
        m_Viewer->AddButton(*pos, Icons_::SPAWN_POINT, 1)
            .SetLayer(1)
            .SetScale(Icons_::SPAWN_POINT_SCALE_1)
            .SetUserData((void*)i)
            .SetCallback(
                [](MapFilter* filter, void* ud) {
                    int idx = (int)ud;
                    if (idx >= 0 && idx < filter->m_Landings.size()) {
                        filter->m_LandingIndex = idx;
                        filter->OnFilterLanding();
                    }
                });
    }
    m_Viewer->SetButtonFlagBits(GetFlags({1}));
}

bool MapFilter::FilterLanding() {
    // 选择落地点
    bool changed = RenderCombo("落地点", m_Landings, m_LandingIndex,
        [this](const std::string_view& loc) {
            if (auto pos = m_Thumbnail.Query(eMinorBase, loc.data()))
                return ivec2tostr(*pos);
            return std::string("--------");
        }
    );

    return changed && m_LandingIndex >= 0 && m_LandingIndex < m_Landings.size();
}

void MapFilter::OnFilterLanding() {
    std::set<std::string_view> tmp;
    m_Thumbnail.Foreach([&tmp, this](const rapidjson::Value& value) {
        auto& landing = m_Landings[m_LandingIndex];
        if (!IsLandHere(value, landing))
            return;
        auto campType = GetCampType(value, "Minor Base", landing);

        tmp.insert(campType);
    });

    // reset
    m_SmallCampTypes.assign(tmp.begin(), tmp.end());;
    auto& landcamp = m_Landings[m_LandingIndex];
    m_NearCamp = m_Thumbnail.Near(landcamp.data());
    m_SmallCampTypeIndex = -1;
    m_CampTypes.clear();
    m_CampTypeIndex = -1;
    m_MapDetail.Reset();

    // map icon
    m_Viewer->ForeachButton([this](MapButton& btn) {
        if (btn.Name() != Icons_::SPAWN_POINT) {
            btn.Delete();
            return;
        }
        if (m_LandingIndex == (int)btn.UserData())
            btn.SetScale(Icons_::SPAWN_POINT_SCALE_2);
        else
            btn.SetScale(Icons_::SPAWN_POINT_SCALE_1);
    });

    if (auto pos = m_Thumbnail.Query(eMajorBase, m_NearCamp.c_str())) {
        m_Viewer->RemoveAllButtons(2);
        m_Viewer->AddButton(*pos, Icons_::MAJOR_BASE, 2)
            .SetScale(Icons_::MAJOR_BASE_SCALE);
    }
    m_Viewer->SetButtonFlagBits(GetFlags({ 1,2 }));
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

void MapFilter::OnFilterSmallCampType() {
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
    m_MapDetail.Reset();
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

void MapFilter::OnFilterNearCamp() {
    auto itr = m_CampTypes.begin();
    std::advance(itr, m_CampTypeIndex);
    m_MapDetail.index = itr->first;

    auto target = m_Thumbnail.Find([this](const rapidjson::Value& value) {
        auto idxItr = value.FindMember("index");
        if (idxItr == value.MemberEnd())
            return false;
        int mapIdx = value["index"].GetInt();
        return m_MapDetail.index == mapIdx;
    });

    if (target)
        m_MapDetail.Load(*target, m_Thumbnail);

    auto& detail = m_MapDetail;
    m_Viewer->RemoveAllButtons(3);
    m_Viewer->AddButton(detail.spawn_point, Icons_::SPAWN_POINT, 3);
    m_Viewer->AddButton(detail.day_1_circle, Icons_::CIRCLE, 3);
    m_Viewer->AddButton(detail.day_2_circle, Icons_::CIRCLE, 3);
    for (const auto& e : detail.major) {
        float scale = 1;
        const auto* name = Icons_::From(e.second, scale);
        m_Viewer->AddButton(e.first, name, 3)
            .SetScale(scale);
    }
    for (const auto& e : detail.minor) {
        float scale = 1;
        const auto* name = Icons_::From(e.second, scale);
        m_Viewer->AddButton(e.first, name, 3)
            .SetScale(scale);
    }
    for (const auto& e : detail.evergaol) {
        m_Viewer->AddButton(e.first, Icons_::EVERGOAL, 3)
            .SetScale(Icons_::EVERGOAL_SCALE);
    }
    for (const auto& e : detail.field) {
        if (e.second[0] == '*') {
            m_Viewer->AddButton(e.first, Icons_::RED_BOSS, 3)
                .SetScale(Icons_::BOSS_SCALE);
        }
        else {
            m_Viewer->AddButton(e.first, Icons_::BOSS, 3)
                .SetScale(Icons_::BOSS_SCALE);
        }
    }
    for (const auto& e : detail.rotted_woods) {
        m_Viewer->AddButton(e.first, Icons_::RED_BOSS, 3)
            .SetScale(Icons_::BOSS_SCALE);
    }
    m_Viewer->AddButton(detail.rot_blessing, Icons_::ROT_BLESSING, 3)
        .SetScale(Icons_::ROT_BLESSING_SCALE);
    m_Viewer->AddButton(detail.frenzy_tower, Icons_::ROT_BLESSING, 3)
        .SetScale(Icons_::ROT_BLESSING_SCALE);
    m_Viewer->AddButton(detail.demon_merchant, Icons_::DEMON_MERCHANT, 3)
        .SetScale(Icons_::DEMON_MERCHANT_SCALE);
    m_Viewer->SetButtonFlagBits(GetFlags({ 3 }));
}
