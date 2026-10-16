#version 330 core
// PostFX composite: scene (HDR-linear float16) + weighted bloom levels,
// then ACES + gamma + grain; relativistic Doppler tint when flying.
// u_BloomStrength == 0.0 => tonemapped scene only (stage 2a no longer
// pixel-exact: tonemap moved here from the station shaders in v3.0.0).
// u_Beta == 0.0 (harness / standing still) => tint block skipped.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom0;
uniform sampler2D u_Bloom1;
uniform sampler2D u_Bloom2;
uniform sampler2D u_Bloom3;
uniform sampler2D u_Bloom4;
uniform float u_BloomStrength;
uniform float u_Time;  // sim time for grain (fixed by the harness => PNG stable)

vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
float compHash(vec2 p) {
  return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

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

// === Ship survival === (v3.3.0): low-shield screen effects (0 = exact pass).
uniform float u_ChromaK;  // chromatic aberration amount 0..1
uniform float u_StaticK;  // interference: noise + vignette 0..1

void main() {
  vec4 sceneT = texelFetch(u_Scene, ivec2(gl_FragCoord.xy), 0);
  vec3 bloom = texture(u_Bloom0, v_TexCoord).rgb * 0.30 +
               texture(u_Bloom1, v_TexCoord).rgb * 0.25 +
               texture(u_Bloom2, v_TexCoord).rgb * 0.20 +
               texture(u_Bloom3, v_TexCoord).rgb * 0.15 +
               texture(u_Bloom4, v_TexCoord).rgb * 0.10;
  vec3 col = sceneT.rgb + bloom * u_BloomStrength;
  if (u_ChromaK > 0.0001) {
    // texture() lives only inside this branch: the amount==0 path must stay
    // texelFetch-exact (no filtering, no extra samples).
    vec2 cabOff = (v_TexCoord - 0.5) * 0.012 * u_ChromaK;
    col = vec3(texture(u_Scene, v_TexCoord + cabOff).r, col.g,
               texture(u_Scene, v_TexCoord - cabOff).b);
  }
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
  // Single tonemap for the whole frame (moved here from station shaders).
  col = acesNarkowicz(col);
  col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
  // Single post-tonemap grain (replaces the per-shader grains).
  col += (compHash(gl_FragCoord.xy + fract(u_Time) * 100.0) - 0.5) * 0.012;
  if (u_StaticK > 0.0001) {
    // Interference wash + edge vignette (loops in the existing compHash).
    float sN = compHash(gl_FragCoord.xy * 0.371 + fract(u_Time) * 97.0);
    float sVig = smoothstep(0.35, 0.85, length(v_TexCoord - 0.5));
    col = mix(col, vec3(sN) * 0.5, u_StaticK * 0.5);
    col *= 1.0 - 0.65 * sVig * u_StaticK;
  }
  FragColor = vec4(col, sceneT.a);
}
