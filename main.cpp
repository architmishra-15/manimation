#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv4/opencv2/opencv.hpp>

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
namespace Colors
{
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
} // namespace Colors

// Abstract Base Class for Animations
class Animation
{
  public:
    virtual ~Animation() = default;
    virtual void generateVertices(std::vector<float> &vertices, float t,
                                  int qualityMult) = 0;
    virtual std::string getName() const = 0;
    virtual bool usePoints() const { return false; }
    virtual void handleInput(int key) {}
    virtual std::string getControls() const { return ""; }
    virtual float getPeriod() const { return 2.0f * M_PI; }
};

// --- Concrete Animation Classes ---

class ParametricSpiral : public Animation
{
  public:
    std::string getName() const override { return "Parametric Spiral"; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int numPoints = 1000 * qualityMult;
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
};

class Lissajous : public Animation
{
  public:
    std::string getName() const override { return "Lissajous Curve"; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int numPoints = 2000 * qualityMult;
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
};

class Helix3D : public Animation
{
  public:
    std::string getName() const override { return "3D Helix"; }
    float getPeriod() const override { return M_PI; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int numPoints = 1500 * qualityMult;
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
};

class SineWaveSurface : public Animation
{
  public:
    std::string getName() const override { return "Sine Wave Surface"; }
    bool usePoints() const override { return true; }
    float getPeriod() const override { return 2.0f * M_PI / 3.0f; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int gridSize = 80 * sqrt(qualityMult);
        const float scale = 3.0f;
        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                float x = (float)i / gridSize * scale - scale / 2;
                float z = (float)j / gridSize * scale - scale / 2;
                float distance = sqrt(x * x + z * z);
                float y =
                    0.6f * sin(distance * 2.5f - t * 3.0f) * exp(-distance * 0.4f);
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
};

class Torus : public Animation
{
  public:
    std::string getName() const override { return "Animated Torus"; }
    float getPeriod() const override { return M_PI; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
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
};

class Hypotrochoid : public Animation
{
  public:
    std::string getName() const override { return "Hypotrochoid"; }
    float getPeriod() const override { return 4.0f * M_PI; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int numPoints = 2000 * qualityMult;
        const float R = 1.0f + 0.3f * sin(t * 0.5f);
        const float r = 0.3f + 0.1f * cos(t * 0.7f);
        const float d = 0.5f + 0.2f * sin(t * 1.3f);
        for (int i = 0; i < numPoints; ++i) {
            float theta = (float)i / numPoints * 2.0f * M_PI;
            float diff = R - r;
            float x = diff * cos(theta) + d * cos(diff / r * theta);
            float y = diff * sin(theta) - d * sin(diff / r * theta);
            float z = 0.0f;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            float hue = fmodf(theta + t, 2.0f * M_PI) / (2.0f * M_PI);
            vertices.push_back(0.5f + 0.5f * sin(2 * M_PI * hue));
            vertices.push_back(0.5f + 0.5f * sin(2 * M_PI * hue + 2.0f));
            vertices.push_back(0.5f + 0.5f * sin(2 * M_PI * hue + 4.0f));
        }
    }
};

class Superformula : public Animation
{
  public:
    std::string getName() const override { return "Superformula"; }
    float getPeriod() const override { return 5.0f * M_PI; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int numPoints = 1000 * qualityMult;
        float m = 6.0f + 4.0f * sin(t * 0.4f);
        float n1 = 0.3f + 1.2f * fabs(sin(t * 0.6f));
        float n2 = 1.0f + 2.0f * fabs(cos(t * 0.5f));
        float n3 = 1.0f + 2.0f * fabs(sin(t * 0.8f));
        const float a = 1, b = 1;
        for (int i = 0; i < numPoints; ++i) {
            float phi = (float)i / numPoints * 2.0f * M_PI;
            float cos_m = cos(m * phi / 4.0f) / a;
            float sin_m = sin(m * phi / 4.0f) / b;
            float r = powf(powf(fabs(cos_m), n2) + powf(fabs(sin_m), n3), -1.0f / n1);
            float x = r * cos(phi);
            float y = r * sin(phi);
            float z = 0.0f;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(0.5f + 0.5f * r);
            vertices.push_back(0.3f + 0.7f * (1 - r));
            vertices.push_back(0.5f + 0.5f * sin(t + phi));
        }
    }
};

class LorenzAttractor : public Animation
{
  public:
    std::string getName() const override { return "Lorenz Attractor"; }
    float getPeriod() const override { return 20.0f * M_PI / 3.0f; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int steps = 5000 * qualityMult;
        float dt = 0.005f;
        float sigma = 10.0f + 5.0f * sin(t * 0.3f);
        float rho = 28.0f + 10.0f * cos(t * 0.5f);
        float beta = 8.0f / 3.0f;
        float x = 0.1f, y = 0.0f, z = 0.0f;
        for (int i = 0; i < steps; ++i) {
            float dx = sigma * (y - x);
            float dy = x * (rho - z) - y;
            float dz = x * y - beta * z;
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
};

class KleinBottle : public Animation
{
  public:
    std::string getName() const override { return "Klein Bottle"; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
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
};

class Gyroid : public Animation
{
  public:
    std::string getName() const override { return "Gyroid Surface"; }
    bool usePoints() const override { return true; }
    float getPeriod() const override { return 10.0f * M_PI / 3.0f; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int base = 50;
        const int grid = base * qualityMult;
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
};

class SphericalHarmonic : public Animation
{
    float sph_legendre(int l, int m, float x)
    {
        return sin(l * acos(x) + m * 0.5f);
    }

  public:
    std::string getName() const override { return "Spherical Harmonic"; }
    float getPeriod() const override { return 20.0f * M_PI / 3.0f; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int latSeg = 40 * qualityMult, lonSeg = 80 * qualityMult;
        int l = 2 + (int)(2.0f * fabs(sin(t * 0.3f)));
        int m = l / 2;
        float eps = 0.2f + 0.3f * fabs(cos(t * 0.4f));
        for (int i = 0; i <= latSeg; ++i) {
            float theta = M_PI * i / latSeg;
            for (int j = 0; j <= lonSeg; ++j) {
                float phi = 2.0f * M_PI * j / lonSeg;
                float Y = sph_legendre(l, m, cos(theta)) * cos(m * phi);
                float R = 1.0f + eps * Y;
                float x = R * sin(theta) * cos(phi);
                float y = R * sin(theta) * sin(phi);
                float z = R * cos(theta);
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                vertices.push_back(0.5f + 0.5f * Y);
                vertices.push_back(0.5f - 0.5f * Y);
                vertices.push_back(0.3f + 0.7f * fabs(sin(t + phi)));
            }
        }
    }
};

class FractalZoom : public Animation
{
  public:
    std::string getName() const override { return "Fractal Zoom"; }
    bool usePoints() const override { return true; }
    float getPeriod() const override { return 10.0f * M_PI; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int res = 200 * sqrt(qualityMult);
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
};

class Phyllotaxis : public Animation
{
  public:
    std::string getName() const override { return "Phyllotaxis"; }
    float getPeriod() const override { return 4.0f * M_PI; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int seeds = 1000 * qualityMult;
        float angle0 = (1.6180339887f + 0.1f * sin(t * 0.5f)) * M_PI;
        for (int n = 0; n < seeds; ++n) {
            float theta = n * angle0;
            float r = 0.02f * sqrtf(n);
            float x = r * cos(theta);
            float y = r * sin(theta);
            float z = 0.0f;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(0.5f + 0.5f * sin(theta + t));
            vertices.push_back(0.5f + 0.5f * cos(theta + t * 1.2f));
            vertices.push_back(0.5f + 0.5f * sin(t));
        }
    }
};

class Tesseract : public Animation
{
  public:
    std::string getName() const override { return "Tesseract 4D Projection"; }
    float getPeriod() const override { return 20.0f * M_PI / 3.0f; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        std::vector<std::array<float, 4>> pts4;
        for (int i = 0; i < 16; i++) {
            std::array<float, 4> v;
            for (int d = 0; d < 4; d++)
                v[d] = (i & (1 << d)) ? 1.0f : -1.0f;
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
};

class WaveInterference : public Animation
{
  public:
    std::string getName() const override { return "Wave Interference Surface"; }
    bool usePoints() const override { return true; }
    float getPeriod() const override { return 20.0f * M_PI / 3.0f; }
    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int grid = 100 * sqrt(qualityMult);
        const float size = 4.0f;
        float k1 = 2.0f + sin(t * 0.3f), k2 = 3.0f + cos(t * 0.4f);
        float omega1 = 1.5f + cos(t * 0.5f), omega2 = 1.0f + sin(t * 0.6f);
        for (int i = 0; i < grid; i++) {
            for (int j = 0; j < grid; j++) {
                float x = (i / (float)grid - 0.5f) * size;
                float z = (j / (float)grid - 0.5f) * size;
                float y = 0.5f * (sin(k1 * x - omega1 * t) + sin(k2 * z - omega2 * t));
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
};

class GravitationalSpacetime : public Animation
{
    float centralMass = 0.5f;
    float maxDeformation = 2.0f;

  public:
    std::string getName() const override
    {
        return "Gravitational Spacetime Curvature";
    }
    std::string getControls() const override
    {
        return "Controls: +/- to change mass, K to reset";
    }
    float getPeriod() const override { return M_PI; }

    void handleInput(int key) override
    {
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
            centralMass += 0.2f;
            if (centralMass > 5.0f)
                centralMass = 5.0f;
            std::cout << "Central Mass: " << centralMass << std::endl;
        } else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) {
            centralMass -= 0.2f;
            if (centralMass < 0.1f)
                centralMass = 0.1f;
            std::cout << "Central Mass: " << centralMass << std::endl;
        } else if (key == GLFW_KEY_K) {
            centralMass = 0.5f;
            std::cout << "Reset Mass: " << centralMass << std::endl;
        }
    }

    void generateVertices(std::vector<float> &vertices, float t,
                          int qualityMult) override
    {
        vertices.clear();
        const int gridSize = 80;
        const float extent = 4.0f;
        const int gridLines = 15;

        // Surface mesh
        for (int i = 0; i < gridSize; ++i) {
            float x = (float(i) / (gridSize - 1)) * 2.0f * extent - extent;
            for (int j = 0; j < gridSize; ++j) {
                float z = (float(j) / (gridSize - 1)) * 2.0f * extent - extent;
                float r = std::sqrt(x * x + z * z);
                float y = 0.0f;
                if (r < extent) {
                    float t_val = r / extent;
                    float depth = centralMass * maxDeformation;
                    float steepness = 4.0f * centralMass;
                    float well = 1.0f / (1.0f + steepness * t_val * t_val);
                    float parabolic = (1.0f - t_val * t_val);
                    float blend = std::exp(-3.0f * t_val);
                    y = -depth * (blend * well + (1.0f - blend) * parabolic);
                }
                vertices.insert(vertices.end(), {x, y, z, 0.6f, 0.6f, 0.6f});
            }
        }

        // Grid lines
        for (int line = 0; line < gridLines; ++line) {
            float coord = (float(line) / (gridLines - 1)) * 2.0f * extent - extent;
            // Vertical
            for (int j = 0; j < gridSize; j += 3) {
                float z = (float(j) / (gridSize - 1)) * 2.0f * extent - extent;
                float r = std::sqrt(coord * coord + z * z);
                float y = 0.01f;
                if (r < extent) {
                    float t_val = r / extent;
                    float depth = centralMass * maxDeformation;
                    float steepness = 4.0f * centralMass;
                    float well = 1.0f / (1.0f + steepness * t_val * t_val);
                    float parabolic = (1.0f - t_val * t_val);
                    float blend = std::exp(-3.0f * t_val);
                    y = -depth * (blend * well + (1.0f - blend) * parabolic) + 0.01f;
                }
                vertices.insert(vertices.end(), {coord, y, z, 1.0f, 1.0f, 1.0f});
            }
            // Horizontal
            for (int i = 0; i < gridSize; i += 3) {
                float x = (float(i) / (gridSize - 1)) * 2.0f * extent - extent;
                float r = std::sqrt(x * x + coord * coord);
                float y = 0.01f;
                if (r < extent) {
                    float t_val = r / extent;
                    float depth = centralMass * maxDeformation;
                    float steepness = 4.0f * centralMass;
                    float well = 1.0f / (1.0f + steepness * t_val * t_val);
                    float parabolic = (1.0f - t_val * t_val);
                    float blend = std::exp(-3.0f * t_val);
                    y = -depth * (blend * well + (1.0f - blend) * parabolic) + 0.01f;
                }
                vertices.insert(vertices.end(), {x, y, coord, 1.0f, 1.0f, 1.0f});
            }
        }
    }
};

// --- Main Application Class ---

class MathAnimation
{
  private:
    GLFWwindow *window;
    GLuint shaderProgram;
    GLuint VAO, VBO;

    std::vector<float> vertices;
    float time;
    int backgroundMode;

    // Animations
    std::vector<std::unique_ptr<Animation>> animations;
    int currentAnimationIndex;

    // Performance settings
    int targetFPS;
    float frameTime;
    int qualityLevel; // 0=Low, 1=Medium, 2=High, 3=Ultra

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

    // Timing
    float deltaTime;
    float lastFrame;

    // Background colors
    std::vector<Color> backgrounds;

    // Video Export
    bool isRecording;
    bool isManualRecording;
    cv::VideoWriter videoWriter;
    int recordingFrameCount;

    // FBO for 4K Recording
    GLuint fbo, textureColorbuffer, rbo;
    const int recordWidth = 3840;
    const int recordHeight = 2160;

  public:
    MathAnimation()
        : time(0.0f), backgroundMode(0), currentAnimationIndex(0),
          windowWidth(1280), // Default to 16:9
          windowHeight(720), targetFPS(60), qualityLevel(2), yaw(-90.0f),
          pitch(0.0f), mouseSensitivity(0.1f), cameraSpeed(2.5f),
          firstMouse(true), lastX(640.0), lastY(360.0), deltaTime(0.0f),
          lastFrame(0.0f), isRecording(false), isManualRecording(false),
          recordingFrameCount(0), fbo(0), textureColorbuffer(0), rbo(0)
    {

        // Initialize animations
        animations.push_back(std::make_unique<ParametricSpiral>());
        animations.push_back(std::make_unique<Lissajous>());
        animations.push_back(std::make_unique<Helix3D>());
        animations.push_back(std::make_unique<SineWaveSurface>());
        animations.push_back(std::make_unique<Torus>());
        animations.push_back(std::make_unique<Hypotrochoid>());
        animations.push_back(std::make_unique<Superformula>());
        animations.push_back(std::make_unique<LorenzAttractor>());
        animations.push_back(std::make_unique<KleinBottle>());
        animations.push_back(std::make_unique<Gyroid>());
        animations.push_back(std::make_unique<SphericalHarmonic>());
        animations.push_back(std::make_unique<FractalZoom>());
        animations.push_back(std::make_unique<Phyllotaxis>());
        animations.push_back(std::make_unique<Tesseract>());
        animations.push_back(std::make_unique<WaveInterference>());
        animations.push_back(std::make_unique<GravitationalSpacetime>());

        // Initialize camera vectors
        cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
        worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        updateCameraVectors();

        for (int i = 0; i < 1024; i++)
            keys[i] = false;

        backgrounds = {Colors::DARK_BLUE, Colors::BLACK,
                       Colors::DARK_PURPLE, Colors::DARK_GREEN,
                       Color(0.1f, 0.1f, 0.15f), Color(0.05f, 0.1f, 0.1f),
                       Color(0.15f, 0.05f, 0.05f)};
    }

    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
        cameraRight = glm::normalize(glm::cross(cameraFront, worldUp));
        cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
    }

    bool initialize()
    {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << "\n";
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(windowWidth, windowHeight,
                                  "Mathematical Functions Animation", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << "\n";
            glfwTerminate();
            return false;
        }

        glfwSetWindowAspectRatio(window, 16, 9); // Enforce 16:9 aspect ratio

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (targetFPS == 60)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);

        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << "\n";
            return false;
        }

        glEnable(GL_DEPTH_TEST);
        updateBackgroundColor();

        if (!createShaderProgram())
            return false;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        if (!initFBO())
            return false;

        return true;
    }

    bool initFBO()
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Create color attachment texture
        glGenTextures(1, &textureColorbuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, recordWidth, recordHeight, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               textureColorbuffer, 0);

        // Create renderbuffer object for depth and stencil attachment
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, recordWidth,
                              recordHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
                      << std::endl;
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    static void framebufferSizeCallback(GLFWwindow *window, int width,
                                        int height)
    {
        MathAnimation *app =
            static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));
        app->windowWidth = width;
        app->windowHeight = height;
        glViewport(0, 0, width, height);
    }

    static void mouseCallback(GLFWwindow *window, double xpos, double ypos)
    {
        MathAnimation *app =
            static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));
        if (app->firstMouse) {
            app->lastX = xpos;
            app->lastY = ypos;
            app->firstMouse = false;
        }
        double xoffset = xpos - app->lastX;
        double yoffset = app->lastY - ypos;
        app->lastX = xpos;
        app->lastY = ypos;
        xoffset *= app->mouseSensitivity;
        yoffset *= app->mouseSensitivity;
        app->yaw += xoffset;
        app->pitch += yoffset;
        if (app->pitch > 89.0f)
            app->pitch = 89.0f;
        if (app->pitch < -89.0f)
            app->pitch = -89.0f;
        app->updateCameraVectors();
    }

    static void scrollCallback(GLFWwindow *window, double xoffset,
                               double yoffset)
    {
        MathAnimation *app =
            static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));
        app->cameraSpeed += yoffset * 0.5f;
        if (app->cameraSpeed < 0.5f)
            app->cameraSpeed = 0.5f;
        if (app->cameraSpeed > 10.0f)
            app->cameraSpeed = 10.0f;
        std::cout << "Camera speed: " << app->cameraSpeed << "\n";
    }

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                            int mods)
    {
        MathAnimation *app =
            static_cast<MathAnimation *>(glfwGetWindowUserPointer(window));
        if (key >= 0 && key < 1024) {
            if (action == GLFW_PRESS)
                app->keys[key] = true;
            else if (action == GLFW_RELEASE)
                app->keys[key] = false;
        }

        if (action == GLFW_PRESS) {
            // Pass key to current animation first
            app->animations[app->currentAnimationIndex]->handleInput(key);

            int prevIndex = app->currentAnimationIndex;

            switch (key) {
                case GLFW_KEY_1:
                    app->currentAnimationIndex = 0;
                    break;
                case GLFW_KEY_2:
                    app->currentAnimationIndex = 1;
                    break;
                case GLFW_KEY_3:
                    app->currentAnimationIndex = 2;
                    break;
                case GLFW_KEY_4:
                    app->currentAnimationIndex = 3;
                    break;
                case GLFW_KEY_5:
                    app->currentAnimationIndex = 4;
                    break;
                case GLFW_KEY_6:
                    app->currentAnimationIndex = 5;
                    break;
                case GLFW_KEY_7:
                    app->currentAnimationIndex = 6;
                    break;
                case GLFW_KEY_8:
                    app->currentAnimationIndex = 7;
                    break;
                case GLFW_KEY_9:
                    app->currentAnimationIndex = 8;
                    break;
                case GLFW_KEY_0:
                    app->currentAnimationIndex = 9;
                    break;
                case GLFW_KEY_Q:
                    app->currentAnimationIndex = 10;
                    break;
                case GLFW_KEY_TAB:
                    app->currentAnimationIndex = 11;
                    break;
                case GLFW_KEY_E:
                    app->currentAnimationIndex = 12;
                    break;
                case GLFW_KEY_R:
                    app->currentAnimationIndex = 13;
                    break;
                case GLFW_KEY_T:
                    app->currentAnimationIndex = 14;
                    break;
                case GLFW_KEY_G:
                    app->currentAnimationIndex = 15;
                    break;

                case GLFW_KEY_F1:
                    app->setTargetFPS(30);
                    break;
                case GLFW_KEY_F2:
                    app->setTargetFPS(60);
                    break;
                case GLFW_KEY_F3:
                    app->setTargetFPS(120);
                    break;
                case GLFW_KEY_F4:
                    app->setTargetFPS(144);
                    break;

                case GLFW_KEY_F5:
                    app->setQuality(0);
                    break;
                case GLFW_KEY_F6:
                    app->setQuality(1);
                    break;
                case GLFW_KEY_F7:
                    app->setQuality(2);
                    break;
                case GLFW_KEY_F8:
                    app->setQuality(3);
                    break;
                case GLFW_KEY_F9:
                    app->toggleRecording(false); // Auto-stop
                    break;
                case GLFW_KEY_F10:
                    app->toggleRecording(true); // Manual
                    break;
                case GLFW_KEY_B:
                    app->backgroundMode =
                        (app->backgroundMode + 1) % app->backgrounds.size();
                    app->updateBackgroundColor();
                    break;
                case GLFW_KEY_V:
                    app->toggleVSync();
                    break;
                case GLFW_KEY_M:
                    app->toggleMouseCursor();
                    break;
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
            }

            if (app->currentAnimationIndex != prevIndex) {
                std::cout << "Switched to: "
                          << app->animations[app->currentAnimationIndex]->getName()
                          << std::endl;
                std::string controls =
                    app->animations[app->currentAnimationIndex]->getControls();
                if (!controls.empty())
                    std::cout << controls << std::endl;
            }
        }
    }

    void toggleMouseCursor()
    {
        static bool cursorEnabled = false;
        cursorEnabled = !cursorEnabled;
        if (cursorEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "Mouse cursor enabled (camera control disabled)" << "\n";
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
            std::cout << "Mouse cursor disabled (camera control enabled)" << "\n";
        }
    }

    void processInput()
    {
        float velocity = cameraSpeed * deltaTime;
        if (keys[GLFW_KEY_W])
            cameraPos += cameraFront * velocity;
        if (keys[GLFW_KEY_S])
            cameraPos -= cameraFront * velocity;
        if (keys[GLFW_KEY_A])
            cameraPos -= cameraRight * velocity;
        if (keys[GLFW_KEY_D])
            cameraPos += cameraRight * velocity;
        if (keys[GLFW_KEY_SPACE])
            cameraPos += cameraUp * velocity;
        if (keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_C])
            cameraPos -= cameraUp * velocity;
    }

    void updateBackgroundColor()
    {
        const Color &bg = backgrounds[backgroundMode];
        glClearColor(bg.r, bg.g, bg.b, 1.0f);
    }

    void setTargetFPS(int fps)
    {
        targetFPS = fps;
        frameTime = 1.0f / fps;
        if (fps == 60)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
        std::cout << "Target FPS set to: " << fps << "\n";
    }

    void setQuality(int quality)
    {
        qualityLevel = quality;
        const char *qualityNames[] = {"Low", "Medium", "High", "Ultra"};
        std::cout << "Quality set to: " << qualityNames[quality] << "\n";
    }

    void toggleVSync()
    {
        static bool vsyncEnabled = true;
        vsyncEnabled = !vsyncEnabled;
        glfwSwapInterval(vsyncEnabled ? 1 : 0);
        std::cout << "VSync " << (vsyncEnabled ? "enabled" : "disabled") << "\n";
    }

    int getQualityMultiplier()
    {
        switch (qualityLevel) {
            case 0:
                return 1;
            case 1:
                return 2;
            case 2:
                return 4;
            case 3:
                return 8;
            default:
                return 4;
        }
    }

    void toggleRecording(bool manual = false)
    {
        if (isRecording) {
            isRecording = false;
            isManualRecording = false;
            videoWriter.release();
            std::cout << "Recording stopped. Saved " << recordingFrameCount
                      << " frames." << std::endl;
        } else {
            std::string filename =
                "animation_export_" + std::to_string(std::time(nullptr)) + ".mp4";

            int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1'); // H.264
            // int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v'); // MPEG-4
            videoWriter.open(filename, fourcc, 60.0,
                             cv::Size(recordWidth, recordHeight), true);

            if (!videoWriter.isOpened()) {
                std::cout << "Failed to open video writer with MPEG-4, trying MJPEG..."
                          << std::endl;
                fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
                videoWriter.open(filename, fourcc, 60.0,
                                 cv::Size(recordWidth, recordHeight), true);

                if (!videoWriter.isOpened()) {
                    std::cerr << "Failed to open video writer!" << std::endl;
                    return;
                }
            }
            isRecording = true;
            isManualRecording = manual;
            recordingFrameCount = 0;
            std::cout << "Recording started: " << filename
                      << " (4K 60FPS Ultra Quality)"
                      << (manual ? " [MANUAL]" : " [AUTO-STOP]") << std::endl;
        }
    }

    void drawScene(float time, int qualityMult, float aspectRatio)
    {
        // Generate vertices
        animations[currentAnimationIndex]->generateVertices(vertices, time,
                                                            qualityMult);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                     vertices.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, time * 0.3f, glm::vec3(0.1f, 1.0f, 0.0f));
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection =
            glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                           GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE,
                           glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1,
                           GL_FALSE, glm::value_ptr(projection));

        if (animations[currentAnimationIndex]->usePoints()) {
            glPointSize(2.0f);
            glDrawArrays(GL_POINTS, 0, vertices.size() / 6);
        } else {
            glLineWidth(2.0f);
            glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 6);
        }
    }

    void render()
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();

        static float time = glfwGetTime();
        time = glfwGetTime();

        // 1. Render to Screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawScene(time, getQualityMultiplier(),
                  (float)windowWidth / (float)windowHeight);

        // 2. Render to FBO if recording
        if (isRecording) {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, recordWidth, recordHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Force Ultra quality (8x) for recording
            drawScene(time, 8, (float)recordWidth / (float)recordHeight);

            cv::Mat pixels(recordHeight, recordWidth, CV_8UC3);
            glReadPixels(0, 0, recordWidth, recordHeight, GL_BGR, GL_UNSIGNED_BYTE,
                         pixels.data);
            cv::Mat flippedPixels;
            cv::flip(pixels, flippedPixels, 0); // Flip vertically
            videoWriter.write(flippedPixels);
            recordingFrameCount++;

            // Auto-stop after one full period (only if not manual recording)
            if (!isManualRecording) {
                float recordedTime =
                    (float)recordingFrameCount / 60.0f; // Assuming 60 FPS recording
                if (recordedTime >= animations[currentAnimationIndex]->getPeriod()) {
                    toggleRecording();
                    std::cout << "Auto-stopped recording after one full cycle ("
                              << recordedTime << "s)" << std::endl;
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO
        }

        glfwSwapBuffers(window);
    }

    void run()
    {
        std::cout << "Mathematical Functions Animation with Mouse Camera Control\n";
        std::cout << "=========================================================\n";
        std::cout << "Mathematical Functions:\n";
        std::cout << "1 - Parametric Spiral\n";
        std::cout << "2 - Lissajous Curve\n";
        std::cout << "3 - 3D Helix\n";
        std::cout << "4 - Sine Wave Surface\n";
        std::cout << "5 - Animated Torus\n";
        std::cout << "6 - Hypotrochoid\n";
        std::cout << "7 - Superformula\n";
        std::cout << "8 - Lorenz Attractor\n";
        std::cout << "9 - Klein Bottle\n";
        std::cout << "0 - Gyroid Surface\n";
        std::cout << "Q - Spherical Harmonic\n";
        std::cout << "TAB - Fractal Zoom\n";
        std::cout << "E - Phyllotaxis\n";
        std::cout << "R - Tesseract 4D Projection\n";
        std::cout << "T - Wave Interference Surface\n";
        std::cout << "G - Gravitational Spacetime Curvature\n";
        std::cout << "\nGravitational Controls:\n";
        std::cout << "+ - Increase central mass\n";
        std::cout << "- - Decrease central mass\n";
        std::cout << "Current mass: " << centralMass << "\n";
        std::cout << "\nCamera Controls:\n";
        std::cout << "Mouse - Look around\n";
        std::cout << "W/A/S/D - Move forward/left/backward/right\n";
        std::cout << "SPACE - Move up\n";
        std::cout << "SHIFT/C - Move down\n";
        std::cout << "Mouse Wheel - Adjust camera speed\n";
        std::cout << "M - Toggle mouse cursor (enable/disable camera)\n";
        std::cout << "K - Reset Camera Position\n";
        std::cout << "\nPerformance:\n";
        std::cout << "F1 - Set FPS to 30\n";
        std::cout << "F2 - Set FPS to 60\n";
        std::cout << "F3 - Set FPS to 120\n";
        std::cout << "F4 - Set FPS to Uncapped\n";
        std::cout << "F5 - Set Quality to Low\n";
        std::cout << "F6 - Set Quality to Medium\n";
        std::cout << "F7 - Set Quality to High\n";
        std::cout << "F8 - Set Quality to Ultra\n";
        std::cout << "\nRecording:\n";
        std::cout << "F9 - Toggle Recording (auto-stop after animation cycle)\n";
        std::cout << "F10 - Toggle Recording (manual control, arbitrary duration)\n";
        std::cout << "\nOther:\n";
        std::cout << "B - Change Background Color\n";
        std::cout << "V - Toggle VSync\n";
        std::cout << "ESC - Exit\n\n";
        std::cout << "Camera speed: " << cameraSpeed << " (use mouse wheel to adjust)\n";

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            render();
        }
    }

    void cleanup()
    {
        if (isRecording) {
            videoWriter.release();
        }
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &textureColorbuffer);
        glDeleteRenderbuffers(1, &rbo);
        glDeleteProgram(shaderProgram);
        glfwTerminate();
    }

    bool createShaderProgram()
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "Vertex shader compilation failed: " << infoLog << "\n";
            return false;
        }
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "Fragment shader compilation failed: " << infoLog << "\n";
            return false;
        }
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

int main(int argc, char **argv)
{
    // CLI Argument Parsing
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --help      Show this help message" << std::endl;
            std::cout << "  -k, --keymaps   Show detailed key mappings" << std::endl;
            return 0;
        } else if (arg == "-k" || arg == "--keymaps") {
            std::cout << "Key Mappings:" << std::endl;
            std::cout << "  1-9, 0, Q, Tab, E, R, T, G : Switch Animations"
                      << std::endl;
            std::cout << "  WASD, Space, Shift         : Move Camera" << std::endl;
            std::cout << "  Mouse                      : Look around" << std::endl;
            std::cout << "  Scroll                     : Adjust Camera Speed"
                      << std::endl;
            std::cout << "  F1-F4                      : Set Target FPS (30, 60, "
                         "120, Uncapped)"
                      << std::endl;
            std::cout << "  F5-F8                      : Set Quality (Low, Med, "
                         "High, Ultra)"
                      << std::endl;
            std::cout << "  F9                         : Toggle Recording (Auto-stop "
                         "after 1 cycle)"
                      << std::endl;
            std::cout << "  F10                        : Toggle Recording (Manual "
                         "start/stop)"
                      << std::endl;
            std::cout << "  M                          : Toggle Mouse Capture"
                      << std::endl;
            std::cout << "  B                          : Cycle Background Color"
                      << std::endl;
            std::cout << "  V                          : Toggle VSync" << std::endl;
            std::cout << "  K                          : Reset Camera" << std::endl;
            std::cout << "  ESC                        : Exit" << std::endl;
            return 0;
        }
    }

    MathAnimation app;
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << "\n";
        return -1;
    }
    app.run();
    app.cleanup();
    return 0;
}
