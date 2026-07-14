#define GL_SILENCE_DEPRECATION // Shuts up Apple's "OpenGL is deprecated"
                               // warnings

#include "engine.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// --- Helper: Read file to string ---
char *read_file(const char *filepath) {
  FILE *file = fopen(filepath, "rb");
  if (!file)
    return NULL;
  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = malloc(length + 1);
  fread(buffer, 1, length, file);
  buffer[length] = '\0';
  fclose(file);
  return buffer;
}

// --- Helper: Compile Shader with Error Checking ---
GLuint compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    SDL_Log("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
  }
  return shader;
}

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
    return -1;
  }

  // Request OpenGL 3.3 Core
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window =
      SDL_CreateWindow("Dynamic Raymarcher", 1280, 720, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(window);

  // Build Shader Program
  char *vert_src = read_file("shaders/raymarch.vert");
  char *frag_src = read_file("shaders/raymarch.frag");

  if (!vert_src || !frag_src) {
    SDL_Log("Failed to load shader files.");
    return -1;
  }

  GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vert_src);
  GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, frag_src);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // Initial Scene Setup
  WorldData world = {0};
  world.object_count = 2;

  // Object 0: Floor
  world.objects[0].position_radius[1] = -1.0f; // y position
  world.objects[0].color_type[0] = 0.2f;       // r
  world.objects[0].color_type[1] = 0.8f;       // g
  world.objects[0].color_type[2] = 0.2f;       // b
  world.objects[0].color_type[3] = 1.0f;       // Type 1: Floor

  // Object 1: Sphere
  world.objects[1].position_radius[0] = 0.0f; // x
  world.objects[1].position_radius[1] = 0.0f; // y
  world.objects[1].position_radius[2] = 5.0f; // z
  world.objects[1].position_radius[3] = 1.0f; // radius
  world.objects[1].color_type[0] = 0.8f;      // r
  world.objects[1].color_type[1] = 0.2f;      // g
  world.objects[1].color_type[2] = 0.2f;      // b
  world.objects[1].color_type[3] = 0.0f;      // Type 0: Sphere

  // Generate Uniform Buffer Object (UBO)
  GLuint ubo;
  glGenBuffers(1, &ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(WorldData), &world, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

  // Get Uniform Locations
  GLint resLoc = glGetUniformLocation(shaderProgram, "u_resolution");
  GLint camLoc = glGetUniformLocation(shaderProgram, "u_camera_pos");

  // Generate and bind a dummy Vertex Array Object
  // Core Profile requires a VAO to be bound for glDrawArrays to work!
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        running = false;

      // Dynamic interaction: Press Space to spawn a random sphere
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE) {
        if (world.object_count < 100) {
          int i = world.object_count;
          world.objects[i].position_radius[0] = ((rand() % 100) / 10.0f) - 5.0f;
          world.objects[i].position_radius[1] = ((rand() % 50) / 10.0f);
          world.objects[i].position_radius[2] = 3.0f + ((rand() % 50) / 10.0f);
          world.objects[i].position_radius[3] = 0.5f; // radius
          world.objects[i].color_type[0] = (rand() % 100) / 100.0f;
          world.objects[i].color_type[1] = (rand() % 100) / 100.0f;
          world.objects[i].color_type[2] = (rand() % 100) / 100.0f;
          world.objects[i].color_type[3] = 0.0f; // Sphere
          world.object_count++;

          // Update GPU memory
          glBindBuffer(GL_UNIFORM_BUFFER, ubo);
          glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(WorldData), &world);
        }
      }
    }

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glUseProgram(shaderProgram);
    glUniform2f(resLoc, (float)w, (float)h);
    glUniform3f(camLoc, 0.0f, 0.0f, 0.0f); // Camera at origin

    // Clear the screen buffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the full-screen triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}