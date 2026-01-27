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
        windowFlags |= SDL_WINDOW_RESIZABLE;

        double width, height;
        emscripten_get_element_css_size("canvas", &width, &height);
        emscripten_set_canvas_element_size("canvas", (int)width, (int)height);
        extern void SetupResizeEvent(MyGame * userData);
        SetupResizeEvent(this);
        
        m_Size.x = int(width);
        m_Size.y = int(height);
        SDL_Log("Size: %d, %d", m_Size.x, m_Size.y);
#endif
        m_Window = SDL_CreateWindow("Nightreign App",
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
        
        // m_MapViewer.Initialize();

        // m_MapFilter.Initialize(&m_MapViewer);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(m_Window, m_Context);
        ImGui_ImplOpenGL3_Init();

    }

    void ProcessInput() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                Stop();
                break;
            }
            if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    int w, h;
                    SDL_GetWindowSize(m_Window, &w, &h);
                    SDL_Log("Resized: %d, %d", w, h);
                }
            }
            ImGui_ImplSDL2_ProcessEvent(&event);
            m_Detector.ProcessEvent(event, m_Size.x, m_Size.y);
            // m_MapViewer.HandleAction(&m_MapFilter, result);
        }
    }

    void Update(float deltaTime) override {
        // m_MapViewer.Constrain();
    }


    void RenderImGui() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        auto framerate = ImGui::GetIO().Framerate;
        std::string fmtFps = "Frame Rate: %.1f";
        ImGui::TextColored(ImVec4(0, 1, 0, 1), fmtFps.c_str(), framerate);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void RenderGL() {
        // m_MapViewer.Render();
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

        // m_MapViewer.Cleanup();

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
    glm::vec4 m_BgColor = { 0.2f,0.8f,0.5f,1 };
    bool m_Resized = false;
    
    MapViewer m_MapViewer;
    MapFilter m_MapFilter;
    ActionDetector m_Detector;
};

#ifdef __EMSCRIPTEN__
extern "C" bool EMSCRIPTEN_KEEPALIVE HandleWindowResize(int eventType, const EmscriptenUiEvent* uiEvent, void* userData) {
    SDL_Log("Resize callback");
    if (auto game = static_cast<MyGame*>(userData)) {
        double width, height;
        emscripten_get_element_css_size("canvas", &width, &height);
        emscripten_set_canvas_element_size("canvas", (int)width, (int)height);
        game->OnResize((int)width, (int)height);
    }

    return true;
}
void SetupResizeEvent(MyGame* userData) {
    int result = emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, userData, false, &HandleWindowResize);
    SDL_Log("emscripten_set_resize_callback: %d", result);
}
#endif

int main(int argc, char* argv[]) {
    MyGame game;
    game.SetTargetFPS(60);
    game.Run();

    return 0;
}