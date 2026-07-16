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
 * Expanded to 64 bytes (4 vec4s) to support complex shapes,
 * arbitrary rotations, and CSG boolean operations.
 */
typedef struct {
  float pos_type[4]; // x, y, z (Position), w (Shape Type ID)
  float rot_op[4];   // x, y, z (Euler Rotation in radians), w (CSG Op: 0=Union,
                     // 1=Sub, 2=Int)
  float params[4];   // x, y, z (Main dimensions e.g. Extents/Radii), w (Extra
                     // param e.g. Roundness/Thickness)
  float color_extra[4]; // r, g, b (Color), w (Unused/Material ID)
} SceneObject;

/*
 * The master UBO payload.
 * Padded explicitly to 16-byte boundaries for strict std140 alignment.
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