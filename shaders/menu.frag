#version 330 core
// menu.frag - main menu backdrop (v3.5.0): "live cosmos" behind the ImGui
// panels. Generated deep-space scene independent of the current game
// station (a fresh boot and a loaded profile get the same hero look):
// slow-drifting nebula wisps, a dust-lane Milky Way band, and a two-tier
// parallax starfield with per-star twinkle. Outputs linear-ish color,
// tone curve is part of the palette constants below.

in vec2 v_TexCoord;
out vec4 fragColor;

uniform float u_Time;
uniform vec2 u_Res;

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
  float amp = 0.55;
  for (int i = 0; i < 4; ++i) {
    v += amp * valueNoise(p);
    p = p * 2.03 + vec2(11.0, 7.0);
    amp *= 0.5;
  }
  return v;
}

// Two-tier sparse star layer (sin-based lattice hash, same v3.5.0 fix as
// pulsar/quasar/nebula: plain fract-style lattice hash clusters star
// offsets into visible dotted columns).
vec3 starLayer(vec2 uv, float scale, float thresh, float gain, float t) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  float h0 = hash2(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash2(id + vec2(7.13, 1.37)), hash2(id + vec2(3.71, 9.41))) - vec2(0.5);
  float d = length(f - off * 0.9);
  float core = exp(-d * d * 300.0);
  float halo = exp(-d * d * 34.0) * 0.06;
  float h1 = hash2(id + vec2(9.17, 2.53));
  float faintTier = pow(h1, 3.0);
  float brightTier = pow(h1, 10.0);
  float tw = 0.75 + 0.25 * sin(t * (1.5 + 2.5 * h1) + h1 * 43.0);
  vec3 tint = mix(vec3(0.65, 0.75, 1.0), vec3(1.0, 0.85, 0.65), hash2(id + vec2(5.31, 0.71)));
  float mag = faintTier * 0.05 + brightTier * 1.35;
  return tint * mask * tw * gain * (core * mag + halo);
}

vec3 mwBand(vec2 uv, float t) {
  float ang = -0.42;
  vec2 r = mat2(cos(ang), -sin(ang), sin(ang), cos(ang)) * uv;
  float bandY = r.y;
  float shape = exp(-bandY * bandY * 9.0);
  float wisps = fbm(r * vec2(3.0, 9.0) + vec2(t * 0.02, 0.0));
  float dust = smoothstep(0.55, 0.75, fbm(r * vec2(4.0, 14.0) + vec2(3.7, t * 0.01)));
  float cl = shape * (0.55 + 0.45 * wisps) * (1.0 - 0.72 * dust);
  vec3 col = mix(vec3(0.20, 0.24, 0.34), vec3(0.38, 0.31, 0.42), wisps);
  return col * cl * 0.60;
}

vec3 nebula(vec2 uv, float t) {
  float n1 = fbm(uv * 2.2 + vec2(t * 0.012, -t * 0.006));
  float n2 = fbm(uv * 4.5 + vec2(-t * 0.008, t * 0.010) + vec2(5.3, 1.1));
  vec3 teal = vec3(0.030, 0.105, 0.125);
  vec3 violet = vec3(0.095, 0.055, 0.165);
  vec3 warm = vec3(0.125, 0.070, 0.050);
  vec3 col = mix(teal, violet, smoothstep(0.35, 0.75, n1));
  col = mix(col, warm, smoothstep(0.55, 0.90, n2) * 0.5);
  float gate = smoothstep(0.30, 0.72, n1) * smoothstep(0.22, 0.60, n2);
  return col * gate * 0.9;
}

void main() {
  float aspect = u_Res.x / max(u_Res.y, 1.0);
  vec2 uv = (v_TexCoord - vec2(0.5)) * vec2(aspect, 1.0);
  float t = u_Time;

  vec3 col = vec3(0.004, 0.006, 0.012);
  // Vertical depth gradient: slightly bluer glow toward the top.
  col += vec3(0.006, 0.008, 0.018) * smoothstep(0.6, -0.55, v_TexCoord.y * 2.0 - 1.0);

  col += nebula(uv, t);
  col += mwBand(uv, t);

  // Parallax star layers: different drift speeds and scales.
  vec2 s1 = uv + vec2(t * 0.004, t * 0.0015);
  vec2 s2 = uv + vec2(t * 0.010, -t * 0.004) + vec2(17.0, 3.0);
  vec2 s3 = uv + vec2(t * 0.018, t * 0.005) + vec2(41.0, 11.0);
  col += starLayer(s1, 38.0, 0.90, 0.9, t);
  col += starLayer(s2, 72.0, 0.93, 0.55, t);
  col += starLayer(s3, 130.0, 0.96, 0.30, t);

  // Soft vignette: keeps panel centers readable, edges cinematic.
  float vig = 1.0 - 0.16 * dot(uv, uv);
  col *= vig;

  fragColor = vec4(col, 1.0);
}
