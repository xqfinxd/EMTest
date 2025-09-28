#include "FontAtlas.h"

FontAtlas::~FontAtlas() {
    Cleanup();
}

bool FontAtlas::Initialize(const char* fontPath, int fontSize, int texSize) {
    if (TTF_Init() == -1) {
        SDL_Log("Failed to initialize SDL: %s\n", TTF_GetError());
        return false;
    }

    font = TTF_OpenFont(fontPath, fontSize);
    if (!font) {
        SDL_Log("Failed to load font: %s\n", TTF_GetError());
        return false;
    }

    textureSize = texSize;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // TODO: don't use alpha
    std::vector<unsigned char> emptyData(textureSize * textureSize * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, emptyData.data());
    
    textureSurface = SDL_CreateRGBSurfaceWithFormat(0, textureSize, textureSize, 32, SDL_PIXELFORMAT_RGBA32);
    if (!textureSurface) {
        SDL_Log("Failed to create font surface: %s\n", TTF_GetError());
        return false;
    }
    
    SDL_FillRect(textureSurface, nullptr, SDL_MapRGBA(textureSurface->format, 0, 0, 0, 0));
    
    return true;
}

const CharInfo& FontAtlas::GetCharInfo(uint32_t charCode) {
    auto it = charMap.find(charCode);
    if (it != charMap.end()) {
        return it->second;
    }
    
    CharInfo charInfo;
    if (GenCharTexture(charCode, charInfo)) {
        charMap[charCode] = charInfo;
        return charMap[charCode];
    }
    
    static CharInfo emptyChar = {0, 0, 0, 0, 0, 0, 0, 0, false};
    return emptyChar;
}

bool FontAtlas::GenCharTexture(uint32_t charCode, CharInfo& charInfo) {
    if (!font) return false;
    
    std::string charUtf8;
    if (charCode <= 0x7F) {
        charUtf8 += static_cast<char>(charCode);
    } else if (charCode <= 0x7FF) {
        charUtf8 += static_cast<char>(0xC0 | (charCode >> 6));
        charUtf8 += static_cast<char>(0x80 | (charCode & 0x3F));
    } else if (charCode <= 0xFFFF) {
        charUtf8 += static_cast<char>(0xE0 | (charCode >> 12));
        charUtf8 += static_cast<char>(0x80 | ((charCode >> 6) & 0x3F));
        charUtf8 += static_cast<char>(0x80 | (charCode & 0x3F));
    } else {
        charUtf8 += static_cast<char>(0xF0 | (charCode >> 18));
        charUtf8 += static_cast<char>(0x80 | ((charCode >> 12) & 0x3F));
        charUtf8 += static_cast<char>(0x80 | ((charCode >> 6) & 0x3F));
        charUtf8 += static_cast<char>(0x80 | (charCode & 0x3F));
    }
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* charSurface = TTF_RenderUTF8_Blended(font, charUtf8.c_str(), white);
    if (!charSurface) {
        SDL_Log("Failed to generate font texture: %s\n", TTF_GetError());
        return false;
    }
    
    SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(charSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(charSurface);
    
    if (!rgbaSurface) {
        SDL_Log("Failed to generate RGBA font texture: %s\n", TTF_GetError());
        return false;
    }
    
    int minx, maxx, miny, maxy, advance;
    TTF_GlyphMetrics32(font, charCode, &minx, &maxx, &miny, &maxy, &advance);
    
    charInfo.width = rgbaSurface->w;
    charInfo.height = rgbaSurface->h;
    charInfo.bearingx = minx;
    charInfo.bearingy = miny;

    int x, y;
    if (!FindEmptySpace(charInfo.width, charInfo.height, x, y)) {
        SDL_FreeSurface(rgbaSurface);
        SDL_Log("No available texture space to add new char: %u\n", charCode);
        return false;
    }
    
    rowHeight = std::max(rowHeight, charInfo.height + 2);
    
    SDL_Rect destRect = {x, y, charInfo.width, charInfo.height};
    SDL_BlitSurface(rgbaSurface, nullptr, textureSurface, &destRect);
    SDL_FreeSurface(rgbaSurface);
    
    charInfo.u0 = 1.f * x / textureSize;
    charInfo.v0 = 1.f * y / textureSize;
    charInfo.u1 = 1.f * (x + charInfo.width) / textureSize;
    charInfo.v1 = 1.f * (y + charInfo.height) / textureSize;
    charInfo.generated = true;
    
    textureDirty = true;
    currentX = x + charInfo.width + 1;
    
    return true;
}

bool FontAtlas::FindEmptySpace(int width, int height, int& x, int& y) {
    if (currentX + width + 1 < textureSize) {
        x = currentX;
        y = currentY;
        return true;
    }
    
    if (currentY + rowHeight + height + 1 < textureSize) {
        currentX = 1;
        currentY += rowHeight + 1;
        rowHeight = height;
        x = currentX;
        y = currentY;
        return true;
    }
    
    return false;
}

void FontAtlas::UpdateTexture() {
    if (textureDirty && textureSurface) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize, textureSize,
                       GL_RGBA, GL_UNSIGNED_BYTE, textureSurface->pixels);
        textureDirty = false;
    }
}

void FontAtlas::Cleanup() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    if (textureSurface) {
        SDL_FreeSurface(textureSurface);
        textureSurface = nullptr;
    }
    charMap.clear();

    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }

    TTF_Quit();
}
