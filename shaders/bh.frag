// astro-voyager black-hole raytrace (GLSL 330).
// Direct port of RuCatGH/blackHole BlackHoleRaytrace.shader fragment part.
// Physics: Schwarzschild null geodesics, RK4, capture r<=Rs*1.001,
// escape r>=u_ShellRadius, fallback dim on max steps.
#version 330 core
out vec4 FragColor;
in vec2 v_TexCoord;
uniform samplerCube u_Skybox;
uniform vec3 u_CameraPosition;
uniform vec3 u_CameraForward;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;
uniform float u_Aspect;
uniform float u_FovY;
uniform float u_Rs;
uniform float u_ShellRadius;
uniform float u_StepSize;
uniform int u_MaxSteps;
const float PI = 3.141592653589793;
const int ABSOLUTE_MAX_STEPS = 8000;
float safeSin(float a) {
  float v = sin(a);
  if (abs(v) < 0.0001) return v < 0.0 ? -0.0001 : 0.0001;
  return v;
}
vec4 cartToSchwarzschild(vec3 p) {
  float r = max(length(p), u_Rs + 0.001);
  return vec4(0.0, r, acos(clamp(p.y / r, -1.0, 1.0)), atan(p.z, p.x));
}
void sphericalBasis(vec4 p, out vec3 eR, out vec3 eT, out vec3 eP) {
  float st = sin(p.z), ct = cos(p.z), sp = sin(p.w), cp = cos(p.w);
  eR = vec3(st * cp, ct, st * sp);
  eT = vec3(ct * cp, -st, ct * sp);
  eP = vec3(-sp, 0.0, cp);
}
vec4 cartDirToVel(vec4 p, vec3 d) {
  vec3 eR, eT, eP; sphericalBasis(p, eR, eT, eP);
  float r = max(p.y, u_Rs + 0.001);
  float sT = safeSin(p.z);
  float dR = dot(d, eR);
  float dT = dot(d, eT) / r;
  float dP = dot(d, eP) / (r * sT);
  float f = max(1.0 - u_Rs / r, 0.0001);
  float sp = (dR * dR) / f + r * r * dT * dT + r * r * sT * sT * dP * dP;
  return vec4(sqrt(max(sp / f, 0.0)), dR, dT, dP);
}
vec3 velToCart(vec4 p, vec4 dp) {
  vec3 eR, eT, eP; sphericalBasis(p, eR, eT, eP);
  float r = max(p.y, u_Rs + 0.001);
  return dp.y * eR + r * dp.z * eT + r * safeSin(p.z) * dp.w * eP;
}
vec4 geoAccel(vec4 p, vec4 dp) {
  float r = max(p.y, u_Rs + 0.001);
  float th = clamp(p.z, 0.0001, PI - 0.0001);
  float rRs = max(r - u_Rs, 0.0001);
  float sT = sin(th), cT = cos(th);
  float cot = cT / safeSin(th);
  float dT = dp.x, dR = dp.y, dTh = dp.z, dP = dp.w;
  float ddT = -(dR * u_Rs * dT) / (r * rRs);
  float ddR = rRs * (pow(dP * sT, 2.0) + dTh * dTh -
              (u_Rs * dT * dT) / (2.0 * r * r * r)) +
              (u_Rs * dR * dR) / (2.0 * r * rRs);
  float ddTh = dP * dP * sT * cT - (2.0 * dR * dTh) / r;
  float ddP = -2.0 * dP * dTh * cot - (2.0 * dR * dP) / r;
  return vec4(ddT, ddR, ddTh, ddP);
}
void deriv(vec4 p, vec4 dp, out vec4 dP, out vec4 dDp) {
  dP = dp; dDp = geoAccel(p, dp);
}
void rk4(float h, inout vec4 p, inout vec4 dp) {
  vec4 aP, bP, cP, dP_, aD, bD, cD, dD;
  deriv(p, dp, aP, aD);
  deriv(p + 0.5 * h * aP, dp + 0.5 * h * aD, bP, bD);
  deriv(p + 0.5 * h * bP, dp + 0.5 * h * bD, cP, cD);
  deriv(p + h * cP, dp + h * cD, dP_, dD);
  p += (h / 6.0) * (aP + 2.0 * bP + 2.0 * cP + dP_);
  dp += (h / 6.0) * (aD + 2.0 * bD + 2.0 * cD + dD);
  p.z = clamp(p.z, 0.0001, PI - 0.0001);
}
void main() {
  vec2 ndc = v_TexCoord * 2.0 - 1.0;
  ndc.x *= u_Aspect;
  float th_ = tan(0.5 * u_FovY);
  vec3 dir = normalize(u_CameraForward + ndc.x * th_ * u_CameraRight +
                       ndc.y * th_ * u_CameraUp);
  vec4 p = cartToSchwarzschild(u_CameraPosition);
  vec4 dp = cartDirToVel(p, dir);
  vec3 fb = dir;
  for (int i = 0; i < ABSOLUTE_MAX_STEPS; i++) {
    if (i >= u_MaxSteps) break;
    rk4(u_StepSize, p, dp);
    if (any(isnan(p)) || any(isnan(dp))) {
      FragColor = vec4(0.02, 0.015, 0.025, 1.0); return;
    }
    fb = normalize(velToCart(p, dp));
    if (p.y <= u_Rs * 1.001) { FragColor = vec4(0.0, 0.0, 0.0, 1.0); return; }
    if (p.y >= u_ShellRadius) {
      FragColor = vec4(texture(u_Skybox, fb).rgb, 1.0); return;
    }
  }
  FragColor = vec4(texture(u_Skybox, fb).rgb * vec3(0.18, 0.16, 0.22), 1.0);
}
