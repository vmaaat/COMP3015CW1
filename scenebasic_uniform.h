#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class SceneBasic_Uniform : public Scene
{
private:
    // Main shader program for cat + floor
    GLSLProgram prog;

    // Skybox shader
    GLSLProgram skyboxProg;

    // Geometry / state
    GLuint vaoHandle;
    int    width, height;

    // Orbit camera parameters
    float orbitYaw;
    float orbitPitch;
    float orbitRadius;

    glm::mat4 rotationMatrix;

    // Cat statue mesh data
    std::vector<float> statuePositions;   // x, y, z per vertex
    std::vector<float> statueTexcoords;   // u, v per vertex
    std::vector<float> statueNormals;     // nx, ny, nz per vertex
    int                 statueVertexCount;

    // Textures
    GLuint statueTexture;
    GLuint planeTexture;

    // Floor plane geometry
    GLuint planeVao;
    GLuint planeVboPos;
    GLuint planeVboTex;
    GLuint planeVboNorm;
    int    planeVertexCount;

    // Skybox geometry + texture
    GLuint skyboxVAO;
    GLuint skyboxVBO;
    GLuint skyboxTexture;

    // Light state (for keyboard toggles)
    bool      light1Enabled;
    bool      light2Enabled;
    glm::vec3 light1ColorOriginal;
    glm::vec3 light2ColorOriginal;

public:
    SceneBasic_Uniform();

    void initScene() override;
    void update(float t) override;
    void render() override;
    void resize(int, int) override;

private:
    void   compile();
    GLuint loadCubemap(const std::vector<std::string>& faces);
};

#endif // SCENEBASIC_UNIFORM_H