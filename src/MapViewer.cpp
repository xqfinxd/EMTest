#include "MapViewer.h"
#include <string>
#include <imgui.h>
#include "GLUtils.h"

static constexpr glm::vec2 ZOOM_RANGE(1, 8);

void MapViewer::InitMapPipeline() {
    GLuint vs = CompileShaderFile(GL_VERTEX_SHADER, DATA_DIR("texture.vert").c_str());
    GLuint fs = CompileShaderFile(GL_FRAGMENT_SHADER, DATA_DIR("texture.frag").c_str());

    m_TexPipeline = glCreateProgram();
    glAttachShader(m_TexPipeline, vs);
    glAttachShader(m_TexPipeline, fs);
    glLinkProgram(m_TexPipeline);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void MapViewer::InitIconPipeline() {
    GLuint vs = CompileShaderFile(GL_VERTEX_SHADER, DATA_DIR("font.vert").c_str());
    GLuint fs = CompileShaderFile(GL_FRAGMENT_SHADER, DATA_DIR("font.frag").c_str());

    m_FontPipeline = glCreateProgram();
    glAttachShader(m_FontPipeline, vs);
    glAttachShader(m_FontPipeline, fs);
    glLinkProgram(m_FontPipeline);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void MapViewer::SetupBuffer(GLuint& vbo, GLuint& vao, int elemSize) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    int drawType = elemSize == 1 ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float) * elemSize, nullptr, drawType);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void MapViewer::UpdateIconBuffer() {
    std::vector<float> vertices;
    for (auto& btn : m_IconList) {
        if (!btn.rect) {
            btn.rect = m_IconAtlas.QueryRect(btn.name.c_str());
        }
        if (0 == (m_IconFlags & btn.layer)) continue;
        if (!btn.rect) continue;

        float hw = btn.rect->z * btn.scale / 2.f;
        float hh = btn.rect->w * btn.scale / 2.f;
        float x = btn.pos.x - (m_MapSize.x / 2.f);
        float y = (m_MapSize.y / 2.f) - btn.pos.y;
        float u0 = 1.f * btn.rect->x / m_IconsSize.x;
        float v0 = 1.f - 1.f * btn.rect->y / m_IconsSize.y;
        float u1 = 1.f * (btn.rect->x + btn.rect->z) / m_IconsSize.x;
        float v1 = 1.f - 1.f * (btn.rect->y + btn.rect->w) / m_IconsSize.y;
        vertices.insert(vertices.end(), {
            x - hw, y - hh, u0, v1,
            x + hw, y - hh, u1, v1,
            x + hw, y + hh, u1, v0,
            x + hw, y + hh, u1, v0,
            x - hw, y + hh, u0, v0,
            x - hw, y - hh, u0, v1,
        });
    }
    if (!vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_IconVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        m_IconVertexSize = vertices.size() / 4;
    }
}

void MapViewer::UpdateFontBuffer() {
    std::vector<float> vertices;

    const float scale = m_FontScale;
    constexpr float textSpace = 20;
    for (auto& btn : m_IconList) {
        if (0 == (m_IconFlags & btn.layer)) continue;

        float cursorX = btn.pos.x;
        float cursorY = btn.pos.y;

        std::wstring wideText = ConvertFrom(btn.text);
        std::vector<glm::vec4> baseVertices;
        baseVertices.reserve(wideText.size());
        float textHeight = 0;
        for (wchar_t charCode : wideText) {
            const auto& charInfo = m_FontAtlas.GetCharInfo((uint32_t)charCode);

            if (!charInfo.generated) continue;

            float x = cursorX;
            x = x - (m_MapSize.x / 2.f);
            float y = cursorY - (charInfo.height) * scale;
            y = (m_MapSize.y / 2.f) - y;
            float h = charInfo.height * scale;
            textHeight = glm::max(h, textHeight);
            float w = charInfo.width * scale;
            float u0 = charInfo.u0;
            float v0 = charInfo.v0;
            float u1 = charInfo.u1;
            float v1 = charInfo.v1;

            baseVertices.insert(baseVertices.end(), {
                {x,     y    , u0, v1},
                {x + w, y    , u1, v1},
                {x + w, y + h, u1, v0},
                {x + w, y + h, u1, v0},
                {x,     y + h, u0, v0},
                {x,     y    , u0, v1},
            });

            cursorX += charInfo.width * scale;
        }
        float offsetx = (cursorX - btn.pos.x) / 2;
        float offsety = textHeight + textSpace;
        if (btn.rect)
            offsety += btn.rect->w * btn.scale / 2;
        std::for_each(baseVertices.begin(), baseVertices.end(),
            [offsetx, offsety, &vertices](const glm::vec4& v) {
                vertices.insert(vertices.end(), {
                    v.x - offsetx, v.y - offsety, v.z, v.w,
                });
            }
        );
    }
    if (!vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_FontVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        m_FontVertexSize = vertices.size() / 4;
    }
}

void MapViewer::DrawMap(const glm::mat4& vpMat) {
    glUseProgram(m_TexPipeline);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 mvpMatrix = vpMat * glm::mat4(1.f);
    GLint mvpLoc = glGetUniformLocation(m_TexPipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_MapTexture);

    glBindVertexArray(m_MapVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void MapViewer::DrawIcons(const glm::mat4& vpMat) {
    if (!m_IconVertexSize) return;

    glUseProgram(m_TexPipeline);
    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 mvpMatrix = vpMat * glm::mat4(1.f);
    GLint mvpLoc = glGetUniformLocation(m_TexPipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_IconsTexture);

    glBindVertexArray(m_IconVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_IconVertexSize);

    glBindVertexArray(0);
}

void MapViewer::DrawTexts(const glm::mat4& vpMat) {
    if (!m_FontVertexSize) return;
    m_FontAtlas.UpdateTexture();

    glUseProgram(m_FontPipeline);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 mvpMatrix = vpMat * glm::mat4(1.f);
    GLint mvpLoc = glGetUniformLocation(m_FontPipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_FontAtlas.GetTextureID());

    GLint colorLoc = glGetUniformLocation(m_FontPipeline, "uColor");
    glUniform4fv(colorLoc, 1, glm::value_ptr(m_FontColor));

    glBindVertexArray(m_FontVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_FontVertexSize);

    glBindVertexArray(0);
}

glm::vec2 MapViewer::GetViewSize() const {
    return m_OriginViewSize / m_Transform.zoom;;
}

glm::vec2 MapViewer::Normalize(glm::vec2 pos) const {
    float x = (pos.x - m_Viewport.x) / m_Viewport.z;
    float y = (pos.y - m_Viewport.y) / m_Viewport.w;
    return glm::vec2(x, y);
}

glm::vec2 MapViewer::Screen2View(glm::vec2 pos) const {
    glm::vec2 vsize = GetViewSize();
    glm::vec2 norm = Normalize(pos);
    return glm::vec2(norm.x * vsize.x, norm.y * vsize.y);
}

glm::vec2 MapViewer::Screen2Map(glm::vec2 pos) const {
    glm::vec2 vsize = GetViewSize();
    // map center to view center
    glm::vec2 vc2mc(m_Transform.offset);
    // view zero(left top) to current point
    glm::vec2 v02c = Screen2View(pos);
    // view center to current point
    glm::vec2 vc2c = v02c - vsize / 2.f;
    return vc2c - vc2mc + glm::vec2(m_MapSize) / 2.f;
}

void MapViewer::OnResizeMap() {
    float vaspect = 1.f * m_Viewport.z / m_Viewport.w;
    float maspect = 1.f * m_MapSize.x / m_MapSize.y;
    m_OriginViewSize = m_MapSize;
    float aspect = vaspect / maspect;
    if (aspect > 1.f)
        m_OriginViewSize.x *= aspect;
    else
        m_OriginViewSize.y /= aspect;
}

void MapViewer::Initialize() {
    InitMapPipeline();
    InitIconPipeline();

    SetupBuffer(m_MapVBO, m_MapVAO, 1);
    SetupBuffer(m_IconVBO, m_IconVAO, 256);
    SetupBuffer(m_FontVBO, m_FontVAO, 2048);

    m_IconsTexture = LoadTexture(
        TEX_DIR("icons.png").c_str(),
        m_IconsSize.x, m_IconsSize.y, true);

    m_IconAtlas.Initialize();
    m_FontAtlas.Initialize(DATA_DIR("simhei.ttf").c_str(), 24, 600);

    ReloadMap("bg");

    vReset();
}

void MapViewer::Cleanup() {
    glDeleteBuffers(1, &m_MapVBO);
    glDeleteVertexArrays(1, &m_MapVAO);
    glDeleteProgram(m_TexPipeline);

    glDeleteVertexArrays(1, &m_FontVAO);
    glDeleteBuffers(1, &m_FontVBO);

    m_FontAtlas.Cleanup();

    glDeleteBuffers(1, &m_IconVBO);
    glDeleteVertexArrays(1, &m_IconVAO);
    glDeleteProgram(m_FontPipeline);
}

void MapViewer::Render() {
    glm::vec2 viewSize = GetViewSize();
    glm::vec2 offset = m_Transform.offset;
    offset.x = -offset.x;

    // left buttom
    glm::vec2 lb = offset - viewSize / 2.f;
    // right top
    glm::vec2 rt = offset + viewSize / 2.f;
    
    glm::mat4 projMatrix = glm::ortho(
        lb.x, rt.x, lb.y, rt.y, -1.0f, 1.0f);

    glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));

    glViewport(m_glViewport.x, m_glViewport.y,
        m_glViewport.z, m_glViewport.w);

    auto vpMat = projMatrix * viewMatrix;
    DrawMap(vpMat);

    auto itr = std::remove_if(m_IconList.begin(), m_IconList.end(),
        [](const MapButton& btn) {
            return btn.layer < 0;
        }
    );
    if (itr != m_IconList.end()) {
        m_IconList.erase(itr, m_IconList.end());
        m_DirtyIcons = true;
    }
    
    if (m_DirtyIcons) {
        UpdateIconBuffer();
        UpdateFontBuffer();
        m_DirtyIcons = false;
    }
    
    DrawIcons(vpMat);
    DrawTexts(vpMat);
}

void MapViewer::RenderImGui() {
    const auto& mpos = ImGui::GetIO().MousePos;
    auto mappos = Screen2Map(glm::vec2(mpos.x, mpos.y));
    std::string fmtLoc = std::string(TR("Map Location")) + ": %d,%d";
    ImGui::ColorEdit4(TR("Font Color").data(), glm::value_ptr(m_FontColor));
    if (ImGui::SliderFloat(TR("Font Scale").data(), &m_FontScale, 0.4f, 2.0f)) {
        m_DirtyIcons = true;
    }
    ImGui::Text(fmtLoc.c_str(), (int)mappos.x, (int)mappos.y);
    auto& view = m_Transform;
    ImGui::SliderFloat(TR("Zoom").data(), &view.zoom, ZOOM_RANGE.x, ZOOM_RANGE.y);
    ImGui::DragFloat2(TR("Offset").data(), glm::value_ptr(view.offset), 2);
    if (ImGui::Button(TR("Reset View").data())) {
        vReset();
    }
}

void MapViewer::BuildFont(const std::string& str) {
    auto ws = ConvertFrom(str);
    std::for_each(ws.begin(), ws.end(),
        [this](const wchar_t& c) {
            m_FontAtlas.GetCharInfo(c);
        }
    );
}

void MapViewer::Constrain() {
    m_Transform.zoom = glm::clamp(m_Transform.zoom,
        ZOOM_RANGE.x, ZOOM_RANGE.y);

    glm::vec2 viewOffset = glm::vec2(m_MapSize) - GetViewSize();
    glm::vec2 range = glm::abs(viewOffset) / 2.f;
    m_Transform.offset = glm::clamp(m_Transform.offset, -range, range);
}

void MapViewer::HandleAction(MapFilter* filter, const MapAction& action) {
    switch (action.type) {
    case MapAction::eSingleTap:
        if (TestPoint(action.pos))
            OnClick(filter, action.pos);
        break;
    case MapAction::eDoubleTap:
        if (TestPoint(action.pos)) {
            vMoveTo(action.pos);
            vZoom(action.scale);
        }
        break;
    case MapAction::eDragMove:
        if (TestPoint(action.pos)) {
            vMove(action.delta);
        }
        break;
    case MapAction::eZoomLocal:
        if (TestPoint(action.pos)) {
            glm::vec2 oldpos = Screen2Map(action.pos);
            vZoom(action.scale);
            glm::vec2 newpos = Screen2Map(action.pos);
            m_Transform.offset += (newpos - oldpos);
        }
        break;
    case MapAction::eZoomCenter:
        if (TestPoint(action.pos)) {
            vZoom(action.scale);
        }
        break;
    default:
        break;
    }
}

void MapViewer::OnClick(MapFilter* filter, glm::vec2 pos)  const {
    auto mapPos = Screen2Map(pos);
    for (auto& btn : m_IconList) {
        if (!btn.rect) continue;
        if (btn.layer < 0) continue;
        if (!btn.onclick) continue;
        if (0 == (btn.layer & m_IconFlags))
            continue;

        float dis = glm::distance(mapPos, btn.pos);
        float range = glm::max(btn.rect->z, btn.rect->w) / 2.f;
        if (dis <= range * 1.4142f * btn.scale) {
            btn.onclick(filter, btn.userdata);
        }
    }
}

void MapViewer::SetViewport(int screenHeight, const glm::ivec4& viewport) {
    m_glViewport = m_Viewport = viewport;
    m_glViewport.y = screenHeight - viewport.w - viewport.y;
    vReset();
    OnResizeMap();
}

void MapViewer::ReloadMap(const char* mapName_) {
    if (m_MapTexture != 0) {
        glDeleteTextures(1, &m_MapTexture);
        m_MapTexture = 0;
    }
    m_MapTexture = LoadTexture(
        TEX_DIR(std::string(mapName_) + ".png").c_str(),
        m_MapSize.x, m_MapSize.y, true);

    float hw = m_MapSize.x / 2.f; // half w
    float hh = m_MapSize.y / 2.f; // half h
    float vertices[] = {
        -hw, -hh, 0.f, 0.f,
         hw, -hh, 1.f, 0.f,
         hw,  hh, 1.f, 1.f,
         hw,  hh, 1.f, 1.f,
        -hw,  hh, 0.f, 1.f,
        -hw, -hh, 0.f, 0.f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, m_MapVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), vertices, GL_DYNAMIC_DRAW);

    vReset();
    OnResizeMap();
}

void MapViewer::vZoom(float value) {
    m_Transform.zoom *= value;
}

void MapViewer::vMove(glm::vec2 offset) {
    glm::vec2 nzero = Screen2View(glm::vec2(0, 0));
    glm::vec2 noffset = Screen2View(offset);
    m_Transform.offset += (noffset - nzero);
}

void MapViewer::vMoveTo(glm::vec2 pos) {
    glm::vec2 mapPos = Screen2Map(pos);
    glm::vec2 mz2mc(m_MapSize / 2);
    m_Transform.offset = mz2mc - mapPos;
}

void MapViewer::vReset() {
    m_Transform.zoom = 1.f;
    m_Transform.offset = glm::vec2(0, 0);
}

bool MapViewer::TestPoint(glm::vec2 pos) const {
    glm::vec4 vp(m_Viewport);
    return pos.x > vp.x && pos.x < vp.x + vp.z
        && pos.y > vp.y && pos.y < vp.y + vp.w;
}
