#version 330 core
// Planet surface: sun=0 gas=1 rocky=2 ice=3. HDR linear + ACES 1.1 + gamma.
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Local;
out vec4 FragColor;
uniform int u_Type;
uniform float u_Seed;
uniform vec3 u_ColA;
uniform vec3 u_ColB;
uniform float u_Time;
uniform vec3 u_CamPos;
uniform float u_Spot;
uniform float u_Clouds;
float hash13(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.11, 0.17, 0.13));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float vnoise3(vec3 p) {
  vec3 i = floor(p);
  vec3 f = fract(p);
  vec3 u = f * f * (3.0 - 2.0 * f);
  float n000 = hash13(i + vec3(0.0, 0.0, 0.0));
  float n100 = hash13(i + vec3(1.0, 0.0, 0.0));
  float n010 = hash13(i + vec3(0.0, 1.0, 0.0));
  float n110 = hash13(i + vec3(1.0, 1.0, 0.0));
  float n001 = hash13(i + vec3(0.0, 0.0, 1.0));
  float n101 = hash13(i + vec3(1.0, 0.0, 1.0));
  float n011 = hash13(i + vec3(0.0, 1.0, 1.0));
  float n111 = hash13(i + vec3(1.0, 1.0, 1.0));
  float nx00 = mix(n000, n100, u.x);
  float nx10 = mix(n010, n110, u.x);
  float nx01 = mix(n001, n101, u.x);
  float nx11 = mix(n011, n111, u.x);
  float nxy0 = mix(nx00, nx10, u.y);
  float nxy1 = mix(nx01, nx11, u.y);
  return mix(nxy0, nxy1, u.z);
}
float fbm4(vec3 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 4; ++i) {
    v += a * vnoise3(p);
    s += a;
    p *= 2.03;
    a *= 0.5;
  }
  return v / max(s, 0.0001);
}
float fbm3(vec3 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 3; ++i) {
    v += a * vnoise3(p);
    s += a;
    p *= 2.03;
    a *= 0.5;
  }
  return v / max(s, 0.0001);
}
vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
  vec3 n = normalize(v_Normal);
  vec3 viewDir = normalize(u_CamPos - v_WorldPos);
  vec3 ln = normalize(v_Local);
  vec3 sunDir = normalize(-v_WorldPos);
  float diff = max(dot(n, sunDir), 0.0);
  vec3 nightAmb = vec3(0.10, 0.16, 0.30) * (0.015 * (1.0 - diff));
  if (u_Type == 0) {
    float gran = fbm4(v_Local * 3.0 + u_Seed + vec3(u_Time * 0.05));
    vec3 ecol = mix(u_ColB, u_ColA, gran) * 5.0;
    float limb = 0.55 + 0.45 * pow(max(dot(n, viewDir), 0.0), 0.6);
    vec3 scol = ecol * limb;
    scol *= 1.1;
    scol = acesNarkowicz(scol);
    scol = pow(clamp(scol, 0.0, 1.0), vec3(1.0 / 2.2));
    FragColor = vec4(scol, 1.0);
    return;
  }
  vec3 albedo = vec3(0.5);
  float land = 0.0;
  if (u_Type == 1) {
    vec3 lp = ln;
    vec3 bp = vec3(lp.x * 2.0, lp.y * 9.0, lp.z * 2.0) + u_Seed;
    float bands = fbm4(bp);
    albedo = mix(u_ColA, u_ColB, bands);
    if (u_Spot > 0.5) {
      vec3 spotDir = normalize(vec3(0.3, -0.25, 0.9));
      float d = distance(ln, spotDir);
      float spot = 1.0 - smoothstep(0.15, 0.35, d);
      albedo = mix(albedo, vec3(0.85, 0.35, 0.2), spot * 0.85);
    }
  } else if (u_Type == 2) {
    float continents = fbm4(v_Local * 3.0 + u_Seed);
    land = smoothstep(0.48, 0.56, continents);
    albedo = mix(u_ColA, u_ColB, land);
    float cap = smoothstep(0.75, 0.9, abs(ln.y));
    albedo = mix(albedo, vec3(0.9, 0.93, 0.95), cap);
    vec3 cp = v_Local * 5.0 + u_Seed + vec3(u_Time * 0.03, 0.0, 0.0);
    float cl = fbm4(cp);
    float clm = smoothstep(0.55, 0.75, cl) * u_Clouds;
    albedo = mix(albedo, vec3(1.0), clm * 0.85);
  } else if (u_Type == 4) {
    albedo = vec3(0.75, 1.0, 0.85);
  } else {
    float b3 = fbm3(v_Local * 4.0 + u_Seed);
    float iceK = b3 * 0.35 + 0.15 * ln.y;
    iceK = clamp(iceK, 0.0, 1.0);
    albedo = mix(u_ColA, u_ColB, iceK);
  }
  vec3 col = albedo * (0.03 + diff) + nightAmb;
  if (u_Type == 2) {
    float night = 1.0 - smoothstep(0.0, 0.08, diff);
    float cc = fbm3(v_Local * 14.0 + u_Seed);
    float cities = smoothstep(0.62, 0.8, cc) * land;
    col += vec3(1.0, 0.75, 0.42) * cities * night * u_Clouds * 2.2;
  }
  if (u_Type == 3) {
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.0);
    col += u_ColB * rim * 0.3;
  }
  if (u_Type == 4) {
    vec3 comaColor = vec3(0.75, 1.0, 0.85) * 3.0;
    float ndotl = max(dot(normalize(v_Normal), normalize(u_CamPos - v_WorldPos)), 0.0);
    col = comaColor * (0.25 + 2.75 * pow(ndotl, 1.5));
  }
  col *= 1.1;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
  FragColor = vec4(col, 1.0);
}
