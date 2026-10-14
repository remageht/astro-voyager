#version 330 core
// Solid warp-ship hull shading: key light + ambient + cool fresnel rim.
in vec3 v_Normal;
in vec3 v_WorldPos;
out vec4 FragColor;
uniform vec3 u_CamPos;
uniform vec3 u_BaseColor;
void main() {
  vec3 n = normalize(v_Normal);
  vec3 L = normalize(vec3(0.42, 0.70, 0.56));
  float ndl = max(dot(n, L), 0.0);
  vec3 V = normalize(u_CamPos - v_WorldPos);
  float fres = pow(1.0 - max(dot(n, V), 0.0), 3.0);
  vec3 col = u_BaseColor * (0.22 + 0.78 * ndl);
  col += vec3(0.45, 0.62, 0.95) * fres * 0.40;
  // v3.0.0: hull art is display-referred; decode to linear HDR.
  FragColor = vec4(pow(max(col, vec3(0.0)), vec3(2.2)), 1.0);
}
