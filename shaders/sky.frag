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
uniform float u_Exposure;
uniform float u_BandK;  // milky-way band density/brightness gain (default 1.0)
// === Relativity === aberration of star positions at high speed:
// apparent directions compress towards u_VelDir (cos' = (cos+b)/(1+b*cos)).
// u_Beta == 0.0 keeps dir untouched (byte-exact harness).
uniform vec3 u_VelDir;
uniform float u_Beta;
// v3.0.0: ACES + gamma moved to post_comp.frag; this shader outputs linear HDR.
void main() {
  vec2 uv = v_TexCoord;
  float t = tan(u_FovY * 0.5);
  vec3 dir = normalize(u_CamFwd + (uv.x * 2.0 - 1.0) * u_Aspect * t * u_CamRight + (uv.y * 2.0 - 1.0) * t * u_CamUp);
  if (u_Beta > 0.001) {
    float c = dot(dir, u_VelDir);
    float c2 = clamp((c + u_Beta) / (1.0 + u_Beta * c), -1.0, 1.0);
    vec3 tang = dir - u_VelDir * c;
    float tl = length(tang);
    if (tl > 1e-5) {
      float s2 = sqrt(max(1.0 - c2 * c2, 0.0));
      dir = normalize(u_VelDir * c2 + tang * (s2 / tl));
    } else {
      dir = u_VelDir * (c2 < 0.0 ? -1.0 : 1.0);
    }
  }
  vec3 col = texture(u_Sky, dir).rgb;
  // Milky-Way band gain: boost density/brightness of the band and the stars
  // along it (band plane dir.y ~ 0, same profile as the cubemap).
  // Neutral when the host leaves u_BandK at 0.0 or sets 1.0 (sgra uses 1.7).
  float bandW = exp(-abs(dir.y) * 5.0);
  col *= 1.0 + max(u_BandK - 1.0, 0.0) * bandW;
  col *= 1.1 * u_Exposure;
  FragColor = vec4(max(col, vec3(0.0)), 1.0);
}
