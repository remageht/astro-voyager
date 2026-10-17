#version 330 core
// Bloom upsample: 9-tap tent filter, drawn additively into the next-higher
// mip so level 0 ends up holding the whole pyramid (Jimenez 2014).
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Source;
uniform vec2 u_Texel;  // 1 / destination resolution

void main() {
  vec2 uv = v_TexCoord;
  vec2 t = u_Texel;
  vec3 col;
  col = texture(u_Source, uv + vec2(-1.0, -1.0) * t).rgb;
  col += texture(u_Source, uv + vec2(0.0, -1.0) * t).rgb * 2.0;
  col += texture(u_Source, uv + vec2(1.0, -1.0) * t).rgb;
  col += texture(u_Source, uv + vec2(-1.0, 0.0) * t).rgb * 2.0;
  col += texture(u_Source, uv).rgb * 4.0;
  col += texture(u_Source, uv + vec2(1.0, 0.0) * t).rgb * 2.0;
  col += texture(u_Source, uv + vec2(-1.0, 1.0) * t).rgb;
  col += texture(u_Source, uv + vec2(0.0, 1.0) * t).rgb * 2.0;
  col += texture(u_Source, uv + vec2(1.0, 1.0) * t).rgb;
  FragColor = vec4(col / 16.0, 1.0);
}
