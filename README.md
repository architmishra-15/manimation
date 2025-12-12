# Mathematical Functions Animation

> Status - Incomplete

## Features

A 3D mathematical visualization program with interactive camera controls and multiple mathematical function animations including:
- Parametric Spiral, Lissajous Curve, 3D Helix
- Sine Wave Surface, Animated Torus, Hypotrochoid
- Superformula, Lorenz Attractor, Klein Bottle
- Gyroid Surface, Spherical Harmonic, Fractal Zoom
- Phyllotaxis, Tesseract 4D Projection
- Wave Interference Surface, Gravitational Spacetime Curvature

## Building

### Prerequisites

**Linux:**
- g++ compiler with C++17 support
- GLFW3, GLEW, OpenGL libraries
- OpenCV 4.x
- pkg-config

Install on Ubuntu/Debian:
```bash
sudo apt-get install build-essential libglfw3-dev libglew-dev libopencv-dev pkg-config
```

**Windows:**
- MinGW-w64 or MSVC compiler
- GLFW3, GLEW, OpenGL libraries
- OpenCV 4.x

### Build Instructions

**Linux:**
```bash
# Build dynamic executable
make run

# Build static executable
make static

# Build both
make all

# Clean build files
make clean

# Show help
make help
```

**Windows (MinGW):**
```bash
# Set library paths (if not in default locations)
set GLFW_DIR=C:/path/to/glfw
set GLEW_DIR=C:/path/to/glew
set OPENCV_DIR=C:/path/to/opencv

# Build dynamic executable
make run

# Or specify paths inline
make run GLFW_DIR=D:/libs/glfw GLEW_DIR=D:/libs/glew OPENCV_DIR=D:/libs/opencv
```

**Windows (MSVC):**
```cmd
# Use MSVC compiler
make run CXX=cl GLFW_DIR=C:/path/to/glfw GLEW_DIR=C:/path/to/glew OPENCV_DIR=C:/path/to/opencv
```

### Environment Variables

You can customize library paths using environment variables:

| Variable | Description | Default (Windows) |
|----------|-------------|-------------------|
| `GLFW_DIR` | GLFW installation directory | `C:/libs/glfw` |
| `GLEW_DIR` | GLEW installation directory | `C:/libs/glew` |
| `OPENCV_DIR` | OpenCV installation directory | `C:/libs/opencv` |
| `SRC` | Source file name | `main.cpp` |
| `OUT_NORMAL` | Output name for dynamic build | `math_animation-2` |
| `OUT_STATIC` | Output name for static build | `main-2` |
| `CXX` | Compiler to use | `g++` (use `cl` for MSVC) |

### Build Examples
```bash
# Linux - standard build
make run

# Windows MinGW - custom paths
make run GLFW_DIR="C:/MyLibs/GLFW" OPENCV_DIR="C:/MyLibs/opencv"

# Change output name
make run OUT_NORMAL=my_animation

# Build with different source file
make run SRC=animation.cpp
```

## Running

### Command Line Options
```bash
# Show help
./math_animation-2 --help

# Show detailed key mappings
./math_animation-2 --keymaps

# Run the program
./math_animation-2
```

### Controls

See `--keymaps` option for complete controls, including:
- Number keys (1-9, 0, Q, Tab, E, R, T, G) for different animations
- WASD + Space/Shift for camera movement
- Mouse for looking around
- F1-F8 for performance settings
- F9/F10 for recording
- And more...

## Troubleshooting

**Linux: pkg-config not found**
```bash
sudo apt-get install pkg-config
```

**Windows: Libraries not found**
- Ensure library paths are correctly set via environment variables
- Check that DLL files are in PATH or same directory as executable
- Verify library paths match your installation

**Compilation errors**
- Ensure C++17 support is available
- Check that all required libraries are installed
- Use `make help` to verify detected configuration

