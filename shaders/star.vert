#version 330 core
// Constellation star billboards: size by magnitude, perspective scale.
layout(location = 0) in vec3 position;
layout(location = 1) in float a_Mag;
uniform mat4 u_MVP;
uniform mat4 u_MV;
uniform float u_PixelScale;
uniform float u_SizeK;
uniform float u_Time;
out float v_Mag;
out float v_Bright;
out float v_Size;
out float v_Tw;
void main() {
  vec4 mv = u_MV * vec4(position, 1.0);
  float dist = max(-mv.z, 0.001);
  float worldSize = u_SizeK * pow(2.0, -0.2 * (a_Mag - 1.0));
  gl_PointSize = clamp(u_PixelScale * worldSize / dist, 1.0, 90.0);
  gl_Position = u_MVP * vec4(position, 1.0);
  v_Mag = a_Mag;
  v_Bright = pow(2.0, -0.4 * (a_Mag - 1.0));
  v_Size = gl_PointSize;
  // Twinkle: each star has its own phase from its position hash.
  // Unset u_Time defaults to 0.0, so frames stay deterministic.
  float twPhase = fract(sin(dot(position.xy + position.zx, vec2(12.9898, 78.233))) * 43758.5453) * 6.28318;
  v_Tw = 0.82 + 0.18 * sin(u_Time * 3.0 + twPhase);
}
