# User-configurable variables
SRC          ?= main.cpp
OUT_NORMAL   ?= math_animation-2
OUT_STATIC   ?= main-2

# Detect operating system
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

# Compiler selection
CXX          ?= g++

# Windows-specific detection
ifeq ($(DETECTED_OS),Windows)
    # Check if using MSVC (cl.exe) or MinGW (g++)
    ifeq ($(CXX),cl)
        USING_MSVC := 1
    else
        USING_MSVC := 0
    endif
    EXE_EXT := .exe
else
    USING_MSVC := 0
    EXE_EXT :=
endif

# Add .exe extension to output names on Windows
OUT_NORMAL_FULL := $(OUT_NORMAL)$(EXE_EXT)
OUT_STATIC_FULL := $(OUT_STATIC)$(EXE_EXT)

# ═══════════════════════════════════════════════════════════
#  Linux Configuration
# ═══════════════════════════════════════════════════════════
ifeq ($(DETECTED_OS),Linux)
    CXXFLAGS := -std=c++17 $(shell pkg-config --cflags opencv4)
    
    NORMAL_LIBS := -lglfw -lGLEW -lEGL -lGL -lGLU -lOpenGL \
                   $(shell pkg-config --libs-only-L opencv4) \
                   -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs
    
    STATIC_PKG_CFG := $(shell pkg-config --static --cflags --libs glew)
    STATIC_LIBS := -pthread -lrt -lm -ldl -lglfw3
    STATIC_FLAGS := -static -static-libgcc -static-libstdc++
endif

# ═══════════════════════════════════════════════════════════
#  Windows MinGW Configuration
# ═══════════════════════════════════════════════════════════
ifeq ($(DETECTED_OS),Windows)
ifeq ($(USING_MSVC),0)
    # User-configurable paths (set via environment or override here)
    GLFW_DIR     ?= C:/libs/glfw
    GLEW_DIR     ?= C:/libs/glew
    OPENCV_DIR   ?= C:/libs/opencv
    
    CXXFLAGS := -std=c++17 \
                -I$(GLFW_DIR)/include \
                -I$(GLEW_DIR)/include \
                -I$(OPENCV_DIR)/include
    
    NORMAL_LIBS := -L$(GLFW_DIR)/lib \
                   -L$(GLEW_DIR)/lib \
                   -L$(OPENCV_DIR)/lib \
                   -lglfw3 -lglew32 -lopengl32 -lglu32 -lgdi32 \
                   -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs
    
    STATIC_LIBS := -L$(GLFW_DIR)/lib \
                   -L$(GLEW_DIR)/lib \
                   -L$(OPENCV_DIR)/lib \
                   -lglfw3 -lglew32 -lopengl32 -lglu32 -lgdi32 \
                   -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs \
                   -static -static-libgcc -static-libstdc++
    
    STATIC_FLAGS := -static -static-libgcc -static-libstdc++
endif
endif

# ═══════════════════════════════════════════════════════════
#  Windows MSVC Configuration
# ═══════════════════════════════════════════════════════════
ifeq ($(DETECTED_OS),Windows)
ifeq ($(USING_MSVC),1)
    # User-configurable paths
    GLFW_DIR     ?= C:/libs/glfw
    GLEW_DIR     ?= C:/libs/glew
    OPENCV_DIR   ?= C:/libs/opencv
    
    CXXFLAGS := /std:c++17 /EHsc \
                /I$(GLFW_DIR)/include \
                /I$(GLEW_DIR)/include \
                /I$(OPENCV_DIR)/include
    
    NORMAL_LIBS := /link \
                   /LIBPATH:$(GLFW_DIR)/lib \
                   /LIBPATH:$(GLEW_DIR)/lib \
                   /LIBPATH:$(OPENCV_DIR)/lib \
                   glfw3.lib glew32.lib opengl32.lib glu32.lib \
                   opencv_core.lib opencv_imgproc.lib opencv_videoio.lib opencv_imgcodecs.lib
    
    STATIC_LIBS := $(NORMAL_LIBS) /MT
    STATIC_FLAGS := /MT
endif
endif

.PHONY: all normal static clean help

all: normal static

help:
	@echo "Mathematical Functions Animation - Build System"
	@echo "================================================"
	@echo ""
	@echo "Detected OS: $(DETECTED_OS)"
ifeq ($(DETECTED_OS),Windows)
ifeq ($(USING_MSVC),1)
	@echo "Compiler: MSVC (cl.exe)"
else
	@echo "Compiler: MinGW (g++)"
endif
endif
	@echo ""
	@echo "Targets:"
	@echo "  make run       - Build and create normal (dynamic) executable"
	@echo "  make static    - Build static executable"
	@echo "  make all       - Build both normal and static executables"
	@echo "  make clean     - Remove built executables"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  SRC=$(SRC)"
	@echo "  OUT_NORMAL=$(OUT_NORMAL_FULL)"
	@echo "  OUT_STATIC=$(OUT_STATIC_FULL)"
	@echo ""
ifeq ($(DETECTED_OS),Windows)
	@echo "Windows Library Paths (override with environment variables):"
	@echo "  GLFW_DIR=$(GLFW_DIR)"
	@echo "  GLEW_DIR=$(GLEW_DIR)"
	@echo "  OPENCV_DIR=$(OPENCV_DIR)"
	@echo ""
	@echo "Example: make run GLFW_DIR=D:/mylibs/glfw GLEW_DIR=D:/mylibs/glew"
endif

# ─────────────── Normal build ───────────────
run: $(SRC)
	@echo "→ Building normal (dynamic) executable: $(OUT_NORMAL_FULL)"
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT_NORMAL_FULL) $(NORMAL_LIBS)

# ─────────────── Static build ───────────────
static: $(SRC)
	@echo "→ Building static executable: $(OUT_STATIC_FULL)"
ifeq ($(USING_MSVC),1)
	$(CXX) $(CXXFLAGS) $(STATIC_FLAGS) $(SRC) /Fe:$(OUT_STATIC_FULL) $(STATIC_LIBS)
else
	$(CXX) $(CXXFLAGS) $(STATIC_FLAGS) $(SRC) -o $(OUT_STATIC_FULL) $(STATIC_LIBS)
endif

# ─────────────── Clean ───────────────
clean:
	@echo "→ Cleaning up"
ifeq ($(DETECTED_OS),Windows)
	-del /Q $(OUT_NORMAL_FULL) $(OUT_STATIC_FULL) 2>nul
else
	-rm -f $(OUT_NORMAL_FULL) $(OUT_STATIC_FULL)
endif
