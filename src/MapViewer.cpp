#include "MapViewer.h"
#include <string>
#include <imgui.h>
#include "GLUtils.h"

static constexpr glm::vec2 ZOOM_RANGE(1, 5);

void MapViewer::InitMapPipeline() {
    float vertices[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint mapVBO, mapEBO;

    glGenVertexArrays(1, &m_MapVAO);
    glGenBuffers(1, &mapVBO);
    glGenBuffers(1, &mapEBO);

    glBindVertexArray(m_MapVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mapEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    GLuint vs = CompileShaderFile(GL_VERTEX_SHADER, DATA_DIR("map.vert").c_str());
    GLuint fs = CompileShaderFile(GL_FRAGMENT_SHADER, DATA_DIR("map.frag").c_str());

    m_MapPipeline = glCreateProgram();
    glAttachShader(m_MapPipeline, vs);
    glAttachShader(m_MapPipeline, fs);
    glLinkProgram(m_MapPipeline);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glDeleteBuffers(1, &mapVBO);
    glDeleteBuffers(1, &mapEBO);
}

void MapViewer::InitIconPipeline() {
    float vertices[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint iconVBO, iconEBO;

    glGenVertexArrays(1, &m_IconVAO);
    glGenBuffers(1, &iconVBO);
    glGenBuffers(1, &iconEBO);

    glBindVertexArray(m_IconVAO);

    glBindBuffer(GL_ARRAY_BUFFER, iconVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iconEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    GLuint vs = CompileShaderFile(GL_VERTEX_SHADER, DATA_DIR("icon.vert").c_str());
    GLuint fs = CompileShaderFile(GL_FRAGMENT_SHADER, DATA_DIR("icon.frag").c_str());

    m_IconPipeline = glCreateProgram();
    glAttachShader(m_IconPipeline, vs);
    glAttachShader(m_IconPipeline, fs);
    glLinkProgram(m_IconPipeline);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glDeleteBuffers(1, &iconVBO);
    glDeleteBuffers(1, &iconEBO);
}

void MapViewer::DrawMap(const glm::mat4& vpMat) {
    glUseProgram(m_MapPipeline);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 modelMatrix = glm::mat4(1.f);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(m_MapSize, 1.f));

    glm::mat4 mvpMatrix = vpMat * modelMatrix;
    GLint mvpLoc = glGetUniformLocation(m_MapPipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_MapTexture);

    glBindVertexArray(m_MapVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void MapViewer::DrawIcon(const glm::mat4& vpMat, const MapButton& btn) {
    if (!btn.rect) return;

    glUseProgram(m_IconPipeline);
    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::vec2 size(btn.rect->z, btn.rect->w);
    glm::mat4 modelMat = glm::mat4(1.f);
    glm::vec2 m02c = glm::vec2(btn.pos) - glm::vec2(m_MapSize) / 2.f;
    modelMat = glm::translate(modelMat, glm::vec3(m02c.x, -m02c.y, 0.f));
    modelMat = glm::scale(modelMat, glm::vec3(size * btn.scale, 1.f));
    
    glm::mat4 mvpMatrix = vpMat * modelMat;
    GLint mvpLoc = glGetUniformLocation(m_IconPipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glm::vec2 texOffset(btn.rect->x, btn.rect->y);
    texOffset /= m_IconsSize;
    glm::vec2 texSize(size);
    texSize /= m_IconsSize;

    texOffset.y = 1 - texOffset.y - texSize.y;
    GLint offsetLoc = glGetUniformLocation(m_IconPipeline, "offset");
    glUniform2fv(offsetLoc, 1, glm::value_ptr(texOffset));

    GLint sizeLoc = glGetUniformLocation(m_IconPipeline, "size");
    glUniform2fv(sizeLoc, 1, glm::value_ptr(texSize));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_IconsTexture);

    glBindVertexArray(m_IconVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

glm::vec2 MapViewer::GetViewSize() const {
    return m_OriginViewSize / m_Transform.zoom;;
}

glm::vec2 MapViewer::Normalize(const glm::vec2& pos) const {
    glm::vec2 vsize = GetViewSize();
    float mapdeltax = (pos.x - m_Viewport.x) * (vsize.x / m_Viewport.z);
    float mapdeltay = (pos.y - m_Viewport.y) * (vsize.y / m_Viewport.w);
    return glm::vec2(mapdeltax, mapdeltay);
}

glm::vec2 MapViewer::Screen2Map(const glm::vec2& pos) const {
    glm::vec2 vsize = GetViewSize();
    // map center to view center
    glm::vec2 vc2mc(m_Transform.offset);
    // view zero(left top) to current point
    glm::vec2 v02c = Normalize(pos);
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

    m_IconsTexture = LoadTexture(
        TEX_DIR("icons.png").c_str(),
        m_IconsSize.x, m_IconsSize.y, true);
    m_Atlas.Initialize();

    ReloadMap("bg");

    vReset();
}

void MapViewer::Cleanup() {
    glDeleteVertexArrays(1, &m_MapVAO);
    glDeleteProgram(m_MapPipeline);

    glDeleteVertexArrays(1, &m_IconVAO);
    glDeleteProgram(m_IconPipeline);
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

    auto vpMat = projMatrix * viewMatrix;
    DrawMap(vpMat);

    auto itr = std::remove_if(m_IconList.begin(), m_IconList.end(),
        [](const MapButton& btn) {
            return btn.layer < 0;
        }
    );
    m_IconList.erase(itr, m_IconList.end());
    for (auto& info : m_IconList) {
        if (!info.rect)
            info.rect = m_Atlas.QueryIcon(info.name.c_str());
        if (0 == (m_IconFlags & info.layer))
            continue;

        DrawIcon(vpMat, info);
    }
}

void MapViewer::RenderImGui() {
    const auto& mpos = ImGui::GetIO().MousePos;
    auto mappos = Screen2Map(glm::vec2(mpos.x, mpos.y));
    ImGui::Text("地图坐标 : %d,%d", (int)mappos.x, (int)mappos.y);
    auto& view = m_Transform;
    ImGui::SliderFloat("缩放", &view.zoom, ZOOM_RANGE.x, ZOOM_RANGE.y);
    ImGui::DragFloat2("偏移", glm::value_ptr(view.offset), 2);
}

void MapViewer::Constrain() {
    m_Transform.zoom = glm::clamp(m_Transform.zoom,
        ZOOM_RANGE.x, ZOOM_RANGE.y);

    glm::vec2 viewOffset = glm::vec2(m_MapSize) - GetViewSize();
    glm::vec2 range = glm::abs(viewOffset) / 2.f;
    m_Transform.offset = glm::clamp(m_Transform.offset, -range, range);
}

void MapViewer::OnClick(MapFilter* filter, int x, int y)  const {
    auto mapPos = Screen2Map(glm::vec2(x, y));
    for (auto& btn : m_IconList) {
        if (!btn.rect) continue;
        if (btn.layer < 0) continue;
        if (!btn.onclick) continue;
        if (0 == (btn.layer & m_IconFlags))
            continue;

        float dis = glm::distance(mapPos, btn.pos);
        float range = glm::max(btn.rect->z, btn.rect->w) / 2;
        if (dis <= range * 1.4142f * btn.scale) {
            btn.onclick(filter, btn.userdata);
        }
    }
}

void MapViewer::SetViewport(const glm::ivec4& viewport) {
    m_Viewport = viewport;
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
    vReset();
    OnResizeMap();
}

void MapViewer::vZoom(float value) {
    m_Transform.zoom += value;
}

void MapViewer::vMove(int x, int y) {
    glm::vec2 offset(x, y);
    m_Transform.offset += Normalize(offset);
}

void MapViewer::vReset() {
    m_Transform.zoom = 1.f;
    m_Transform.offset = glm::vec2(0, 0);
}

bool MapViewer::TestPoint(int x, int y) const {
    const auto& vp = m_Viewport;
    return x > vp.x && x < vp.x + vp.z
        && y > vp.y && y < vp.y + vp.w;
}
