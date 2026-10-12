#version 330 core
// Comet ion tail: additive ellipsoid, bright head at -X, blue tail to +X.
// Reuses planet.vert (varyings v_WorldPos/v_Normal/v_Local, unit sphere).
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Local;
out vec4 FragColor;
uniform float u_Seed;
uniform vec3 u_ColA;
uniform float u_Exposure;
vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
  float lx = v_Local.x * 0.5 + 0.5;
  float brightness = pow(1.0 - lx, 2.0);
  float rr = length(v_Local.yz);
  float body = 1.0 - smoothstep(0.0, 1.0, rr);
  vec3 col = u_ColA * (0.15 + 2.2 * brightness * body);
  float alpha = clamp(brightness * body * 1.2, 0.0, 1.0);
  col *= 1.1 * u_Exposure;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
  FragColor = vec4(col, alpha);
}
