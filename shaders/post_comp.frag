#version 330 core
// PostFX composite: scene + weighted bloom levels + relativistic Doppler.
// u_BloomStrength == 0.0 => pixel-exact passthrough (stage 2a).
// u_Beta == 0.0 (harness / standing still) => tint block skipped,
// output stays byte-exact.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom0;
uniform sampler2D u_Bloom1;
uniform sampler2D u_Bloom2;
uniform sampler2D u_Bloom3;
uniform sampler2D u_Bloom4;
uniform float u_BloomStrength;

// === Relativity === observer velocity: Doppler tint per view ray.
// Ahead of the motion -> blue + brighter, behind -> red + dimmer
// (relativistic beaming). Zero when the camera stands still.
uniform vec3 u_CamFwd;
uniform vec3 u_CamRight;
uniform vec3 u_CamUp;
uniform float u_FovY;
uniform float u_Aspect;
uniform vec3 u_VelDir;
uniform float u_Beta;

void main() {
  vec4 sceneT = texelFetch(u_Scene, ivec2(gl_FragCoord.xy), 0);
  vec3 bloom = texture(u_Bloom0, v_TexCoord).rgb * 0.30 +
               texture(u_Bloom1, v_TexCoord).rgb * 0.25 +
               texture(u_Bloom2, v_TexCoord).rgb * 0.20 +
               texture(u_Bloom3, v_TexCoord).rgb * 0.15 +
               texture(u_Bloom4, v_TexCoord).rgb * 0.10;
  vec3 col = sceneT.rgb + bloom * u_BloomStrength;
  if (u_Beta > 0.001) {
    vec2 ndc = v_TexCoord * 2.0 - 1.0;
    float t = tan(u_FovY * 0.5);
    vec3 ray = normalize(u_CamFwd + ndc.x * u_Aspect * t * u_CamRight +
                         ndc.y * t * u_CamUp);
    // s in (-beta, +beta): > 0 towards the motion (blueshift),
    // < 0 behind the motion (redshift).
    float s = u_Beta * dot(ray, u_VelDir);
    vec3 tint = s > 0.0 ? mix(vec3(1.0), vec3(0.72, 0.88, 1.45), s)
                        : mix(vec3(1.0), vec3(1.55, 0.62, 0.45), -s);
    float gain = 1.0 + 0.6 * s;  // beaming: front brighter, rear dimmer
    col *= tint * gain;
  }
  FragColor = vec4(col, sceneT.a);
}
