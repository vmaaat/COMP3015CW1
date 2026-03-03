#include "scenebasic_uniform.h"

// TinyObjLoader implementation must be defined in exactly one .cpp
#define TINYOBJLOADER_IMPLEMENTATION
#include "helper/tiny_obj_loader.h"

// stb_image implementation for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "helper/stb/stb_image.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>

#include "helper/glutils.h"
#include <GLFW/glfw3.h>

using std::string;
using std::cerr;
using std::endl;
using glm::vec3;

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
SceneBasic_Uniform::SceneBasic_Uniform()
    : vaoHandle(0),
    width(800),
    height(600),
    orbitYaw(0.0f),
    orbitPitch(0.3f),
    orbitRadius(3.0f),
    rotationMatrix(1.0f),
    statueVertexCount(0),
    statueTexture(0),
    planeTexture(0),
    planeVao(0),
    planeVboPos(0),
    planeVboTex(0),
    planeVboNorm(0),
    planeVertexCount(0),
    skyboxVAO(0),
    skyboxVBO(0),
    skyboxTexture(0),
    light1Enabled(true),
    light2Enabled(true),
    light1ColorOriginal(2.0f, 1.8f, 1.2f), //warm light
    light2ColorOriginal(0.6f, 0.8f, 2.0f) //cold light
{
}

// -------------------------------------------------------------
// Cubemap loader
// -------------------------------------------------------------
GLuint SceneBasic_Uniform::loadCubemap(const std::vector<std::string>& faces)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int widthImg, heightImg, channels;

    // Cubemaps generally should not be flipped
    stbi_set_flip_vertically_on_load(false);

    for (size_t i = 0; i < faces.size(); ++i)
    {
        unsigned char* data = stbi_load(faces[i].c_str(),
            &widthImg, &heightImg, &channels, 0);
        if (data)
        {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i),
                0,
                format,
                widthImg, heightImg,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data);
            stbi_image_free(data);
        }
        else
        {
            cerr << "Failed to load cubemap face: " << faces[i] << endl;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return texID;
}

// -------------------------------------------------------------
// Init Scene
// -------------------------------------------------------------
void SceneBasic_Uniform::initScene()
{
    compile();

    std::cout << std::endl;
    prog.printActiveUniforms();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.16f, 0.22f, 1.0f); // background clear (behind skybox if depth test fails)

    // ---------------------------------------------------------
    // Load cat statue model (OBJ) using tinyobjloader
    // ---------------------------------------------------------
    {
        std::string inputFile = "media/models/statueCat.obj";

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t>    shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool ret = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            inputFile.c_str(),
            "media/models/"
        );

        if (!warn.empty()) {
            std::cout << "tinyobj warning: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "tinyobj error: " << err << std::endl;
        }
        if (!ret) {
            std::cerr << "Failed to load OBJ file: " << inputFile << std::endl;
            exit(EXIT_FAILURE);
        }

        statuePositions.clear();
        statueTexcoords.clear();
        statueNormals.clear();

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                // Position
                int vIndex = 3 * index.vertex_index;
                float vx = attrib.vertices[vIndex + 0];
                float vy = attrib.vertices[vIndex + 1];
                float vz = attrib.vertices[vIndex + 2];

                statuePositions.push_back(vx);
                statuePositions.push_back(vy);
                statuePositions.push_back(vz);

                // Texcoords
                if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
                    int tIndex = 2 * index.texcoord_index;
                    float u = attrib.texcoords[tIndex + 0];
                    float v = attrib.texcoords[tIndex + 1];

                    statueTexcoords.push_back(u);
                    statueTexcoords.push_back(1.0f - v);
                }
                else {
                    statueTexcoords.push_back(0.0f);
                    statueTexcoords.push_back(0.0f);
                }

                // Normals
                if (!attrib.normals.empty() && index.normal_index >= 0) {
                    int nIndex = 3 * index.normal_index;
                    float nx = attrib.normals[nIndex + 0];
                    float ny = attrib.normals[nIndex + 1];
                    float nz = attrib.normals[nIndex + 2];

                    statueNormals.push_back(nx);
                    statueNormals.push_back(ny);
                    statueNormals.push_back(nz);
                }
                else {
                    statueNormals.push_back(0.0f);
                    statueNormals.push_back(0.0f);
                    statueNormals.push_back(1.0f);
                }
            }
        }

        statueVertexCount = static_cast<int>(statuePositions.size() / 3);

        std::cout << "Loaded statueCat.obj with "
            << statueVertexCount << " vertices." << std::endl;
    }

    // ---------------------------------------------------------
    // Load cat statue texture (staue1Color.png)
    // ---------------------------------------------------------
    {
        int texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load("media/models/staue1Color.png",
            &texWidth, &texHeight, &texChannels, 0);

        if (!data) {
            std::cerr << "Failed to load texture image: media/models/staue1Color.png" << std::endl;
        }
        else {
            glGenTextures(1, &statueTexture);
            glBindTexture(GL_TEXTURE_2D, statueTexture);

            GLenum format = (texChannels == 4) ? GL_RGBA : GL_RGB;

            glTexImage2D(GL_TEXTURE_2D, 0, format,
                texWidth, texHeight, 0,
                format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            std::cout << "Loaded texture staue1Color.png ("
                << texWidth << "x" << texHeight << ")" << std::endl;
        }
    }

    // ---------------------------------------------------------
    // Create statue VBOs / VAO
    // ---------------------------------------------------------
    {
        GLuint vboHandles[3];
        glGenBuffers(3, vboHandles);
        GLuint positionBufferHandle = vboHandles[0];
        GLuint texcoordBufferHandle = vboHandles[1];
        GLuint normalBufferHandle = vboHandles[2];

        glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(statuePositions.size() * sizeof(float)),
            statuePositions.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferHandle);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(statueTexcoords.size() * sizeof(float)),
            statueTexcoords.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, normalBufferHandle);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(statueNormals.size() * sizeof(float)),
            statueNormals.data(),
            GL_STATIC_DRAW);

        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

#if defined(__APPLE__)
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

        glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferHandle);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

        glBindBuffer(GL_ARRAY_BUFFER, normalBufferHandle);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
#else
        glBindVertexBuffer(0, positionBufferHandle, 0, sizeof(GLfloat) * 3);
        glBindVertexBuffer(1, texcoordBufferHandle, 0, sizeof(GLfloat) * 2);
        glBindVertexBuffer(2, normalBufferHandle, 0, sizeof(GLfloat) * 3);

        glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(0, 0);

        glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(1, 1);

        glVertexAttribFormat(2, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(2, 2);
#endif
        glBindVertexArray(0);
    }

    // ---------------------------------------------------------
    // Load floor texture (stone.png)
    // ---------------------------------------------------------
    {
        int texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load("media/models/grass.png",
            &texWidth, &texHeight, &texChannels, 0);

        if (!data) {
            std::cerr << "Failed to load floor texture stone.png" << std::endl;
        }
        else {
            glGenTextures(1, &planeTexture);
            glBindTexture(GL_TEXTURE_2D, planeTexture);

            GLenum format = (texChannels == 4) ? GL_RGBA : GL_RGB;

            glTexImage2D(GL_TEXTURE_2D, 0, format,
                texWidth, texHeight, 0,
                format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            std::cout << "Loaded floor texture stone.png ("
                << texWidth << "x" << texHeight << ")" << std::endl;
        }
    }

    // ---------------------------------------------------------
    // Create a simple ground plane
    // ---------------------------------------------------------
    {
        std::vector<float> planePos = {
            -5.0f, 0.0f, -5.0f,
             5.0f, 0.0f, -5.0f,
             5.0f, 0.0f,  5.0f,

            -5.0f, 0.0f, -5.0f,
             5.0f, 0.0f,  5.0f,
            -5.0f, 0.0f,  5.0f
        };

        std::vector<float> planeTex = {
            0.0f, 0.0f,
            8.0f, 0.0f,
            8.0f, 8.0f,

            0.0f, 0.0f,
            8.0f, 8.0f,
            0.0f, 8.0f
        };

        std::vector<float> planeNorm(18, 0.0f);
        for (int i = 0; i < 6; ++i) {
            planeNorm[i * 3 + 0] = 0.0f;
            planeNorm[i * 3 + 1] = 1.0f;
            planeNorm[i * 3 + 2] = 0.0f;
        }

        planeVertexCount = 6;

        glGenVertexArrays(1, &planeVao);
        glGenBuffers(1, &planeVboPos);
        glGenBuffers(1, &planeVboTex);
        glGenBuffers(1, &planeVboNorm);

        glBindVertexArray(planeVao);

        glBindBuffer(GL_ARRAY_BUFFER, planeVboPos);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(planePos.size() * sizeof(float)),
            planePos.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glBindBuffer(GL_ARRAY_BUFFER, planeVboTex);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(planeTex.size() * sizeof(float)),
            planeTex.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glBindBuffer(GL_ARRAY_BUFFER, planeVboNorm);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(planeNorm.size() * sizeof(float)),
            planeNorm.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glBindVertexArray(0);
    }

    // ---------------------------------------------------------
    // Create skybox cube geometry
    // ---------------------------------------------------------
    {
        float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);

        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices),
            skyboxVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glBindVertexArray(0);
    }

    // ---------------------------------------------------------
    // Load dungeon cubemap textures
    // ---------------------------------------------------------
    {
        std::vector<std::string> faces = {
            "media/skybox/skybox_right.png",
            "media/skybox/skybox_left.png",
            "media/skybox/skybox_top.png",
            "media/skybox/skybox_bottom.png",
            "media/skybox/skybox_front.png",
            "media/skybox/skybox_back.png"
        };

        skyboxTexture = loadCubemap(faces);

        skyboxProg.use();
        GLuint skyboxHandle = skyboxProg.getHandle();
        GLint skyboxLoc = glGetUniformLocation(skyboxHandle, "skybox");
        if (skyboxLoc != -1) {
            glUniform1i(skyboxLoc, 0);
        }
    }

    // Bind diffuseTex sampler for main shader to texture unit 0
    prog.use();
    GLuint programHandle = prog.getHandle();
    GLint texLoc = glGetUniformLocation(programHandle, "diffuseTex");
    if (texLoc != -1) {
        glUniform1i(texLoc, 0);
    }
}

// -------------------------------------------------------------
// Compile shaders
// -------------------------------------------------------------
void SceneBasic_Uniform::compile()
{
    try {
        // Main shader
        prog.compileShader("shader/basic_uniform.vert");
        prog.compileShader("shader/basic_uniform.frag");
        prog.link();

        // Skybox shader
        skyboxProg.compileShader("shader/skybox.vert");
        skyboxProg.compileShader("shader/skybox.frag");
        skyboxProg.link();

        prog.use();
    }
    catch (GLSLProgramException& e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}

// -------------------------------------------------------------
// Update (orbit camera + light toggles)
// -------------------------------------------------------------
void SceneBasic_Uniform::update(float t)
{
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) return;

    float orbitSpeed = 0.02f;
    float radiusSpeed = 0.05f;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        orbitYaw -= orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        orbitYaw += orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        orbitPitch += orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        orbitPitch -= orbitSpeed;

    const float pitchUpperLimit = 1.2f;   
    const float pitchLowerLimit = -0.1f;  

    if (orbitPitch > pitchUpperLimit)  orbitPitch = pitchUpperLimit;
    if (orbitPitch < pitchLowerLimit)  orbitPitch = pitchLowerLimit;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        orbitRadius -= radiusSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        orbitRadius += radiusSpeed;

    if (orbitRadius < 1.5f)  orbitRadius = 1.5f;
    if (orbitRadius > 10.0f) orbitRadius = 10.0f;

    light1Enabled = (glfwGetKey(window, GLFW_KEY_K) != GLFW_PRESS);
    light2Enabled = (glfwGetKey(window, GLFW_KEY_L) != GLFW_PRESS);
}

// -------------------------------------------------------------
// Render
// -------------------------------------------------------------
void SceneBasic_Uniform::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera matrices
    float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    vec3 target(0.0f, 0.0f, 0.0f);
    float x = orbitRadius * cosf(orbitPitch) * sinf(orbitYaw);
    float y = orbitRadius * sinf(orbitPitch);
    float z = orbitRadius * cosf(orbitPitch) * cosf(orbitYaw);
    vec3 eye(x, y, z);
    vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, target, up);

    // -----------------------------------------
    // 1) Draw skybox
    // -----------------------------------------
    glDepthFunc(GL_LEQUAL); // skybox should pass when depth is 1.0

    skyboxProg.use();
    GLuint skyboxHandle = skyboxProg.getHandle();
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));

    GLint viewLoc = glGetUniformLocation(skyboxHandle, "view");
    GLint projLoc = glGetUniformLocation(skyboxHandle, "projection");

    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewNoTrans[0][0]);
    }
    if (projLoc != -1) {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS); // reset

    // -----------------------------------------
    // 2) Draw floor + cat with main shader
    // -----------------------------------------
    prog.use();
    GLuint programHandle = prog.getHandle();

    // Lighting uniforms
    GLint viewPosLoc = glGetUniformLocation(programHandle, "viewPos");
    if (viewPosLoc != -1) {
        glUniform3f(viewPosLoc, eye.x, eye.y, eye.z);
    }

    // Make light 1 follow the camera (slightly above the eye for nicer highlight)
    GLint l1PosLoc = glGetUniformLocation(programHandle, "light1.position");
    GLint l1ColLoc = glGetUniformLocation(programHandle, "light1.color");
    if (l1PosLoc != -1) {
        vec3 light1Pos = eye + vec3(0.0f, 0.5f, 0.0f);   // small Y offset so it's not *inside* the camera
        glUniform3f(l1PosLoc, light1Pos.x, light1Pos.y, light1Pos.z);
    }
    if (l1ColLoc != -1) {
        vec3 col = light1Enabled ? light1ColorOriginal : vec3(0.0f);
        glUniform3f(l1ColLoc, col.x, col.y, col.z);
    }

    GLint l2PosLoc = glGetUniformLocation(programHandle, "light2.position");
    GLint l2ColLoc = glGetUniformLocation(programHandle, "light2.color");
    if (l2PosLoc != -1) {
        glUniform3f(l2PosLoc, 3.0f, 2.0f, 1.0f);
    }
    if (l2ColLoc != -1) {
        vec3 col = light2Enabled ? light2ColorOriginal : vec3(0.0f);
        glUniform3f(l2ColLoc, col.x, col.y, col.z);
    }

    GLint matLoc = glGetUniformLocation(programHandle, "RotationMatrix");

    // Draw floor
    if (matLoc != -1) {
        glm::mat4 planeModel = glm::mat4(1.0f);
        const float planeY = -0.55f;
        planeModel = glm::translate(planeModel, glm::vec3(0.0f, planeY, 0.0f));

        rotationMatrix = proj * view * planeModel;
        glUniformMatrix4fv(matLoc, 1, GL_FALSE, &rotationMatrix[0][0]);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTexture);

    glBindVertexArray(planeVao);
    glDrawArrays(GL_TRIANGLES, 0, planeVertexCount);
    glBindVertexArray(0);

    // Draw cat statue
    if (matLoc != -1) {
        glm::mat4 model = glm::mat4(1.0f);
        const float catScale = 0.15f;
        const float catY = -3.5f;

        model = glm::scale(model, glm::vec3(catScale, catScale, catScale));
        model = glm::translate(model, glm::vec3(0.0f, catY, 0.0f));

        rotationMatrix = proj * view * model;
        glUniformMatrix4fv(matLoc, 1, GL_FALSE, &rotationMatrix[0][0]);
    }

    glBindTexture(GL_TEXTURE_2D, statueTexture);
    glBindVertexArray(vaoHandle);
    glDrawArrays(GL_TRIANGLES, 0, statueVertexCount);
    glBindVertexArray(0);
}

// -------------------------------------------------------------
// Resize
// -------------------------------------------------------------
void SceneBasic_Uniform::resize(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, w, h);
} 