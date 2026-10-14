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
// v3.0.0: ACES + gamma moved to post_comp.frag; this shader outputs linear HDR.
void main() {
  float lx = v_Local.x * 0.5 + 0.5;
  float brightness = pow(1.0 - lx, 2.0);
  float rr = length(v_Local.yz);
  float body = 1.0 - smoothstep(0.0, 1.0, rr);
  vec3 col = u_ColA * (0.15 + 2.2 * brightness * body);
  float alpha = clamp(brightness * body * 1.2, 0.0, 1.0);
  col *= 1.1 * u_Exposure;
  FragColor = vec4(max(col, vec3(0.0)), alpha);
}
