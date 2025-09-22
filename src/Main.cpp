#include <iostream>
#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "GameLoop.h"
#include "MapViewer.h"

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
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
            return;
        }

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
            return;
        }

#ifdef __EMSCRIPTEN__
        m_LocalRenderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED);
        m_Context = SDL_GL_GetCurrentContext();
#else
        m_Context = SDL_GL_CreateContext(m_Window);
#endif
        if (!m_Context) {
            SDL_Log("Failed to Get Context: %s\n", SDL_GetError());
            return;
        }

#ifdef WIN32
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return;
        }
#endif
        GLint majorVersion, minorVersion;
        glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
        SDL_Log("OpenGL Version: %d.%d", majorVersion, minorVersion);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_Renderer.Initialize("assets/normal.png", "assets/icon.png");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(m_Window, m_Context);
        ImGui_ImplOpenGL3_Init();

        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        //io.Fonts->AddFontFromFileTTF("assets/DroidSans.ttf", 16);
        //io.Fonts->AddFontDefault();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    bool MouseInMap(int x, int y) const {
        return x > 0 && x < m_ViewPort.x
            && y > 0 && y < m_ViewPort.y;
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
                if (MouseInMap(event.wheel.mouseX, event.wheel.mouseY)) {
                    if (event.wheel.y > 0)
                        m_MapView.zoom += 1.f;
                    else if (event.wheel.y < 0)
                        m_MapView.zoom -= 1.f;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT
                    && event.button.clicks == 1
                    && MouseInMap(event.button.x, event.button.y))
                    m_IsDrag = true;
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_MIDDLE)
                    ResetView();
                if (event.button.button == SDL_BUTTON_LEFT)
                    m_IsDrag = false;
                break;
            case SDL_MOUSEMOTION:
                if (m_IsDrag) {
                    glm::vec2 move(event.motion.xrel, -event.motion.yrel);
                    glm::vec2 viewport = m_ViewPort;
                    m_MapView.offset += move / viewport;
                }
                break;
            }
        }
    }
    void Update(float deltaTime) override {
        ConstrainView();
    }

    void RenderImGui() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // 窗口的 ID 和 标题
        ImGuiID dockspaceID = ImGui::GetID("##ui.dock_space");
        const char* UI_DOCK_WINDOW = "##ui.dock_window";
        const char* UI_PROPERTY_BOX = "Property##ui.property";
        const char* UI_VIEW_BOX = "##ui.view";


        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        int windowFlags = ImGuiWindowFlags_NoDecoration // 无装饰
            | ImGuiWindowFlags_NoMove                   // 不可移动
            | ImGuiWindowFlags_NoBackground             // 无背景（背景透明）
            | ImGuiWindowFlags_NoDocking                // 不可停靠
            | ImGuiWindowFlags_NoBringToFrontOnFocus    // 无法设置前台和聚焦
            | ImGuiWindowFlags_NoNavFocus               // 无法通过键盘和手柄聚焦
            ;

        // 压入样式设置
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);            // 无边框
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // 无边界
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);              // 无圆角
        ImGui::Begin(UI_DOCK_WINDOW, 0, windowFlags); // 开始停靠窗口
        ImGui::PopStyleVar(3);                        // 弹出样式设置
        {

            // 判断是否开启停靠
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
                // 判断是否有根节点，防止一直重建
                if (!ImGui::DockBuilderGetNode(dockspaceID)) {
                    // 移除根节点
                    ImGui::DockBuilderRemoveNode(dockspaceID);

                    // 创建根节点
                    ImGuiID root = ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);

                    // 设置根节点位置大小
                    ImGui::DockBuilderSetNodePos(root, { 0.0f, 0.0f });
                    ImGui::DockBuilderSetNodeSize(root, ImGui::GetWindowSize());

                    // 分割停靠空间
                    ImGuiID leftTopNode, leftBottomNode;
                    // 根节点分割左节点
                    ImGuiID leftNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Left, 0.75f, &leftTopNode, &leftBottomNode);

                    // 设置节点停靠窗口
                    ImGui::DockBuilderDockWindow(UI_VIEW_BOX, leftTopNode);     // 左上节点
                    if (ImGuiDockNode* MainNode = ImGui::DockBuilderGetNode(leftTopNode)) {
                        MainNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
                    }
                    
                    ImGui::DockBuilderDockWindow(UI_PROPERTY_BOX, leftBottomNode); // 左下节点

                    // 结束停靠设置
                    ImGui::DockBuilderFinish(dockspaceID);

                    // 设置焦点窗口
                    ImGui::SetWindowFocus(UI_VIEW_BOX);
                }
            }

            // 创建停靠空间
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
        ImGui::End(); // 结束停靠窗口

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);            // 无边框
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // 无边界
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);              // 无圆角
        ImGui::Begin(UI_VIEW_BOX, nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_DockNodeHost);
        ImGui::PopStyleVar(3); // 弹出样式设置
        {
            auto framerate = ImGui::GetIO().Framerate;
            ImGui::Text("FPS: %.1f", framerate);
        }
        ImGui::End();
        
        // 右侧边栏 - 控制面板
        ImGui::Begin(UI_PROPERTY_BOX, nullptr, ImGuiWindowFlags_NoCollapse);
        {
            auto framerate = ImGui::GetIO().Framerate;
            ImGui::Text("FPS: %.1f", framerate);

            ImGui::Checkbox("showgrid", &m_MapView.showGrid);
            if (m_MapView.showGrid)
                ImGui::ColorEdit4("gridcolor", glm::value_ptr(m_MapView.gridColor));
            ImGui::SliderFloat("zoom", &m_MapView.zoom, m_ZoomRange.x, m_ZoomRange.y);
            ImGui::DragFloat2("offset", glm::value_ptr(m_MapView.offset), 0.05f);
        }

        ImGui::End();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void RenderGL() {
        // 清除颜色和深度缓冲
        glViewport(0, 0, m_ViewPort.x, m_ViewPort.y);
        
        m_Renderer.Render(m_MapView, m_ViewPort);
    }

    void Render() override {
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderGL();

        RenderImGui();

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
        if (m_LocalRenderer) {
            SDL_DestroyRenderer(m_LocalRenderer);
            m_LocalRenderer = nullptr;
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
    SDL_Renderer* m_LocalRenderer = nullptr;
    SDL_GLContext m_Context = nullptr;

    glm::ivec2 m_Size = { 800, 600 };
    glm::ivec2 m_ViewPort = { 600, 600 };
    glm::vec2 m_ZoomRange = { 1.f, 5.f };
    
    MapViewer m_Renderer;
    MapView m_MapView;

    bool m_IsDrag = false;
};

int main(int argc, char* argv[]) {
    MyGame game;
    game.SetTargetFPS(60);
    game.Run();

    return 0;
}