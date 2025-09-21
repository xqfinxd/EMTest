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

#include "stb_image.h"
#include "GameLoop.h"
#include "MapRenderer.h"

GLuint CompileShader(GLenum type, const char* source) {
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

GLuint LoadTexture(const std::string& path, int& width, int& height, bool flip) {
    stbi_set_flip_vertically_on_load(flip);

    int nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;

    return texture;
}

glm::vec2 FixViewSize(const glm::vec2& viewPort) {
    glm::vec2 viewSize(1.f, 1.f);
    float aspect = (float)viewPort.x / viewPort.y;
    if (aspect > 1.0f)
        viewSize.x *= aspect;
    else
        viewSize.y /= aspect;
    return viewSize;
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
            m_Size.x, m_Size.y, windowFlags);
        if (!m_Window) {
            SDL_Log("Failed to Create Window: %s\n", SDL_GetError());
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
        glViewport(0, 0, m_ViewPort.x, m_ViewPort.y);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_Renderer.Initialize("assets/normal.png", "assets/icon.png");

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
            case SDL_MOUSEWHEEL:
                if (event.wheel.y > 0)
                    m_MapView.zoom += 1.f;
                else if (event.wheel.y < 0)
                    m_MapView.zoom -= 1.f;
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_MIDDLE)
                    ResetView();
                break;
            }
        }
    }
    void Update(float deltaTime) override {
        
    }

    void Render() override {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Text("Hello World");
            ImGui::Checkbox("showgrid", &m_MapView.showGrid);
            if (m_MapView.showGrid)
                ImGui::ColorEdit4("gridcolor", glm::value_ptr(m_MapView.gridColor));
            ImGui::SliderFloat("zoom", &m_MapView.zoom, m_ZoomRange.x, m_ZoomRange.y);
            ImGui::DragFloat2("offset", glm::value_ptr(m_MapView.offset), 0.05f);
        }
        ConstrainView();

        ImGui::Render();

        // 清除颜色和深度缓冲
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_Renderer.Render(m_MapView, m_ViewPort);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_Window);
    }

    void Cleanup() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        m_Renderer.Cleanup();

        if (m_Context) {
            SDL_GL_DeleteContext(m_Context);
            m_Context = nullptr;
        }
        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
        SDL_Quit();
    }

    void ResetView() {
        m_MapView.zoom = 1.f;
        m_MapView.offset = glm::vec2(0, 0);
    }

    void ConstrainView() {
        m_MapView.zoom = glm::clamp(m_MapView.zoom, m_ZoomRange.x, m_ZoomRange.y);
        auto viewSize = FixViewSize(m_ViewPort);
        auto offsetRange = glm::vec2(m_MapView.zoom, m_MapView.zoom) - viewSize;
        m_MapView.offset = glm::clamp(m_MapView.offset, offsetRange / -2.f, offsetRange / 2.f);
    }

private:
    // SDL 窗口和渲染器
    SDL_Window* m_Window = nullptr;
    SDL_GLContext m_Context = nullptr;

    glm::ivec2 m_Size = { 800, 600 };
    glm::ivec2 m_ViewPort = { 600, 600 };
    glm::vec2 m_ZoomRange = { 1.f, 5.f };
    
    LandmarkMapRenderer m_Renderer;
    MapView m_MapView;
};

int main(int argc, char* argv[]) {
    MyGame game;
    game.SetTargetFPS(60);
    game.Run();

    return 0;
}