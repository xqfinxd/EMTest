#include "MapViewer.h"
#include "GLUtils.h"
#include <algorithm>
#include <string>
#include <imgui.h>

static constexpr glm::vec2 ZOOM_RANGE(1, 5);

void MapViewer::InitImagePipeline() {
    float vertices[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &m_ImageVAO);
    glGenBuffers(1, &m_ImageVBO);
    glGenBuffers(1, &m_ImageEBO);

    glBindVertexArray(m_ImageVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_ImageVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ImageEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    GLuint vs = CompileShaderFile(GL_VERTEX_SHADER, GetDataPath("image.vert").c_str());
    GLuint fs = CompileShaderFile(GL_FRAGMENT_SHADER, GetDataPath("image.frag").c_str());

    m_ImagePipeline = glCreateProgram();
    glAttachShader(m_ImagePipeline, vs);
    glAttachShader(m_ImagePipeline, fs);
    glLinkProgram(m_ImagePipeline);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void MapViewer::DrawMap(const glm::mat4& vpMat) {
    glm::mat4 modelMatrix = glm::mat4(1.f);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(m_MapSize, 1.f));

    glm::mat4 mvpMatrix = vpMat * modelMatrix;
    GLint mvpLoc = glGetUniformLocation(m_ImagePipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glm::vec2 texOffset(0, 0);
    GLint offsetLoc = glGetUniformLocation(m_ImagePipeline, "offset");
    glUniform2fv(offsetLoc, 1, glm::value_ptr(texOffset));

    glm::vec2 texSize(1, 1);
    GLint sizeLoc = glGetUniformLocation(m_ImagePipeline, "size");
    glUniform2fv(sizeLoc, 1, glm::value_ptr(texSize));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_MapTexture);

    glBindVertexArray(m_ImageVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void MapViewer::DrawIcon(const glm::mat4& vpMat, const std::string& name, glm::ivec2 pos) {
    auto iconRect = m_Atlas->GetRect(Icons_::BOSS);
    if (!iconRect)
        return;
    glm::ivec2 size(iconRect->z, iconRect->w);

    glm::mat4 modelMatrix = glm::mat4(1.f);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(size, 1.f));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(pos, 0.f));

    glm::mat4 mvpMatrix = vpMat * modelMatrix;
    GLint mvpLoc = glGetUniformLocation(m_ImagePipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glm::vec2 texOffset(iconRect->x, iconRect->y);
    texOffset /= m_IconsSize;
    glm::vec2 texSize(size);
    texSize /= m_IconsSize;

    texOffset.y = 1 - texOffset.y - texSize.y;
    GLint offsetLoc = glGetUniformLocation(m_ImagePipeline, "offset");
    glUniform2fv(offsetLoc, 1, glm::value_ptr(texOffset));

    GLint sizeLoc = glGetUniformLocation(m_ImagePipeline, "size");
    glUniform2fv(sizeLoc, 1, glm::value_ptr(texSize));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_IconsTexture);

    glBindVertexArray(m_ImageVAO);
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
    glm::vec2 mc2vc(m_Transform.offset.x, -m_Transform.offset.y);
    // view zero(left top) to current point
    glm::vec2 v02c = Normalize(pos);
    // view center to current point
    glm::vec2 vc2c = v02c - vsize / 2.f;
    return vc2c + mc2vc + glm::vec2(m_MapSize) / 2.f;
}

void MapViewer::OnResizeMap() {
    float aspect = 1.f * m_Viewport.z / m_Viewport.w;
    m_OriginViewSize = m_MapSize;
    if (aspect > 1.0f)
        m_OriginViewSize.x *= aspect;
    else
        m_OriginViewSize.y /= aspect;
}

void MapViewer::Initialize() {
    InitImagePipeline();

    m_IconsTexture = LoadTexture(
        GetTexPath("icons").c_str(),
        m_IconsSize.x, m_IconsSize.y, true);
    m_Atlas = CreateAtlasMgr<IconMgr>("icons.json");

    ReloadMap(Maps_::ROTTED_WOODS);

    vReset();
}

void MapViewer::Cleanup() {
    glDeleteVertexArrays(1, &m_ImageVAO);
    glDeleteBuffers(1, &m_ImageVBO);
    glDeleteBuffers(1, &m_ImageEBO);
    glDeleteProgram(m_ImagePipeline);
}

void MapViewer::Render() {
    glm::vec2 viewSize = GetViewSize();
    glm::vec2 offset = m_Transform.offset;
    
    glm::mat4 projMatrix = glm::ortho(
        offset.x - viewSize.x/2.f, offset.x + viewSize.x/2.f,
        offset.y - viewSize.y/2.f, offset.y + viewSize.y/2.f,
        -1.0f, 1.0f
    );

    glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));

    glUseProgram(m_ImagePipeline);
    auto vpMat = projMatrix * viewMatrix;
    DrawMap(vpMat);
    DrawIcon(vpMat, Icons_::ROT_BLESSING, {0, 0});
}

void MapViewer::RenderImGui() {
    ImGui::Separator();
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

void MapViewer::SetViewport(const glm::ivec4& viewport) {
    m_Viewport = viewport;
    vReset();
    OnResizeMap();
}

void MapViewer::ReloadMap(const char* mapName) {
    if (m_MapTexture != 0) {
        glDeleteTextures(1, &m_MapTexture);
        m_MapTexture = 0;
    }
    m_MapTexture = LoadTexture(
        GetTexPath(mapName).c_str(),
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
