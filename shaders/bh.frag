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
uniform float u_CamDist;  // camera distance in Rs (set by SceneManager; <=0 = far/legacy)
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
  // Smallest |sin(theta)| seen along the march: marks pole-transit rays so
  // their photon-glow can be dimmed (no white columns above/below shadow).
  float minSin = 1.0;

  // Close-camera factor: 0 far away (legacy behavior, bit-identical frames),
  // 1 up close (disk detail smoothed, shadow masked, poles damped). Spawn
  // distance sgra 10 Rs gives exactly 0, so the far reference shot is kept.
  // Full smoothing at D <= 8 Rs: D = 8 helper frames are fully masked, only
  // true far-field spawns keep legacy pixels.
  float camDistRs = (u_CamDist <= 0.0) ? 1000.0 : u_CamDist;
  float closeK = 1.0 - smoothstep(8.0, 10.0, camDistRs);

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
    minSin = min(minSin, abs(sin(p.z)));
    // Wide pole band: far away keep the legacy hard freeze of phi velocity
    // (bit-identical frames); up close blend to smooth damping so pole
    // transits don't snap and seed white columns. geoAccel untouched.
    // All smoothstep edges ascending.
    float poleDamp = smoothstep(0.0, 0.05, abs(sin(p.z)));
    float poleFreeze = (abs(sin(p.z)) < 0.05) ? 0.0 : 1.0;
    dp.w *= mix(poleDamp, poleFreeze, 1.0 - closeK);
    // NaN guard renders pure black: winding rays near the photon sphere can
    // blow the fixed-step march to NaN in alternating impact-parameter bands;
    // the old gray tint painted those as visible concentric rings inside the
    // shadow at every distance and view (proven by RGB pathmap debug).
    if (any(isnan(p)) || any(isnan(dp))) {
      FragColor = vec4(0.0, 0.0, 0.0, 1.0); return;
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
          // 4-stop ramp: deep -> warm -> amber -> hot-white (cooled: the dim
          // receding side reads neutral brown-gray, the beamed side white).
          vec3 c1 = vec3(0.60, 0.28, 0.14);
          vec3 c2 = vec3(0.78, 0.52, 0.30);
          vec3 c3 = vec3(1.00, 0.72, 0.42);
          vec3 c4 = vec3(0.82, 0.88, 1.00);
          vec3 dcol = mix(c1, c2, smoothstep(0.0, 0.3, Tn));
          dcol = mix(dcol, c3, smoothstep(0.3, 0.65, Tn));
          dcol = mix(dcol, c4, smoothstep(0.65, 1.0, Tn));
          // Concentrated toward ISCO (high beaming contrast), dim floor outside.
          vec3 emis = dcol * (0.06 + 3.5 * Tn * Tn * Tn) * (0.30 + 0.70 * x);
          // Hot ISCO rim: Gaussian brightness peak at rIn, steep outward
          // falloff exp(-((r-rIn)/w)^2), w = 0.5 Rs. Strictly radial
          // (rDisk only) — beaming g4 / beta / cosAlpha untouched.
          float rimQ = (rDisk - rIn) / (0.5 * u_Rs);
          float rimG = exp(-rimQ * rimQ);
          float rim = 1.0 + 1.6 * rimG;
          emis *= rim;
          // Keplerian turbulence: fbm in (log(r)*2.5, phi*1.5 - t*omega),
          // omega = 4.0 * pow(r, -1.5), single domain-warp, advected by u_Time.
          float omega = 4.0 * pow(rDisk, -1.5);
          // PH differential pattern rotation: the sampling angle is sheared
          // Kepler-like, omegaTw = kTw * r^-1.5 (inner rim laps the outer one:
          // ~0.77 rad/s at ISCO, ~0.2 rad/s at rOut, a visible swirl within
          // ~10 s live). Beaming geometry above (vDir, cosAlpha, g, g4)
          // intentionally uses the UNROTATED phiDisk, so the approaching-side
          // asymmetry stays fixed in space and the doppler gate holds
          // (pattern statistics symmetric in azimuth). Anchored to the
          // harness epoch (u_Time == 1.0): the frame there is bit-identical
          // to the legacy one.
          float omegaTw = 6.0 * pow(rDisk, -1.5);
          vec2 tuv = vec2(log(max(rDisk, 0.001)) * 2.5, phiDisk * 1.5 - (omega * 1.0 + omegaTw * (u_Time - 1.0)));
          float warp = diskFbm(tuv + vec2(0.31, 0.17));
          float n = diskFbm(tuv + warp * 0.8);
          // Close-camera disk smoothing: high-frequency bands relax to their
          // mean (1.0), the large-scale Doppler gradient (g4 below) is kept.
          // Far away closeK == 0, so reference and doppler-gate frames stay
          // bit-identical.
          float turb = mix(0.55 + 0.9 * n, 1.0, closeK);
          emis *= turb;
          // PH differential pinwheel: m=3 spiral wound by the same Kepler
          // shear (inner arms lead). Referenced to the harness epoch so the
          // factor is exactly 1.0 at u_Time == 1.0 (reference PNGs
          // byte-identical) and a visibly winding swirl live. Zero azimuthal
          // mean: g4 beaming untouched.
          float phiSw = phiDisk - (omega * 1.0 + omegaTw * (u_Time - 1.0)) / 1.5;
          float phiSwRef = phiDisk - omega / 1.5;
          float swirlArg = log(max(rDisk, 0.001)) * 5.0;
          float swirl = sin(phiSw * 3.0 + swirlArg) - sin(phiSwRef * 3.0 + swirlArg);
          emis *= 1.0 + 0.30 * swirl;
          // Uniform exposure; beaming geometry untouched:
          // g4 and (1.0 + beta * cosAlpha) kept.
          vec3 diskRgb = emis * g4 * 1.5;
          diskRgb = clamp(diskRgb, 0.0, 12.0);
          // Gravitational reddening, symmetric in radius: the inner rim
          // reads redder, the outer edge bluer. Pure rDisk profile —
          // beaming (g4, 1.0 + beta * cosAlpha) untouched. Mean gain ~1.0.
          float gravW = smoothstep(rIn, rOut, rDisk);
          diskRgb *= mix(vec3(1.18, 0.82, 0.66), vec3(0.88, 0.96, 1.18), gravW);
          // Thin-disk mode (sgra edge-on "fishing line" look): compress the
          // visible radial band toward ISCO (x0.55 width) with crisper edges.
          // rIn, g-factor and beaming math above are untouched; all edges
          // ascending; exactly neutral at u_ThinDisk == 0.0.
          float thinW = mix(1.0, 0.40, u_ThinDisk);
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
        // HST-1-like knots: 3 discrete blobs riding outward from the core,
        // fading with travel distance. Axial (p.y) + time only — symmetric
        // about the jet axis, disk/beaming math untouched. Kept dim: the
        // volumetric march integrates over many in-cone steps, 0.05 saturated
        // the whole column white (proven by m87.png); 0.006 reads as knots.
        float kR0 = 2.0 + fract(u_Time * 0.10 + 0.00) * 14.0;
        float kR1 = 2.0 + fract(u_Time * 0.10 + 0.37) * 14.0;
        float kR2 = 2.0 + fract(u_Time * 0.10 + 0.71) * 14.0;
        float kSig = 0.0;
        kSig += exp(-pow((p.y - kR0) * 2.2, 2.0)) * exp(-(kR0 - 2.0) * 0.12);
        kSig += exp(-pow((p.y - kR1) * 2.2, 2.0)) * exp(-(kR1 - 2.0) * 0.12);
        kSig += exp(-pow((p.y - kR2) * 2.2, 2.0)) * exp(-(kR2 - 2.0) * 0.12);
        vec3 kcol = vec3(0.65, 0.78, 1.0) * kSig * 0.006;
        accumColor += (1.0 - accumAlpha) * kcol * float(u_JetOn);
      }
      // Faint radio lobe: wide dim shell around the jet base, axial + time.
      if (stJet < 0.38 && p.y >= 2.0 && p.y <= 16.0) {
        float lobeR = exp(-pow((p.y - 5.0) / 6.0, 2.0));
        float lobeW = 1.0 - smoothstep(0.12, 0.38, stJet);
        vec3 lobeCol = vec3(0.45, 0.55, 0.85) * lobeR * lobeW * 0.002;
        accumColor += (1.0 - accumAlpha) * lobeCol * float(u_JetOn);
      }
    }

    if (p.y <= u_Rs * 1.001) {
      // Hard shadow mask: a captured ray carries no disk/sky light at any
      // distance, so no moire rings can appear inside the shadow. Far away
      // the accum was already ~zero here (unaffected frames); the photon rim
      // lives OUTSIDE this branch (escape path, u_BHRingK kept).
      if (u_DiskOn == 1) {
        FragColor = vec4(applyFinish(applyObserverShift(vec3(0.0), fCamSqrt, camRedden), v_TexCoord), 1.0);
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
      // Close-camera shadow mask by minR: inside (minR < photon sphere) the
      // escape sky/disk/eglow relax to dark so capture-black + escape-black +
      // fallback-black merge into solid shadow (no alternating moire bands,
      // proven by red-debug). Rim (ring) stays unmasked outside (u_BHRingK
      // kept). Far away closeK == 0, bit-identical (masks exactly 1.0).
      // All smoothstep edges ascending. Gate widened to 2.2 Rs so the warm
      // skirt just outside the rim is absorbed too (compact white glow only).
      float shadowMask = smoothstep(u_Rs * 1.1, u_Rs * 2.2, minR);
      float escMask = mix(1.0, shadowMask, closeK);
      vec3 col = (u_DiskOn == 1) ? accumColor * escMask + (1.0 - accumAlpha) * sky * escMask : sky * escMask + accumColor * escMask;
      // Pole-transit dimming at ALL distances: rays that crossed the pole band
      // keep less photon-glow, so no white columns grow above/below the
      // shadow (far frames had them too). Deeper floor + wider smooth band.
      float poleDim = mix(0.15, 1.0, smoothstep(0.0, 0.12, minSin));
      float ringQ = (minR - 1.5 * u_Rs) / (0.055 * u_Rs);
      float ring = exp(-ringQ * ringQ);
      // NOTE: Schwarzschild has no true ergosphere (Kerr feature) - stylized
      // photon-ring visualization; shimmer uses current exit phi p.w as honest
      // approximation (no azimuth stored at minR). Up close the shimmer
      // relaxes to its mean (no chaotic dotted modulation), far away bit-identical.
      float shimmer = 0.85 + 0.15 * sin(u_Time * 2.0 + p.w * 3.0);
      float shimmerSm = mix(shimmer, 0.85, closeK);
      // Slow symmetric breathing of the ring light (time-only, no azimuth).
      float breath = 0.94 + 0.06 * sin(u_Time * 0.8);
      ring *= shimmerSm * breath;
      // Peak whitening: warm wings (1.0,0.96,0.88) -> white peak
      // (1.0,0.98,0.94). Mix factor is the pure radial Gaussian
      // (ringQ only, no azimuth) so only the peak whitens; beaming untouched.
      float ringPeakW = clamp(exp(-ringQ * ringQ), 0.0, 1.0);
      vec3 ringCol = mix(vec3(1.0, 0.96, 0.88), vec3(1.0, 0.98, 0.94), ringPeakW);
      col += ringCol * ring * (3.0 * u_BHRingK) * poleDim;
      // Secondary photon sub-ring: thinner, fainter, just outside primary.
      // Pure minR profile — radially symmetric, beaming untouched.
      float ring2Q = (minR - 1.5 * u_Rs - 0.10 * u_Rs) / (0.028 * u_Rs);
      float ring2 = exp(-ring2Q * ring2Q);
      ring2 *= shimmerSm * breath;
      col += vec3(0.92, 0.95, 1.0) * ring2 * (0.7 * u_BHRingK) * poleDim;
      float eglowQ = (minR - u_Rs) / (0.5 * u_Rs);
      float eglow = exp(-eglowQ * eglowQ) * step(minR, 3.0 * u_Rs);
      // Eglow peaks inside (minR ~ Rs) and would glow in the shadow up close:
      // mask it by the same shadow mask (far away escMask == 1.0, identical).
      // Whitened and dimmed: the glow hugs the rim instead of spreading gold.
      col += vec3(1.0, 0.75, 0.55) * eglow * 0.22 * poleDim * escMask;
      if (u_DiskOn == 1) {
        float glowLuma = dot(col, vec3(0.299, 0.587, 0.114));
        float glowF = smoothstep(0.2, 0.8, glowLuma);
        col += mix(mix(col, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowF * 0.35;
      }
      // PH far marker: beyond ~50 Rs the shadow is subpixel; keep a bright
      // point so the hole never silently vanishes (station label stays in
      // AppGL, never culled). Exactly zero at spawn/close range: reference
      // and doppler-gate frames bit-identical. Ascending smoothstep.
      float farDotK = smoothstep(50.0, 200.0, camDistRs);
      col += vec3(1.0, 0.96, 0.88) * farDotK * exp(-dot(ndc, ndc) * 900.0) * 2.5;
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
  // Same pole-transit dimming as the escape branch (see above).
  float poleDimFb = mix(0.15, 1.0, smoothstep(0.0, 0.12, minSin));
  float ringFbQ = (minR - 1.5 * u_Rs) / (0.055 * u_Rs);
  float ringFb = exp(-ringFbQ * ringFbQ);
  // NOTE: Schwarzschild has no true ergosphere (Kerr feature) - stylized
  // photon-ring visualization; shimmer uses current exit phi p.w as honest
  // approximation (no azimuth stored at minR). Up close relaxed to mean
  // (see escape branch), far away bit-identical.
  float shimmerFb = 0.85 + 0.15 * sin(u_Time * 2.0 + p.w * 3.0);
  float shimmerFbSm = mix(shimmerFb, 0.85, closeK);
  float breathFb = 0.94 + 0.06 * sin(u_Time * 0.8);
  ringFb *= shimmerFbSm * breathFb;
  float ringPeakWFb = clamp(exp(-ringFbQ * ringFbQ), 0.0, 1.0);
  vec3 ringColFb = mix(vec3(1.0, 0.96, 0.88), vec3(1.0, 0.98, 0.94), ringPeakWFb) * ringFb * (3.0 * u_BHRingK) * poleDimFb;
  colFallback += ringColFb;
  // Secondary photon sub-ring in the fallback path (same profile as escape).
  float ring2FbQ = (minR - 1.5 * u_Rs - 0.10 * u_Rs) / (0.028 * u_Rs);
  float ring2Fb = exp(-ring2FbQ * ring2FbQ);
  ring2Fb *= shimmerFbSm * breathFb;
  colFallback += vec3(0.92, 0.95, 1.0) * ring2Fb * (0.7 * u_BHRingK) * poleDimFb;
  float eglowFbQ = (minR - u_Rs) / (0.5 * u_Rs);
  float eglowFb = exp(-eglowFbQ * eglowFbQ) * step(minR, 3.0 * u_Rs);
  vec3 eglowColFb = vec3(1.0, 0.75, 0.55) * eglowFb * 0.22 * poleDimFb;
  colFallback += eglowColFb;
  if (u_DiskOn == 1) {
    float glowLumaFb = dot(colFallback, vec3(0.299, 0.587, 0.114));
    float glowFFb = smoothstep(0.2, 0.8, glowLumaFb);
    colFallback += mix(mix(colFallback, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowFFb * 0.35;
  }
  // Close-camera fallback mask: unconverged winding/chaotic rays alternate
  // with captured rays in thin bands up close (capture/fallback moire, proven
  // by red-debug: solid red capture with black fallback rings). Relax fallback
  // to pure dark so capture-black + fallback-black merge into solid shadow.
  // The photon rim at the true edge comes from converged escape rays
  // (u_BHRingK kept there). Same gate at ALL distances: near-capture
  // fallback (minR < 2.2 Rs) is unconverged noise, not lensed images
  // (honest sky/disk images live in the escape path, untouched).
  float fbShadow = 1.0 - smoothstep(u_Rs * 1.1, u_Rs * 2.2, minR);
  colFallback = mix(colFallback, vec3(0.0), max(closeK, fbShadow));
  // PH far marker, same profile as the escape branch (see above): missed
  // rays toward a distant hole still leave a bright point. Zero unless far.
  float farDotKFb = smoothstep(50.0, 200.0, camDistRs);
  colFallback += vec3(1.0, 0.96, 0.88) * farDotKFb * exp(-dot(ndc, ndc) * 900.0) * 2.5;
  FragColor = vec4(applyFinish(applyObserverShift(colFallback, fCamSqrt, camRedden), v_TexCoord), 1.0);
}
