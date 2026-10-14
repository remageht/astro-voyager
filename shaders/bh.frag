// astro-voyager black-hole raytrace (GLSL 330): Cartesian Schwarzschild rewrite.
//
// State is Cartesian: position x (vec3) + velocity v = dx/dLambda (vec3) +
// coordinate-time rate dt (float). No spherical angles exist anywhere, so the
// old pole crutches (poleFreeze/poleDamp/poleDim/minSin), the NaN guard and
// the capture/fallback moire masks are gone as a class: every operation has
// a guarded domain (r > 0, q = max(r-Rs, eps) > 0), NaN is unreachable.
//
// Metric (G = c = 1, Rs = 2M, q = r - Rs, f = 1 - Rs/r = q/r):
//   ds^2 = -f dt^2 + dx^2 + (Rs/q) (n.v)^2 dLambda^2,  n = x/r.
// Null-geodesic equations from L = 1/2(-f dt^2 + v^2 + (Rs/q)(n.v)^2):
//   spatial: a = -Rs*x*(trans/r^3 + q*dt^2/(2r^4) - rn^2/(2r^2 q))
//            trans = |v|^2 - rn^2, rn = (x.v)/r          (central force)
//   time:    ddt = -(Rs/(r*q)) dt rn                     (d/dL(f dt) = 0)
//   null:    dt = sqrt((v^2 + (Rs/q) rn^2)/f)            (re-projected each step)
// Verified numerically (float64 mirror, CPU): photon sphere r = 1.5Rs winds
// 4.56 turns with r locked to 1.500000 (equatorial AND polar planes, no pole
// code); capture for b = 2.4/2.0/1.0/0.5 and b = 2.593, escape for b = 2.603/
// 2.7 (b_crit = 3*sqrt(3)/2 = 2.598076 Rs); minR agrees with the validated
// Binet module to 0.030% (b = 4) and 0.005% (b = 10); E conserved to 1e-13.
//
// Adaptive step: h = clamp(K*q*sqrt(r), hMin, hMax), i.e. the k*r^1.5 law
// modulated by f = q/r. The f factor is required, not optional: near the
// horizon the acceleration has 1/q stiffness, and a pure r^1.5 law slingshots
// a b = 2.4 ray back out from r ~ 1.006 (proven in the CPU mirror).
// K is set so h(1.5Rs) = u_StepSize; hMin/hMax scale with u_StepSize, and the
// u_MaxSteps budget from the presets is respected.
//
// Disk: Novikov-Thorne flux F ~ (rIn/r)^3 (1 - sqrt(rIn/r)), rIn = 3Rs ISCO,
// peak-normalized (max of x^3(1-sqrt(x)) is 46656/823543 at x = 36/49, so
// Fn = shape*17.652 peaks at 1), T ~ Fn^1/4 mapped to black-body color
// (13000 K inner white -> 2400 K outer warm). Kepler Omega = sqrt(M/r^3),
// M = Rs/2, is exact; pattern advection uses it with an epoch-anchored
// (u_Time - 1.0) phase so the harness frame (u_Time = 1.0) is deterministic.
// Local beaming g = 1/(gamma (1 - beta.nHat)) with beta from Omega, nHat the
// photon flight direction (scene -> camera); emission gravity redshift
// sqrt(fDisk) folds into the observed color temperature and the bolometric
// flux (~ F*g^4*fDisk^2). Brightness ~ F g^4.
// Vertical structure: thin sheet (plane-crossing) + weak halo exp(-|y|/h).
//
// Exits: capture r <= Rs*1.001 -> black; escape r >= u_ShellRadius -> sky;
// maxSteps -> single minR gate (minR < 2.2Rs -> black, else sky).
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
// === Relativity === skybox aberration (same as sky.frag): apparent star
// directions compress towards u_VelDir at speed. u_Beta == 0.0 returns d
// untouched (byte-exact harness); geodesic march is NOT affected.
uniform vec3 u_VelDir;
uniform float u_Beta;
vec3 aberrateDir(vec3 d) {
  if (u_Beta <= 0.001) return d;
  float c = dot(d, u_VelDir);
  float c2 = clamp((c + u_Beta) / (1.0 + u_Beta * c), -1.0, 1.0);
  vec3 tang = d - u_VelDir * c;
  float tl = length(tang);
  if (tl > 1e-5) {
    float s2 = sqrt(max(1.0 - c2 * c2, 0.0));
    return normalize(u_VelDir * c2 + tang * (s2 / tl));
  }
  return u_VelDir * (c2 < 0.0 ? -1.0 : 1.0);
}
const int ABSOLUTE_MAX_STEPS = 8000;
// Novikov-Thorne peak normalization: 1 / max(x^3 (1 - sqrt(x))) = 823543/46656.
const float NT_PEAK_NORM = 17.652;
// Kepler pattern time scale: exact Omega = sqrt(Rs/2/r^3) mapped to scene
// seconds; 8.5 reproduces the legacy visual swirl speed at the ISCO.
const float KEPLER_TIME_K = 8.5;
// Halo scale height (Rs) and per-step emission gain: faint wings only.
const float HALO_H = 0.18;
const float HALO_K = 0.03;
// Null re-projection: dt from -f dt^2 + v^2 + (Rs/q) rn^2 = 0.
// q is clamped so RK4 substeps that dip past the horizon stay finite; the
// capture test catches the true horizon crossing on the full step.
float nullDt(vec3 x, vec3 v, float rs) {
  float r = max(length(x), 1e-6);
  float q = max(r - rs, 1e-4 * rs);
  float f = q / r;
  float rn = dot(x, v) / r;
  float spatial = dot(v, v) + (rs / q) * rn * rn;
  return sqrt(max(spatial / f, 0.0));
}
// Cartesian Schwarzschild geodesic acceleration (affine parameter).
// Central force: a is parallel to x (spherical symmetry, no poles).
void cartAccel(vec3 x, vec3 v, float dt, float rs, out vec3 a, out float ddt) {
  float r = max(length(x), 1e-6);
  float q = max(r - rs, 1e-4 * rs);
  float rn = dot(x, v) / r;
  float trans = dot(v, v) - rn * rn;
  float r2 = r * r;
  float r3 = r2 * r;
  float r4 = r3 * r;
  float s = trans / r3 + q * dt * dt / (2.0 * r4) - rn * rn / (2.0 * r2 * q);
  a = (-rs * s) * x;
  ddt = -(rs / (r * q)) * dt * rn;
}
// Project the velocity back onto the null cone (null-drift correction).
void projectNull(vec3 x, inout vec3 v, inout float dt, float rs) {
  dt = nullDt(x, v, rs);
}
void cartRk4(float h, inout vec3 x, inout vec3 v, inout float dt, float rs) {
  vec3 a1, a2, a3, a4;
  float d1, d2, d3, d4;
  cartAccel(x, v, dt, rs, a1, d1);
  vec3 x2 = x + (0.5 * h) * v;
  vec3 v2 = v + (0.5 * h) * a1;
  cartAccel(x2, v2, dt + 0.5 * h * d1, rs, a2, d2);
  vec3 x3 = x + (0.5 * h) * v2;
  vec3 v3 = v + (0.5 * h) * a2;
  cartAccel(x3, v3, dt + 0.5 * h * d2, rs, a3, d3);
  vec3 x4 = x + h * v3;
  vec3 v4 = v + h * a3;
  cartAccel(x4, v4, dt + h * d3, rs, a4, d4);
  x += (h / 6.0) * (v + 2.0 * v2 + 2.0 * v3 + v4);
  v += (h / 6.0) * (a1 + 2.0 * a2 + 2.0 * a3 + a4);
  dt += (h / 6.0) * (d1 + 2.0 * d2 + 2.0 * d3 + d4);
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
// HDR-linear finish (v3.0.0): exposure gain + vignette only. ACES + gamma
// + grain moved to post_comp.frag; this returns unbounded linear color.
vec3 applyFinish(vec3 col, vec2 uv) {
  col *= 1.1 * u_Exposure;
  float d = distance(uv, vec2(0.5));
  float vig = 1.0 - 0.3 * smoothstep(0.2, 0.8, d);
  col *= vig;
  return max(col, vec3(0.0));
}
// Novikov-Thorne flux shape (unnormalized): F ~ (rIn/r)^3 (1 - sqrt(rIn/r)).
float ntShape(float r, float rIn) {
  float x = rIn / r;
  return pow(x, 3.0) * max(1.0 - sqrt(x), 0.0);
}
void main() {
  vec2 ndc = v_TexCoord * 2.0 - 1.0;
  ndc.x *= u_Aspect;
  float th_ = tan(0.5 * u_FovY);
  vec3 dir = normalize(u_CameraForward + ndc.x * th_ * u_CameraRight +
                       ndc.y * th_ * u_CameraUp);
  // Cartesian march state: position x, velocity v = dx/dLambda, time rate dt.
  vec3 x = u_CameraPosition;
  vec3 v = dir;
  float r = length(x);
  float dt = nullDt(x, v, u_Rs);
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
  float rIn = u_Rs * 3.0;   // ISCO: r = 6M = 3 Rs (holy, never change)
  float rOut = u_Rs * 7.0;   // compact ring: camera at 10 Rs stays outside

  // Close-camera factor: 0 far away, 1 up close (disk detail smoothed).
  // Spawn distance sgra 10 Rs gives exactly 0, so the far reference shot is
  // kept. This is LOD, not a crutch: only turbulence relaxes to its mean.
  float camDistRs = (u_CamDist <= 0.0) ? 1000.0 : u_CamDist;
  float closeK = 1.0 - smoothstep(8.0, 10.0, camDistRs);

  // Adaptive step: h = clamp(K*q*sqrt(r), hMin, hMax), i.e. the k*r^1.5 law
  // with the horizon-regularizing f = q/r factor (see header). K is set so
  // h(1.5Rs) = u_StepSize; bounds scale with u_StepSize (preset semantics).
  float kStep = u_StepSize / (0.5 * u_Rs * sqrt(1.5 * u_Rs));
  float hMin = 0.004 * u_StepSize;
  float hMax = 8.0 * u_StepSize;

  for (int i = 0; i < ABSOLUTE_MAX_STEPS; i++) {
    if (i >= u_MaxSteps) break;
    if (r <= u_Rs * 1.001) break;
    if (r >= u_ShellRadius) break;
    vec3 xPrev = x;
    float q = max(r - u_Rs, 1e-4 * u_Rs);
    float hAdapt = clamp(kStep * q * sqrt(r), hMin, hMax);
    cartRk4(hAdapt, x, v, dt, u_Rs);
    projectNull(x, v, dt, u_Rs);
    r = length(x);
    if (r < minR) minR = r;
    fb = normalize(v);

    if (u_DiskOn == 1) {
      // Thin sheet: exact plane-crossing interpolation in position space.
      if (xPrev.y * x.y <= 0.0 && abs(xPrev.y - x.y) > 1e-6) {
        float tDisk = clamp(abs(xPrev.y) / (abs(xPrev.y) + abs(x.y)), 0.0, 1.0);
        vec3 xDisk = mix(xPrev, x, tDisk);
        float rDisk = length(xDisk);
        float phiDisk = atan(xDisk.z, xDisk.x);
        if (rDisk >= rIn && rDisk <= rOut) {
          float fDisk = max(1.0 - u_Rs / rDisk, 0.0001);

          // Exact Keplerian angular velocity Omega = sqrt(M/r^3), M = Rs/2.
          // Local static-frame orbital speed beta = r*Omega/sqrt(fDisk);
          // at ISCO beta = 0.5 exactly.
          float beta = sqrt(0.5 * u_Rs / (rDisk * fDisk));
          float gamma = 1.0 / sqrt(max(1.0 - beta * beta, 0.0001));
          vec3 vDir = vec3(-sin(phiDisk), 0.0, cos(phiDisk));

          // Photon flight direction (scene -> camera) in the local static
          // frame: radial leg rn/sqrt(f), transverse leg unstretched.
          vec3 nrm = xDisk / max(rDisk, 1e-6);
          float rn = dot(xDisk, v) / max(rDisk, 1e-6);
          vec3 nLoc = rn * nrm / sqrt(fDisk) + (v - rn * nrm);
          vec3 flight = -normalize(nLoc);
          float betaDot = dot((beta * vDir), flight);

          // Local beaming: g = 1/(gamma (1 - beta.nHat)). The approaching
          // side (material velocity aligned with the flight direction)
          // brightens, the receding side dims.
          float g = clamp(1.0 / (gamma * (1.0 - betaDot)), 0.05, 5.0);
          // Emission gravity redshift (GR energetics): the observed bolometric
          // flux scales as F*(sqrt(fDisk)*g)^4 = F*g^4*fDisk^2 (photon energy
          // plus arrival rate; the observer side lives in applyObserverShift).

          // Novikov-Thorne flux F ~ r^-3 (1 - sqrt(rIn/r)), T ~ F^0.25.
          float shape = pow(rIn / rDisk, 3.0) * max(1.0 - sqrt(rIn / rDisk), 0.0);
          float Fn = shape * NT_PEAK_NORM;
          float Tn = pow(max(Fn, 0.0), 0.25);

          // Bolometric brightness ~ F*g^4*fDisk^2 (Doppler g^4 times the
          // emission gravity-redshift energetics), clamped for no blowout.
          float g4 = g * g * g * g;
          float brightness = shape * g4 * fDisk * fDisk;

          // Black-body color: local temperature from Tn (13000 K at the
          // flux peak -> 2400 K at the rim), observed through emission
          // gravity redshift sqrt(fDisk) and Doppler sqrt(g).
          float tEmit = mix(2400.0, 13000.0, Tn);
          float tObs = tEmit * sqrt(max(g, 0.001) * fDisk);
          vec3 dcol = blackBodyColor(tObs);
          // Concentrated toward ISCO (high beaming contrast), dim floor outside.
          float xx = rIn / rDisk;
          vec3 emis = dcol * (0.03 + 4.0 * Tn * Tn * Tn * Tn) * (0.10 + 0.90 * xx * xx * xx);
          // Hot ISCO rim: Gaussian brightness peak at rIn, steep outward
          // falloff exp(-((r-rIn)/w)^2), w = 0.5 Rs. Strictly radial.
          float rimQ = (rDisk - rIn) / (0.5 * u_Rs);
          float rimG = exp(-rimQ * rimQ);
          float rim = 1.0 + 1.6 * rimG;
          emis *= rim;
          // Keplerian turbulence: fbm in (log(r)*2.5, phi*1.5 - phase),
          // phase = Omega*(u_Time - 1.0)*KEPLER_TIME_K. Exact Kepler shear
          // (inner laps outer); anchored to the harness epoch u_Time == 1.0
          // so the reference frame is deterministic. Beaming geometry above
          // (vDir, g, g4) uses the UNROTATED phiDisk, so the approaching-side
          // asymmetry stays fixed in space and the doppler gate holds.
          float omega = sqrt(0.5 * u_Rs / (rDisk * rDisk * rDisk));
          float phAdv = omega * (u_Time - 1.0) * KEPLER_TIME_K;
          vec2 tuv = vec2(log(max(rDisk, 0.001)) * 2.5, phiDisk * 1.5 - phAdv);
          float warp = diskFbm(tuv + vec2(0.31, 0.17));
          float nse = diskFbm(tuv + warp * 0.8);
          // Close-camera disk smoothing: high-frequency bands relax to their
          // mean (1.0), the large-scale Doppler gradient (g4) is kept.
          // Far away closeK == 0, so reference frames stay deterministic.
          float turb = mix(0.55 + 0.9 * nse, 1.0, closeK);
          emis *= turb;
          // Differential pinwheel: m=3 spiral wound by the same Kepler
          // shear. Zero at the harness epoch (phase difference), visibly
          // winding live. Zero azimuthal mean: g4 beaming untouched.
          float phiSw = phiDisk - phAdv / 1.5;
          float swirlArg = log(max(rDisk, 0.001)) * 5.0;
          float swirl = sin(phiSw * 3.0 + swirlArg) - sin(phiDisk * 3.0 + swirlArg);
          emis *= 1.0 + 0.30 * swirl;
          // Uniform exposure; beaming geometry untouched.
          vec3 diskRgb = emis * g4 * 1.5;
          diskRgb = clamp(diskRgb, 0.0, 12.0);
          diskRgb *= fDisk * fDisk;
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
      // Weak halo: optically thin exp(-|y|/h) wings around the sheet.
      // Emission scales with the step length (path integral), so the
      // adaptive step does not bias it. Dim by construction.
      if (abs(x.y) < 1.5 * u_Rs && r >= rIn && r <= rOut) {
        float hw = exp(-abs(x.y) / (HALO_H * u_Rs));
        float hShape = ntShape(r, rIn);
        float hTn = pow(max(hShape * NT_PEAK_NORM, 0.0), 0.25);
        vec3 hcol = blackBodyColor(mix(2400.0, 13000.0, hTn)) * hShape;
        accumColor += (1.0 - accumAlpha) * hcol * hw * hAdapt * HALO_K;
      }
      // ISCO corona: compact hot glow hugging the inner rim above/below
      // the sheet (funnel base). Axisymmetric NT-weighted white-blue,
      // volumetric (x hAdapt) like the jet/halo; per-crossing beaming math
      // untouched, so the doppler asymmetry is preserved.
      if (u_DiskOn == 1 && r >= rIn && r <= rIn + 3.0 * u_Rs) {
        float corR = (r - rIn) / (1.2 * u_Rs);
        float corY = abs(x.y) / (0.55 * u_Rs);
        float corShape = ntShape(r, rIn);
        vec3 corCol = vec3(0.66, 0.78, 1.0) * corShape * NT_PEAK_NORM * 0.05;
        accumColor += (1.0 - accumAlpha) * corCol *
                      exp(-corR * corR) * exp(-corY * corY) * hAdapt * 0.05;
      }
    }

    // M87* relativistic jet: polar cones along +-Y, additive glow, no alpha.
    // Independent of u_DiskOn so it shows with disk on or off.
    // Volumetric emission is step-invariant: scaled by hAdapt/0.125 (legacy
    // far step), so the adaptive march integrates the same totals as before.
    // v2.7: longer reach (0.08 falloff), brighter core, helical filaments.
    if (u_JetOn == 1) {
      float jetStepK = hAdapt / 0.125;
      float axSin = length(x.xz) / max(r, 1e-6);
      float phiJet = atan(x.z, x.x);
      if (axSin < 0.14 && r >= 2.0 && r <= 32.0) {
        float streaks = diskFbm(vec2(r * 1.5 - u_Time * 0.4, phiJet * 2.0));
        float knots = pow(max(streaks, 0.0), 3.0);
        // Helical filament: two strands winding along the jet axis.
        float helix = 0.5 + 0.5 * sin(phiJet * 2.0 + r * 0.55 - u_Time * 1.6);
        float helix2 = 0.5 + 0.5 * sin(phiJet * 2.0 + 3.14159 + r * 0.55 - u_Time * 1.6);
        float filament = max(helix, helix2);
        // Tuned: volumetric emission integrates over many in-cone steps;
        // 0.1+ saturates white; 0.045 + filaments reads as a structured jet.
        vec3 jcol = vec3(0.60, 0.75, 1.0) * (knots + 0.35 * filament * knots)
            * exp(-(r - 2.0) * 0.08) * 0.045 * jetStepK;
        accumColor += (1.0 - accumAlpha) * jcol * float(u_JetOn);
        // HST-1-like knots: 3 discrete blobs riding outward from the core,
        // fading with travel distance. Axial (r) + time only — symmetric
        // about the jet axis, disk/beaming math untouched. Kept dim: the
        // volumetric march integrates over many in-cone steps, 0.05 saturated
        // the whole column white (proven by m87.png); 0.006 reads as knots.
        float kR0 = 2.0 + fract(u_Time * 0.10 + 0.00) * 18.0;
        float kR1 = 2.0 + fract(u_Time * 0.10 + 0.37) * 18.0;
        float kR2 = 2.0 + fract(u_Time * 0.10 + 0.71) * 18.0;
        float kSig = 0.0;
        kSig += exp(-pow((r - kR0) * 2.0, 2.0)) * exp(-(kR0 - 2.0) * 0.08);
        kSig += exp(-pow((r - kR1) * 2.0, 2.0)) * exp(-(kR1 - 2.0) * 0.08);
        kSig += exp(-pow((r - kR2) * 2.0, 2.0)) * exp(-(kR2 - 2.0) * 0.08);
        vec3 kcol = vec3(0.65, 0.78, 1.0) * kSig * 0.008 * jetStepK;
        accumColor += (1.0 - accumAlpha) * kcol * float(u_JetOn);
      }
      // Faint radio lobe: wide dim shell around the jet base, axial + time.
      if (axSin < 0.42 && r >= 2.0 && r <= 20.0) {
        float lobeR = exp(-pow((r - 6.0) / 7.0, 2.0));
        float lobeW = 1.0 - smoothstep(0.14, 0.42, axSin);
        vec3 lobeCol = vec3(0.45, 0.55, 0.85) * lobeR * lobeW * 0.003 * jetStepK;
        accumColor += (1.0 - accumAlpha) * lobeCol * float(u_JetOn);
      }
    }
  }

  if (r <= u_Rs * 1.001) {
    // Captured rays carry no disk/sky light: hard black shadow.
    if (u_DiskOn == 1) {
      FragColor = vec4(applyFinish(applyObserverShift(vec3(0.0), fCamSqrt, camRedden), v_TexCoord), 1.0);
    } else {
      FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    return;
  }
  if (r >= u_ShellRadius) {
    vec3 skyDir = aberrateDir(fb);
    vec3 sky = texture(u_Skybox, skyDir).rgb;
    // Milky-Way band gain: boost sky along the band plane (fb.y ~ 0),
    // same profile as the procedural cubemap. Neutral at u_BandK == 1.0.
    float bandW = exp(-abs(skyDir.y) * 5.0);
    sky *= 1.0 + (u_BandK - 1.0) * bandW;
    // Einstein-ring lensing: rays that grazed the photon sphere magnify
    // the lensed sky. Peak at minR ~ 1.5 Rs, falls off by ~3.5 Rs.
    float lensQ = (minR - 1.5 * u_Rs) / (1.1 * u_Rs);
    float lensMag = 1.0 + 0.55 * exp(-lensQ * lensQ);
    sky *= lensMag;
    // Jet glow lives in accumColor with no alpha; add it even with disk off
    // (sgra accumulates nothing, so its disk-off frame is unchanged).
    // Capture branch above stays black: the hole absorbs light.
    vec3 col = (u_DiskOn == 1) ? accumColor + (1.0 - accumAlpha) * sky : sky + accumColor;
    float ringQ = (minR - 1.5 * u_Rs) / (0.055 * u_Rs);
    float ring = exp(-ringQ * ringQ);
    // NOTE: Schwarzschild has no true ergosphere (Kerr feature) - stylized
    // photon-ring visualization. Shimmer uses the exit azimuth as honest
    // approximation (no azimuth stored at minR).
    float shimmer = 0.85 + 0.15 * sin(u_Time * 2.0 + atan(v.z, v.x) * 3.0);
    // Slow symmetric breathing of the ring light (time-only, no azimuth).
    float breath = 0.94 + 0.06 * sin(u_Time * 0.8);
    ring *= shimmer * breath;
    // Peak whitening: warm wings (1.0,0.96,0.88) -> white peak
    // (1.0,0.98,0.94). Mix factor is the pure radial Gaussian
    // (ringQ only, no azimuth) so only the peak whitens; beaming untouched.
    float ringPeakW = clamp(exp(-ringQ * ringQ), 0.0, 1.0);
    vec3 ringCol = mix(vec3(1.0, 0.96, 0.88), vec3(1.0, 0.98, 0.94), ringPeakW);
    col += ringCol * ring * (3.0 * u_BHRingK);
    // Secondary photon sub-ring: thinner, fainter, just outside primary.
    // Pure minR profile — radially symmetric, beaming untouched.
    float ring2Q = (minR - 1.5 * u_Rs - 0.10 * u_Rs) / (0.028 * u_Rs);
    float ring2 = exp(-ring2Q * ring2Q);
    ring2 *= shimmer * breath;
    col += vec3(0.92, 0.95, 1.0) * ring2 * (0.7 * u_BHRingK);
    // Tertiary sub-ring (n=3): hairline, dimmer, just outside secondary.
    // Same minR-profile discipline: radially symmetric, beaming untouched.
    float ring3Q = (minR - 1.5 * u_Rs - 0.16 * u_Rs) / (0.016 * u_Rs);
    float ring3 = exp(-ring3Q * ring3Q);
    ring3 *= shimmer * breath;
    col += vec3(0.88, 0.93, 1.0) * ring3 * (0.35 * u_BHRingK);
    float eglowQ = (minR - u_Rs) / (0.5 * u_Rs);
    float eglow = exp(-eglowQ * eglowQ) * step(minR, 3.0 * u_Rs);
    // Whitened and dimmed: the glow hugs the rim instead of spreading gold.
    col += vec3(1.0, 0.75, 0.55) * eglow * 0.22;
    if (u_DiskOn == 1) {
      // Bloom of the accumulated DISK light only (luma from accumColor):
      // bare sky never blooms, so disk-off/disk-on skies match exactly and
      // the doppler subtraction sees pure disk signal. The whitening mix
      // still uses col, so disk+ring pixels bloom exactly as before.
      float glowLuma = dot(accumColor, vec3(0.299, 0.587, 0.114));
      float glowF = smoothstep(0.2, 0.8, glowLuma);
      col += mix(mix(col, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowF * 0.35;
    }
    // Far marker: beyond ~50 Rs the shadow is subpixel; keep a bright
    // point so the hole never silently vanishes. Exactly zero at
    // spawn/close range: reference frames deterministic. Ascending smoothstep.
    float farDotK = smoothstep(50.0, 200.0, camDistRs);
    col += vec3(1.0, 0.96, 0.88) * farDotK * exp(-dot(ndc, ndc) * 900.0) * 2.5;
    FragColor = vec4(applyFinish(applyObserverShift(col, fCamSqrt, camRedden), v_TexCoord), 1.0);
    return;
  }

  // Step budget exhausted without capture or escape: single minR gate.
  // Near-capture rays (minR < 2.2Rs) are unconverged winding noise -> black;
  // the rest resolve to the sky they were heading for.
  vec3 fbDir = aberrateDir(fb);
  vec3 skyFallback = texture(u_Skybox, fbDir).rgb * vec3(0.18, 0.16, 0.22);
  float bandWFb = exp(-abs(fbDir.y) * 5.0);
  skyFallback *= 1.0 + (u_BandK - 1.0) * bandWFb;
  // Same jet-glow rule as the escape branch: visible with disk off too.
  vec3 colFallback = (u_DiskOn == 1)
                         ? accumColor + (1.0 - accumAlpha) * skyFallback
                         : skyFallback + accumColor;
  if (u_DiskOn == 1) {
    // Same disk-only bloom rule as the escape branch (see above).
    float glowLumaFb = dot(accumColor, vec3(0.299, 0.587, 0.114));
    float glowFFb = smoothstep(0.2, 0.8, glowLumaFb);
    colFallback += mix(mix(colFallback, vec3(1.0), 0.3), vec3(1.0), 0.5) * glowFFb * 0.35;
  }
  float fbGate = smoothstep(u_Rs * 1.1, u_Rs * 2.2, minR);
  colFallback = mix(vec3(0.0), colFallback, fbGate);
  // Far marker, same profile as the escape branch (see above): missed
  // rays toward a distant hole still leave a bright point. Zero unless far.
  float farDotKFb = smoothstep(50.0, 200.0, camDistRs);
  colFallback += vec3(1.0, 0.96, 0.88) * farDotKFb * exp(-dot(ndc, ndc) * 900.0) * 2.5;
  FragColor = vec4(applyFinish(applyObserverShift(colFallback, fCamSqrt, camRedden), v_TexCoord), 1.0);
}
