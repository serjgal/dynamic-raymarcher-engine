#ifndef ENGINE_H
#define ENGINE_H

// Conditionally load the modern OpenGL Core Profile headers
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL_opengl.h>
#endif

/*
 * SceneObject perfectly matches the GLSL std140 layout.
 * Designed with vec4 memory blocks (float[4]) to guarantee GPU alignment
 * and allow future expansion (e.g., using 'w' components for CSG operations).
 */
typedef struct {
  float position_radius[4]; // x, y, z, w (radius/size)
  float color_type[4];      // r, g, b, w (shape type ID)
} SceneObject;

/*
 * The master UBO payload.
 * Padded explicitly to 16-byte boundaries.
 */
typedef struct {
  int object_count;
  int pad[3]; // Align to 16 bytes for strict std140 rules
  SceneObject objects[100];
} WorldData;

// --- Engine Utilities ---
char *read_file(const char *filepath);
GLuint compile_shader(GLenum type, const char *source);

#endif // ENGINE_H