#version 330 core
// Bright pass + downsample: u_Threshold > 0 extracts highlights (first
// level), u_Threshold == 0 is a plain bilinear box downsample (next levels).
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Source;
uniform float u_Threshold;

void main() {
  vec3 c = texture(u_Source, v_TexCoord).rgb;
  vec3 b = max(c - u_Threshold, vec3(0.0));
  FragColor = vec4(b, 1.0);
}
