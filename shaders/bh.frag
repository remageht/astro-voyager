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
uniform int u_DiskOn;
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
  vec3 accumColor = vec3(0.0);
  float accumAlpha = 0.0;

  for (int i = 0; i < ABSOLUTE_MAX_STEPS; i++) {
    if (i >= u_MaxSteps) break;
    vec4 pPrev = p;
    float smoothFactor = smoothstep(8.0 * u_Rs, 3.0 * u_Rs, p.y);
    float hNear = u_StepSize;
    float hFar = u_StepSize * 2.5;
    float hAdapt = mix(hFar, hNear, smoothFactor);
    rk4(hAdapt, p, dp);
    if (any(isnan(p)) || any(isnan(dp))) {
      FragColor = vec4(0.02, 0.015, 0.025, 1.0); return;
    }
    fb = normalize(velToCart(p, dp));

    if (u_DiskOn == 1) {
      float yPrev = cos(pPrev.z);
      float yCurr = cos(p.z);
      if (yPrev * yCurr <= 0.0 && abs(yPrev - yCurr) > 1e-6) {
        float tDisk = clamp(abs(yPrev) / (abs(yPrev) + abs(yCurr)), 0.0, 1.0);
        float rDisk = mix(pPrev.y, p.y, tDisk);
        float phiDisk = mix(pPrev.w, p.w, tDisk);
        float rIn = u_Rs * 2.6;
        float rOut = u_Rs * 10.0;
        if (rDisk >= rIn && rDisk <= rOut) {
          float v = sqrt(clamp(0.5 * u_Rs / rDisk, 0.0, 0.49));
          vec3 vDir = vec3(-sin(phiDisk), 0.0, cos(phiDisk));
          vec3 vDisk = v * vDir;
          vec3 rayDir = normalize(velToCart(p, dp));
          float betaPar = clamp(dot(vDisk, -rayDir), -0.85, 0.85);
          float gamma = 1.0 / sqrt(max(1.0 - v * v, 0.01));
          float doppler = 1.0 / (gamma * (1.0 - betaPar));
          float gravRedshift = sqrt(max(1.0 - u_Rs / rDisk, 0.01));
          float g = clamp(doppler * gravRedshift, 0.1, 3.5);

          float tProfile = pow(rIn / rDisk, 1.25) * sqrt(max(1.0 - sqrt(rIn / rDisk), 0.0));
          float ringMod = 0.8 + 0.2 * sin(16.0 * (rDisk / u_Rs) + 2.0 * phiDisk);
          float brightness = tProfile * ringMod * pow(g, 3.5);

          vec3 coolCol = vec3(1.0, 0.35, 0.08) * 0.9;
          vec3 hotCol = vec3(1.0, 0.95, 0.8) + vec3(-0.15, 0.1, 0.4) * (g - 1.0);
          vec3 emitCol = mix(coolCol, hotCol, smoothstep(0.7, 1.3, g));

          vec3 diskRgb = emitCol * brightness * 4.0;
          float diskAlpha = clamp(brightness * 2.2, 0.0, 0.95);

          accumColor += (1.0 - accumAlpha) * diskRgb;
          accumAlpha += (1.0 - accumAlpha) * diskAlpha;

          if (accumAlpha >= 0.98) {
            FragColor = vec4(accumColor, 1.0);
            return;
          }
        }
      }
    }

    if (p.y <= u_Rs * 1.001) {
      if (u_DiskOn == 1) {
        FragColor = vec4(accumColor, 1.0);
      } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
      }
      return;
    }
    if (p.y >= u_ShellRadius) {
      if (u_DiskOn == 1) {
        FragColor = vec4(accumColor + (1.0 - accumAlpha) * texture(u_Skybox, fb).rgb, 1.0);
      } else {
        FragColor = vec4(texture(u_Skybox, fb).rgb, 1.0);
      }
      return;
    }
  }

  if (u_DiskOn == 1) {
    FragColor = vec4(accumColor + (1.0 - accumAlpha) * texture(u_Skybox, fb).rgb * vec3(0.18, 0.16, 0.22), 1.0);
  } else {
    FragColor = vec4(texture(u_Skybox, fb).rgb * vec3(0.18, 0.16, 0.22), 1.0);
  }
}
