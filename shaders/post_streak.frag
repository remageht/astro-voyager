#version 330 core
// Anamorphic streak: a wide horizontal smear of a coarse bloom level,
// tinted and added in the composite (aspect-driven lens artefact).
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Source;
uniform vec2 u_Texel;

void main() {
  vec3 sum = vec3(0.0);
  float wsum = 0.0;
  for (int i = -5; i <= 5; ++i) {
    float fi = float(i);
    float w = exp(-fi * fi * 0.14);
    sum += texture(u_Source,
                   v_TexCoord + vec2(fi * 3.5 * u_Texel.x, 0.0)).rgb * w;
    wsum += w;
  }
  FragColor = vec4(sum / wsum, 1.0);
}
