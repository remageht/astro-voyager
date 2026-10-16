#version 330 core
// 3C 273 quasar impostor (P2 revision): HDR core + relativistic jets +
// NT-disk + corona halo + de Vaucouleurs host + hard shadow + photon ring.
// Adapted from the WebGL Schwarzschild-reference ideas (FS_RAY + bloom):
// jet double-helix/knots/cocoon/beaming, disk NT+bbody/two-phase blend,
// exponential corona, hard shadow, m=1 host, dust torus. Refit to our
// GLSL 3.3 impostor: screen-space uv, u_Time epoch 1.0 (harness determinism),
// u_QCoreK/u_QZoom semantics kept, u_Exposure last, ACES+gamma as project.
// No bloom pass (separate task). No reference background stars (own skybox).
// Diffraction spikes removed (cheap look).
out vec4 FragColor;
in vec2 v_TexCoord;
uniform vec3 u_CoreColor;
uniform float u_Time;
uniform float u_Exposure;
uniform float u_QCoreK;  // quasar core gain (default 1.0; qso 1.5): core+ring+corona up, host down
uniform float u_QZoom;  // close-up magnification from camera distance (neutral 1.0)

float hash2(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float vnoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float a = hash21(i);
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));
  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm4(vec2 p) {
  float s = 0.0;
  float a = 0.5;
  mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
  for (int i = 0; i < 4; i++) {
    s += a * vnoise(p);
    p = m * p;
    a *= 0.5;
  }
  return s / 0.9375;
}

// Blackbody after Tanner (piecewise-logarithmic, result ^2.2), linear space.
vec3 bbody(float Tk) {
  float t = clamp(Tk, 1000.0, 40000.0) / 100.0;
  float rr;
  float gg;
  float bb;
  if (t <= 66.0) {
    rr = 1.0;
    gg = clamp(0.39008158 * log(t) - 0.63184144, 0.0, 1.0);
  } else {
    float u = t - 60.0;
    rr = clamp(1.29293619 * pow(u, -0.13320476), 0.0, 1.0);
    gg = clamp(1.12989086 * pow(u, -0.07551485), 0.0, 1.0);
  }
  if (t >= 66.0) {
    bb = 1.0;
  } else if (t <= 19.0) {
    bb = 0.0;
  } else {
    bb = clamp(0.54320679 * log(t - 10.0) - 1.19625409, 0.0, 1.0);
  }
  return pow(vec3(rr, gg, bb), vec3(2.2));
}

// v3.0.0: ACES + gamma moved to post_comp.frag; this shader outputs linear HDR.

vec3 starLayer(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  // v3.5.0: sin-based lattice hash (hash2) instead of hash21: the latter
  // clusters on integer lattices, which aligned the star offsets into
  // visible dotted columns. Jitter is near-full-cell so the field reads
  // Poisson-like.
  float h0 = hash2(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash2(id + vec2(7.13, 1.37)), hash2(id + vec2(3.71, 9.41))) - vec2(0.5);
  float d = length(f - off * 0.9);
  float glow = exp(-d * d * 42.0);
  float h1 = hash2(id + vec2(9.17, 2.53));
  float bright = pow(h1, 12.0) * gain;
  vec3 tint = mix(vec3(0.65, 0.75, 1.0), vec3(1.0, 0.85, 0.65), hash2(id + vec2(5.31, 0.71)));
  return mask * glow * bright * tint;
}

// Disk plasma density: differential rotation, spiral arms cos(2phi+2.4*ln r),
// fbm turbulence + rings. rq is the effective Rs-radius (ISCO = 3 at inner edge).
float diskDensity(vec2 xz, float rq, float t) {
  float th = t * 0.707 * pow(rq, -1.5);
  float c = cos(th);
  float s = sin(th);
  vec2 q = vec2(c * xz.x + s * xz.y, -s * xz.x + c * xz.y);
  float phi = atan(xz.y, xz.x) - th;
  float arms = 0.5 + 0.5 * cos(2.0 * phi + 2.4 * log(max(rq, 1e-3)));
  arms *= arms;
  float n = fbm4(q * 0.55);
  float n2 = fbm4(q * 1.7 + n * 2.2);
  float n3 = fbm4(q * 5.0 - n2 * 1.5);
  float rings = 0.5 + 0.5 * sin(rq * 4.5 + n * 5.0);
  float d = 0.38 + 0.55 * (n - 0.5) + 0.5 * (n2 - 0.5) + 0.38 * (n3 - 0.5)
          + 0.50 * (arms - 0.4) + 0.12 * (rings - 0.5);
  return clamp(d, 0.04, 1.5);
}

void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  // Close-up zoom driven by camera distance (SceneManager sets u_QZoom from
  // spawn/dist, clamped >= 1). Far away qz == 1.0: framing anchored.
  float qz = (u_QZoom <= 0.0) ? 1.0 : u_QZoom;
  uv /= qz;
  // Up-close detail relaxation: 0 at qz == 1.
  float qCloseK = smoothstep(2.0, 6.0, qz);
  // Harness epoch: live evolution anchored so time == 1.0 is deterministic.
  float tE = u_Time - 1.0;
  float r = length(uv);

  // Hard black-hole shadow absorbs everything inside r < 0.015, at any zoom.
  float shadow = smoothstep(0.010, 0.015, r);

  // Near-black sky + own starfield layers (reference background NOT ported).
  vec3 col = vec3(0.004, 0.005, 0.009);
  col += starLayer(uv, 90.0, 0.992, 1.4);
  col += starLayer(uv + vec2(13.1), 190.0, 0.985, 0.55);

  // Torn elliptical host (de Vaucouleurs r^1/4), m=1 lopsidedness only
  // (no m=2 flower), fbm raggedness, dust-torus extinction at mid radii.
  // Dimmed as the core gain rises (x0.7 at u_QCoreK == 2.2); neutral at 1.0.
  float hostK = mix(1.0, 0.7, clamp((u_QCoreK - 1.0) / 1.2, 0.0, 1.0));
  vec2 euv = vec2(uv.x, uv.y * 1.6);
  float re = length(euv);
  float reS = max(re, 1e-4);
  vec3 hostCol = vec3(0.55, 0.47, 0.38) * 0.040 * exp(-pow(reS / 0.2, 0.25));
  hostCol *= 1.0 - smoothstep(0.3, 1.0, re);
  float hostAng = atan(euv.y, euv.x) - u_Time * 0.05;
  float hostM1 = 0.5 + 0.5 * sin(hostAng + 1.3 + re * 4.0);
  float rag = fbm4(euv * 14.0 + vec2(3.7)) - 0.5;
  hostCol *= 1.0 + (0.60 * (hostM1 - 0.5) + 0.90 * rag) * exp(-re * 4.0);
  float dTor = (re - 0.38) / 0.10;
  hostCol *= 1.0 - 0.55 * exp(-dTor * dTor);
  hostCol *= hostK * shadow;
  col += hostCol;

  // Accretion disk: NT temperature profile + bbody color, Kepler shear,
  // spiral arms, fbm rings; two-phase blend (half-period shifted copies,
  // weights wA/(1-wA)) so the winding never grows unbounded. Disk plane is
  // the flattened vec2(1.0, 2.2) projection; approaching side is -x.
  float diskR = length(uv * vec2(1.0, 2.2));
  float rn = diskR / 0.015;
  float rq = rn + 2.0;
  float PER = 40.0;
  float tD = tE * 2.5;
  float tA = mod(tD, PER);
  float tB = mod(tD + 0.5 * PER, PER);
  float wA = 1.0 - abs(2.0 * tA / PER - 1.0);
  vec2 dxz = vec2(uv.x, uv.y * 2.2) * 38.0;
  float dens = wA * diskDensity(dxz, rq, tA) + (1.0 - wA) * diskDensity(dxz, rq, tB);
  float densEff = mix(dens, 0.80, qCloseK * 0.35);
  float edge = smoothstep(0.015, 0.019, diskR) * (1.0 - smoothstep(0.09, 0.13, diskR));
  float beta = sqrt(0.5 / rq);
  float gam = inversesqrt(1.0 - beta * beta);
  float side = -uv.x / max(r, 1e-3);
  float bn = beta * side;
  float dop = clamp(1.0 / (gam * (1.0 - bn)), 0.4, 3.0);
  float grav = sqrt(max(1.0 - 1.0 / rq, 0.02));
  float Tobs = clamp(21000.0 * pow(3.0 / rq, 0.85) * dop * grav, 1000.0, 40000.0);
  vec3 dcol = bbody(Tobs);
  float inten = pow(Tobs / 8500.0, 2.2);
  vec3 diskTerm = dcol * (inten * densEff * 0.34) * edge * shadow;
  diskTerm *= mix(1.0, 0.55, qCloseK);
  col += diskTerm;

  // Corona: exponential atmosphere over the disk, peaking at the inner edge,
  // white-blue, mildly beamed. It (not a naked gain) feeds the blinding halo.
  float ayC = abs(uv.y);
  float hc = 0.012 + 0.004 * rn;
  float eC = exp(-ayC / hc) * pow(3.0 / max(rq, 2.4), 1.5)
           * (1.0 - smoothstep(0.09, 0.16, r)) * smoothstep(0.015, 0.019, diskR);
  float betaC = sqrt(0.5 / max(rq, 1.5));
  float bm = clamp(pow(1.0 / (1.0 - betaC * side), 2.5), 0.25, 4.0);
  vec3 corCol = vec3(0.62, 0.76, 1.0) * eC * bm * 0.20;
  corCol *= mix(1.0, 0.25, qCloseK);
  col += corCol * shadow;

  // Relativistic jets along x (+x approaching, boosted; -x de-beamed):
  // double helical filaments (two cos threads, pi shift, ^3), travelling
  // shock knots (sin^5), wide weak cocoon, blue->magenta gradient along the
  // length, smooth turn-on at the base, fade-out at the tip, 0.92c beaming
  // with sign(s) for the two sides, clamped gain.
  // v3.2.0 precession: the jet axis slowly nods around its mean
  // inclination (S-shaped sway). sin(tE * ...) is exactly zero at the
  // harness epoch (tE = 0), so reference frames stay deterministic.
  float precA = sin(tE * 0.08) * 0.028;
  float tilt = uv.y + (0.12 + precA) * uv.x + sin(u_Time * 0.3) * 0.008;
  float wig = 0.0020 * sin(abs(uv.x) * 18.0 - tE * 1.3);
  float rho = abs(tilt - wig);
  float s = uv.x;
  float ay = abs(s);
  // v3.2.0: slightly wider jets so the helix/thread structure reads at the
  // spawn framing too (was 0.0045 + 0.011*ay).
  float wJ = 0.006 + 0.016 * ay;
  float xJ = rho / max(wJ, 1e-5);
  float coreJ = exp(-xJ * xJ * 1.6);
  float tJ = tE * 1.3;
  float tK = tE * 2.2;
  float phJ = ay * 21.0 - tJ;
  float helix = pow(0.5 + 0.5 * cos(phJ), 3.0) + pow(0.5 + 0.5 * cos(phJ + 3.14159265), 3.0);
  float dTh = (xJ - 0.72) * 2.8;
  float thread = exp(-dTh * dTh) * helix;
  // v3.2.0: coarser travelling knots (19 -> 11 per unit, gentler wobble).
  float knot = pow(0.5 + 0.5 * sin(ay * 11.0 - tK + 1.1 * sin(ay * 4.0)), 5.0);
  float fadeIn = smoothstep(0.008, 0.030, ay);
  float fadeOut = 1.0 - smoothstep(0.26, 0.95, ay);
  float wisp = 0.7 + 0.3 * sin(s * 40.0 + ay * 18.0 - tK * 0.6)
                   * sin(tilt * 90.0 - ay * 14.0 + tK * 0.4);
  float cocoon = exp(-xJ * xJ * 0.09) * 0.05;
  float eJ = (0.30 * coreJ + 0.9 * thread + cocoon) * (0.25 + 1.7 * knot) * fadeIn * fadeOut * wisp;
  // Ejected blob: a fresh shock born at the base every 9 s, riding outward
  // to ay ~ 0.5 and dying. Zero at the harness epoch (tE == 0 -> phase 0,
  // born at the base inside fadeIn), so reference frames stay pixel-identical.
  float ejPh = fract(tE / 9.0);
  float ejR = 0.01 + ejPh * 0.5;
  float ejEnv = smoothstep(0.0, 0.08, ejPh) * (1.0 - smoothstep(0.55, 1.0, ejPh));
  float ejSig = exp(-pow((ay - ejR) * 22.0, 2.0)) * exp(-xJ * xJ * 1.2);
  eJ += 1.1 * ejSig * ejEnv;
  float tt = clamp(ay / 0.85, 0.0, 1.2);
  vec3 jc = mix(vec3(0.42, 0.68, 1.0), vec3(0.72, 0.25, 0.95), smoothstep(0.08, 0.85, tt));
  jc = mix(jc, vec3(0.88, 0.96, 1.0), knot * 0.45 * coreJ);
  float betaJ = 0.92;
  float gamJ = inversesqrt(1.0 - betaJ * betaJ);
  float mu = 0.7;
  float dop0 = 1.0 / gamJ;
  float beam = (s >= 0.0)
      ? pow((1.0 / (gamJ * (1.0 - betaJ * mu))) / dop0, 2.2)
      : pow((1.0 / (gamJ * (1.0 + betaJ * mu))) / dop0, 2.2);
  beam = clamp(beam, 0.02, 12.0);
  float gate = 1.0 - smoothstep(0.2, 0.9, r);
  vec3 jetCol = jc * eJ * beam * 0.55;
  jetCol *= mix(1.0, 0.30, qCloseK);
  // Dust-torus extinction crosses the jet too (dark gap at mid radii).
  jetCol *= 1.0 - 0.45 * exp(-dTor * dTor);
  if (xJ < 5.0) {
    col += jetCol * gate * shadow;
  }

  // HDR core with tight falloff, 2-3 px feel. No cross spikes.
  // BH vs quasar look: a black hole shows a dark shadow + lensed thin disk
  // with a dark core; a quasar core outshines everything (u_QCoreK) + jets.
  // The shadow absorbs core light inside r < 0.015.
  float coreFall = 1.0 / (1.0 + r * r * 20000.0);
  // v3.2.0: 10.0 -> 8.0 so the new arcs/ring structure is not washed out by
  // the naked core; still a blinding quasar from afar (x u_QCoreK x proxDim).
  vec3 coreBase = vec3(1.0, 0.98, 0.94) * 8.0;
  vec3 tint = u_CoreColor / vec3(1.0, 0.85, 0.65);
  float flick = 0.94 + 0.05 * sin(u_Time * 2.3) + 0.02 * sin(u_Time * 5.7);
  // Slow mode (~0.2 Hz in tE): zero at the harness epoch, alive otherwise.
  flick += 0.015 * sin(tE * 1.1);
  // Proximity dimming: blinding from afar, compact structured core/ring/halo
  // up close (no milky blowout at qso_close). Disk/jet keep their own
  // milder close gating so structure keeps reading.
  float proxDim = mix(1.0, 0.08, qCloseK);
  col += coreBase * tint * coreFall * flick * shadow * u_QCoreK * proxDim;

  // Thin white photon ring hugging the shadow edge, slight m=1 asymmetry
  // (spin fake). Shadow-gated so nothing leaks into the black disc.
  float ringD = (r - 0.0195) * 150.0;
  float ringB = exp(-ringD * ringD);
  float ang = atan(uv.y, uv.x) - u_Time * 0.35;
  float asym = 0.72 + 0.28 * sin(ang);
  col += vec3(1.0, 0.96, 0.90) * ringB * asym * (2.6 * u_QCoreK) * proxDim * shadow;

  // v3.2.0 lensed secondary arcs: the disk's higher-order images seen over
  // and under the shadow as two thin crescents. arcPar traces the parabola
  // |uv.y| = 0.024 + 0.9*uv.x^2 in the disk's flattened frame; the two
  // exponential gates cut it to a ring-shaped arc pair. Intensity rides the
  // core gain with a mild close-camera relax (unlike the core's proxDim),
  // stays shadow-gated and is time-independent, so reference frames are
  // deterministic.
  float arcPar = abs(uv.y) - (0.024 + 0.9 * uv.x * uv.x);
  float arcSig = exp(-pow(arcPar * 240.0, 2.0));
  float arcGate = exp(-uv.x * uv.x * 420.0) * (1.0 - smoothstep(0.05, 0.12, r));
  vec3 arcCol = vec3(0.72, 0.80, 1.0) * arcSig * arcGate *
                (0.55 * u_QCoreK) * mix(1.0, 0.35, qCloseK) * shadow;
  col += arcCol;

  // Core-touch hint: a soft white flash when the camera pushes into the
  // core shell (qz > 8, i.e. D < ~3 at spawn 25) to accompany the solid
  // slide handled on CPU. Zero for all reference framings (spawn qz = 1,
  // qso_close qz = 5): pixel-identical. Ascending smoothstep, u_QZoom read
  // only. Station label stays in AppGL, never culled.
  float touchK = smoothstep(8.0, 12.0, qz);
  float touchPulse = 0.75 + 0.25 * sin(u_Time * 6.0);
  col += vec3(1.0, 0.97, 0.92) * touchK * touchPulse * exp(-r * r * 40.0) * 0.6;

  // Vignette about 0.3.
  float vig = 1.0 - 0.3 * clamp(dot(uv * 0.7, uv * 0.7), 0.0, 1.0);
  col *= vig;

  // Linear exposure gain (tonemap + gamma + grain: post_comp.frag).
  col *= u_Exposure;

  FragColor = vec4(max(col, vec3(0.0)), 1.0);
}
