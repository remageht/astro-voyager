#version 330 core
// Constellation lines: simple colored polyline, MVP.
layout(location = 0) in vec3 position;
uniform mat4 u_MVP;
out vec3 v_Color;
uniform vec3 u_LineColor;
void main() {
  v_Color = u_LineColor;
  gl_Position = u_MVP * vec4(position, 1.0);
}
