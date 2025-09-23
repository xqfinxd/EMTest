#include "MapViewer.h"
#include "GLUtils.h"

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

void MapViewer::Initialize() {
    InitImagePipeline();

    // º”‘ÿŒ∆¿Ì
    m_MapTexture = LoadTexture("assets/normal.png",
        m_MapSize.x, m_MapSize.y, true);
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

    glm::vec2 pos(0, 0);
    glm::vec2 offset(0, 0);
    offset.x = m_MapSize.x * view.offset.x / viewPort.x;
    offset.y = m_MapSize.y * view.offset.y / viewPort.y;
    viewSize /= view.zoom;

    glm::mat4 projMatrix = glm::ortho(
        (offset.x - viewSize.x)/2.f, (offset.x + viewSize.x)/2.f,
        (offset.y - viewSize.y)/2.f, (offset.y + viewSize.y)/2.f,
        -1.0f, 1.0f
    );

    glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));
    
    glm::mat4 modelMatrix = glm::mat4(1.f);
    glm::vec2 scale(m_MapSize);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, 1.f));

    // ‰÷»æµÿÕº
    glUseProgram(m_ImagePipeline);
    {
        glm::mat4 mvpMatrix = projMatrix * viewMatrix * modelMatrix;
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
}

void MapViewer::Constrain(MapView& view, const glm::vec2& viewPort) {
    view.zoom = glm::clamp(view.zoom, 1.f, 5.f);
    glm::vec2 viewSize = m_MapSize;
    SetViewSize(viewSize, viewPort);
    viewSize /= view.zoom;
    glm::ivec2 range = m_MapSize - glm::ivec2(viewSize);
    view.offset = glm::clamp(view.offset, -range / 2, range / 2);
}
