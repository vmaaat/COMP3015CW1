# COMP3015 - Assignment 1

## OpenGL Rendering Project

------------------------------

### Overview

This project demonstrates a modern OpenGL rendering pipeline implemented in C++ using custom vertex and fragment shaders. This scene contains:
- An imported OBJ model
- A textured ground plane
- A cubemap skybox
- A multi light Blinn-Phong shading model
- A real-time orbit camera system

The focus of this project was implementing per-fragment lighting and dynamic shader-based rendering.

------------------------------

### Development Environment

- IDE: Visual Studio 2022
- Operating System: Windows 10
- Architecture: x64
- Graphics API: OpenGL 4.6

External libraries used:
- GLFW - Window creation and input handling
- GLAD - OpenGL function loader
- TinyOBJLoader - OBJ model loading
- stb_image - Texture loading

------------------------------

### How the Project Works

# Program Structure
- main.cpp: creates the window and initialises the scene.
- scenebasic_uniform.cpp: Handles;
- - Shader compilation.
  - Asset loading.
  - Camera logic.
  - Lighting updates.
  - Rendering loop.
- shader/basic_uniform.vert: Vertex shader handling transformation of vertices.
- shader/basic_uniform.frag: Fragment shader implementing the lighting model.
- shader/skybox.vert: Skybox shader using cubemap textures.
- shader/skybox.frag: Skybox shader using cubemap textures.

------------------------------

### Shading Model

The fragment shader implements a Blinn-Phong lighting model.

Each light contributes:
- Ambient - base illuination to prevent full darkness
- Diffuse - Lambertian reflectance using dot(normal, lightDir)
- Specular - Blinn-Phong half-vector method for highlights

Lighting is calculated per-fragment, producing smoother and more accurate shading than per-vertex lighting.

Two light sources are implemented:
1. A static light in the scene.
2. A dynamic light that follows the camera position.
Lighting can be toggled at runtime to demonstrate shader functionality.

------------------------------

### Camera System

The camera uses spherical coordinates for orbiting:

- Yaw/Pitch control horizontal and vertical rotation
- Radius controls the zoom
- Pitch is clamped to prevent inversion
- Zoom is lamped to prevent clipping through objects

------------------------------

### Controls:
- W / S - Adjust camera pitch
- A / D - Adjust camera yaw
- Q / E - Zoom in / out
- K - Toggle camera following light
- L - Toggle static light

------------------------------

### Problems Encountered 

During development, several issues were resolved:
- Tecture flipping inconsistencies from stb_image
- Camera clipping through geometry
- Skybox depth testing issues

Each issue was addressed by adjusting shader parameters, clamping camera values and correcting build settings.

------------------------------

### Executable

The project includes a Release build executable that runs independently of Visual Studio. All required shader and media resources are required in the project folder.

------------------------------

### Youtube Video



------------------------------
