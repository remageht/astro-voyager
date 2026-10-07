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
const float T_DISK = 10000.0;  // reference inner-disk temperature, Kelvin
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
// Re-project dt/dLambda after an RK4 step so the geodesic stays null (ds2 = 0):
// f = 1 - Rs/r; spatial = dR^2/f + r^2 dTheta^2 + r^2 sin^2(Theta) dPhi^2;
// dt = sqrt(spatial / f). Mirrored bit-for-bit by renormalizeTime() in
// src/Geodesic.cpp; keep both versions in sync.
void renormalizeTime(inout vec4 p, inout vec4 dp) {
  float r = max(p.y, u_Rs + 0.001);
  float th = clamp(p.z, 0.0001, PI - 0.0001);
  float sT = safeSin(th);
  float f = max(1.0 - u_Rs / r, 0.0001);
  float spatial = (dp.y * dp.y) / f + r * r * dp.z * dp.z +
                  r * r * sT * sT * dp.w * dp.w;
  dp.x = sqrt(max(spatial / f, 0.0));
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
// Planckian-locus colour ramp (Tanner Helland approximation of black-body
// colour), normalized to unit luminance so it can be scaled by radiance.
vec3 blackBodyColor(float tKelvin) {
  float t = clamp(tKelvin, 1000.0, 40000.0) / 100.0;
  float r, g, b;
  if (t <= 66.0) {
    r = 1.0;
    g = clamp(0.39008157876 * log(t) - 0.63184144378, 0.0, 1.0);
  } else {
    r = clamp(1.29293618606 * pow(t - 60.0, -0.1332047592), 0.0, 1.0);
    g = clamp(1.12989086089 * pow(t - 60.0, -0.0755148492), 0.0, 1.0);
  }
  if (t >= 66.0) {
    b = 1.0;
  } else if (t <= 19.0) {
    b = 0.0;
  } else {
    b = clamp(0.54320678911 * log(t - 10.0) - 1.19625408914, 0.0, 1.0);
  }
  return vec3(r, g, b) / max(0.2126 * r + 0.7152 * g + 0.0722 * b, 0.001);
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
    float smoothFactor = 1.0 - smoothstep(3.0 * u_Rs, 8.0 * u_Rs, p.y);
    float hNear = u_StepSize;
    float hFar = u_StepSize * 2.5;
    float hAdapt = mix(hFar, hNear, smoothFactor);
    rk4(hAdapt, p, dp);
    renormalizeTime(p, dp);
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
        float rIn = u_Rs * 3.0;   // ISCO: r = 6M = 3 Rs (2.6 Rs was inside it)
        float rOut = u_Rs * 10.0;
        if (rDisk >= rIn && rDisk <= rOut) {
          float fDisk = max(1.0 - u_Rs / rDisk, 0.0001);

          // Circular-orbit speed in the local static frame:
          // beta = sqrt(M/r) / sqrt(1 - Rs/r), gamma = 1 / sqrt(1 - beta^2).
          float beta = sqrt(max(0.5 * u_Rs / rDisk, 0.0) / fDisk);
          float gamma = 1.0 / sqrt(max(1.0 - beta * beta, 0.0001));
          vec3 vDir = vec3(-sin(phiDisk), 0.0, cos(phiDisk));

          // Photon direction in the local static frame (metric normalization):
          // proper radial length dr/sqrt(f), tangential r dTheta, r sin(Theta) dPhi.
          vec3 eR, eT, eP; sphericalBasis(p, eR, eT, eP);
          vec3 dLoc = dp.y * eR / sqrt(fDisk) + p.y * dp.z * eT +
                      p.y * safeSin(p.z) * dp.w * eP;
          vec3 dHat = normalize(dLoc);
          float cosAlpha = dot(dHat, vDir);

          // Bardeen g-factor: g = sqrt(1 - Rs/r) / (gamma (1 - beta cosAlpha)).
          float g = clamp(sqrt(fDisk) / (gamma * (1.0 - beta * cosAlpha)),
                          0.05, 5.0);

          // Novikov-Thorne flux F ~ r^-3 (1 - sqrt(rIn/r)), T ~ F^0.25.
          float shape = pow(rIn / rDisk, 3.0) * max(1.0 - sqrt(rIn / rDisk), 0.0);
          float tProf = pow(shape, 0.25);
          float tObs = pow(max(g, 0.001), 0.25);

          // Bolometric brightness ~ F g^4 (g^4 beaming), clamped for no blowout.
          float t4 = tProf * tProf * tProf * tProf;
          float g4 = g * g * g * g;
          float brightness = t4 * g4;

          vec3 diskRgb = blackBodyColor(T_DISK * tProf * tObs) *
                         clamp(brightness * 3.0, 0.0, 4.0);
          float diskAlpha = clamp(brightness * 0.9, 0.0, 0.9);

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
