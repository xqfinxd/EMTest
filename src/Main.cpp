#include <iostream>
#include <SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include "GameLoop.h"
#include "MapViewer.h"
#include "MapFilter.h"

extern bool g_EnableChinese;
extern std::unique_ptr<Translator> g_Translator;

#ifdef __EMSCRIPTEN__
EM_JS(void, SetupJSResizeListener, (void* game), {
        window.addEventListener('resize', function() {
            Module._HandleWindowResize(game, window.innerWidth, window.innerHeight);
        });
    }
);
int CutWidth(int w) {
    return w - 20;
}
int CutHeight(int h) {
    return h - 80;
}
#endif
class ActionDetector {
public:
    struct TouchPoint {
        glm::vec2 startPos{};
        glm::vec2 curPos{};
        uint32_t startTime = 0;
        bool active = false;
    };
    
    MapAction ProcessEvent(const SDL_Event& event, int w, int h) {
        MapAction result = { MapAction::eNone };
        if (!hasTouchDevice) {
            hasTouchDevice = SDL_GetNumTouchDevices() > 0;
        }

        switch (event.type) {
        case SDL_FINGERDOWN:
            HandleFingerDown(result, event.tfinger, w, h);
            break;

        case SDL_FINGERUP:
            HandleFingerUp(result, event.tfinger, w, h);
            break;

        case SDL_FINGERMOTION:
            HandleFingerMotion(result, event.tfinger, w, h);
            break;

        case SDL_MOUSEBUTTONDOWN:
            HandleMouseDown(result, event.button);
            break;

        case SDL_MOUSEBUTTONUP:
            HandleMouseUp(result, event.button);
            break;

        case SDL_MOUSEMOTION:
            HandleMouseMotion(result, event.motion);
            break;

        case SDL_MOUSEWHEEL:
            HandleMouseWheel(result, event.wheel);
            break;
        }

        return result;
    }

private:
    bool CheckInterval(uint32_t timestamp) const {
        return timestamp - lastTapTime <= DOUBLE_CLICK_INTERVAL;
    }

    bool CheckDistance(glm::vec2 v1, glm::vec2 v2) const {
        float dis = glm::distance(v1, v2);
        return dis <= DOUBLE_CLICK_DISTANCE;
    }

    void HandleFingerDown(MapAction& result, const SDL_TouchFingerEvent& event, int w, int h) {
        if (activeTouches.size() == 2)
            return;
        TouchPoint& point = activeTouches[event.fingerId];
        point.startPos.x = point.curPos.x = event.x;
        point.startPos.y = point.curPos.y = event.y;
        point.startTime = event.timestamp;
        point.active = true;

        uint32_t delta = event.timestamp - lastTapTime;
        if (lastTapTime > 0
            && CheckInterval(event.timestamp)
            && CheckDistance(point.curPos, lastTapPos)) {
            
            result.type = MapAction::eDoubleTap;
            result.pos.x = event.x * w;
            result.pos.y = event.y * h;
            result.scale = 2.f;

            lastTapTime = 0;
        }
        else {
            size_t count = activeTouches.size();
            if (count == 1) {
                lastTapTime = event.timestamp;
                lastTapPos = { event.x,event.y };
            }
            else if (count == 2) {
                // zoom start
                result.type = MapAction::eZoomLocal;
                UpdateZoomData(result, w, h);
            }
        }
    }

    void HandleFingerUp(MapAction& result, const SDL_TouchFingerEvent& event, int w, int h) {
        auto it = activeTouches.find(event.fingerId);
        if (it != activeTouches.end()) {
            TouchPoint& point = it->second;

            const size_t count = activeTouches.size();
            if (count == 1) {
                if (CheckDistance(point.startPos, point.curPos)) {
                    result.type = MapAction::eSingleTap;
                    result.pos.x = event.x * w;
                    result.pos.y = event.y * h;
                }
            }
            else if (count == 2) {
                // zoom end
                ClearZoomData();
            }

            activeTouches.erase(it);
        }
    }

    void HandleFingerMotion(MapAction& result, const SDL_TouchFingerEvent& event, int w, int h) {
        auto it = activeTouches.find(event.fingerId);
        if (it != activeTouches.end()) {
            TouchPoint& point = it->second;
            point.curPos = { event.x,event.y };

            const size_t count = activeTouches.size();
            if (count == 1) {
                result.type = MapAction::eDragMove;
                result.pos.x = event.x * w;
                result.pos.y = event.y * h;
                result.delta.x = event.dx * w;
                result.delta.y = event.dy * h;
            }
            else if (count == 2) {
                result.type = MapAction::eZoomLocal;
                UpdateZoomData(result, w, h);
            }
        }
    }

    void ClearZoomData() {
        initialDis = 0;
        curPos = { 0,0 };
        curZoom = 1;
    }

    void UpdateZoomData(MapAction& result, int w, int h) {
        if (activeTouches.size() < 2) return;
        const auto& point1 = activeTouches[0];
        const auto& point2 = activeTouches[1];

        float dis = glm::distance(point1.curPos, point2.curPos);
        glm::vec2 center = (point1.curPos + point2.curPos) / 2.0f;

        result.pos.x = center.x * w;
        result.pos.y = center.y * h;
        if (initialDis == 0.0f) {
            initialDis = dis;
            curPos = center;
            curZoom = 1;
            result.scale = 1;
            result.delta = { 0,0 };
        }
        else {
            float scale = dis / initialDis;
            result.scale = scale / curZoom;
            glm::vec2 delta = center - curPos;
            result.delta.x = delta.x * w;
            result.delta.y = delta.y * h;
            curZoom = scale;
            curPos = center;
        }
    }

    void HandleMouseDown(MapAction& result, const SDL_MouseButtonEvent& event) {
        if (hasTouchDevice)
            return;

        if (event.button == SDL_BUTTON_LEFT) {
            if (event.clicks == 1) {
                isDragging = true;
            }
            else if (event.clicks == 2) {
                result.type = MapAction::eDoubleTap;
                result.pos = { event.x,event.y };
                result.scale = 2.f;
            }
        }
        else if (event.button == SDL_BUTTON_MIDDLE) {
            result.type = MapAction::eLongTouch;
            result.pos = { event.x,event.y };
        }
    }

    void HandleMouseUp(MapAction& result, const SDL_MouseButtonEvent& event) {
        if (hasTouchDevice)
            return;

        if (event.button == SDL_BUTTON_LEFT) {
            if (isDragging) {
                isDragging = false;
            }

            if(event.clicks == 1) {
                result.type = MapAction::eSingleTap;
                result.pos = { event.x,event.y };
            }
        }
    }

    void HandleMouseMotion(MapAction& result, const SDL_MouseMotionEvent& event) {
        if (hasTouchDevice)
            return;

        if (isDragging) {
            result.type = MapAction::eDragMove;
            result.pos = { event.x,event.y };
            result.delta = { event.xrel,event.yrel };
        }
    }

    void HandleMouseWheel(MapAction& result, const SDL_MouseWheelEvent& event) {
        if (hasTouchDevice)
            return;

        result.type = MapAction::eZoomCenter;
        result.pos = { event.mouseX,event.mouseY };
        if (event.y > 0)
            result.scale = 2.f;
        else if (event.y < 0)
            result.scale = 0.5f;
    }

private:
    using TouchPoints = std::unordered_map<SDL_FingerID, TouchPoint>;
    
    bool hasTouchDevice = false;

    // mouse and finger
    bool isDragging = false;

    // only for finger
    TouchPoints activeTouches;
    // check double tap
    uint32_t lastTapTime = 0;
    glm::vec2 lastTapPos;

    float initialDis = 0;
    float curZoom = 1.0f;
    glm::vec2 curPos{ 0,0 };

    const uint32_t DOUBLE_CLICK_INTERVAL = 200;
    const float DOUBLE_CLICK_DISTANCE = 0.05f;
};

class MyGame : public GameLoop {
protected:
    void Initialize() override {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
            return;
        }

        int windowFlags = SDL_WINDOW_SHOWN;
        m_Size.x = 1200;
        m_Size.y = 900;
#ifndef __EMSCRIPTEN__
        windowFlags |= SDL_WINDOW_OPENGL;
#else
        int w_ = EM_ASM_INT({
            return window.innerWidth;
        });
        int h_ = EM_ASM_INT({
            return window.innerHeight;
        });
        SetupJSResizeListener(this);
        m_Size.x = CutWidth(w_);
        m_Size.y = CutHeight(h_);
#endif
        m_Window = SDL_CreateWindow("Nightreign Map Filter",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            m_Size.x, m_Size.y, windowFlags);
        if (!m_Window) {
            SDL_Log("Failed to Create Window: %s", SDL_GetError());
            return;
        }

#ifdef __EMSCRIPTEN__
        m_LocalRenderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED);
        m_Context = SDL_GL_GetCurrentContext();
#else
        m_Context = SDL_GL_CreateContext(m_Window);
#endif
        if (!m_Context) {
            SDL_Log("Failed to Get Context: %s", SDL_GetError());
            return;
        }

#ifdef WIN32
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return;
        }
#endif
        glEnable(GL_BLEND);
        
        m_MapViewer.Initialize();

        m_MapFilter.Initialize(&m_MapViewer);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(m_Window, m_Context);
        ImGui_ImplOpenGL3_Init();

        float devicePixelRatio = 1.f;
#ifdef __EMSCRIPTEN__
        devicePixelRatio = (float)EM_ASM_DOUBLE({
            return window.devicePixelRatio || 1.0;
        });
#else
        auto displayIdx = SDL_GetWindowDisplayIndex(m_Window);
        float vdpi = 96.f;
        if (SDL_GetDisplayDPI(displayIdx, nullptr, nullptr, &vdpi) == 0)
            devicePixelRatio = vdpi / 96.f;
#endif
        SDL_Log("Device Pixel Ratio: %f", devicePixelRatio);

        float uiScale = 1.f;
        if (devicePixelRatio >= 3.0) {
            uiScale = 1.5f;
        }
        else if (devicePixelRatio >= 2.0) {
            uiScale = 1.2f;
        }
        else {
            uiScale = 1.0f;
        }
        SDL_Log("UI Scale: %f", uiScale);

        auto& io = ImGui::GetIO();
        
        ImVector<ImWchar> myRange;
        ImFontGlyphRangesBuilder myGlyph;
        auto&& used = g_Translator->Collect();
        m_MapViewer.BuildFont(used);
        myGlyph.AddText(used.c_str());
        myGlyph.BuildRanges(&myRange);

        io.Fonts->AddFontFromFileTTF(DATA_DIR("simhei.ttf").c_str(),
            16, nullptr, myRange.Data);
        io.Fonts->Build();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = nullptr;

        io.FontGlobalScale = uiScale;

        // 设置样式缩放
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(uiScale);
        style.TouchExtraPadding = ImVec2{ 10,8 };
    }

    void ProcessInput() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                Stop();
                break;
            }
            ImGui_ImplSDL2_ProcessEvent(&event);
            auto result = m_Detector.ProcessEvent(event, m_Size.x, m_Size.y);
            m_MapViewer.HandleAction(&m_MapFilter, result);
        }
    }

    void Update(float deltaTime) override {
        m_MapViewer.Constrain();
    }


    void RenderImGui() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // 窗口的 ID 和 标题
        ImGuiID dockID = ImGui::GetID("##ui.dockspace");
        std::string rootWin = "##ui.root";
        std::string settingWin = std::string(TR("Settings")) + "##ui.settings";
        std::string viewWin = "##ui.view";
        std::string filterWin = std::string(TR("Filter")) + "##ui.filter";

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        int windowFlags = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            ;

        // root begin
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin(rootWin.c_str(), 0, windowFlags);
        ImGui::PopStyleVar(3);
        do {
            auto configs = ImGui::GetIO().ConfigFlags;
            if (0 == (configs & ImGuiConfigFlags_DockingEnable))
                break;
            if (ImGui::DockBuilderGetNode(dockID) && !m_Resized)
                break;
            m_Resized = false;
            ImGui::DockBuilderRemoveNode(dockID);

            ImGuiID root = ImGui::DockBuilderAddNode(dockID, ImGuiDockNodeFlags_DockSpace);

            ImGui::DockBuilderSetNodePos(root, { 0.0f, 0.0f });
            ImGui::DockBuilderSetNodeSize(root, viewport->WorkSize);

            ImGuiID viewNode, settingNode, filterNode;
            int w = viewport->WorkSize.x;
            int h = viewport->WorkSize.y;
            if (w > h) {
                float spaceRate = glm::max(0.25f, 1.f * (w - h) / w);
                viewNode = ImGui::DockBuilderSplitNode(root,
                    ImGuiDir_Left, 1 - spaceRate, nullptr, &settingNode);
                settingNode = ImGui::DockBuilderSplitNode(settingNode,
                    ImGuiDir_Up, 0.4f, nullptr, &filterNode);
            }
            else {
                float spaceRate = glm::max(0.4f, 1.f * (h - w) / h);
                viewNode = ImGui::DockBuilderSplitNode(root,
                    ImGuiDir_Up, 1 - spaceRate, nullptr, &settingNode);
                settingNode = ImGui::DockBuilderSplitNode(settingNode,
                    ImGuiDir_Left, 0.4f, nullptr, &filterNode);
            }
            
            ImGui::DockBuilderDockWindow(viewWin.c_str(), viewNode);
            if (auto MainNode = ImGui::DockBuilderGetNode(viewNode)) {
                MainNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            }
            ImGui::DockBuilderDockWindow(settingWin.c_str(), settingNode);
            ImGui::DockBuilderDockWindow(filterWin.c_str(), filterNode);
            ImGui::DockBuilderFinish(dockID);
        } while (false);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::DockSpace(dockID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::End();
        // root end

        // view begin
        ImGui::Begin(viewWin.c_str(), nullptr, ImGuiWindowFlags_NoBackground);
        {
            auto framerate = ImGui::GetIO().Framerate;
            std::string fmtFps = std::string(TR("Frame Rate")) + ": %.1f";
            ImGui::TextColored(ImVec4(0,1,0,1), fmtFps.c_str(), framerate);
            auto curWin = ImGui::GetCurrentWindow();
            m_MapViewer.SetViewport(m_Size.y, glm::ivec4(
                curWin->Pos.x, curWin->Pos.y,
                curWin->Size.x, curWin->Size.y));
        }
        ImGui::End();
        // view end

        // setting begin
        ImGui::Begin(settingWin.c_str(), nullptr, ImGuiWindowFlags_NoCollapse| ImGuiWindowFlags_NoMove);
        {
            ImGui::Text("Ver 1.0.251003");
            std::string fmtBgColor(TR("Bg Color"));
            ImGui::ColorEdit4(fmtBgColor.c_str(), glm::value_ptr(m_BgColor));
            if (ImGui::Checkbox("Chinese", &g_EnableChinese)) {
                m_Resized = true;
            }
            ImGui::Separator();
            m_MapViewer.RenderImGui();
        }
        ImGui::End();
        // setting end

        // filter begin
        ImGui::Begin(filterWin.c_str(), nullptr, ImGuiWindowFlags_NoCollapse| ImGuiWindowFlags_NoMove);
        {
            m_MapFilter.RenderImGui();
        }
        ImGui::End();
        // filter end
        
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void RenderGL() {
        m_MapViewer.Render();
    }

    void Render() override {
        glClearColor(m_BgColor.r, m_BgColor.g,
            m_BgColor.b, m_BgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderGL();

        RenderImGui();

        SDL_GL_SwapWindow(m_Window);
    }

    void Cleanup() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        m_MapViewer.Cleanup();

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

public:
    void OnResize(int w, int h) {
        SDL_Log("Resize : %d, %d", w, h);
        SDL_SetWindowSize(m_Window, w, h);
        m_Resized = true;
    }

private:
    SDL_Window* m_Window = nullptr;
    SDL_Renderer* m_LocalRenderer = nullptr;
    SDL_GLContext m_Context = nullptr;

    glm::ivec2 m_Size{};
    glm::vec2 m_ZoomRange = { 1.f, 5.f };
    glm::vec4 m_BgColor = { 0.8f,0.8f,0.8f,1 };
    bool m_Resized = false;
    
    MapViewer m_MapViewer;
    MapFilter m_MapFilter;
    ActionDetector m_Detector;
};

#ifdef __EMSCRIPTEN__
extern "C" void EMSCRIPTEN_KEEPALIVE HandleWindowResize(void* ud, int w, int h) {
    if (auto game = static_cast<MyGame*>(ud)) {
        game->OnResize(w, h);
    }
}
#endif

int main(int argc, char* argv[]) {
    MyGame game;
    game.SetTargetFPS(60);
    game.Run();

    return 0;
}