#pragma once

#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <memory>

#include "MapDefines.h"
#include "AssetUtils.h"

class MapViewer {
public:
    struct Transform {
        glm::vec2 offset{ 0,0 };
        float zoom = 1.0f;
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

    std::unique_ptr<IconMgr> m_Atlas;

    void InitImagePipeline();
    void DrawMap(const glm::mat4& vpMat);
    void DrawIcon(const glm::mat4& vpMat, const std::string& name, glm::ivec2 pos);

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

    void vZoom(float value);
    void vMove(int x, int y);
    void vReset();
    bool TestPoint(int x, int y) const;
};
