#include "MapRenderer.h"

// ��ɫ��Դ����
const char* mapVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* mapFragmentShader = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D mapTexture;
uniform bool showGrid;
uniform vec4 gridColor;

void main() {
    FragColor = texture(mapTexture, TexCoord);
    
    // ��������
    if (showGrid) {
        vec2 gridPos = TexCoord * 20.0;
        vec2 grid = abs(fract(gridPos - 0.5) - 0.5) / fwidth(gridPos);
        float line = min(grid.x, grid.y);
        float gridIntensity = 1.0 - min(line, 1.0);
        FragColor = mix(FragColor, gridColor, gridIntensity * 0.3);
    }
}
)";

const char* landmarkVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform vec2 position;
uniform float scale;

void main() {
    vec2 worldPos = position + aPos * scale;
    gl_Position = projection * view * vec4(worldPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* landmarkFragmentShader = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D landmarkTexture;
uniform vec4 tintColor;

void main() {
    vec4 texColor = texture(landmarkTexture, TexCoord);
    if (texColor.a < 0.1) discard; // ����͸������
    FragColor = texColor * tintColor;
}
)";

GLuint CompileShader(GLenum type, const char* source);
GLuint LoadTexture(const std::string& path, int& width, int& height, bool flip);

void LandmarkMapRenderer::setupMapBuffers() {
    float vertices[] = {
        // λ��          // ��������
        -0.5f, -0.5f,   0.0f, 0.0f,
        0.5f, -0.5f,   1.f, 0.0f,
        0.5f,  0.5f,   1.f, 1.0f,
        -0.5f,  0.5f,   0.0f, 1.0f
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint EBO;
    glGenVertexArrays(1, &mapVAO);
    glGenBuffers(1, &mapVBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(mapVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glDeleteBuffers(1, &EBO);
}

void LandmarkMapRenderer::setupLandmarkBuffers() {
    // �ر�ʹ���ı��Σ�������ԭ��
    float vertices[] = {
        // λ��          // ��������
        -0.5f, -0.5f,   0.0f, 0.0f,
        0.5f, -0.5f,   1.0f, 0.0f,
        0.5f,  0.5f,   1.0f, 1.0f,
        -0.5f,  0.5f,   0.0f, 1.0f
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint EBO;
    glGenVertexArrays(1, &landmarkVAO);
    glGenBuffers(1, &landmarkVBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(landmarkVAO);

    glBindBuffer(GL_ARRAY_BUFFER, landmarkVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glDeleteBuffers(1, &EBO);
}

void LandmarkMapRenderer::generateRandomLandmarks(int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(0.1f, 0.9f);
    std::uniform_real_distribution<float> scaleDist(0.02f, 0.05f);
    std::uniform_real_distribution<float> colorDist(0.7f, 1.0f);
    std::uniform_int_distribution<int> texDist(0, 15); // ������16�ֵر�

    glm::vec4 colors[] = {
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // ��
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // ��
        glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), // ��
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // ��
        glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), // ��
        glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)  // ��
    };

    for (int i = 0; i < count; ++i) {
        Landmark landmark;
        landmark.position = glm::vec2(posDist(gen), posDist(gen));
        landmark.scale = scaleDist(gen);
        landmark.tintColor = colors[i % 6];
        landmark.textureIndex = texDist(gen);
        landmarks.push_back(landmark);
    }

    std::cout << "Generated " << landmarks.size() << " random landmarks" << std::endl;
}

void LandmarkMapRenderer::Initialize(const std::string& mapPath, const std::string& atlasPath) {
    // �����ͼ��ɫ��
    GLuint mapVS = CompileShader(GL_VERTEX_SHADER, mapVertexShader);
    GLuint mapFS = CompileShader(GL_FRAGMENT_SHADER, mapFragmentShader);

    mapShaderProgram = glCreateProgram();
    glAttachShader(mapShaderProgram, mapVS);
    glAttachShader(mapShaderProgram, mapFS);
    glLinkProgram(mapShaderProgram);

    // ����ر���ɫ��
    GLuint landmarkVS = CompileShader(GL_VERTEX_SHADER, landmarkVertexShader);
    GLuint landmarkFS = CompileShader(GL_FRAGMENT_SHADER, landmarkFragmentShader);

    landmarkShaderProgram = glCreateProgram();
    glAttachShader(landmarkShaderProgram, landmarkVS);
    glAttachShader(landmarkShaderProgram, landmarkFS);
    glLinkProgram(landmarkShaderProgram);

    glDeleteShader(mapVS);
    glDeleteShader(mapFS);
    glDeleteShader(landmarkVS);
    glDeleteShader(landmarkFS);

    // ��������
    mapTexture = LoadTexture(mapPath, mapWidth, mapHeight, true);
    landmarkAtlasTexture = LoadTexture(atlasPath, atlasWidth, atlasHeight, false);

    // ����ر�ϼ���8x8������
    landmarksPerRow = 8;

    // ���û�����
    setupMapBuffers();
    setupLandmarkBuffers();

    // ��������ر�
    generateRandomLandmarks(50);
}

void LandmarkMapRenderer::Cleanup() {
    glDeleteVertexArrays(1, &mapVAO);
    glDeleteBuffers(1, &mapVBO);
    glDeleteVertexArrays(1, &landmarkVAO);
    glDeleteBuffers(1, &landmarkVBO);
    glDeleteTextures(1, &mapTexture);
    glDeleteTextures(1, &landmarkAtlasTexture);
    glDeleteProgram(mapShaderProgram);
    glDeleteProgram(landmarkShaderProgram);
}

extern glm::vec2 FixViewSize(const glm::vec2& viewPort);
void LandmarkMapRenderer::Render(const MapView& view, const glm::vec2& viewPort) {
    // ����ͶӰ����
    auto viewSize = FixViewSize(viewPort);
    glm::mat4 projMatrix = glm::ortho(
        -viewSize.x/2, viewSize.x/2,
        -viewSize.y/2, viewSize.y/2,
        -1.0f, 1.0f
    );

    glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0, 0, 1.f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 modelMatrix = glm::mat4(1.f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(view.offset, 0.f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(view.zoom, view.zoom, 1.f));

    glm::mat4 mvpMatrix = projMatrix * viewMatrix * modelMatrix;

    // ��Ⱦ��ͼ
    glUseProgram(mapShaderProgram);

    GLint mvpLoc = glGetUniformLocation(mapShaderProgram, "mvp");
    GLint showGridLoc = glGetUniformLocation(mapShaderProgram, "showGrid");
    GLint gridColorLoc = glGetUniformLocation(mapShaderProgram, "gridColor");

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
    glUniform1i(showGridLoc, view.showGrid);
    glUniform4fv(gridColorLoc, 1, glm::value_ptr(view.gridColor));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture);

    glBindVertexArray(mapVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // ��Ⱦ�ر�
    glUseProgram(landmarkShaderProgram);

    auto projLoc = glGetUniformLocation(landmarkShaderProgram, "projection");
    auto viewLoc = glGetUniformLocation(landmarkShaderProgram, "view");
    GLint positionLoc = glGetUniformLocation(landmarkShaderProgram, "position");
    GLint scaleLoc = glGetUniformLocation(landmarkShaderProgram, "scale");
    GLint tintColorLoc = glGetUniformLocation(landmarkShaderProgram, "tintColor");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, landmarkAtlasTexture);

    glBindVertexArray(landmarkVAO);

    for (const auto& landmark : landmarks) {
        glUniform2fv(positionLoc, 1, glm::value_ptr(landmark.position));
        glUniform1f(scaleLoc, landmark.scale);
        glUniform4fv(tintColorLoc, 1, glm::value_ptr(landmark.tintColor));

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}
