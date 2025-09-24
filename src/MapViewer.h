#ifdef WIN32
#include <glad/glad.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "MapDefines.h"

class MapViewer {
public:
    struct View {
        glm::ivec2 offset;
        float zoom = 1.0f;

        void Reset() {
            zoom = 1.f;
            offset = glm::vec2(0, 0);
        }
    };

private:
    using IconMap = std::unordered_map<std::string, glm::ivec4>;
    GLuint m_ImagePipeline;
    GLuint m_ImageVAO = 0;
    GLuint m_ImageVBO = 0;
    GLuint m_ImageEBO = 0;

    GLuint m_MapTexture = 0;
    glm::ivec2 m_MapSize;

    GLuint m_IconsTexture = 0;
    glm::ivec2 m_IconsSize;
    IconMap m_IconMap;

    View m_View;

    void InitImagePipeline();
    void DrawMap(const glm::mat4& vpMat);
    void DrawIcon(const glm::mat4& vpMat, const std::string& name, glm::ivec2 pos);

    glm::vec2 GetViewSize(const glm::ivec2& viewPort) const;

public:
    void Initialize();
    void Cleanup();
    void Render(const glm::vec2& viewPort);
    void Constrain(const glm::vec2& viewPort);

    View& GetView() { return m_View; }
    const View& GetView() const {
        return const_cast<MapViewer*>(this)->GetView();
    }
};
