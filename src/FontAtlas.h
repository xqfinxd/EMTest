#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <SDL.h>
#include <SDL_ttf.h>
#include "GLUtils.h"

struct CharInfo {
    float u0;
    float v0;
    float u1;
    float v1;
    int width;
    int height;
    bool generated;
};

class FontAtlas {
public:
    ~FontAtlas();
    
    bool Initialize(const char* font, int fontSize, int texSize);
    const CharInfo& GetCharInfo(uint32_t charCode);
    GLuint GetTextureID() const { return textureID; }
    void Cleanup();
    void UpdateTexture();
    
private:
    bool GenCharTexture(uint32_t charCode, CharInfo& charInfo);
    bool FindEmptySpace(int width, int height, int& x, int& y);
    
    TTF_Font* font = nullptr;
    GLuint textureID = 0;
    int textureSize = 0;
    int currentX = 0;
    int currentY = 0;
    int rowHeight = 0;
    
    std::map<uint32_t, CharInfo> charMap;
    SDL_Surface* textureSurface;
    bool textureDirty;
};
