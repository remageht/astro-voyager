#version 330 core
// PBR-lite warp-ship hull: GGX/Smith/Schlick direct key light driven by the
// scene's dominant source (station star/disk), cool anti-key rim, and sky
// IBL (mip-filtered local cubemap): ambient diffuse + roughness-blurred
// reflections. Base colors stay display-referred; decode to linear here.
// Output is linear HDR (ACES + bloom live in the composite).
in vec3 v_Normal;
in vec3 v_WorldPos;
out vec4 FragColor;
uniform vec3 u_CamPos;
uniform vec3 u_BaseColor;
uniform vec3 u_KeyDir;    // surface -> dominant scene light
uniform vec3 u_KeyColor;  // color * intensity
uniform float u_Metal;
uniform float u_Rough;
uniform samplerCube u_Env;
uniform float u_EnvMaxLod;

const float kPi = 3.14159265;

float dGGX(float NoH, float a) {
  float a2 = a * a;
  float d = NoH * NoH * (a2 - 1.0) + 1.0;
  return a2 / (kPi * d * d + 1e-7);
}
float vSmith(float NoV, float NoL, float a) {
  float a2 = a * a;
  float lv = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
  float ll = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
  return 0.5 / max(lv + ll, 1e-5);
}
vec3 fSchlick(float u, vec3 f0) { return f0 + (1.0 - f0) * pow(1.0 - u, 5.0); }

void main() {
  vec3 n = normalize(v_Normal);
  vec3 V = normalize(u_CamPos - v_WorldPos);
  float NoV = max(dot(n, V), 1e-4);
  float rough = clamp(u_Rough, 0.05, 1.0);
  vec3 albedo = pow(max(u_BaseColor, vec3(0.0)), vec3(2.2));
  vec3 f0 = mix(vec3(0.04), albedo, u_Metal);
  vec3 kd = (vec3(1.0) - f0) * (1.0 - u_Metal);

  // Key light.
  vec3 L = normalize(u_KeyDir);
  float NoL = max(dot(n, L), 0.0);
  vec3 H = normalize(L + V);
  float NoH = max(dot(n, H), 0.0);
  float VoH = max(dot(V, H), 0.0);
  vec3 spec = fSchlick(VoH, f0) * dGGX(NoH, rough) * vSmith(NoV, NoL, rough);
  vec3 col = (kd * albedo / kPi + spec) * u_KeyColor * NoL;

  // Cool rim from the anti-key direction — silhouette readability.
  vec3 rimDir = normalize(-L + vec3(0.0, 0.45, 0.0));
  float rimNoL = max(dot(n, rimDir), 0.0);
  float rimFres = pow(1.0 - NoV, 3.0);
  col += vec3(0.45, 0.62, 0.95) * rimFres * (0.30 + 0.22 * rimNoL);

  // Sky IBL: diffuse from the coarsest mip, reflections by roughness lod.
  vec3 R = reflect(-V, n);
  vec3 envSpec = textureLod(u_Env, R, rough * u_EnvMaxLod).rgb;
  vec3 envDiff = textureLod(u_Env, n, u_EnvMaxLod).rgb;
  vec3 fEnv = f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NoV, 5.0);
  col += envDiff * albedo * kd * 2.6;  // ambient gain: sky is dim HDR
  col += envSpec * fEnv * (1.0 - 0.55 * rough);
  FragColor = vec4(col, 1.0);
}
