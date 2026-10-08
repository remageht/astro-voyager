#version 330 core
// M1 Crab Nebula Supernova Remnant impostor sprite (GLSL 330)
// Complex filamentary expanding gas shell with synchrotron radiation core.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;

float hash2(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float valueNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float a = hash2(i);
  float b = hash2(i + vec2(1.0, 0.0));
  float c = hash2(i + vec2(0.0, 1.0));
  float d = hash2(i + vec2(1.0, 1.0));
  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  mat2 rot = mat2(0.8, -0.6, 0.6, 0.8);
  for (int i = 0; i < 4; ++i) {
    v += a * valueNoise(p);
    p = rot * p * 2.0;
    a *= 0.5;
  }
  return v;
}

void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  uv.x *= u_Aspect;

  vec2 p = vec2(uv.x * 1.25, uv.y * 0.95);
  float r = length(p);

  // Central synchrotron glow (bluish continuum from Crab pulsar)
  float pulsar = exp(-r * 8.0) * 2.2;
  float syncCore = exp(-r * 3.0) * 0.9;

  // Turbulent filaments
  float f = fbm(p * 5.0 + vec2(u_Time * 0.015, 0.0));
  float filaments = smoothstep(0.4, 0.7, f) * smoothstep(0.85, 0.15, r) * 1.6;
  float outerFibers = smoothstep(0.48, 0.78, fbm(p * 9.0)) * smoothstep(0.9, 0.25, r) * 1.3;

  vec3 syncCol = vec3(0.35, 0.7, 0.98);
  vec3 hAlphaCol = vec3(0.95, 0.28, 0.18);
  vec3 o3Col = vec3(0.18, 0.88, 0.75);

  vec3 col = pulsar * vec3(1.0, 0.98, 0.9) +
             syncCore * syncCol +
             filaments * hAlphaCol +
             outerFibers * o3Col;

  FragColor = vec4(col, 1.0);
}
