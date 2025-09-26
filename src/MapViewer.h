#pragma once

#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif
#include <vector>
#include <iostream>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "AssetUtils.h"

class MapFilter;

class MapViewer {
public:
    using Callback = std::function<void(MapFilter*, void*)>;
    struct Transform {
        glm::vec2 offset{ 0,0 };
        float zoom = 1.0f;
    };

    struct MapButton {
        MapButton(glm::vec2 pos_, const char* name_) {
            pos = pos_;
            name = name_;
        }
        glm::vec2 pos{ 0,0 };
        std::string name;

        float scale = 1;
        std::string text;
        void* userdata = nullptr;
        Callback onclick;
    };

private:
    GLuint m_ImagePipeline = 0;
    GLuint m_ImageVAO = 0;
    GLuint m_ImageVBO = 0;
    GLuint m_ImageEBO = 0;

    GLuint m_MapTexture = 0;
    glm::ivec2 m_MapSize{};

    GLuint m_IconsTexture = 0;
    glm::ivec2 m_IconsSize;

    Transform m_Transform;
    glm::ivec4 m_Viewport{};
    glm::vec2 m_OriginViewSize{};

    IconAtlas m_Atlas;
    std::vector<MapButton> m_IconList;

    void InitImagePipeline();
    void DrawMap(const glm::mat4& vpMat);
    void DrawIcon(const glm::mat4& vpMat, const MapButton& btn);

    glm::vec2 GetViewSize() const;
    glm::vec2 Normalize(const glm::vec2& pos) const;
    glm::vec2 Screen2Map(const glm::vec2& pos) const;
    void OnResizeMap();

public:
    void Initialize();
    void Cleanup();
    void Render();
    void RenderImGui();
    void Constrain();

    void SetViewport(const glm::ivec4& viewport);
    void ReloadMap(const char* mapName);
    std::vector<MapButton>& MakeIcons() {
        return m_IconList;
    }

    void vZoom(float value);
    void vMove(int x, int y);
    void vReset();
    bool TestPoint(int x, int y) const;
};
