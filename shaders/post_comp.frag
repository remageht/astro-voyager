#version 330 core
// PostFX composite: scene + weighted bloom levels.
// u_BloomStrength == 0.0 => pixel-exact passthrough (stage 2a).
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom0;
uniform sampler2D u_Bloom1;
uniform sampler2D u_Bloom2;
uniform sampler2D u_Bloom3;
uniform sampler2D u_Bloom4;
uniform float u_BloomStrength;

void main() {
  vec4 sceneT = texelFetch(u_Scene, ivec2(gl_FragCoord.xy), 0);
  vec3 bloom = texture(u_Bloom0, v_TexCoord).rgb * 0.30 +
               texture(u_Bloom1, v_TexCoord).rgb * 0.25 +
               texture(u_Bloom2, v_TexCoord).rgb * 0.20 +
               texture(u_Bloom3, v_TexCoord).rgb * 0.15 +
               texture(u_Bloom4, v_TexCoord).rgb * 0.10;
  vec3 col = sceneT.rgb + bloom * u_BloomStrength;
  FragColor = vec4(col, sceneT.a);
}
