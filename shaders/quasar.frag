#version 330 core
// Quasar impostor: bright core + thin disk + two jets, additive feel.
// Cheap (no GR), used for station qso-3c273.
out vec4 FragColor;
in vec2 v_TexCoord;
uniform vec3 u_CoreColor;
uniform float u_Time;
void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  float r = length(uv);
  float core = exp(-r * r * 18.0) * 2.2;
  float disk = exp(-abs(uv.y) * 22.0) * exp(-r * 2.5);
  float jet = exp(-abs(uv.x) * 30.0) * exp(-r * 1.5) * 0.6;
  float flick = 0.95 + 0.05 * sin(u_Time * 3.0 + r * 10.0);
  vec3 col = (core * u_CoreColor + disk * vec3(0.7, 0.8, 1.0) +
              jet * vec3(0.4, 0.6, 1.0)) * flick;
  FragColor = vec4(col, 1.0);
}
