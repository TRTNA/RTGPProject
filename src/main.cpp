/*
author: Davide Gadia

Real-Time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <../imgui/imgui.h>
#include <../imgui/imgui_impl_glfw.h>
#include <../imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
#error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, and for FPS camera
#include <utils/utils.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// Std. Includes
#include <string>
#include <vector>
#include <array>
using std::string;
using std::vector;
using std::array;

#define SHADERS_DIR_PATH "shaders"
#define TEXTURES_DIR_PATH "../textures"
#define MODELS_DIR_PATH "../models"

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void apply_camera_movements();
void SetupShader(int shader_program);
void PrintCurrentShader(int subroutine);
void RenderObjects(Shader &shader);
void PerformShadowMapping(Shader &shadowShader, GLuint depthMapFBO);
void PerformIlluminationPass(Shader &shader, glm::vec3 &absorptionCoeff, glm::vec3 &scatteringCoeff, float gCoeff);
void PerformSkyboxPass(Shader &shader, Model &skyboxCube, glm::vec3 &absorptionCoeff, glm::vec3 &scatteringCoeff, float gCoeff);
void RenderAxis(Shader& shader, ArrowLine& xAxis, ArrowLine& yAxis, ArrowLine& zAxis);
ArrowLine CreateArrowLine(const vector<glm::vec3>& pointsPos, const glm::vec4& color);
void CreateSceneObjects(Model& planeModel, Model& sphereModel, Model& cubeModel);
void PerformSkyBoxPass(Shader& shader, Model &skyboxCube);



// dimensions of application's window
GLuint screenWidth = 1200, screenHeight = 900;

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
vector<string> shaders;
// we initialize an array of booleans for each keyboard key
bool keys[1024];

bool menuIsActive = true;

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//CAMERA PARAMETERS
// View matrix: the camera moves, so we just set to indentity now
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);
// we create a camera. We pass the initial position as a paramenter to the constructor. The last boolean tells if we want a camera "anchored" to the ground
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_FALSE);
bool detachMouseFromCamera = false;

// point light pos
glm::vec3 lightPos = glm::vec3(0.0f, 30.0f, 15.0f);

// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 0.4f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// UV repetitions
GLfloat repeat = 1.0;

std::vector<std::unique_ptr<Object>> objects;

CubeMap *cubeMap = nullptr;
Texture2D *debugTex;

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
constexpr float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
const float near = 0.1f;
const float far = 100.0f;
const glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

int width, height;


int skyboxTechnique = 0;
int phaseFunction = 0;

array<GLuint, 4> illuminationShaderSubroutines;
array<GLuint, 4> skyboxShaderSubroutines;

int main()
{
    // initw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "main", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &width, &height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // AXIS CREATION
    const glm::vec4 xColorVec = glm::vec4(1.0f, 0.0f, 0.0f, 0.7f);
    const vector<glm::vec3> xPointsPositions{
        glm::vec3(-1000.0f, 0.0f, 0.0f),
        glm::vec3(1000.0f, 0.0f, 0.0f),
        glm::vec3(1000.0f, 50.0f, 0.0f),
        glm::vec3(1000.0f, -50.0f, 0.0f),
        glm::vec3(1050.0f, 0.0f, 0.0f)};
    ArrowLine xAxis = CreateArrowLine(xPointsPositions, xColorVec);

    const glm::vec4 yColorVec = glm::vec4(0.0f, 1.0f, 0.0f, 0.7f);
    const vector<glm::vec3> yPointsPositions{
        glm::vec3(0.0f, -1000.0f, 0.0f),
        glm::vec3(0.0f, 1000.0f, 0.0f),
        glm::vec3(50.0f, 1000.0f, 0.0f),
        glm::vec3(-50.0f, 1000.0f, 0.0f),
        glm::vec3(0.0f, 1050.0f, 0.0f)};
    ArrowLine yAxis = CreateArrowLine(xPointsPositions, xColorVec);

    const glm::vec4 zColorVec = glm::vec4(1.0f, 0.0f, 0.0f, 0.7f);
    const vector<glm::vec3> zPointsPositions{
        glm::vec3(0.0f, 0.0f, -1000.0f),
        glm::vec3(0.0f, 0.0f, 1000.0f),
        glm::vec3(0.0f, 50.0f, 1000.0f),
        glm::vec3(0.0f, -50.0f, 1000.0f),
        glm::vec3(0.0f, 0.0f, 1050.0f)};
    ArrowLine zAxis = CreateArrowLine(zPointsPositions, zColorVec);

    // SHADERS
    Shader shadow_shader(SHADERS_DIR_PATH "/shadowmap.vert", SHADERS_DIR_PATH "/shadowmap.frag", SHADERS_DIR_PATH "/shadowmap.geom");
    Shader illumination_shader(SHADERS_DIR_PATH "/object_partmedia.vert", SHADERS_DIR_PATH "/object_partmedia.frag");
    Shader flat_shader(SHADERS_DIR_PATH "/flat.vert", SHADERS_DIR_PATH "/flat.frag");
    Shader skybox_partmedia_shader(SHADERS_DIR_PATH "/skybox_partmedia.vert", SHADERS_DIR_PATH "/skybox_partmedia.frag");
    Shader skybox_fog_shader(SHADERS_DIR_PATH "/skybox_fog.vert", SHADERS_DIR_PATH "/skybox_fog.frag");

    SetupShader(illumination_shader.Program);
    PrintCurrentShader(current_subroutine);

    // TEXTURES
    cubeMap = new CubeMap(TEXTURES_DIR_PATH "/cube/Maskonaive2/");
    cubeMap->Load();

    debugTex = new Texture2D(TEXTURES_DIR_PATH "/UV_Grid_Sm.png", Texture2DType::DIFFUSE);
    debugTex->Load();

    // MODELS
    Model planeModel = Model(MODELS_DIR_PATH "/plane.obj");
    Model cubeModel(MODELS_DIR_PATH "/cube.obj"); // used for the environment map
    Model sphereModel(MODELS_DIR_PATH "/sphere.obj");

    // SCENE SETUP
    CreateSceneObjects(planeModel, sphereModel, cubeModel);

    // DEPTH MAP CONFIGURATION
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Projection matrix of the camera: FOV angle, aspect ratio, near and far planes
    projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, near, far);

    // PARTICIPATING MEDIA
    glm::vec3 absorptionCoeff = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 scatteringCoeff = glm::vec3(0.150f, 0.150f, 0.150f);
    float gCoeff = 0.0f;

    // ImGui SETUP
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    // Constant shaders' values setup
    shadow_shader.Use();
    glUniform1f(glGetUniformLocation(shadow_shader.Program, "far_plane"), far);

    illumination_shader.Use();
    glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(illumination_shader.Program, "Kd"), Kd);
    glUniform1f(glGetUniformLocation(illumination_shader.Program, "alpha"), alpha);
    glUniform1f(glGetUniformLocation(illumination_shader.Program, "F0"), F0);
    glUniform1f(glGetUniformLocation(illumination_shader.Program, "far_plane"), far);
    glUniform1f(glGetUniformLocation(illumination_shader.Program, "repeat"), repeat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, debugTex->GetTextureId());
    glUniform1i(glGetUniformLocation(illumination_shader.Program, "tex"), 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    glUniform1i(glGetUniformLocation(illumination_shader.Program, "depthMap"), 2);

    illuminationShaderSubroutines = {
        glGetSubroutineIndex(illumination_shader.Program, GL_FRAGMENT_SHADER, "miePhaseFunc"), 
        glGetSubroutineIndex(illumination_shader.Program, GL_FRAGMENT_SHADER, "rayleighPhaseFunc"),
        glGetSubroutineIndex(illumination_shader.Program, GL_FRAGMENT_SHADER, "schlickPhaseFunc"),
        glGetSubroutineIndex(illumination_shader.Program, GL_FRAGMENT_SHADER, "uniformPhaseFunc")
    };

    skybox_partmedia_shader.Use();
    glUniform1f(glGetUniformLocation(skybox_partmedia_shader.Program, "far_plane_vert"), far);
    glUniformMatrix4fv(glGetUniformLocation(skybox_partmedia_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap->GetId());
    glUniform1i(glGetUniformLocation(skybox_partmedia_shader.Program, "skyboxTex"), 3);
    glUniform1f(glGetUniformLocation(skybox_partmedia_shader.Program, "far_plane"), far);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    glUniform1i(glGetUniformLocation(skybox_partmedia_shader.Program, "depthMap"), 2);

    skyboxShaderSubroutines = {
        glGetSubroutineIndex(skybox_partmedia_shader.Program, GL_FRAGMENT_SHADER, "miePhaseFunc"), 
        glGetSubroutineIndex(skybox_partmedia_shader.Program, GL_FRAGMENT_SHADER, "rayleighPhaseFunc"),
        glGetSubroutineIndex(skybox_partmedia_shader.Program, GL_FRAGMENT_SHADER, "schlickPhaseFunc"),
        glGetSubroutineIndex(skybox_partmedia_shader.Program, GL_FRAGMENT_SHADER, "uniformPhaseFunc")
    };

    skybox_fog_shader.Use();
    glUniformMatrix4fv(glGetUniformLocation(skybox_fog_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
    float fogDensity = 2.0f;
    glUniform1f(glGetUniformLocation(skybox_fog_shader.Program, "fogDensity"), fogDensity);
    glm::vec3 fogColor = glm::vec3(0.5f, 0.5f, 0.5f);
    glUniform3fv(glGetUniformLocation(skybox_fog_shader.Program, "fogColor"), 1, glm::value_ptr(fogColor));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap->GetId());
    glUniform1i(glGetUniformLocation(skybox_fog_shader.Program, "tCube"), 3);


    flat_shader.Use();
    glUniformMatrix4fv(glGetUniformLocation(flat_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(flat_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));

    // Rendering loop: this code is executed at each frame
    while (!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        apply_camera_movements();

        PerformShadowMapping(shadow_shader, depthMapFBO);

        view = camera.GetViewMatrix();

        PerformIlluminationPass(illumination_shader, absorptionCoeff, scatteringCoeff, gCoeff);

        if (skyboxTechnique == 0) {
            PerformSkyBoxPass(skybox_fog_shader, cubeModel);
        } else  {
            PerformSkyboxPass(skybox_partmedia_shader, cubeModel, absorptionCoeff, scatteringCoeff, gCoeff);
        }

        RenderAxis(flat_shader, xAxis, yAxis, zAxis);

        // GUI RENDERING
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {650.f,460.f });
        ImGui::Begin("Tools", &menuIsActive, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();
        ImGui::BeginChild("Participating media rendering", ImVec2(600, 270), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Participating media coefficients:");
        ImGui::Indent();
        ImGui::SliderFloat("absorptionCoefficient_R", &absorptionCoeff.x, 0.0f, 1.0f);
        ImGui::SliderFloat("absorptionCoefficient_G", &absorptionCoeff.y, 0.0f, 1.0f);
        ImGui::SliderFloat("absorptionCoefficient_B", &absorptionCoeff.z, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::SliderFloat("scatteringCoefficient_R", &scatteringCoeff.x, 0.0f, 1.0f);
        ImGui::SliderFloat("scatteringCoefficient_G", &scatteringCoeff.y, 0.0f, 1.0f);
        ImGui::SliderFloat("scatteringCoefficient_B", &scatteringCoeff.z, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::SliderFloat("g coefficient", &gCoeff, -1.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Phase function:");
        ImGui::RadioButton("Mie", &phaseFunction, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Rayleigh", &phaseFunction, 1);
        ImGui::RadioButton("Schlick", &phaseFunction, 2);
        ImGui::SameLine();
        ImGui::RadioButton("Uniform", &phaseFunction, 3);
        ImGui::EndChild();

        ImGui::BeginChild("Point light", ImVec2(600, 100), true);
        ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Point light");
        ImGui::Indent();
        ImGui::SliderFloat("light x", &lightPos[0], -100.0f, 100.0f);
        ImGui::SliderFloat("light y", &lightPos[1], -100.0f, 100.0f);
        ImGui::SliderFloat("light z", &lightPos[2], -100.0f, 100.0f);

        ImGui::EndChild();

        ImGui::BeginChild("Skybox options", ImVec2(600, 80), true);
        ImGui::TextColored(ImVec4(0.0, 1.0, 1.0, 1.0), "Skybox rendering technique");
        ImGui::Indent();
        ImGui::RadioButton("Volumetric Fog Skybox", &skyboxTechnique, 0);
        ImGui::RadioButton("Participating Media Skybox", &skyboxTechnique, 1);
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    illumination_shader.Delete();
    shadow_shader.Delete();
    skybox_partmedia_shader.Delete();
    flat_shader.Delete();
    delete cubeMap;
    delete debugTex;

    glfwTerminate();
    return 0;
}

ArrowLine CreateArrowLine(const vector<glm::vec3>& pointsPos, const glm::vec4& color) {
    vector<Point> points;
    for (auto &pos : pointsPos)
    {
        Point temp;
        temp.Position = pos;
        temp.Color = color;
        points.push_back(temp);
    }

    return ArrowLine(points, 2);
}

void CreateSceneObjects(Model& planeModel, Model& sphereModel, Model& cubeModel) {
    auto floor = std::make_unique<Object>("Floor");
    floor->SetModel(&planeModel);
    Transform &floorTransform = floor->GetTransform();
    floorTransform.Scale(glm::vec3(2.0f));
    objects.push_back(std::move(floor));

    auto negZWall = std::make_unique<Object>("NegZWall");
    negZWall->SetModel(&planeModel);
    Transform &negZWallTransform = negZWall->GetTransform();
    negZWallTransform.SetPosition(glm::vec3(0.0f, 0.0f, -15.0f));
    negZWallTransform.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f));
    negZWallTransform.Scale(glm::vec3(2.0f));
    objects.push_back(std::move(negZWall));

    auto posXWall = std::make_unique<Object>("PosXWall");
    posXWall->SetModel(&planeModel);
    Transform &posXWallTransform = posXWall->GetTransform();
    posXWallTransform.SetPosition(glm::vec3(15.0f, 0.0f, 0.0f));
    posXWallTransform.Rotate(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(90.0f));
    posXWallTransform.Scale(glm::vec3(2.0f));
    objects.push_back(std::move(posXWall));

    auto cube1Obj = std::make_unique<Object>("Cube 1");
    cube1Obj->SetModel(&cubeModel);
    Transform &cube1Transform = cube1Obj->GetTransform();
    cube1Transform.SetPosition(glm::vec3(0.0f, 4.0f, -3.5f));
    cube1Transform.Scale(glm::vec3(0.5f));
    objects.push_back(std::move(cube1Obj));

    auto cube2Obj = std::make_unique<Object>("Cube 2");
    cube2Obj->SetModel(&cubeModel);
    Transform &cube2Transform = cube2Obj->GetTransform();
    cube2Transform.SetPosition(glm::vec3(-5.0f, 4.0f, -1.5f));
    cube2Transform.Scale(glm::vec3(0.2));
    objects.push_back(std::move(cube2Obj));

    auto sphere1Obj = std::make_unique<Object>("Sphere 1");
    sphere1Obj->SetModel(&sphereModel);
    Transform &sphere1Transform = sphere1Obj->GetTransform();
    sphere1Transform.SetPosition(glm::vec3(5.0f, 4.0f, -6.0f));
    sphere1Transform.Scale(glm::vec3(0.5));
    objects.push_back(std::move(sphere1Obj));

}
void PerformShadowMapping(Shader &shadowShader, GLuint depthMapFBO)
{
    glm::mat4 shadowTransforms[6] = {};
    shadowTransforms[0] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
    shadowTransforms[1] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
    shadowTransforms[2] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
    shadowTransforms[3] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
    shadowTransforms[4] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
    shadowTransforms[5] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

    shadowShader.Use();

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    // we activate the FBO for the depth map rendering
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (unsigned int i = 0; i < 6; ++i)
    {
        string name = "shadowMatrices[" + std::to_string(i) + "]";
        glUniformMatrix4fv(glGetUniformLocation(shadowShader.Program, name.c_str()), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
    }
    glUniform3fv(glGetUniformLocation(shadowShader.Program, "lightPos"), 1, glm::value_ptr(lightPos));

    RenderObjects(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PerformIlluminationPass(Shader &shader, glm::vec3 &absorptionCoeff, glm::vec3 &scatteringCoeff, float gCoeff)
{

    // we "clear" the frame and z buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // we set the viewport for the final rendering step
    glViewport(0, 0, width, height);

    // illumination pass
    shader.Use();

    const GLuint selectedPhaseFunc = (GLuint) phaseFunction;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &selectedPhaseFunc);

    // VERTEX SHADER'S UNIFORMS
    // model matrix is set by object.cpp when render call is fired
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

    // FRAGMENT SHADER'S UNIFORMS
    glUniform3fv(glGetUniformLocation(shader.Program, "wLightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shader.Program, "wCameraPos"), 1, glm::value_ptr(camera.Position));

    // Participating media
    glUniform3fv(glGetUniformLocation(shader.Program, "absorptionCoeff"), 1, glm::value_ptr(absorptionCoeff));
    glUniform3fv(glGetUniformLocation(shader.Program, "scatteringCoeff"), 1, glm::value_ptr(scatteringCoeff));
    glUniform1f(glGetUniformLocation(shader.Program, "g"), gCoeff);

    RenderObjects(shader);
}

void PerformSkyBoxPass(Shader& shader, Model &skyboxCube) {
    shader.Use();
    glDepthFunc(GL_LEQUAL);

    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));

    skyboxCube.Draw();

    glDepthFunc(GL_LESS);
}

void PerformSkyboxPass(Shader &shader, Model &skyboxCube, glm::vec3 &absorptionCoeff, glm::vec3 &scatteringCoeff, float gCoeff)
{
    // skybox
    shader.Use();
    const GLuint selectedPhaseFunc = (GLuint) phaseFunction;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &selectedPhaseFunc);
    glDepthFunc(GL_LEQUAL);

    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));
    glm::mat4 inverseViewProjection = glm::inverse((projection * glm::mat4(glm::mat3(view))));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "inverseViewProjMatrix"), 1, GL_FALSE, glm::value_ptr(inverseViewProjection));
    glUniform1f(glGetUniformLocation(shader.Program, "width"), width);
    glUniform1f(glGetUniformLocation(shader.Program, "height"), height);


    // FOR PARTMEDIA SKYBOX
    glUniform3fv(glGetUniformLocation(shader.Program, "wLightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shader.Program, "wCameraPos"), 1, glm::value_ptr(camera.Position));

    // Participating media
    glUniform3fv(glGetUniformLocation(shader.Program, "absorptionCoeff"), 1, glm::value_ptr(absorptionCoeff));
    glUniform3fv(glGetUniformLocation(shader.Program, "scatteringCoeff"), 1, glm::value_ptr(scatteringCoeff));
    glUniform1f(glGetUniformLocation(shader.Program, "g"), gCoeff);

    skyboxCube.Draw();

    glDepthFunc(GL_LESS);
}

void RenderAxis(Shader& shader, ArrowLine& xAxis, ArrowLine& yAxis, ArrowLine& zAxis) {
    // AXIS RENDERING
        shader.Use();
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        xAxis.Draw();
        yAxis.Draw();
        zAxis.Draw();
}

//////////////////////////////////////////
// we render the objects. We pass also the current rendering step, and the depth map generated in the first step, which is used by the shaders of the second step
void RenderObjects(Shader &shader)
{
    for (size_t i = 0; i < objects.size(); i++)
    {
        objects[i]->Render(shader, view);
    }
}

///////////////////////////////////////////
// The function parses the content of the Shader Program, searches for the Subroutine type names,
// the subroutines implemented for each type, print the names of the subroutines on the terminal, and add the names of
// the subroutines to the shaders vector, which is used for the shaders swapping
void SetupShader(int program)
{
    int maxSub, maxSubU, countActiveSU;
    GLchar name[256];
    int len, numCompS;

    // global parameters about the Subroutines parameters of the system
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSub);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubU);
    std::cout << "Max Subroutines:" << maxSub << " - Max Subroutine Uniforms:" << maxSubU << std::endl;

    // get the number of Subroutine uniforms (only for the Fragment shader, due to the nature of the exercise)
    // it is possible to add similar calls also for the Vertex shader
    glGetProgramStageiv(program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);

    // print info for every Subroutine uniform
    for (int i = 0; i < countActiveSU; i++)
    {

        // get the name of the Subroutine uniform (in this example, we have only one)
        glGetActiveSubroutineUniformName(program, GL_FRAGMENT_SHADER, i, 256, &len, name);
        // print index and name of the Subroutine uniform
        std::cout << "Subroutine Uniform: " << i << " - name: " << name << std::endl;

        // get the number of subroutines
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);

        // get the indices of the active subroutines info and write into the array s
        int *s = new int[numCompS];
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_COMPATIBLE_SUBROUTINES, s);
        std::cout << "Compatible Subroutines:" << std::endl;

        // for each index, get the name of the subroutines, print info, and save the name in the shaders vector
        for (int j = 0; j < numCompS; ++j)
        {
            glGetActiveSubroutineName(program, GL_FRAGMENT_SHADER, s[j], 256, &len, name);
            std::cout << "\t" << s[j] << " - " << name << "\n";
            shaders.push_back(name);
        }
        std::cout << std::endl;

        delete[] s;
    }
}

/////////////////////////////////////////
// we print on console the name of the currently used shader subroutine
void PrintCurrentShader(int subroutine)
{
    std::cout << "Current shader subroutine: " << shaders[subroutine] << std::endl;
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    GLuint new_subroutine;

    // if ESC is pressed, we close the application
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // pressing a key number, we change the shader applied to the models
    // if the key is between 1 and 9, we proceed and check if the pressed key corresponds to
    // a valid subroutine
    if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_9) && action == GLFW_PRESS)
    {
        // "1" to "9" -> ASCII codes from 49 to 59
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 9
        // we subtract 1 to have indices from 0 to 8
        new_subroutine = (key - '0' - 1);
        // if the new index is valid ( = there is a subroutine with that index in the shaders vector),
        // we change the value of the current_subroutine variable
        // NB: we can just check if the new index is in the range between 0 and the size of the shaders vector,
        // avoiding to use the std::find function on the vector
        if (new_subroutine < shaders.size())
        {
            current_subroutine = new_subroutine;
            PrintCurrentShader(current_subroutine);
        }
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        detachMouseFromCamera = !detachMouseFromCamera;

        glfwSetInputMode(window, GLFW_CURSOR, detachMouseFromCamera ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;
}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    // we move the camera view following the mouse cursor
    // we calculate the offset of the mouse cursor from the position in the last frame
    // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // offset of mouse cursor position
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    // the new position will be the previous one for the next frame
    lastX = xpos;
    lastY = ypos;

    // we pass the offset to the Camera class instance in order to update the rendering
    if (!detachMouseFromCamera)
    {

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}
