# QtShaderCanvas

A lightweight, high-performance Qt Widgets C++ library for rendering ShaderToy-compatible fragment shaders.

Developed for Qt 6, **QtShaderCanvas** completely abstracts OpenGL boilerplate, allowing you to easily run, display, and interact with GLSL shaders inside standard desktop applications (e.g. as custom background elements, in specialized media viewers, or inside node/shader editors).

---

## Key Features

- **Pure Qt Widgets**: Designed purely for C++ `QWidget` / `QOpenGLWidget` applications (no QML or Qt Quick required).
- **Boilerplate-free API**: Simply drop the widget in a layout, point it to a shader file, and call `play()`.
- **ShaderToy Compatibility**: Automatic wrapper support for `mainImage(out vec4, in vec2)` and standard inputs.
- **Auto Hot Reload**: Watches the loaded `.glsl`/`.frag` file on disk via `QFileSystemWatcher` and instantly updates the canvas upon save without resetting time/frame states. Keeps rendering the last valid state if compilation fails.
- **High-DPI Aware**: Automatically adapts OpenGL viewport scaling using the device's physical pixel ratio.
- **Full Playback Controls**: Easily start, pause, stop, or step through frames, with adjustable FPS caps.
- **Shader Playground App**: Includes a full-featured integrated IDE/playground environment (see below).

---

## Shader Playground Application

The library comes with a feature-rich, high-performance **Shader Playground** demonstration application (`QtShaderCanvasExample`). 

### Capabilities & Features:
- **Interactive GLSL Editor**: Live, real-time code editing using an integrated code engine with standard edit actions (`Undo`, `Redo`, `Cut`, `Copy`, `Paste`).
- **Debounced Compilation**: Recompiles shader code on-the-fly 300ms after you stop typing.
- **Live Uniform Inspector**: Displays real-time values of active uniforms:
  - `iTime` (Playback time in seconds)
  - `iFrame` (Rendered frame index)
  - `iResolution` (Viewport dimensions adjusted for High-DPI physical scale)
  - `iMouse` (Interactive mouse click/drag positions)
  - `iDate` (Current system date/time)
- **Dynamic Texture Binding**: Bind textures to input channels (e.g. `iChannel0`) using the UI. Supports built-in templates (e.g., Lush Green Image) or browsing and uploading local image files dynamically.
- **Real-Time Error Console**: A dedicated terminal-style log output showing compiler messages, success stamps, or red-highlighted error logs with precise timestamps.
- **Dynamic Examples Menu**: Automatically scans the `GLSL-Shaders` folder at startup and populates a menu allowing you to open and play preloaded shaders instantly.
- **Drag & Drop**: Simply drop any `.glsl` or `.frag` file anywhere in the window to open it in the editor.
- **Playback & Speed Configs**: Standard Play/Pause/Stop/Step buttons coupled with a hot-reload toggle and a real-time FPS limit slider (1 to 120 FPS).

---

## Supported Uniforms

`QtShaderCanvas` automatically binds and updates the following uniforms (compatible with ShaderToy syntax):

| Uniform | Type | Description |
|---|---|---|
| `iResolution` | `vec3` | Viewport resolution in physical pixels (width, height, aspect/dpi-ratio) |
| `iTime` | `float` | Playback time in seconds |
| `iTimeDelta` | `float` | Frame render delta time in seconds |
| `iFrame` | `int` | Current playback frame index |
| `iMouse` | `vec4` | Mouse coordinates: `xy` current drag pos, `zw` last mouse down pos (negated when mouse is up) |
| `iDate` | `vec4` | Calendar date and time: `(year, month [0-11], day, time_in_seconds)` |
| `iChannel0` - `3` | `sampler2D` | Texture binding channels (used for texture inputs) |
| `iChannelResolution` | `vec3[4]` | Resolutions of texture channels |

---

## Quick Start Example

Add `QtShaderCanvas` to your application layout in just a few lines:

```cpp
#include <QtShaderCanvas.h>

// In your MainWindow or parent widget constructor
auto *canvas = new QtShaderCanvas(this);
layout->addWidget(canvas);

// Load any ShaderToy-compatible .glsl file
canvas->loadShader("aurora.glsl");

// Control playback
canvas->setFpsLimit(60); // Optional: Default is 60 FPS
canvas->play();
```

---

## Build Types & Compilation Options

This library supports multiple build configurations to suit library developers and application integrators.

Ensure you have **Qt 6** (with Widgets, OpenGL, and OpenGLWidgets components) and a compiler supporting **C++17** installed.

### 1. Build Modes (Release vs. Debug)
- **Release Build** (Optimized for performance):
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j$(nproc)
  ```
- **Debug Build** (Includes debugging symbols):
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build -j$(nproc)
  ```

### 2. Library Linkage (Shared vs. Static)
By default, the library is configured to build as a shared library (`.so` / `.dll`).
- **Shared Library (Default)**:
  ```bash
  cmake -S . -B build -DBUILD_SHARED_LIBS=ON
  cmake --build build -j$(nproc)
  ```
- **Static Library** (For standalone static binaries):
  ```bash
  cmake -S . -B build -DBUILD_SHARED_LIBS=OFF
  cmake --build build -j$(nproc)
  ```

### 3. Exclude Examples & Playground
If you only need the core library and want to skip compiling the demo app and its code editor dependencies:
- **Build Core Library Only**:
  ```bash
  cmake -S . -B build -DBUILD_EXAMPLES=OFF
  cmake --build build -j$(nproc)
  ```

---

## Running the Playground

If you built with examples enabled (`BUILD_EXAMPLES=ON`), you can run the Shader Playground application:

```bash
# Set library path for shared linkage (on Linux)
LD_LIBRARY_PATH=build ./build/examples/QtShaderCanvasExample
```

---

## Repository Structure

- `include/`: Public headers (`QtShaderCanvas.h`).
- `src/`: Private implementation detail files (`QtShaderCanvas.cpp`).
- `shaders/`: Sample ShaderToy fragment files (`default.frag`, `star_nest.frag`).
- `GLSL-Shaders/`: Additional preloaded shaders showcasing different complex visual math patterns (Clouds, Rainforest, Dubstep, Octagrams, etc.).
- `QCodeEngine-C/`: Vendored dependency for the code editor inside the Shader Playground.
- `examples/`: Code for the Shader Playground application (`QtShaderCanvasExample`).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

