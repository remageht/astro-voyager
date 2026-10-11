#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
uniform mat4 u_MVP;
uniform mat4 u_Model;
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec3 v_Local;
void main() {
  v_Local = position;
  vec4 wp = u_Model * vec4(position, 1.0);
  v_WorldPos = wp.xyz;
  v_Normal = mat3(u_Model) * normal;
  gl_Position = u_MVP * u_Model * vec4(position, 1.0);
}
