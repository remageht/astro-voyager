#version 330 core
in vec3 v_Color;
out vec4 FragColor;
uniform float u_Time;

void main() {
  // Gentle figure glow pulse. Unset u_Time defaults to 0.0 -> exactly 1.0,
  // so stations that never set it (sol orbits/belt) render bit-identical.
  float pulse = 1.0 + 0.10 * sin(u_Time * 1.5);
  FragColor = vec4(v_Color * pulse, 1.0);
}
