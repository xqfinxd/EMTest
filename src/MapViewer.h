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
#include <cmath>
#include <random>


// 地标结构
struct Landmark {
    glm::vec2 position;
    float scale;
    glm::vec4 tintColor;
    int textureIndex; // 在地标合集纹理中的索引
};

// 地图视图结构
struct MapView {
    glm::vec2 offset = glm::vec2(0.0f);
    float zoom = 1.0f;
    
    bool showGrid = true;
    glm::vec4 gridColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
};

class MapViewer {
private:
    GLuint mapShaderProgram;
    GLuint landmarkShaderProgram;
    GLuint mapVAO, mapVBO;
    GLuint landmarkVAO, landmarkVBO;
    GLuint mapTexture;
    GLuint landmarkAtlasTexture;
    int mapWidth, mapHeight;
    int atlasWidth, atlasHeight;
    int landmarksPerRow;

    std::vector<Landmark> landmarks;

    void setupMapBuffers();

    void setupLandmarkBuffers();

    void generateRandomLandmarks(int count);

public:
    void Initialize(const std::string& mapPath, const std::string& atlasPath);
    void Cleanup();
    void Render(const MapView& view, const glm::vec2& viewPort);

    int getMapWidth() const { return mapWidth; }
    int getMapHeight() const { return mapHeight; }
};
