#include "GameLoop.h"
#include <iostream>

GameLoop::~GameLoop() {
    Cleanup();
}

void GameLoop::Run() {
    if (m_Running) return;

    // 调用子类初始化
    Initialize();

    m_Running = true;
    m_LastFrameTime = SDL_GetTicks();

    // 平台特定的运行方式
#ifdef __EMSCRIPTEN__
    // Emscripten 的非阻塞回调
    emscripten_set_main_loop_arg(MainLoopWrapper, this, 0, 1);
#else
    // Windows原生平台的阻塞式循环
    while (m_Running) {
        MainLoop();
    }
    Cleanup();
#endif
}

void GameLoop::MainLoopWrapper(void* userData) {
    try {
        GameLoop* game = static_cast<GameLoop*>(userData);
        game->MainLoop();
    }
    catch (...) {
        std::cout << "hello \n";
    }
}

void GameLoop::MainLoop() {
    if (!m_Running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    if (m_Paused) {
        SDL_Delay(1000 / 30); // 暂停时以30帧运行
        return;
    }

    // 计算帧时间
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - m_LastFrameTime) / 1000.0f;
    m_LastFrameTime = currentTime;

    // 限制最大deltaTime避免物理问题
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }

    // 游戏逻辑执行
    ProcessInput();
    Update(deltaTime);
    Render();

    // 帧率控制（仅原生平台需要）
#ifndef __EMSCRIPTEN__
    if (m_TargetFPS > 0) {
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        Uint32 minFrameTime = 1000 / m_TargetFPS;
        if (frameTime < minFrameTime) {
            SDL_Delay(minFrameTime - frameTime);
        }
    }
#endif
}

void GameLoop::Stop() {
    m_Running = false;
}

void GameLoop::Pause() {
    m_Paused = true;
}

void GameLoop::Resume() {
    m_Paused = false;
    m_LastFrameTime = SDL_GetTicks(); // 重置时间避免大的deltaTime
}

void GameLoop::Cleanup() {
}