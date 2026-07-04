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
| `iChannel0` - `3` | `sampler2D` | Texture binding channels (placeholders declared for compiler compatibility) |
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

## Building the Library

Ensure you have **Qt 6** and a compiler supporting **C++17** installed.

### CMake Build

1. Configure the build:
   ```bash
   cmake -S . -B build
   ```
2. Build the library and demo application:
   ```bash
   cmake --build build -j$(nproc)
   ```
3. Run the demonstration application:
   ```bash
   LD_LIBRARY_PATH=build ./build/examples/QtShaderCanvasExample
   ```

---

## Repository Structure

- `include/`: Public headers (`QtShaderCanvas.h`).
- `src/`: Private implementation detail files (`QtShaderCanvas.cpp`).
- `shaders/`: Sample ShaderToy fragment files (`default.frag`, `star_nest.frag`).
- `examples/`: Multi-featured demonstration app including a control sidebar, file inspector, drag-and-drop file loads, and error logger.
