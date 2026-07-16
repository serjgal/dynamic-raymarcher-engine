# Dynamic SDF Raymarching Engine

A real-time, purely procedural 3D rendering engine built in **C**, **SDL3**, and **OpenGL 3.3 Core**. 

Unlike traditional rasterization engines that send explicit vertex geometry to the GPU, this engine utilizes **Sphere Tracing** and **Signed Distance Functions (SDFs)**. To achieve real-time, dynamic world-editing without expensive runtime shader recompilation, it leverages a **Data-Driven Architecture** communicating over Uniform Buffer Objects (UBOs).

## Key Features

*   **Purely Procedural Geometry:** No polygons, no vertices. The entire world is generated mathematically on a single full-screen quad. Supports 15 distinct built-in mathematical primitives (Sphere, Box, Torus, Hexagonal Prism, Capsule, etc.) manipulatable via arbitrary 3D Euler rotations.
*   **Constructive Solid Geometry (CSG):** Combine, subtract, and intersect objects dynamically in real-time. Operations are logically isolated from one another via multi-pass CSG Group IDs.
*   **Physical Lighting & Shadows:** Calculates Lambertian surface lighting powered by a highly efficient tetrahedral normal estimator. Casts procedural soft shadows and limits light contamination via designated Light Group IDs.
*   **Zero-Recompilation Editing:** Add, remove, and modify shapes in real-time. The engine alters memory state rather than shader source code.
*   **Data-Driven UBO Bridge:** A strict `std140` memory layout synchronizes a C struct with the GLSL environment, allowing the fragment shader to iterate through scene arrays procedurally.
*   **Cross-Platform Architecture:** Built to run natively on macOS and Linux using stable OpenGL 3.3 Core standards.

## Architecture Overview

Most procedural shaders hardcode shapes into the GLSL math. This project extracts the *state* of the world from the *evaluation* of the world:

1.  **The Controller (C):** Manages windowing, input events, and a master `WorldData` array of objects (shapes, CSG modifiers, and isolated light sources).
2.  **The Bridge (OpenGL):** Uploads the struct directly to GPU memory via `glBufferSubData`.
3.  **The Evaluator (GLSL):** A unified fragment shader reads the shared memory and executes a raymarching loop, evaluating distances against the dynamic list of objects to synthesize the final image.

## Build Instructions

### Prerequisites
*   A C11 compiler (`gcc` or `clang`)
*   `make`
*   SDL3 library installed (`brew install sdl3` or `apt-get install libsdl3-dev`)

### Compiling and Running
```bash
# Clone the repository
git clone [https://github.com/yourusername/dynamic-raymarcher.git](https://github.com/yourusername/dynamic-raymarcher.git)

# Build the project
make

# Run the engine
./raymarcher
```

### Controls
*   **[W, A, S, D]**: Move Forward, Left, Backward, and Right
*   **[Q, E]**: Descend and Ascend along the world Y-axis
*   **[Mouse]**: Look around (Pitch and Yaw)
*   **[SPACE]**: Spawn a random procedurally generated sphere into the world instantly.
*   **[ESC]**: Release mouse capture and exit the engine
*   *(Expandable via the C event loop to include full camera movement and UI integration)*.

---
*Developed by Serj Galstyan as a technical exploration of low-level graphics programming and systems architecture.*