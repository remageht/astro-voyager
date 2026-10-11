#version 330 core
// Constellation star sprite: soft round billboard, color by magnitude.
in float v_Mag;
in float v_Bright;
out vec4 FragColor;
void main() {
  float d = length(gl_PointCoord - vec2(0.5)) * 2.0;
  float alpha = 1.0 - smoothstep(0.15, 1.0, d);
  float core = 1.0 - smoothstep(0.0, 0.45, d);
  vec3 bright = vec3(0.75, 0.85, 1.0);
  vec3 mid = vec3(1.0, 0.97, 0.92);
  vec3 dim = vec3(1.0, 0.75, 0.5);
  vec3 starCol = mix(bright, mid, smoothstep(1.0, 2.0, v_Mag));
  starCol = mix(starCol, dim, smoothstep(3.0, 4.5, v_Mag));
  vec3 col = starCol * (0.35 + v_Bright) + vec3(1.0) * core * v_Bright * 0.8;
  float a = clamp(alpha * (0.35 + v_Bright), 0.0, 1.0);
  FragColor = vec4(col, a);
}
