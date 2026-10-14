#version 330 core
// Separable gaussian blur: 5-tap linear-sampled gaussian (weights
// 0.227027 / 0.316216 / 0.070270 at offsets 0, +-1.384615, +-3.230769).
// u_Dir is the texel step along the blur axis (H then V per level).
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Source;
uniform vec2 u_Dir;

void main() {
  vec2 uv = v_TexCoord;
  vec3 c = texture(u_Source, uv).rgb * 0.227027;
  c += texture(u_Source, uv + u_Dir * 1.384615).rgb * 0.316216;
  c += texture(u_Source, uv - u_Dir * 1.384615).rgb * 0.316216;
  c += texture(u_Source, uv + u_Dir * 3.230769).rgb * 0.070270;
  c += texture(u_Source, uv - u_Dir * 3.230769).rgb * 0.070270;
  FragColor = vec4(c, 1.0);
}
