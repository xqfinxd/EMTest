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

// 地标结构
struct Landmark {
    glm::vec2 position;
    float scale;
    glm::vec4 tintColor;
    int textureIndex; // 在地标合集纹理中的索引
};

// 地图视图结构
struct MapView {
    glm::ivec2 offset;
    float zoom = 1.0f;
    
    bool showGrid = true;
    glm::vec4 gridColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
};

class MapViewer {
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

    void InitImagePipeline();
    void DrawMap(const glm::mat4& vpMat);
    void DrawIcon(const glm::mat4& vpMat, const std::string& name, glm::ivec2 pos);

public:
    void Initialize();
    void Cleanup();
    void Render(const MapView& view, const glm::vec2& viewPort);
    void Constrain(MapView& view, const glm::vec2& viewPort);
};
