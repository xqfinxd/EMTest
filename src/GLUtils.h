#pragma once

#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint CompileShader(GLenum type, const char* source);
GLuint CompileShaderFile(GLenum type, const char* path);
GLuint LoadTexture(const char* path, int& width, int& height, bool flip);
