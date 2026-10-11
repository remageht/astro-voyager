#version 330 core
// Saturn-like ring: radial stripes + Cassini gap. Normal blending.
in float v_R01;
out vec4 FragColor;
uniform float u_Seed;
uniform vec3 u_ColA;
float hash12(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}
float vnoise2(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = hash12(i);
  float b = hash12(i + vec2(1.0, 0.0));
  float c = hash12(i + vec2(0.0, 1.0));
  float d = hash12(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float fbm3(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 3; ++i) {
    v += a * vnoise2(p);
    s += a;
    p *= 2.03;
    a *= 0.5;
  }
  return v / max(s, 0.0001);
}
void main() {
  float stripes = fbm3(vec2(v_R01 * 24.0, u_Seed));
  float edge = (1.0 - smoothstep(0.0, 0.06, v_R01)) * (1.0 - smoothstep(0.94, 1.0, v_R01));
  float alpha = stripes * edge * 0.85;
  float gap = abs(v_R01 - 0.65);
  alpha *= 1.0 - 0.9 * (1.0 - smoothstep(0.0, 0.05, gap));
  alpha = clamp(alpha, 0.0, 1.0);
  vec3 col = u_ColA * (0.55 + 0.45 * stripes);
  FragColor = vec4(col, alpha);
}
