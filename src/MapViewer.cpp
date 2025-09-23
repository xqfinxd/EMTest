#include "MapViewer.h"
#include <SDL_log.h>
#include "GLUtils.h"
#include "nlohmann/json.hpp"
#include <fstream>

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

    GLuint vs = CompileShaderFile(GL_VERTEX_SHADER, "assets/image.vert");
    GLuint fs = CompileShaderFile(GL_FRAGMENT_SHADER, "assets/image.frag");

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
    auto iter = m_IconMap.find(name);
    if (iter == m_IconMap.end())
        return;
    glm::ivec2 size(iter->second.z, iter->second.w);

    glm::mat4 modelMatrix = glm::mat4(1.f);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(size, 1.f));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(pos, 0.f));

    glm::mat4 mvpMatrix = vpMat * modelMatrix;
    GLint mvpLoc = glGetUniformLocation(m_ImagePipeline, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glm::vec2 texOffset(iter->second.x, iter->second.y);
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

void MapViewer::Initialize() {
    InitImagePipeline();

    // 加载纹理
    m_MapTexture = LoadTexture("assets/normal.png",
        m_MapSize.x, m_MapSize.y, true);
    m_IconsTexture = LoadTexture("assets/icons.png",
        m_IconsSize.x, m_IconsSize.y, true);

    do {
        using json = nlohmann::json;
        const char* path = "assets/icons.json";

        json iconConfig;
        std::ifstream jsonFile(path);
        if (!jsonFile.is_open()) {
            SDL_Log("Could not open the file %s\n", path);
            break;
        }
        jsonFile >> iconConfig;

        if (!iconConfig.is_array())
            break;

        for (size_t i = 0; i < iconConfig.size(); i++) {
            auto name = iconConfig[i].at("type").get<std::string>();
            glm::ivec4 rect;
            auto offsetField = iconConfig[i].at("offset").get<std::string>();
            auto sizeField = iconConfig[i].at("size").get<std::string>();
            sscanf(offsetField.c_str(), "%d,%d", &rect.x, &rect.y);
            sscanf(sizeField.c_str(), "%d,%d", &rect.z, &rect.w);
            m_IconMap[name] = rect;
        }
    } while (false);
}

void MapViewer::Cleanup() {
    glDeleteVertexArrays(1, &m_ImageVAO);
    glDeleteBuffers(1, &m_ImageVBO);
    glDeleteBuffers(1, &m_ImageEBO);
    glDeleteProgram(m_ImagePipeline);

    glDeleteTextures(1, &m_MapTexture);
}

extern void SetViewSize(glm::vec2& viewSize, const glm::ivec2& viewPort);
void MapViewer::Render(const MapView& view, const glm::vec2& viewPort) {
    glm::vec2 viewSize(m_MapSize);
    SetViewSize(viewSize, viewPort);
    viewSize /= view.zoom;

    glm::vec2 pos(0, 0);
    glm::vec2 offset(0, 0);
    offset.x = viewSize.x * view.offset.x / viewPort.x;
    offset.y = viewSize.y * view.offset.y / viewPort.y;
    
    glm::mat4 projMatrix = glm::ortho(
        offset.x - viewSize.x/2.f, offset.x + viewSize.x/2.f,
        offset.y - viewSize.y/2.f, offset.y + viewSize.y/2.f,
        -1.0f, 1.0f
    );

    glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));
    
    // 渲染地图
    glUseProgram(m_ImagePipeline);
    auto vpMat = projMatrix * viewMatrix;
    DrawMap(vpMat);
    DrawIcon(vpMat, "Rot Blessing", {0, 0});
}

void MapViewer::Constrain(MapView& view, const glm::vec2& viewPort) {
    view.zoom = glm::clamp(view.zoom, 1.f, 5.f);
    glm::vec2 viewSize = m_MapSize;
    SetViewSize(viewSize, viewPort);
    viewSize /= view.zoom;
    glm::ivec2 range = m_MapSize - glm::ivec2(viewSize);
    view.offset = glm::clamp(view.offset, -range / 2, range / 2);
}
