#version 330 core
// Solid warp-ship hull: ellipsoid parts with lambert + fresnel rim.
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 a_Aux;  // sphere: normal; ring: a_R (unused)
uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform vec3 u_InvScale;   // inverse part scale for correct ellipsoid normals
uniform int u_FlatNormal;  // 1 = ring: local normal +Y
out vec3 v_Normal;
out vec3 v_WorldPos;
void main() {
  vec4 wp = u_Model * vec4(position, 1.0);
  v_WorldPos = wp.xyz;
  vec3 n = (u_FlatNormal == 1) ? vec3(0.0, 1.0, 0.0)
                                : a_Aux * u_InvScale * u_InvScale;
  v_Normal = mat3(u_Model) * n;
  gl_Position = u_MVP * wp;
}
