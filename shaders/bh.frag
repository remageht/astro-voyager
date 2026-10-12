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
uniform int u_JetOn;
uniform float u_Time;
uniform float u_Exposure;
uniform float u_BHRingK;  // photon-ring/rim brightness gain (default 1.0; sgra 1.8, m87 1.25)
uniform float u_ThinDisk;  // thin-disk mode 0/1 (sgra 1.0): radial x0.55 band + crisper edges
uniform float u_BandK;  // milky-way band gain (default 1.0; sgra 1.7)
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
  // Pole-singularity fix: freeze phi contribution when sinTheta < 1e-3.
  // Near poles 1/sin blows up and RK4 goes unstable (dotted line artifact),
  // so zero the dPhi term for this step. CPU side untouched.
  if (abs(sin(p.z)) < 1e-3) {
    dP = 0.0;
  }
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
  // Wide pole band: ignore phi velocity locally. Near poles phi = atan2 is
  // noise-dominated and cot amplifies it into dp.w DURING rk4 substeps
  // (post-step zeroing is too late) -> random whirl counts -> dotted column
  // of max-step fallback pixels. Freezing dP here keeps transits smooth.
  if (abs(sT) < 0.05) { dP = 0.0; }
  float ddT = -(dR * u_Rs * dT) / (r * rRs);
  float ddR = rRs * (pow(dP * sT, 2.0) + dTh * dTh -
              (u_Rs * dT * dT) / (2.0 * r * r * r)) +
              (u_Rs * dR * dR) / (2.0 * r * rRs);
  float ddTh = dP * dP * sT * cT - (2.0 * dR * dTh) / r;
  float ddP = -2.0 * dP * dTh * cot - (2.0 * dR * dP) / r;
  // Pole-singularity fix: drop cot-dependent ddPhi term when sinTheta < 1e-3.
  // cot can reach 1e4 near poles and destabilizes RK4, so keep only the
  // regular radial part for this step.
  if (abs(sT) < 1e-3) {
    ddP = -(2.0 * dR * dP) / r;
  }
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
// Observer-side shift of the final radiance: sqrt(fCam) dimming plus a
// reddening power stretch pow(color, vec3(1/max(sqrt(fCam), 0.2))).
vec3 applyObserverShift(vec3 color, float fCamSqrt, vec3 camRedden) {
  float b = max(fCamSqrt, 0.0);
  vec3 c = max(color, vec3(0.0));
  return b * pow(c, camRedden);
}
// Value-noise helpers for disk turbulence (ASCII only, 5 octaves).
float diskHash(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}
float diskNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = diskHash(i);
  float b = diskHash(i + vec2(1.0, 0.0));
  float c = diskHash(i + vec2(0.0, 1.0));
  float d = diskHash(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float diskFbm(vec2 p) {
  float v = 0.0;
  float amp = 0.5;
  for (int k = 0; k < 5; k++) {
    v += amp * diskNoise(p);
    p *= 2.03;
    amp *= 0.5;
  }
  return v;
}
// ACES filmic (Narkowicz fit) + gamma + vignette + animated grain.
// Applied to the final combined color, grain runs AFTER tonemap.
vec3 acesTonemap(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
float finishHash(vec2 p) {
  return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}
vec3 applyFinish(vec3 col, vec2 uv) {
  col *= 1.1 * u_Exposure;
  col = acesTonemap(col);
  col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
  float d = distance(uv, vec2(0.5));
  float vig = 1.0 - 0.3 * smoothstep(0.2, 0.8, d);
  col *= vig;
  float gr = finishHash(gl_FragCoord.xy + fract(u_Time) * 100.0);
  col += (gr - 0.5) * 0.012;
  return col;
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

  // Gravitational redshift of the static observer: light arriving from a
  // far source is dimmed by sqrt(fCam) and its spectrum is stretched towards
  // red. There is no full darkness at the clamp (fCam = 0 remains visible).
  float rCam = max(length(u_CameraPosition), u_Rs * 1.001);
  float fCam = max(1.0 - u_Rs / rCam, 0.0);
  float fCamSqrt = max(sqrt(fCam), 0.0);
  vec3 camRedden = vec3(1.0 / max(fCamSqrt, 0.2));

  // NOTE: Schwarzschild has no true ergosphere (Kerr feature) - the glow
  // below is a stylized photon-ring visualization at the photon sphere.
  float minR = 1e30;

  for (int i = 0; i < ABSOLUTE_MAX_STEPS; i++) {
    if (i >= u_MaxSteps) break;
    vec4 pPrev = p;
    float smoothFactor = 1.0 - smoothstep(3.0 * u_Rs, 8.0 * u_Rs, p.y);
    float hNear = u_StepSize;
    float hFar = u_StepSize * 2.5;
    float hAdapt = mix(hFar, hNear, smoothFactor);
    rk4(hAdapt, p, dp);
    renormalizeTime(p, dp);
    if (p.y < minR) minR = p.y;
    // Wide pole band: hard-freeze phi velocity. Near the poles phi = atan2
    // is noise-dominated and 1/sin amplifies it into dp.w BEFORE the 1e-3
    // guards engage; zeroing dp.w every step keeps the transit smooth.
    if (abs(sin(p.z)) < 0.05) { dp.w = 0.0; }
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
        float rOut = u_Rs * 7.0;   // compact ring: camera at 10 Rs stays outside
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

          // Bardeen g-factor: g = sqrt(1 - Rs/r) / (gamma (1 + beta cosAlpha)).
          // cosAlpha is measured along the tracing direction (camera -> scene);
          // the photon travels scene -> camera, hence the sign: an approaching
          // disk element must brighten (blue-shift), not dim.
          float g = clamp(sqrt(fDisk) / (gamma * (1.0 + beta * cosAlpha)),
                          0.05, 5.0);

          // Novikov-Thorne flux F ~ r^-3 (1 - sqrt(rIn/r)), T ~ F^0.25.
          float shape = pow(rIn / rDisk, 3.0) * max(1.0 - sqrt(rIn / rDisk), 0.0);
          float tProf = pow(shape, 0.25);
          float tObs = pow(max(g, 0.001), 0.25);

          // Bolometric brightness ~ F g^4 (g^4 beaming), clamped for no blowout.
          float t4 = tProf * tProf * tProf * tProf;
          float g4 = g * g * g * g;
          float brightness = t4 * g4;

          // Novikov-Thorne normalized temperature:
          // Tn = (x^3 * (1 - sqrt(x)) * 2.11)^0.25, x = rIn / r.
          float x = rIn / rDisk;
          float Tn = pow(x * x * x * (1.0 - sqrt(x)) * 2.11, 0.25);
          // 4-stop ramp: deep -> orange -> amber -> hot.
          vec3 c1 = vec3(0.95, 0.18, 0.02);
          vec3 c2 = vec3(1.00, 0.45, 0.08);
          vec3 c3 = vec3(1.00, 0.72, 0.30);
          vec3 c4 = vec3(0.88, 0.90, 1.00);
          vec3 dcol = mix(c1, c2, smoothstep(0.0, 0.3, Tn));
          dcol = mix(dcol, c3, smoothstep(0.3, 0.65, Tn));
          dcol = mix(dcol, c4, smoothstep(0.65, 1.0, Tn));
          // Concentrated toward ISCO (high beaming contrast), dim floor outside.
          vec3 emis = dcol * (0.06 + 3.5 * Tn * Tn * Tn) * (0.30 + 0.70 * x);
          // White-hot inner rim: uniform boost, beaming untouched.
          float rim = 1.0 + 2.2 * (1.0 - smoothstep(rIn, rIn * 1.18, rDisk));
          emis *= rim;
          // Keplerian turbulence: fbm in (log(r)*2.5, phi*1.5 - t*omega),
          // omega = 4.0 * pow(r, -1.5), single domain-warp, advected by u_Time.
          float omega = 4.0 * pow(rDisk, -1.5);
          vec2 tuv = vec2(log(max(rDisk, 0.001)) * 2.5, phiDisk * 1.5 - u_Time * omega);
          float warp = diskFbm(tuv + vec2(0.31, 0.17));
          float n = diskFbm(tuv + warp * 0.8);
          float turb = 0.55 + 0.9 * n;
          emis *= turb;
          // Uniform exposure; beaming geometry untouched:
          // g4 and (1.0 + beta * cosAlpha) kept.
          vec3 diskRgb = emis * g4 * 1.5;
          diskRgb = clamp(diskRgb, 0.0, 12.0);
          // Thin-disk mode (sgra edge-on "fishing line" look): compress the
          // visible radial band toward ISCO (x0.55 width) with crisper edges.
          // rIn, g-factor and beaming math above are untouched; all edges
          // ascending; exactly neutral at u_ThinDisk == 0.0.
          float thinW = mix(1.0, 0.55, u_ThinDisk);
          float rOutThin = rIn + (rOut - rIn) * thinW;
          float edgeIn = smoothstep(rIn, rIn + (rOutThin - rIn) * 0.06 + 0.001, rDisk);
          float edgeOut = 1.0 - smoothstep(rOutThin - (rOutThin - rIn) * 0.12, rOutThin, rDisk);
          float thinMask = mix(1.0, edgeIn * edgeOut, u_ThinDisk);
          diskRgb *= thinMask;
          float diskAlpha = clamp(brightness * 1.2, 0.0, 0.9) * thinMask;

          accumColor += (1.0 - accumAlpha) * diskRgb;
          accumAlpha += (1.0 - accumAlpha) * diskAlpha;

          if (accumAlpha >= 0.98) {
            vec3 glowCol = accumColor;
            float glowLuma = dot(glowCol, vec3(0.299, 0.587, 0.114));
            float glowF = smoothstep(0.2, 0.8, glowLuma);
            glowCol += mix(mix(glowCol, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowF * 0.35;
            FragColor = vec4(applyFinish(applyObserverShift(glowCol, fCamSqrt, camRedden), v_TexCoord),
                             1.0);
            return;
          }
        }
      }
    }

    // M87* relativistic jet: polar cones along +-Y, additive glow, no alpha.
    // Independent of u_DiskOn so it shows with disk on or off.
    if (u_JetOn == 1) {
      float stJet = abs(sin(p.z));
      if (stJet < 0.12 && p.y >= 2.0 && p.y <= 25.0) {
        float streaks = diskFbm(vec2(p.y * 1.5 - u_Time * 0.4, p.w * 2.0));
        float knots = pow(max(streaks, 0.0), 3.0);
        // Tuned: volumetric emission integrates over many in-cone steps;
        // 0.008 was invisible, 0.1+ saturates white; 0.03 reads as a jet.
        vec3 jcol = vec3(0.60, 0.75, 1.0) * knots * exp(-(p.y - 2.0) * 0.12) * 0.03;
        accumColor += (1.0 - accumAlpha) * jcol * float(u_JetOn);
      }
    }

    if (p.y <= u_Rs * 1.001) {
      if (u_DiskOn == 1) {
        FragColor = vec4(applyFinish(applyObserverShift(accumColor, fCamSqrt, camRedden), v_TexCoord), 1.0);
      } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
      }
      return;
    }
    if (p.y >= u_ShellRadius) {
      vec3 sky = texture(u_Skybox, fb).rgb;
      // Milky-Way band gain: boost sky along the band plane (fb.y ~ 0),
      // same profile as the procedural cubemap. Neutral at u_BandK == 1.0.
      float bandW = exp(-abs(fb.y) * 5.0);
      sky *= 1.0 + (u_BandK - 1.0) * bandW;
      // Jet glow lives in accumColor with no alpha; add it even with disk off
      // (sgra accumulates nothing, so its disk-off frame is unchanged).
      // Capture branch above stays black: the hole absorbs light.
      vec3 col = (u_DiskOn == 1) ? accumColor + (1.0 - accumAlpha) * sky : sky + accumColor;
      float ring = exp(-pow((minR - 1.5 * u_Rs) / (0.08 * u_Rs), 2.0));
      // NOTE: Schwarzschild has no true ergosphere (Kerr feature) - stylized
      // photon-ring visualization; shimmer uses current exit phi p.w as honest
      // approximation (no azimuth stored at minR).
      float shimmer = 0.85 + 0.15 * sin(u_Time * 2.0 + p.w * 3.0);
      ring *= shimmer;
      col += vec3(1.0, 0.85, 0.6) * ring * (2.5 * u_BHRingK);
      float eglow = exp(-pow((minR - u_Rs) / (0.5 * u_Rs), 2.0)) * step(minR, 3.0 * u_Rs);
      col += vec3(1.0, 0.55, 0.25) * eglow * 0.5;
      if (u_DiskOn == 1) {
        float glowLuma = dot(col, vec3(0.299, 0.587, 0.114));
        float glowF = smoothstep(0.2, 0.8, glowLuma);
        col += mix(mix(col, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowF * 0.35;
      }
      FragColor = vec4(applyFinish(applyObserverShift(col, fCamSqrt, camRedden), v_TexCoord), 1.0);
      return;
    }
  }

  vec3 skyFallback = texture(u_Skybox, fb).rgb * vec3(0.18, 0.16, 0.22);
  float bandWFb = exp(-abs(fb.y) * 5.0);
  skyFallback *= 1.0 + (u_BandK - 1.0) * bandWFb;
  // Same jet-glow rule as the escape branch: visible with disk off too.
  vec3 colFallback = (u_DiskOn == 1)
                         ? accumColor + (1.0 - accumAlpha) * skyFallback
                         : skyFallback + accumColor;
  float ringFb = exp(-pow((minR - 1.5 * u_Rs) / (0.08 * u_Rs), 2.0));
  // NOTE: Schwarzschild has no true ergosphere (Kerr feature) - stylized
  // photon-ring visualization; shimmer uses current exit phi p.w as honest
  // approximation (no azimuth stored at minR).
  float shimmerFb = 0.85 + 0.15 * sin(u_Time * 2.0 + p.w * 3.0);
  ringFb *= shimmerFb;
  colFallback += vec3(1.0, 0.85, 0.6) * ringFb * (2.5 * u_BHRingK);
  float eglowFb = exp(-pow((minR - u_Rs) / (0.5 * u_Rs), 2.0)) * step(minR, 3.0 * u_Rs);
  colFallback += vec3(1.0, 0.55, 0.25) * eglowFb * 0.5;
  if (u_DiskOn == 1) {
    float glowLumaFb = dot(colFallback, vec3(0.299, 0.587, 0.114));
    float glowFFb = smoothstep(0.2, 0.8, glowLumaFb);
    colFallback += mix(mix(colFallback, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowFFb * 0.35;
  }
  FragColor = vec4(applyFinish(applyObserverShift(colFallback, fCamSqrt, camRedden), v_TexCoord), 1.0);
}
