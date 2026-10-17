#version 330 core
// Bloom downsample: 13-tap Jimenez filter (SIGGRAPH 2014). On the first
// level (u_First == 1) the scene HDR is clamped, soft-knee thresholded and
// Karis-weighted (firefly suppression); the later levels are plain
// downsamples of the already-extracted highlights.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Source;
uniform vec2 u_Texel;       // 1 / source resolution
uniform int u_First;        // 1 = scene -> highlights, 0 = further mips
uniform float u_Threshold;  // soft-knee start (linear HDR)
uniform float u_Knee;       // soft-knee half-width

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// Threshold + clamp on the first level; plain read afterwards. The branch
// is uniform (u_First), so texture() stays in uniform control flow.
vec3 fetch(vec2 uv) {
  vec3 c = texture(u_Source, uv).rgb;
  if (u_First == 1) {
    c = clamp(c, vec3(0.0), vec3(64.0));  // raymarch NaN/Inf guard
    float l = luma(c);
    float rq = clamp(l - (u_Threshold - u_Knee), 0.0, 2.0 * u_Knee);
    rq = rq * rq / (4.0 * u_Knee + 1e-5);
    float contrib = max(rq, l - u_Threshold) / max(l, 1e-5);
    c *= contrib;
  }
  return c;
}

void main() {
  vec2 uv = v_TexCoord;
  vec2 t = u_Texel;
  vec3 a = fetch(uv + vec2(-2.0, 2.0) * t);
  vec3 b = fetch(uv + vec2(0.0, 2.0) * t);
  vec3 c = fetch(uv + vec2(2.0, 2.0) * t);
  vec3 d = fetch(uv + vec2(-2.0, 0.0) * t);
  vec3 e = fetch(uv);
  vec3 f = fetch(uv + vec2(2.0, 0.0) * t);
  vec3 g = fetch(uv + vec2(-2.0, -2.0) * t);
  vec3 h = fetch(uv + vec2(0.0, -2.0) * t);
  vec3 i = fetch(uv + vec2(2.0, -2.0) * t);
  vec3 j = fetch(uv + vec2(-1.0, 1.0) * t);
  vec3 k = fetch(uv + vec2(1.0, 1.0) * t);
  vec3 l = fetch(uv + vec2(-1.0, -1.0) * t);
  vec3 m = fetch(uv + vec2(1.0, -1.0) * t);
  vec3 col;
  if (u_First == 1) {
    // Karis-average the four near-tap groups (0.25 / (1 + luma) weights);
    // far groups keep the plain kernel weights.
    float wj = 1.0 / (1.0 + luma(j));
    float wk = 1.0 / (1.0 + luma(k));
    float wl = 1.0 / (1.0 + luma(l));
    float wm = 1.0 / (1.0 + luma(m));
    vec3 jk = (j * wj + k * wk) / max(wj + wk, 1e-5);
    vec3 lm = (l * wl + m * wm) / max(wl + wm, 1e-5);
    col = e * 0.125 + (a + c + g + i) * 0.03125 +
          (b + d + f + h) * 0.0625 + (jk + lm) * 0.25;
  } else {
    col = e * 0.125 + (a + c + g + i) * 0.03125 +
          (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
  }
  FragColor = vec4(col, 1.0);
}
