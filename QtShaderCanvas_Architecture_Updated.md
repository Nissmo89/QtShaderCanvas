# QtShaderCanvas

## Vision

A lightweight Qt Widgets library for rendering modern GLSL shader
backgrounds with a simple API.

## Goals

-   Qt Widgets only (no QML)
-   Drop-in shader loading
-   ShaderToy-style uniforms
-   Hot reload
-   High DPI
-   Cross-platform

## Architecture

``` text
Application
    |
ShaderWidget
    |
Renderer
 ├─ OpenGL Context
 ├─ ShaderManager
 ├─ UniformManager
 ├─ Fullscreen Quad
 └─ Render Loop
```

### Core modules

-   ShaderWidget: public QWidget/QOpenGLWidget API.
-   ShaderManager: compile, cache, reload shaders.
-   UniformManager: iTime, iResolution, iMouse, iFrame, iDeltaTime.
-   FileWatcher: automatic reload.
-   Renderer: OpenGL drawing.

## Example API

``` cpp
auto *bg = new ShaderWidget(this);
bg->loadShader(":/shaders/aurora.frag");
bg->start();
```

## Folder layout

``` text
QtShaderCanvas/
├── include/
├── src/
├── shaders/
├── examples/
├── docs/
└── tests/
```

## Development roadmap

### v0.1

-   Window rendering
-   Single fragment shader
-   Time & resolution uniforms
-   Resize support

### v0.2

-   Mouse input
-   Hot reload
-   FPS control
-   Transparency

### v0.3

-   Shader presets
-   Texture uniforms
-   Screenshot API

### v0.4

-   Multi-pass rendering
-   Framebuffers
-   Feedback effects

### v1.0

-   Stable API
-   Documentation
-   Examples
-   CI builds

## Nice-to-have

-   Shader gallery
-   Theme integration
-   Visual shader inspector
-   Performance overlay

## Design principles

-   Hide OpenGL boilerplate.
-   Keep API minimal.
-   Make adding a new shader as easy as adding a file.
-   Separate rendering backend from user-facing widget.

------------------------------------------------------------------------

# Project Direction Update (July 2026)

## Project Name

-   Repository: **QtShaderCanvas**
-   Main Widget: **QtShaderCanvas**

## Vision

QtShaderCanvas aims to become a **ShaderToy-compatible rendering canvas
for Qt Widgets**, not merely a wrapper around QOpenGLWidget.

### Primary Goals

-   ShaderToy-compatible runtime
-   Qt Widgets only (no QML)
-   Hide all OpenGL boilerplate
-   Hot shader reloading
-   Cross-platform
-   High-DPI aware

## Compatibility

### Supported

-   Native GLSL
-   ShaderToy-compatible shaders
-   Ghostty ShaderToy-style shaders (where compatible)

### Planned

-   Multi-pass shaders
-   Shader packs
-   Post-processing pipelines

## Automatic Uniforms

-   iResolution
-   iTime
-   iTimeDelta
-   iFrame
-   iMouse
-   iDate
-   iChannel0
-   iChannel1
-   iChannel2
-   iChannel3
-   iChannelResolution
-   iSampleRate (future)

## Developer Experience

``` cpp
QtShaderCanvas *canvas = new QtShaderCanvas(this);
canvas->loadShader("aurora.glsl");
canvas->play();
```

No OpenGL setup should be required by library users.

## Long-term Objective

Become the easiest way to run ShaderToy and compatible GLSL shaders
inside Qt Widgets applications.
