#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <libavutil/mathematics.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Vertex shader source
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragColor = aColor;
}
)";

// Fragment shader source
const char *fragmentShaderSource = R"(
#version 330 core
in vec3 FragColor;
out vec4 color;

void main()
{
    color = vec4(FragColor, 1.0);
}
)";

using namespace std;

// Color palette
struct Color {
  float r, g, b;
  Color(float r, float g, float b) : r(r), g(g), b(b) {}
};

// Predefined colors
namespace Colors {
const Color BLACK(0.0f, 0.0f, 0.0f);
const Color WHITE(1.0f, 1.0f, 1.0f);
const Color RED(1.0f, 0.2f, 0.2f);
const Color GREEN(0.2f, 1.0f, 0.2f);
const Color BLUE(0.2f, 0.4f, 1.0f);
const Color YELLOW(1.0f, 1.0f, 0.2f);
const Color PURPLE(0.8f, 0.2f, 1.0f);
const Color ORANGE(1.0f, 0.6f, 0.2f);
const Color CYAN(0.2f, 1.0f, 1.0f);
const Color MAGENTA(1.0f, 0.2f, 0.8f);
const Color DARK_BLUE(0.05f, 0.05f, 0.2f);
const Color DARK_GREEN(0.05f, 0.2f, 0.05f);
const Color DARK_PURPLE(0.15f, 0.05f, 0.2f);
}  // namespace Colors

class MathAnimation {
 private:
  GLFWwindow *window;
  GLuint shaderProgram;
  GLuint VAO, VBO;

  vector<float> vertices;
  // Animation parameters
  float time;
  int animationMode;
  int backgroundMode;

  // Gravitational field parameters
  float centralMass;
  float maxDeformation;

  // Performance settings
  int targetFPS;
  float frameTime;
  float lastFrameTime;
  int qualityLevel;  // 0=Low, 1=Medium, 2=High, 3=Ultra

  // Window dimensions
  int windowWidth, windowHeight;

  // Camera parameters
  glm::vec3 cameraPos;
  glm::vec3 cameraFront;
  glm::vec3 cameraUp;
  glm::vec3 cameraRight;
  glm::vec3 worldUp;

  // Mouse camera control
  float yaw;
  float pitch;
  float mouseSensitivity;
  float cameraSpeed;
  bool firstMouse;
  double lastX, lastY;

  // Movement keys state
  bool keys[1024];

  // Camera movement timing
  float deltaTime;
  float lastFrame;

  // Background colors
  std::vector<Color> backgrounds;

 public:
  MathAnimation()
      : time(0.0f),
        animationMode(0),
        backgroundMode(0),
        windowWidth(1200),
        windowHeight(900),
        targetFPS(60),
        qualityLevel(2),
        yaw(-90.0f),
        pitch(0.0f),
        mouseSensitivity(0.1f),
        cameraSpeed(2.5f),
        firstMouse(true),
        lastX(600.0),
        lastY(450.0),
        deltaTime(0.0f),
        lastFrame(0.0f) {
    // Initialize camera vectors
    cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
    worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    updateCameraVectors();

    // Initialize keys array
    for (int i = 0; i < 1024; i++) {
      keys[i] = false;
    }

    // Initialize background colors
    backgrounds = {
        Colors::DARK_BLUE,          // Deep blue
        Colors::BLACK,              // Pure black
        Colors::DARK_PURPLE,        // Dark purple
        Colors::DARK_GREEN,         // Dark green
        Color(0.1f, 0.1f, 0.15f),   // Dark gray-blue
        Color(0.05f, 0.1f, 0.1f),   // Dark teal
        Color(0.15f, 0.05f, 0.05f)  // Dark red
    };
  }

  void updateCameraVectors() {
    // Calculate the new front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    // Re-calculate the right and up vector
    cameraRight = glm::normalize(glm::cross(cameraFront, worldUp));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
  }

  bool initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << "\n";
      return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(windowWidth, windowHeight, "Mathematical Functions Animation", NULL, NULL);
    if (!window) {
      std::cerr << "Failed to create GLFW window" << "\n";
      glfwTerminate();
      return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Capture mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set VSync based on target FPS
    if (targetFPS == 60) {
      glfwSwapInterval(1);  // VSync on
    } else {
      glfwSwapInterval(0);  // VSync off for custom FPS
    }

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
      std::cerr << "Failed to initialize GLEW" << "\n";
      return false;
    }

    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    updateBackgroundColor();

    // Create shader program
    if (!createShaderProgram()) {
      return false;
    }

    // Generate buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    return true;
  }

  static void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    MathAnimation *app = static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));
    app->windowWidth = width;
    app->windowHeight = height;
    glViewport(0, 0, width, height);
  }

  static void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
    MathAnimation *app = static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));

    if (app->firstMouse) {
      app->lastX = xpos;
      app->lastY = ypos;
      app->firstMouse = false;
    }

    double xoffset = xpos - app->lastX;
    double yoffset = app->lastY - ypos;  // Reversed since y-coordinates go from bottom to top
    app->lastX = xpos;
    app->lastY = ypos;

    xoffset *= app->mouseSensitivity;
    yoffset *= app->mouseSensitivity;

    app->yaw += xoffset;
    app->pitch += yoffset;

    // Constrain pitch
    if (app->pitch > 89.0f) app->pitch = 89.0f;
    if (app->pitch < -89.0f) app->pitch = -89.0f;

    app->updateCameraVectors();
  }

  static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    MathAnimation *app = static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));

    // Adjust camera speed with scroll wheel
    app->cameraSpeed += yoffset * 0.5f;
    if (app->cameraSpeed < 0.5f) app->cameraSpeed = 0.5f;
    if (app->cameraSpeed > 10.0f) app->cameraSpeed = 10.0f;

    std::cout << "Camera speed: " << app->cameraSpeed << "\n";
  }

  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    MathAnimation *app = static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));

    // Handle key press/release for movement
    if (key >= 0 && key < 1024) {
      if (action == GLFW_PRESS)
        app->keys[key] = true;
      else if (action == GLFW_RELEASE)
        app->keys[key] = false;
    }

    if (action == GLFW_PRESS) {
      switch (key) {
        // Numbers 1-9, 0 (functions 1-10)
        case GLFW_KEY_1:
          app->animationMode = 0;
          break;  // Parametric spiral
        case GLFW_KEY_2:
          app->animationMode = 1;
          break;  // Lissajous curve
        case GLFW_KEY_3:
          app->animationMode = 2;
          break;  // 3D helix
        case GLFW_KEY_4:
          app->animationMode = 3;
          break;  // Sine wave surface
        case GLFW_KEY_5:
          app->animationMode = 4;
          break;  // Torus
        case GLFW_KEY_6:
          app->animationMode = 5;
          break;  // Hypotrochoid
        case GLFW_KEY_7:
          app->animationMode = 6;
          break;  // Superformula
        case GLFW_KEY_8:
          app->animationMode = 7;
          break;  // Lorenz attractor
        case GLFW_KEY_9:
          app->animationMode = 8;
          break;  // Klein Bottle
        case GLFW_KEY_0:
          app->animationMode = 9;
          break;  // Gyroid Surface

        // Letters Q-T (functions 11-15)
        case GLFW_KEY_Q:
          app->animationMode = 10;
          break;            // Spherical Harmonic
        case GLFW_KEY_TAB:  // Use TAB instead of W for fractal zoom to avoid conflict with movement
          app->animationMode = 11;
          break;  // Fractal Zoom
        case GLFW_KEY_E:
          app->animationMode = 12;
          break;  // Phyllotaxis
        case GLFW_KEY_R:
          app->animationMode = 13;
          break;  // Tesseract 4D Projection
        case GLFW_KEY_T:
          app->animationMode = 14;
          break;  // Wave Interference Surface
        case GLFW_KEY_G:
          app->animationMode = 15;
          break;  // Gravitational Spacetime Curvature

        // Mass control for gravitational field
        case GLFW_KEY_EQUAL:   // '+' key (usually requires shift)
        case GLFW_KEY_KP_ADD:  // Numpad +
          app->increaseMass();
          break;
        case GLFW_KEY_MINUS:        // '-' key
        case GLFW_KEY_KP_SUBTRACT:  // Numpad -
          app->decreaseMass();
          break;

        case GLFW_KEY_K:          // Reset and bring to the beginning position
                app->cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);  // Reset camera position
                app->centralMass = 0.5f;
                app->generateGravitationalSpacetime(app->vertices, 0.0f);
                std::cout << "Reset Mass: " << app->centralMass << std::endl;
                break;


        // Performance controls
        case GLFW_KEY_F1:
          app->setTargetFPS(30);
          break;  // 30 FPS
        case GLFW_KEY_F2:
          app->setTargetFPS(60);
          break;  // 60 FPS
        case GLFW_KEY_F3:
          app->setTargetFPS(120);
          break;  // 120 FPS
        case GLFW_KEY_F4:
          app->setTargetFPS(144);
          break;  // 144 FPS

        // Quality controls
        case GLFW_KEY_F5:
          app->setQuality(0);
          break;  // Low quality
        case GLFW_KEY_F6:
          app->setQuality(1);
          break;  // Medium quality
        case GLFW_KEY_F7:
          app->setQuality(2);
          break;  // High quality
        case GLFW_KEY_F8:
          app->setQuality(3);
          break;  // Ultra quality

        // Background color cycling
        case GLFW_KEY_B:  // Cycle background colors
          app->backgroundMode = (app->backgroundMode + 1) % app->backgrounds.size();
          app->updateBackgroundColor();
          break;

        case GLFW_KEY_V:  // Toggle VSync
          app->toggleVSync();
          break;

        // Mouse cursor toggle
        case GLFW_KEY_M:  // Toggle mouse cursor
          app->toggleMouseCursor();
          break;

        // Exit application
        case GLFW_KEY_ESCAPE:
          glfwSetWindowShouldClose(window, true);
          break;
      }
    }
  }

  void increaseMass() {
        centralMass += 0.2f;
        if (centralMass > 5.0f) centralMass = 5.0f;
        std::cout << "Central Mass: " << centralMass << std::endl;
    }
    
    void decreaseMass() {
        centralMass -= 0.2f;
        if (centralMass < 0.1f) centralMass = 0.1f;
        std::cout << "Central Mass: " << centralMass << std::endl;
    }


  void toggleMouseCursor() {
    static bool cursorEnabled = false;
    cursorEnabled = !cursorEnabled;

    if (cursorEnabled) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      std::cout << "Mouse cursor enabled (camera control disabled)" << "\n";
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      firstMouse = true;  // Reset mouse to avoid jump
      std::cout << "Mouse cursor disabled (camera control enabled)" << "\n";
    }
  }

  void processInput() {
    float velocity = cameraSpeed * deltaTime;

    if (keys[GLFW_KEY_W]) cameraPos += cameraFront * velocity;
    if (keys[GLFW_KEY_S]) cameraPos -= cameraFront * velocity;
    if (keys[GLFW_KEY_A]) cameraPos -= cameraRight * velocity;
    if (keys[GLFW_KEY_D]) cameraPos += cameraRight * velocity;
    if (keys[GLFW_KEY_SPACE]) cameraPos += cameraUp * velocity;
    if (keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_C]) cameraPos -= cameraUp * velocity;
  }

  void updateBackgroundColor() {
    const Color &bg = backgrounds[backgroundMode];
    glClearColor(bg.r, bg.g, bg.b, 1.0f);
  }

  void setTargetFPS(int fps) {
    targetFPS = fps;
    frameTime = 1.0f / fps;

    // Update VSync based on FPS
    if (fps == 60) {
      glfwSwapInterval(1);  // VSync on for 60 FPS
    } else {
      glfwSwapInterval(0);  // VSync off for custom FPS
    }

    std::cout << "Target FPS set to: " << fps << "\n";
  }

  void setQuality(int quality) {
    qualityLevel = quality;
    const char *qualityNames[] = {"Low", "Medium", "High", "Ultra"};
    std::cout << "Quality set to: " << qualityNames[quality] << "\n";
  }

  void toggleVSync() {
    static bool vsyncEnabled = true;
    vsyncEnabled = !vsyncEnabled;
    glfwSwapInterval(vsyncEnabled ? 1 : 0);
    std::cout << "VSync " << (vsyncEnabled ? "enabled" : "disabled") << "\n";
  }

  int getQualityMultiplier() {
    switch (qualityLevel) {
      case 0:
        return 1;  // Low: 1x
      case 1:
        return 2;  // Medium: 2x
      case 2:
        return 4;  // High: 4x
      case 3:
        return 8;  // Ultra: 8x
      default:
        return 4;
    }
  }

  // Placeholder for missing math functions
  float sph_legendre(int l, int m, float x) {
    // Simplified spherical harmonic - just use sine/cosine approximation
    return sin(l * acos(x) + m * 0.5f);
  }

  void generateParametricSpiral(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int numPoints = 1000 * getQualityMultiplier();

    for (int i = 0; i < numPoints; ++i) {
      float param = (float)i / numPoints * 10.0f * M_PI;
      float radius = 0.8f + 0.4f * sin(param * 0.1f + t);

      float x = radius * cos(param + t);
      float y = sin(param * 0.3f + t * 0.5f) * 0.5f;
      float z = radius * sin(param + t);

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      float r = 0.6f + 0.4f * sin(param * 0.2f + t);
      float g = 0.6f + 0.4f * cos(param * 0.15f + t * 1.5f);
      float b = 0.6f + 0.4f * sin(param * 0.3f + t * 0.8f);

      vertices.push_back(r);
      vertices.push_back(g);
      vertices.push_back(b);
    }
  }

  void generateLissajous(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int numPoints = 2000 * getQualityMultiplier();

    for (int i = 0; i < numPoints; ++i) {
      float param = (float)i / numPoints * 4.0f * M_PI;

      float x = 1.2f * sin(3.0f * param + t);
      float y = 1.0f * sin(2.0f * param + t * 0.7f);
      float z = 0.8f * sin(5.0f * param + t * 1.3f);

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      float r = 0.7f + 0.3f * sin(param + t);
      float g = 0.7f + 0.3f * sin(param + t + 2.0f * M_PI / 3.0f);
      float b = 0.7f + 0.3f * sin(param + t + 4.0f * M_PI / 3.0f);

      vertices.push_back(r);
      vertices.push_back(g);
      vertices.push_back(b);
    }
  }

  void generate3DHelix(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int numPoints = 1500 * getQualityMultiplier();

    for (int i = 0; i < numPoints; ++i) {
      float param = (float)i / numPoints * 12.0f * M_PI;
      float amplitude = 1.0f + 0.3f * sin(t * 2.0f);

      float x = amplitude * cos(param + t);
      float y = (param / (6.0f * M_PI) - 1.0f) * 1.5f;
      float z = amplitude * sin(param + t);

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      float intensity = (float)i / numPoints;
      vertices.push_back(0.8f * intensity + 0.2f);
      vertices.push_back(0.8f * (1.0f - intensity) + 0.2f);
      vertices.push_back(0.7f + 0.3f * sin(t + param));
    }
  }

  void generateSineWaveSurface(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int gridSize = 80 * sqrt(getQualityMultiplier());
    const float scale = 3.0f;

    for (int i = 0; i < gridSize; ++i) {
      for (int j = 0; j < gridSize; ++j) {
        float x = (float)i / gridSize * scale - scale / 2;
        float z = (float)j / gridSize * scale - scale / 2;

        float distance = sqrt(x * x + z * z);
        float y = 0.6f * sin(distance * 2.5f - t * 3.0f) * exp(-distance * 0.4f);

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        float heightIntensity = (y + 0.6f) * 0.8f + 0.2f;
        float r = 0.3f + 0.7f * heightIntensity;
        float g = 0.2f + 0.6f * sin(distance * 0.5f + t);
        float b = 0.8f + 0.2f * cos(distance * 0.3f + t * 1.2f);

        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
      }
    }
  }

  void generateTorus(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int qualityMult = getQualityMultiplier();
    const int majorSegments = 60 * qualityMult;
    const int minorSegments = 40 * qualityMult;
    const float majorRadius = 1.2f;
    const float minorRadius = 0.4f + 0.2f * sin(t * 2.0f);

    for (int i = 0; i < majorSegments; ++i) {
      for (int j = 0; j < minorSegments; ++j) {
        float u = 2.0f * M_PI * i / majorSegments + t;
        float v = 2.0f * M_PI * j / minorSegments;

        float x = (majorRadius + minorRadius * cos(v)) * cos(u);
        float y = minorRadius * sin(v);
        float z = (majorRadius + minorRadius * cos(v)) * sin(u);

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        float r = 0.6f + 0.4f * cos(u + t);
        float g = 0.6f + 0.4f * sin(v + t * 1.3f);
        float b = 0.6f + 0.4f * sin(u + v + t * 0.7f);

        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
      }
    }
  }

  void generateHypotrochoid(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int numPoints = 2000 * getQualityMultiplier();
    const float R = 1.0f + 0.3f * sin(t * 0.5f);
    const float r = 0.3f + 0.1f * cos(t * 0.7f);
    const float d = 0.5f + 0.2f * sin(t * 1.3f);

    for (int i = 0; i < numPoints; ++i) {
      float θ = (float)i / numPoints * 2.0f * M_PI;
      float diff = R - r;
      float x = diff * cos(θ) + d * cos(diff / r * θ);
      float y = diff * sin(θ) - d * sin(diff / r * θ);
      float z = 0.0f;

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      float hue = fmodf(θ + t, 2.0f * M_PI) / (2.0f * M_PI);
      vertices.push_back(0.5f + 0.5f * sin(2 * M_PI * hue));
      vertices.push_back(0.5f + 0.5f * sin(2 * M_PI * hue + 2.0f));
      vertices.push_back(0.5f + 0.5f * sin(2 * M_PI * hue + 4.0f));
    }
  }

  void generateSuperformula(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int numPoints = 1000 * getQualityMultiplier();
    float m = 6.0f + 4.0f * sin(t * 0.4f);
    float n1 = 0.3f + 1.2f * fabs(sin(t * 0.6f));
    float n2 = 1.0f + 2.0f * fabs(cos(t * 0.5f));
    float n3 = 1.0f + 2.0f * fabs(sin(t * 0.8f));
    const float a = 1, b = 1;

    for (int i = 0; i < numPoints; ++i) {
      float φ = (float)i / numPoints * 2.0f * M_PI;
      float cos_m = cos(m * φ / 4.0f) / a;
      float sin_m = sin(m * φ / 4.0f) / b;
      float r = powf(powf(fabs(cos_m), n2) + powf(fabs(sin_m), n3), -1.0f / n1);
      float x = r * cos(φ);
      float y = r * sin(φ);
      float z = 0.0f;

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      vertices.push_back(0.5f + 0.5f * r);
      vertices.push_back(0.3f + 0.7f * (1 - r));
      vertices.push_back(0.5f + 0.5f * sin(t + φ));
    }
  }

  void generateLorenzAttractor(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int steps = 5000 * getQualityMultiplier();
    float dt = 0.005f;
    float σ = 10.0f + 5.0f * sin(t * 0.3f);
    float ρ = 28.0f + 10.0f * cos(t * 0.5f);
    float β = 8.0f / 3.0f;
    float x = 0.1f, y = 0.0f, z = 0.0f;

    for (int i = 0; i < steps; ++i) {
      float dx = σ * (y - x);
      float dy = x * (ρ - z) - y;
      float dz = x * y - β * z;
      x += dx * dt;
      y += dy * dt;
      z += dz * dt;

      vertices.push_back(x * 0.1f);
      vertices.push_back(y * 0.1f - 0.5f);
      vertices.push_back(z * 0.1f - 0.5f);

      float speed = sqrtf(dx * dx + dy * dy + dz * dz);
      vertices.push_back(fminf(1.0f, speed * 0.05f));
      vertices.push_back(0.2f + 0.8f * fabs(sinf(speed + t)));
      vertices.push_back(1.0f - fminf(1.0f, speed * 0.05f));
    }
  }

  void generateKleinBottle(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int qualityMult = getQualityMultiplier();
    const int uSeg = 100 * qualityMult, vSeg = 50 * qualityMult;
    float r = 1.5f + 0.3f * sin(t);

    for (int iu = 0; iu < uSeg; ++iu) {
      for (int iv = 0; iv < vSeg; ++iv) {
        float u = (float)iu / uSeg * 2.0f * M_PI;
        float v = (float)iv / vSeg * 2.0f * M_PI;
        float x = (r + cos(u / 2) * sin(v) - sin(u / 2) * sin(2 * v)) * cos(u);
        float y = (r + cos(u / 2) * sin(v) - sin(u / 2) * sin(2 * v)) * sin(u);
        float z = sin(u / 2) * sin(v) + cos(u / 2) * sin(2 * v);

        vertices.push_back(x * 0.3f);
        vertices.push_back(y * 0.3f);
        vertices.push_back(z * 0.3f);

        vertices.push_back(0.5f + 0.5f * sin(u + t));
        vertices.push_back(0.5f + 0.5f * cos(v + t * 1.2f));
        vertices.push_back(0.5f + 0.5f * sin(u + v + t * 0.7f));
      }
    }
  }

  void generateGyroid(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int base = 50;
    const int qm = getQualityMultiplier();  // 1,2,4,8
    const int grid = base * qm;             // 50,100,200,400
    // then maybe cap it
    const int maxGrid = 120;
    const int finalGrid = std::min(grid, maxGrid);
    float level = sin(t * 0.6f) * 0.5f;

    for (int i = 0; i < finalGrid; ++i) {
      for (int j = 0; j < finalGrid; ++j) {
        for (int k = 0; k < finalGrid; ++k) {
          float x = (i / (float)finalGrid - 0.5f) * 4.0f;
          float y = (j / (float)finalGrid - 0.5f) * 4.0f;
          float z = (k / (float)finalGrid - 0.5f) * 4.0f;
          float v = sin(x) * cos(y) + sin(y) * cos(z) + sin(z) * cos(x);

          if (fabs(v - level) < 0.05f) {
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            float c = (v - level + 0.05f) / 0.1f;
            vertices.push_back(c);
            vertices.push_back(1.0f - c);
            vertices.push_back(0.5f + 0.5f * sin(t));
          }
        }
      }
    }
  }

  void generateSphericalHarmonic(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int qualityMult = getQualityMultiplier();
    const int latSeg = 40 * qualityMult, lonSeg = 80 * qualityMult;
    int ℓ = 2 + (int)(2.0f * fabs(sin(t * 0.3f)));
    int m = ℓ / 2;
    float eps = 0.2f + 0.3f * fabs(cos(t * 0.4f));

    for (int i = 0; i <= latSeg; ++i) {
      float θ = M_PI * i / latSeg;
      for (int j = 0; j <= lonSeg; ++j) {
        float φ = 2.0f * M_PI * j / lonSeg;
        float Y = sph_legendre(ℓ, m, cos(θ)) * cos(m * φ);
        float R = 1.0f + eps * Y;
        float x = R * sin(θ) * cos(φ);
        float y = R * sin(θ) * sin(φ);
        float z = R * cos(θ);

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        vertices.push_back(0.5f + 0.5f * Y);
        vertices.push_back(0.5f - 0.5f * Y);
        vertices.push_back(0.3f + 0.7f * fabs(sin(t + φ)));
      }
    }
  }

  void generateFractalZoom(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int res = 200 * sqrt(getQualityMultiplier());
    float zoom = 1.5f + 0.5f * sin(t * 0.2f);
    float cx = -0.5f + 0.2f * cos(t * 0.3f);
    float cy = 0.0f + 0.2f * sin(t * 0.4f);

    for (int i = 0; i < res; ++i) {
      for (int j = 0; j < res; ++j) {
        float x0 = (i / (float)res - 0.5f) * zoom + cx;
        float y0 = (j / (float)res - 0.5f) * zoom + cy;
        float x = 0, y = 0;
        int iter = 0, maxI = 100;

        while (x * x + y * y < 4.0f && iter < maxI) {
          float xt = x * x - y * y + x0;
          y = 2 * x * y + y0;
          x = xt;
          ++iter;
        }

        float h = iter / (float)maxI;
        vertices.push_back(i / (float)res - 0.5f);
        vertices.push_back(h * 1.0f - 0.5f);
        vertices.push_back(j / (float)res - 0.5f);

        vertices.push_back(h);
        vertices.push_back(0.5f * h);
        vertices.push_back(1.0f - h);
      }
    }
  }

  void generatePhyllotaxis(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int seeds = 1000 * getQualityMultiplier();
    float angle0 = (1.6180339887f + 0.1f * sin(t * 0.5f)) * M_PI;

    for (int n = 0; n < seeds; ++n) {
      float θ = n * angle0;
      float r = 0.02f * sqrtf(n);
      float x = r * cos(θ);
      float y = r * sin(θ);
      float z = 0.0f;

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      vertices.push_back(0.5f + 0.5f * sin(θ + t));
      vertices.push_back(0.5f + 0.5f * cos(θ + t * 1.2f));
      vertices.push_back(0.5f + 0.5f * sin(t));
    }
  }

  void generateTesseract4D(std::vector<float> &vertices, float t) {
    vertices.clear();
    std::vector<std::array<float, 4>> pts4;
    for (int i = 0; i < 16; i++) {
      std::array<float, 4> v;
      for (int d = 0; d < 4; d++) v[d] = (i & (1 << d)) ? 1.0f : -1.0f;
      pts4.push_back(v);
    }

    float c = cos(t * 0.3f), s = sin(t * 0.3f);
    for (auto &v : pts4) {
      float x = v[0], w = v[3];
      v[0] = c * x - s * w;
      v[3] = s * x + c * w;
    }

    float dist = 3.0f + sin(t * 0.5f);
    for (auto &v : pts4) {
      float w = 1.0f / (dist - v[3]);
      float x = v[0] * w, y = v[1] * w, z = v[2] * w;

      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      vertices.push_back(0.5f + 0.5f * (v[3]));
      vertices.push_back(1.0f - 0.5f * (v[3]));
      vertices.push_back(0.5f + 0.5f * sin(t));
    }
  }

  void generateWaveInterference(std::vector<float> &vertices, float t) {
    vertices.clear();
    const int grid = 100 * sqrt(getQualityMultiplier());
    const float size = 4.0f;
    float k1 = 2.0f + sin(t * 0.3f), k2 = 3.0f + cos(t * 0.4f);
    float ω1 = 1.5f + cos(t * 0.5f), ω2 = 1.0f + sin(t * 0.6f);

    for (int i = 0; i < grid; i++) {
      for (int j = 0; j < grid; j++) {
        float x = (i / (float)grid - 0.5f) * size;
        float z = (j / (float)grid - 0.5f) * size;
        float y = 0.5f * (sin(k1 * x - ω1 * t) + sin(k2 * z - ω2 * t));

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        float h = (y + 1.0f) * 0.5f;
        vertices.push_back(h);
        vertices.push_back(1.0f - h);
        vertices.push_back(0.5f + 0.5f * sin(t));
      }
    }
  }

  void generateGravitationalSpacetime(std::vector<float> &vertices, float t) {
    GLuint VAO, VBO;
        vertices.clear();
        
        const int gridSize = 80;
        const float extent = 4.0f;
        const int gridLines = 15;
        
        // Surface mesh with gravitational deformation
        for (int i = 0; i < gridSize; ++i) {
            float x = (float(i) / (gridSize - 1)) * 2.0f * extent - extent;
            for (int j = 0; j < gridSize; ++j) {
                float z = (float(j) / (gridSize - 1)) * 2.0f * extent - extent;
                
                float r = std::sqrt(x*x + z*z);
                float y = 0.0f;
                
                if (r < extent) {
                    // Realistic gravitational well formula
                    float t = r / extent;
                    float depth = centralMass * maxDeformation;
                    
                    // Combination of steep center and smooth edges
                    float steepness = 4.0f * centralMass;
                    float well = 1.0f / (1.0f + steepness * t * t);
                    float parabolic = (1.0f - t * t);
                    float blend = std::exp(-3.0f * t);
                    
                    y = -depth * (blend * well + (1.0f - blend) * parabolic);
                }
                
                // Add vertex with grey color
                vertices.insert(vertices.end(), {x, y, z, 0.6f, 0.6f, 0.6f});
            }
        }
        
        // Grid lines overlaid on surface
        for (int line = 0; line < gridLines; ++line) {
            float coord = (float(line) / (gridLines - 1)) * 2.0f * extent - extent;
            
            // Vertical grid lines
            for (int j = 0; j < gridSize; j += 3) {
                float z = (float(j) / (gridSize - 1)) * 2.0f * extent - extent;
                float r = std::sqrt(coord*coord + z*z);
                float y = 0.01f; // Slightly above surface
                
                if (r < extent) {
                    float t = r / extent;
                    float depth = centralMass * maxDeformation;
                    float steepness = 4.0f * centralMass;
                    float well = 1.0f / (1.0f + steepness * t * t);
                    float parabolic = (1.0f - t * t);
                    float blend = std::exp(-3.0f * t);
                    
                    y = -depth * (blend * well + (1.0f - blend) * parabolic) + 0.01f;
                }
                
                vertices.insert(vertices.end(), {coord, y, z, 1.0f, 1.0f, 1.0f});
            }
            
            // Horizontal grid lines  
            for (int i = 0; i < gridSize; i += 3) {
                float x = (float(i) / (gridSize - 1)) * 2.0f * extent - extent;
                float r = std::sqrt(x*x + coord*coord);
                float y = 0.01f;
                
                if (r < extent) {
                    float t = r / extent;
                    float depth = centralMass * maxDeformation;
                    float steepness = 4.0f * centralMass;
                    float well = 1.0f / (1.0f + steepness * t * t);
                    float parabolic = (1.0f - t * t);
                    float blend = std::exp(-3.0f * t);
                    
                    y = -depth * (blend * well + (1.0f - blend) * parabolic) + 0.01f;
                }
                
                vertices.insert(vertices.end(), {x, y, coord, 1.0f, 1.0f, 1.0f});
            }
        }        
    }



  //   void generateGravitationalSpacetime(std::vector<float> &vertices, float t) {
  //     vertices.clear();
  //     const int gridSize = 100 * sqrt(getQualityMultiplier());
  //     const float extent = 4.0f;

  //     // Central mass position (can be animated)
  //     float massX = 0.5f * sin(t * 0.3f);
  //     float massZ = 0.5f * cos(t * 0.3f);

  //     // Generate spacetime grid surface
  //     for (int i = 0; i < gridSize; ++i) {
  //       for (int j = 0; j < gridSize; ++j) {
  //         float x = (float)i / (gridSize - 1) * extent - extent / 2.0f;
  //         float z = (float)j / (gridSize - 1) * extent - extent / 2.0f;

  //         // Distance from the central mass
  //         float dx = x - massX;
  //         float dz = z - massZ;
  //         float r = sqrt(dx * dx + dz * dz);

  //         // Gravitational potential with adjustable mass
  //         // φ = -GM/r, but we'll use a modified version to prevent singularity
  //         float minRadius = 0.1f;  // Prevent division by zero
  //         float effectiveRadius = std::max(r, minRadius);

  //         // Spacetime curvature: y displacement proportional to gravitational potential
  //         float gravitationalPotential = -centralMass / effectiveRadius;

  //         // Apply curvature with smooth falloff
  //         float curvature = gravitationalPotential * maxDeformation;

  //         // Add some time-based oscillation to show dynamic effects
  //         float timeEffect = 0.1f * sin(t * 2.0f + r * 3.0f) * centralMass;

  //         float y = curvature + timeEffect;

  //         vertices.push_back(x);
  //         vertices.push_back(y);
  //         vertices.push_back(z);

  //         // Color based on curvature depth and mass
  //         float depth = -y / maxDeformation;  // Normalize depth
  //         float redIntensity = std::min(1.0f, std::max(0.0f, depth * 0.8f + 0.2f));
  //         float greenIntensity = std::min(1.0f, std::max(0.1f, 1.0f - depth * 0.5f));
  //         float blueIntensity = std::min(1.0f, std::max(0.1f, 0.3f + depth * 0.4f));

  //         // Add mass-dependent coloring
  //         float massInfluence = centralMass / 5.0f;  // Normalize to 0-1
  //         redIntensity = std::min(1.0f, redIntensity + massInfluence * 0.3f);

  //         vertices.push_back(redIntensity);
  //         vertices.push_back(greenIntensity);
  //         vertices.push_back(blueIntensity);
  //       }
  //     }

  //     // Add the central mass as a bright sphere
  //     const int spherePoints = 50;
  //     for (int i = 0; i < spherePoints; ++i) {
  //       float theta = 2.0f * M_PI * i / spherePoints;
  //       float radius = 0.05f + 0.02f * centralMass;  // Size based on mass

  //       float x = massX + radius * cos(theta);
  //       float z = massZ + radius * sin(theta);
  //       float y = -0.1f;  // Slightly above the surface

  //       vertices.push_back(x);
  //       vertices.push_back(y);
  //       vertices.push_back(z);

  //       // Bright white/yellow for the mass
  //       vertices.push_back(1.0f);
  //       vertices.push_back(1.0f - 0.2f * sin(t * 3.0f));
  //       vertices.push_back(0.3f);
  //     }

  //     // Add grid lines to better show the curvature
  //     const int gridLines = 20;
  //     for (int line = 0; line < gridLines; ++line) {
  //       // Vertical lines
  //       float x = (float)line / (gridLines - 1) * extent - extent / 2.0f;
  //       for (int j = 0; j < gridSize; j += 2) {
  //         float z = (float)j / (gridSize - 1) * extent - extent / 2.0f;

  //         float dx = x - massX;
  //         float dz = z - massZ;
  //         float r = sqrt(dx * dx + dz * dz);
  //         float effectiveRadius = std::max(r, 0.1f);
  //         float y = -centralMass / effectiveRadius * maxDeformation;

  //         vertices.push_back(x);
  //         vertices.push_back(y + 0.01f);  // Slightly above surface
  //         vertices.push_back(z);

  //         vertices.push_back(0.8f);
  //         vertices.push_back(0.8f);
  //         vertices.push_back(0.9f);
  //       }

  //       // Horizontal lines
  //       float z = (float)line / (gridLines - 1) * extent - extent / 2.0f;
  //       for (int i = 0; i < gridSize; i += 2) {
  //         float x = (float)i / (gridSize - 1) * extent - extent / 2.0f;

  //         float dx = x - massX;
  //         float dz = z - massZ;
  //         float r = sqrt(dx * dx + dz * dz);
  //         float effectiveRadius = std::max(r, 0.1f);
  //         float y = -centralMass / effectiveRadius * maxDeformation;

  //         vertices.push_back(x);
  //         vertices.push_back(y + 0.01f);  // Slightly above surface
  //         vertices.push_back(z);

  //         vertices.push_back(0.8f);
  //         vertices.push_back(0.8f);
  //         vertices.push_back(0.9f);
  //       }
  //     }
  //   }

  void render() {
    // Calculate delta time for smooth movement
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Process input for camera movement
    processInput();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static float time = glfwGetTime();
    time = glfwGetTime();

    // Generate vertices based on current animation mode

    switch (animationMode) {
      case 0:
        generateParametricSpiral(vertices, time);
        break;
      case 1:
        generateLissajous(vertices, time);
        break;
      case 2:
        generate3DHelix(vertices, time);
        break;
      case 3:
        generateSineWaveSurface(vertices, time);
        break;
      case 4:
        generateTorus(vertices, time);
        break;
      case 5:
        generateHypotrochoid(vertices, time);
        break;
      case 6:
        generateSuperformula(vertices, time);
        break;
      case 7:
        generateLorenzAttractor(vertices, time);
        break;
      case 8:
        generateKleinBottle(vertices, time);
        break;
      case 9:
        generateGyroid(vertices, time);
        break;
      case 10:
        generateSphericalHarmonic(vertices, time);
        break;
      case 11:
        generateFractalZoom(vertices, time);
        break;
      case 12:
        generatePhyllotaxis(vertices, time);
        break;
      case 13:
        generateTesseract4D(vertices, time);
        break;
      case 14:
        generateWaveInterference(vertices, time);
        break;
      case 15:
        generateGravitationalSpacetime(vertices, time);
        break;
    }

    // Update vertex buffer
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Use shader program
    glUseProgram(shaderProgram);

    // Set up matrices
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, time * 0.3f, glm::vec3(0.1f, 1.0f, 0.0f));

    // Use the camera's view matrix
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    // Send matrices to shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Draw
    if (animationMode == 3 || animationMode == 9 || animationMode == 11 || animationMode == 14) {
      glPointSize(2.0f);
      glDrawArrays(GL_POINTS, 0, vertices.size() / 6);
    } else {
      glLineWidth(2.0f);
      glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 6);
    }

    glfwSwapBuffers(window);
  }

  void run() {
    cout << "Mathematical Functions Animation with Mouse Camera Control\n";
    cout << "=========================================================\n";
    cout << "Mathematical Functions:\n";
    cout << "1 - Parametric Spiral\n";
    cout << "2 - Lissajous Curve\n";
    cout << "3 - 3D Helix\n";
    cout << "4 - Sine Wave Surface\n";
    cout << "5 - Animated Torus\n";
    cout << "6 - Hypotrochoid\n";
    cout << "7 - Superformula\n";
    cout << "8 - Lorenz Attractor\n";
    cout << "9 - Klein Bottle\n";
    cout << "0 - Gyroid Surface\n";
    cout << "Q - Spherical Harmonic\n";
    cout << "TAB - Fractal Zoom\n";
    cout << "E - Phyllotaxis\n";
    cout << "R - Tesseract 4D Projection\n";
    cout << "T - Wave Interference Surface\n";
    cout << "G - Gravitational Spacetime Curvature\n";
    cout << "\nGravitational Controls:\n";
    cout << "+ - Increase central mass\n";
    cout << "- - Decrease central mass\n";
    cout << "Current mass: " << centralMass << "\n";
    cout << "\nCamera Controls:\n";
    cout << "Mouse - Look around\n";
    cout << "W/A/S/D - Move forward/left/backward/right\n";
    cout << "SPACE - Move up\n";
    cout << "SHIFT/C - Move down\n";
    cout << "Mouse Wheel - Adjust camera speed\n";
    cout << "M - Toggle mouse cursor (enable/disable camera)\n";
    cout << "\nPerformance:\n";
    cout << "F1-F4 - Set FPS (30/60/120/144)\n";
    cout << "F5-F8 - Set Quality (Low/Medium/High/Ultra)\n";
    cout << "\nOther:\n";
    cout << "B - Change Background Color\n";
    cout << "V - Toggle VSync\n";
    cout << "K - Reset Camera Position\n";
    cout << "ESC - Exit\n\n";
    cout << "Camera speed: " << cameraSpeed << " (use mouse wheel to adjust)\n";

    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      render();
    }
  }

  void cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
  }

 private:
  bool createShaderProgram() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check compilation
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      std::cerr << "Vertex shader compilation failed: " << infoLog << "\n";
      return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      std::cerr << "Fragment shader compilation failed: " << infoLog << "\n";
      return false;
    }

    // Create shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      std::cerr << "Shader program linking failed: " << infoLog << "\n";
      return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
  }
};

int main() {
  MathAnimation app;

  if (!app.initialize()) {
    std::cerr << "Failed to initialize application" << "\n";
    return -1;
  }

  app.run();
  app.cleanup();

  return 0;
}
