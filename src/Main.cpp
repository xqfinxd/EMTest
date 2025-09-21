#include <iostream>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif

#include "GameLoop.h"

// 立方体的8个顶点坐标 (x, y, z)
static GLfloat vertices[] = {
    // 前面
    -0.5f, -0.5f,  0.5f,  // 0: 左下前
     0.5f, -0.5f,  0.5f,  // 1: 右下前
     0.5f,  0.5f,  0.5f,  // 2: 右上前
    -0.5f,  0.5f,  0.5f,  // 3: 左上前

    // 后面
    -0.5f, -0.5f, -0.5f,  // 4: 左下后
     0.5f, -0.5f, -0.5f,  // 5: 右下后
     0.5f,  0.5f, -0.5f,  // 6: 右上后
    -0.5f,  0.5f, -0.5f   // 7: 左上后
};

// 每个顶点的颜色 (R, G, B)
static GLfloat colors[] = {
    1.0f, 0.0f, 0.0f,  // 0: 红
    0.0f, 1.0f, 0.0f,  // 1: 绿
    0.0f, 0.0f, 1.0f,  // 2: 蓝
    1.0f, 1.0f, 0.0f,  // 3: 黄

    1.0f, 0.0f, 1.0f,  // 4: 紫
    0.0f, 1.0f, 1.0f,  // 5: 青
    0.5f, 0.5f, 0.5f,  // 6: 灰
    1.0f, 0.5f, 0.0f   // 7: 橙
};

// 索引缓冲：定义组成立方体12个三角形的36个顶点
static GLubyte indices[] = {
    // 前面
    0, 1, 2,    // 第一个三角形
    2, 3, 0,    // 第二个三角形

    // 右面
    1, 5, 6,
    6, 2, 1,

    // 后面
    5, 4, 7,
    7, 6, 5,

    // 左面
    4, 0, 3,
    3, 7, 4,

    // 上面
    3, 2, 6,
    6, 7, 3,

    // 下面
    4, 5, 1,
    1, 0, 4
};

// 顶点着色器源码
const char* vertexShaderSource = R"(
attribute vec3 aPosition;
attribute vec3 aColor;
uniform mat4 uMVPMatrix;
varying vec3 vColor;
void main() {
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    vColor = aColor;
}
)";

// 片段着色器源码  
const char* fragmentShaderSource = R"(
precision mediump float;
varying vec3 vColor;
void main() {
    gl_FragColor = vec4(vColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // 检查编译错误
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation error: %s\n", infoLog);
    }
    return shader;
}

GLuint initShaders() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 检查链接错误
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        printf("Program linking error: %s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

class MyGame : public GameLoop {
protected:
    void Initialize() override {
        // 初始化 SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
            return;
        }

        // 创建窗口
        int windowFlags = SDL_WINDOW_SHOWN;
#ifndef __EMSCRIPTEN__
        windowFlags |= SDL_WINDOW_OPENGL;
#endif
        m_Window = SDL_CreateWindow("EMTest",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800, 600, windowFlags);
        if (!m_Window) {
            SDL_Log("Failed to Create Window: %s\n", SDL_GetError());
            SDL_Quit();
            return;
        }

        // 创建渲染器
        m_Renderer = SDL_CreateRenderer(m_Window, -1,
            SDL_RENDERER_ACCELERATED |
            SDL_RENDERER_PRESENTVSYNC);
        if (!m_Renderer) {
            SDL_Log("渲染器创建失败: %s\n", SDL_GetError());
            SDL_DestroyWindow(m_Window);
            SDL_Quit();
            return;
        }

#ifdef __EMSCRIPTEN__
        m_Context = SDL_GL_GetCurrentContext();
#else
        m_Context = SDL_GL_CreateContext(m_Window);
#endif
        if (!m_Context) {
            SDL_Log("上下文创建失败: %s\n", SDL_GetError());
            SDL_DestroyRenderer(m_Renderer);
            SDL_DestroyWindow(m_Window);
            SDL_Quit();
            return;
        }

#ifdef WIN32
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return;
        }
#endif

        glEnable(GL_DEPTH_TEST);

        shaderProgram = initShaders();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        // 顶点缓冲
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), nullptr, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);

        // 索引缓冲
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // 设置顶点属性
        GLuint positionLoc = glGetAttribLocation(shaderProgram, "aPosition");
        glEnableVertexAttribArray(positionLoc);
        glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        GLuint colorLoc = glGetAttribLocation(shaderProgram, "aColor");
        glEnableVertexAttribArray(colorLoc);
        glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)sizeof(vertices));

        glBindVertexArray(0);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(m_Window, m_Context);
        ImGui_ImplOpenGL3_Init();

        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.Fonts->AddFontFromFileTTF("assets/DroidSans.ttf", 16);
        io.Fonts->AddFontDefault();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void ProcessInput() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type) {
            case SDL_QUIT:
                Stop();
                break;
            }
        }
    }
    void Update(float deltaTime) override {
        // 更新游戏逻辑
        m_RotationAngle += 0.1f;
    }
    void Render() override {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        {
            static float f = 0.f;
            ImGui::Text("Hello World");
            ImGui::SliderFloat("float", &f, 0.f, 1.f);
        }

        ImGui::Render();

        // 清除颜色和深度缓冲
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // 计算MVP矩阵

        // 1. 计算模型矩阵 (物体变换)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(m_RotationAngle),
            glm::vec3(0.5f, 1.0f, 0.0f));

        // 计算 MVP
        glm::mat4 view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f
        );

        glm::mat4 mvp = projection * view * model;

        GLuint mvpLoc = glGetUniformLocation(shaderProgram, "uMVPMatrix");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);
        glBindVertexArray(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_Window);
    }
    void Cleanup() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        if (m_Context) {
            SDL_GL_DeleteContext(m_Context);
            m_Context = nullptr;
        }
        if (m_Renderer) {
            SDL_DestroyRenderer(m_Renderer);
            m_Renderer = nullptr;
        }
        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
        SDL_Quit();
    }

private:
    // SDL 窗口和渲染器
    SDL_Window* m_Window = nullptr;
    SDL_Renderer* m_Renderer = nullptr;
    SDL_GLContext m_Context = nullptr;
    // 游戏状态变量
    float m_RotationAngle = 0.f;
    GLuint shaderProgram;
    GLuint vao, vbo, ebo;
};

int main(int argc, char* argv[]) {
    MyGame game;
    game.SetTargetFPS(60);
    game.Run();

    return 0;
}