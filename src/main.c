#define GL_SILENCE_DEPRECATION

#include "engine.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Math Helpers ---
void normalize(float v[3]) {
  float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (len > 0.0f) {
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
  }
}

void cross(float a[3], float b[3], float res[3]) {
  res[0] = a[1] * b[2] - a[2] * b[1];
  res[1] = a[2] * b[0] - a[0] * b[2];
  res[2] = a[0] * b[1] - a[1] * b[0];
}

// --- File and Shader Helpers ---
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

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window =
      SDL_CreateWindow("Dynamic Raymarcher", 1280, 720, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(window);

  char *vert_src = read_file("shaders/raymarch.vert");
  char *frag_src = read_file("shaders/raymarch.frag");

  GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vert_src);
  GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, frag_src);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  WorldData world = {0};
  world.object_count = 2;

  world.objects[0].position_radius[1] = -1.0f;
  world.objects[0].color_type[0] = 0.2f;
  world.objects[0].color_type[1] = 0.8f;
  world.objects[0].color_type[2] = 0.2f;
  world.objects[0].color_type[3] = 1.0f;

  world.objects[1].position_radius[0] = 0.0f;
  world.objects[1].position_radius[1] = 0.0f;
  world.objects[1].position_radius[2] = 5.0f;
  world.objects[1].position_radius[3] = 1.0f;
  world.objects[1].color_type[0] = 0.8f;
  world.objects[1].color_type[1] = 0.2f;
  world.objects[1].color_type[2] = 0.2f;
  world.objects[1].color_type[3] = 0.0f;

  GLuint ubo;
  glGenBuffers(1, &ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(WorldData), &world, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

  GLint resLoc = glGetUniformLocation(shaderProgram, "u_resolution");
  GLint camPosLoc = glGetUniformLocation(shaderProgram, "u_camera_pos");
  GLint camFrontLoc = glGetUniformLocation(shaderProgram, "u_camera_front");
  GLint camUpLoc = glGetUniformLocation(shaderProgram, "u_camera_up");
  GLint camRightLoc = glGetUniformLocation(shaderProgram, "u_camera_right");

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // --- Camera State ---
  float cam_pos[3] = {0.0f, 0.0f, 0.0f};
  float cam_front[3] = {0.0f, 0.0f, 1.0f};
  float cam_up[3] = {0.0f, 1.0f, 0.0f};
  float cam_right[3] = {1.0f, 0.0f, 0.0f};
  float world_up[3] = {0.0f, 1.0f, 0.0f};

  float yaw = 90.0f; // 90 degrees faces +Z in our setup
  float pitch = 0.0f;
  float cam_speed = 5.0f;
  float mouse_sensitivity = 0.1f;

  // Lock the mouse to the window for FPS controls
  SDL_SetWindowRelativeMouseMode(window, true);

  Uint64 last_time = SDL_GetTicks();
  bool running = true;

  while (running) {
    // Delta Time Calculation
    Uint64 current_time = SDL_GetTicks();
    float dt = (current_time - last_time) / 1000.0f;
    last_time = current_time;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        running = false;

      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        running = false; // ESC to quit easily since mouse is locked
      }

      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE) {
        if (world.object_count < 100) {
          int i = world.object_count;
          world.objects[i].position_radius[0] = ((rand() % 100) / 10.0f) - 5.0f;
          world.objects[i].position_radius[1] = ((rand() % 50) / 10.0f);
          world.objects[i].position_radius[2] = 3.0f + ((rand() % 50) / 10.0f);
          world.objects[i].position_radius[3] = 0.5f;
          world.objects[i].color_type[0] = (rand() % 100) / 100.0f;
          world.objects[i].color_type[1] = (rand() % 100) / 100.0f;
          world.objects[i].color_type[2] = (rand() % 100) / 100.0f;
          world.objects[i].color_type[3] = 0.0f;
          world.object_count++;

          glBindBuffer(GL_UNIFORM_BUFFER, ubo);
          glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(WorldData), &world);
        }
      }

      // Mouse Look
      if (event.type == SDL_EVENT_MOUSE_MOTION) {
        yaw += event.motion.xrel * mouse_sensitivity;
        pitch -= event.motion.yrel * mouse_sensitivity;

        // Constrain pitch to avoid screen flipping
        if (pitch > 89.0f)
          pitch = 89.0f;
        if (pitch < -89.0f)
          pitch = -89.0f;
      }
    }

    // --- Calculate New Camera Vectors ---
    float rad_yaw = yaw * M_PI / 180.0f;
    float rad_pitch = pitch * M_PI / 180.0f;

    cam_front[0] = cosf(rad_yaw) * cosf(rad_pitch);
    cam_front[1] = sinf(rad_pitch);
    cam_front[2] = sinf(rad_yaw) * cosf(rad_pitch);
    normalize(cam_front);

    cross(cam_front, world_up, cam_right);
    normalize(cam_right);

    cross(cam_right, cam_front, cam_up);
    normalize(cam_up);

    // --- Keyboard Movement (Smooth continuous input) ---
    const bool *state = SDL_GetKeyboardState(NULL);
    float velocity = cam_speed * dt;

    if (state[SDL_SCANCODE_W]) {
      cam_pos[0] += cam_front[0] * velocity;
      cam_pos[1] += cam_front[1] * velocity;
      cam_pos[2] += cam_front[2] * velocity;
    }
    if (state[SDL_SCANCODE_S]) {
      cam_pos[0] -= cam_front[0] * velocity;
      cam_pos[1] -= cam_front[1] * velocity;
      cam_pos[2] -= cam_front[2] * velocity;
    }
    if (state[SDL_SCANCODE_A]) {
      cam_pos[0] -= cam_right[0] * velocity;
      cam_pos[1] -= cam_right[1] * velocity;
      cam_pos[2] -= cam_right[2] * velocity;
    }
    if (state[SDL_SCANCODE_D]) {
      cam_pos[0] += cam_right[0] * velocity;
      cam_pos[1] += cam_right[1] * velocity;
      cam_pos[2] += cam_right[2] * velocity;
    }
    if (state[SDL_SCANCODE_E]) { // Ascend
      cam_pos[1] += velocity;
    }
    if (state[SDL_SCANCODE_Q]) { // Descend
      cam_pos[1] -= velocity;
    }

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glUseProgram(shaderProgram);
    glUniform2f(resLoc, (float)w, (float)h);
    glUniform3f(camPosLoc, cam_pos[0], cam_pos[1], cam_pos[2]);
    glUniform3f(camFrontLoc, cam_front[0], cam_front[1], cam_front[2]);
    glUniform3f(camUpLoc, cam_up[0], cam_up[1], cam_up[2]);
    glUniform3f(camRightLoc, cam_right[0], cam_right[1], cam_right[2]);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}