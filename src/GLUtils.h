#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif

#include <glm/glm.hpp>
#include <glm/ext.hpp>

GLuint CompileShader(GLenum type, const char* source);
GLuint LoadTexture(const char* path, int& width, int& height, bool flip);