#version 330 core
// Constellation star sprite: soft round billboard, color by magnitude.
in float v_Mag;
in float v_Bright;
in float v_Size;
in float v_Tw;
out vec4 FragColor;
float hash12(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}
float vnoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = hash12(i);
  float b = hash12(i + vec2(1.0, 0.0));
  float c = hash12(i + vec2(0.0, 1.0));
  float d = hash12(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
void main() {
  vec3 bright = vec3(0.60, 0.74, 1.0);
  vec3 mid = vec3(1.0, 0.95, 0.88);
  vec3 dim = vec3(1.0, 0.62, 0.34);
  vec3 starCol = mix(bright, mid, smoothstep(1.0, 2.0, v_Mag));
  starCol = mix(starCol, dim, smoothstep(3.0, 4.5, v_Mag));
  float d = length(gl_PointCoord - vec2(0.5)) * 2.0;
  if (v_Size > 36.0) {
    float disc = 1.0 - smoothstep(0.85, 1.0, d);
    float limb = 0.45 + 0.55 * sqrt(max(1.0 - d * d, 0.0));
    vec2 gp = gl_PointCoord * 6.0 + vec2(v_Mag * 13.7, v_Mag * 7.3);
    float g = 0.65 * vnoise(gp) + 0.35 * vnoise(gp * 2.03 + vec2(11.7, 5.3));
    vec3 col = starCol * (0.75 + 0.5 * g) * limb * 2.2 * disc;
    float t = (d - 1.05) * 6.0;
    float ring = exp(-t * t);
    col += starCol * ring * 0.8;
    col *= v_Tw;
    float a = clamp(disc + ring * 0.5, 0.0, 1.0);
    FragColor = vec4(col, a);
  } else {
    float alpha = 1.0 - smoothstep(0.15, 1.0, d);
    float core = 1.0 - smoothstep(0.0, 0.45, d);
    vec3 col = starCol * (0.35 + v_Bright) + vec3(1.0) * core * v_Bright * 0.8;
    col *= v_Tw;
    float a = clamp(alpha * (0.35 + v_Bright), 0.0, 1.0);
    FragColor = vec4(col, a);
  }
}
