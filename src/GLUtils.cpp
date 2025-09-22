#include "GLUtils.h"
#include "stb_image.h"
#include <SDL_log.h>
#include <fstream>

GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // ºÏ≤È±‡“Î¥ÌŒÛ
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        SDL_Log("Shader compilation error: %s\n", infoLog);
    }
    return shader;
}

GLuint CompileShaderFile(GLenum type, const char* path) {
    std::ifstream inputFile(path, std::ios::in);
    if (!inputFile.is_open()) {
        SDL_Log("Could not open the file %s\n", path);
        return 0;
    }
    inputFile.seekg(0, std::ios::end);
    std::string source;
    size_t count = inputFile.tellg();
    source.resize(count);
    inputFile.seekg(0, std::ios::beg);
    inputFile.read(source.data(), count);
    return CompileShader(type, source.c_str());
}

GLuint LoadTexture(const char* path, int& width, int& height, bool flip) {
    stbi_set_flip_vertically_on_load(flip);

    int nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) {
        SDL_Log("Failed to load texture: %s", path);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    return texture;
}