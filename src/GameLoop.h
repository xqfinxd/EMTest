#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <SDL.h>

class GameLoop {
public:
    virtual ~GameLoop();

    void Run();
    void Stop();

    void Pause();
    void Resume();
    bool IsPaused() const { return m_Paused; }

    void SetTargetFPS(int fps) { m_TargetFPS = fps; }
    int GetTargetFPS() const { return m_TargetFPS; }

protected:
    virtual void Initialize() = 0;
    virtual void ProcessInput() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Cleanup();

private:
    void MainLoop();
    static void MainLoopWrapper(void* userData);

    bool m_Running = false;
    bool m_Paused = false;
    int m_TargetFPS = 30;
    Uint32 m_LastFrameTime = 0;
};
