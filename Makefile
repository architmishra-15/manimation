# User‑configurable variables
SRC          ?= main.cpp
OUT_NORMAL   ?= math_animation-2.exe
OUT_STATIC   ?= main-2.exe

# Compiler & flags
CXX          := g++
CXXFLAGS     := -std=c++17

# Normal build settings
NORMAL_INCLUDES := -I C:/msys64/mingw64/include
NORMAL_LIB_PATH := -L C:/msys64/mingw64/lib
NORMAL_LIBS     := -lglfw3 -lglew32 -lopengl32 -lgdi32 \
                   -lavformat -lavcodec -lavutil -lswscale \
                   -luser32 -lkernel32 -lshell32 -lwinmm -lole32 -ladvapi32

# Static build settings
STATIC_PKG_CFG  := $(shell pkg-config --static --cflags --libs \
                     glfw3 glew libavformat libavcodec libavutil libswscale)
STATIC_EXTRA    := -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32 \
                   -pthread

.PHONY: all normal static clean

all: normal static

# ─────────────── Normal build ───────────────
run: $(SRC)
	@echo "→ Building normal (dynamic) executable: $(OUT_NORMAL)"
	$(CXX) $(CXXFLAGS) \
	  $(SRC) \
	  -o $(OUT_NORMAL) \
	  $(NORMAL_INCLUDES) $(NORMAL_LIB_PATH) $(NORMAL_LIBS)

# ─────────────── Static build ───────────────
static: $(SRC)
	@echo "→ Building static executable: $(OUT_STATIC)"
	$(CXX) $(CXXFLAGS) -static -static-libgcc -static-libstdc++ \
	  $(SRC) \
	  -o $(OUT_STATIC) \
	  $(STATIC_PKG_CFG) $(STATIC_EXTRA)

# ─────────────── Clean ───────────────
clean:
	@echo "→ Cleaning up"
	-rm -f $(OUT_NORMAL) $(OUT_STATIC)
