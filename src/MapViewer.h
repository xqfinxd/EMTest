#pragma once

#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif
#include <vector>
#include <iostream>
#include <functional>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "AssetUtils.h"
#include "FontAtlas.h"

class MapFilter;
using Callback = std::function<void(MapFilter*, void*)>;

class MapButton {
    friend class MapViewer;
public:
    MapButton(glm::vec2 pos_, const char* name_, int layer_) {
        pos = pos_;
        name = name_;
        SetLayer(layer_);
    }
    MapButton& SetScale(float value) {
        scale = value;
        return *this;
    }
    MapButton& SetCallback(Callback&& func) {
        onclick = func;
        return *this;
    }
    MapButton& SetUserData(void* ud) {
        userdata = ud;
        return *this;
    }
    MapButton& SetText(string_view str) {
        text = str;
        return *this;
    }
    MapButton& SetLayer(int value) {
        layer = 1 << value;
        return *this;
    }
    void Delete() {
        layer = -1;
    }
    std::string_view Name() const {
        return name;
    }
    const void* UserData() const {
        return userdata;
    }

private:
    glm::vec2 pos{ 0,0 };
    std::string name;
    const glm::ivec4* rect = nullptr;

    float scale = 1;
    std::string text;
    int layer = -1;
    void* userdata = nullptr;
    Callback onclick;
};

class MapViewer {
public:
    struct Transform {
        glm::vec2 offset{ 0,0 };
        float zoom = 1.0f;
    };

private:
    GLuint m_TexPipeline = 0;
    GLuint m_MapVAO = 0;
    GLuint m_MapVBO = 0;
    GLuint m_MapTexture = 0;
    glm::ivec2 m_MapSize{};

    GLuint m_FontPipeline = 0;
    GLuint m_IconVAO = 0;
    GLuint m_IconVBO = 0;
    GLuint m_IconsTexture = 0;
    glm::ivec2 m_IconsSize;
    IconAtlas m_IconAtlas;
    std::vector<MapButton> m_IconList;
    int m_IconFlags = -1;
    bool m_DirtyIcons = true;
    uint32_t m_IconVertexSize = 0;

    FontAtlas m_FontAtlas;
    GLuint m_FontVAO = 0;
    GLuint m_FontVBO = 0;
    uint32_t m_FontVertexSize = 0;
    glm::vec4 m_FontColor{ 1,1,0,1 };
    float m_FontScale = 0.6f;

    Transform m_Transform;
    glm::ivec4 m_Viewport{};
    glm::vec2 m_OriginViewSize{};

    void InitMapPipeline();
    void InitIconPipeline();
    void SetupBuffer(GLuint& vbo, GLuint& vao, int elemSize);

    void DrawMap(const glm::mat4& vpMat);
    void DrawIcons(const glm::mat4& vpMat);
    void DrawTexts(const glm::mat4& vpMat);
    void UpdateIconBuffer();
    void UpdateFontBuffer();

    glm::vec2 GetViewSize() const;
    glm::vec2 Normalize(const glm::vec2& pos) const;
    glm::vec2 Screen2Map(const glm::vec2& pos) const;
    void OnResizeMap();

public:
    void Initialize();
    void Cleanup();
    void Render();
    void RenderImGui();
    void BuildFont(const std::string& str);

    void Constrain();
    void OnClick(MapFilter* filter, int x, int y) const;

    void SetViewport(const glm::ivec4& viewport);
    void ReloadMap(const char* mapName);

    void ForeachButton(std::function<void(MapButton&)>&& func) {
        std::for_each(m_IconList.begin(), m_IconList.end(), func);
    }
    void RemoveAllButtons() {
        ForeachButton([](MapButton& btn) { btn.Delete(); });
    }
    void SetButtonFlagBits(int flags) {
        m_IconFlags = flags;
    }
    void RemoveAllButtons(int layer) {
        ForeachButton([layer](MapButton& btn) {
            int value = 1 << layer;
            if (btn.layer == value)
                btn.Delete(); 
        });
    }
    MapButton& AddButton(const glm::vec2& pos, const char* name) {
        m_IconList.emplace_back(pos, name, 0);
        m_DirtyIcons = true;
        return m_IconList.back();
    }
    MapButton& AddButton(const glm::vec2& pos, const char* name, int layer) {
        m_IconList.emplace_back(pos, name, layer);
        m_DirtyIcons = true;
        return m_IconList.back();
    }
    
    void vZoom(float value);
    void vMove(int x, int y);
    void vReset();
    bool TestPoint(int x, int y) const;
};
