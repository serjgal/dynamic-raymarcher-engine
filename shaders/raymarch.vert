#version 330 core

// Generates a full-screen triangle without needing Vertex Buffer Objects in C
void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    gl_Position = vec4(x, y, 0.0, 1.0);
}