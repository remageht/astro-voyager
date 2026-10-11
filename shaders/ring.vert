#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in float a_R;
uniform mat4 u_MVP;
uniform mat4 u_Model;
out float v_R01;
out vec3 v_WorldPos;
void main() {
  v_R01 = a_R;
  vec4 wp = u_Model * vec4(position, 1.0);
  v_WorldPos = wp.xyz;
  gl_Position = u_MVP * u_Model * vec4(position, 1.0);
}
