#version 330 core
// Sky background: cubemap lookup by reconstructed view dir + ACES.
in vec2 v_TexCoord;
out vec4 FragColor;
uniform vec3 u_CamFwd;
uniform vec3 u_CamRight;
uniform vec3 u_CamUp;
uniform float u_Aspect;
uniform float u_FovY;
uniform samplerCube u_Sky;
vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
  vec2 uv = v_TexCoord;
  float t = tan(u_FovY * 0.5);
  vec3 dir = normalize(u_CamFwd + (uv.x * 2.0 - 1.0) * u_Aspect * t * u_CamRight + (uv.y * 2.0 - 1.0) * t * u_CamUp);
  vec3 col = texture(u_Sky, dir).rgb;
  col *= 1.1;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
  FragColor = vec4(col, 1.0);
}
