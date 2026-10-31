#version 330 core
// Supernova SN 1987A (Large Magellanic Cloud, 168 kly).
// Iconic morphology:
//   - Tilted Central Equatorial Ring with ~30 shock-heated "hot spot pearls"
//   - Pair of upper and lower outer polar rings forming an hourglass envelope
//   - Inner asymmetric "keyhole" debris ejecta heated by 44Ti/56Co decay
//   - Compact neutron star remnant (pulsar wind nebula core)
//   - Expanding forward blast shock wave (v ~ 8000 km/s)
// Stages controlled by u_Phase (0.0 .. 1.0) corresponding to years 1987 .. 2026+:
//   0.00 .. 0.20 - Year 1987: Core collapse flash, initial flash ionization of ring
//   0.20 .. 0.50 - 1995-2003: Blast wave arrives at inner edge, first pearls ignite
//   0.50 .. 0.80 - 2004-2017: Full "string of pearls" blazes with thermal X-ray/optical shock
//   0.80 .. 1.00 - 2018-2026+: Shock punches past ring, inner keyhole ejecta & JWST crescent
// Output: Linear HDR (ACES tonemapped downstream by post_comp.frag).

out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;
uniform float u_Phase;      // 0.0 .. 1.0 (1987 .. 2026+ timeline)
uniform vec2  u_ObjNdc;     // object anchor in NDC
uniform float u_ObjZoom;    // close-up magnification
uniform vec3  u_CamPar;     // parallax from camera angle

// Hash & Noise utilities
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

// Background stars of Large Magellanic Cloud (LMC)
vec3 starLayer(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  float h0 = hash2(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash2(id + vec2(7.13, 1.37)), hash2(id + vec2(3.71, 9.41))) - vec2(0.5);
  float d = length(f - off * 0.9);
  float glow = exp(-d * d * 44.0);
  float h1 = hash2(id + vec2(9.17, 2.53));
  float bright = pow(h1, 14.0) * gain;
  vec3 tint = mix(vec3(0.70, 0.85, 1.00), vec3(1.00, 0.90, 0.70), hash2(id + vec2(5.31, 0.71)));
  return mask * glow * bright * tint;
}

// Elliptical ring evaluation helper
// Returns distance to ellipse with radius R and aspect ratio compression factor
float evalEllipseDist(vec2 p, float r, float yStretch) {
  vec2 ep = vec2(p.x, p.y * yStretch);
  return abs(length(ep) - r);
}

void main() {
  vec2 p = v_TexCoord * 2.0 - 1.0;
  vec2 starUV = p;
  p -= u_ObjNdc;
  p.x *= u_Aspect;
  p /= max(u_ObjZoom, 0.1);
  p += u_CamPar.xy * 0.15;

  float phase = clamp(u_Phase, 0.0, 1.0);
  float tE = u_Time * 0.25;

  // Rotation of SN 1987A ring system on the sky (~43 deg inclination, position angle tilt)
  float angle = 0.38; // ~22 deg rotation
  float ca = cos(angle);
  float sa = sin(angle);
  vec2 pr = mat2(ca, -sa, sa, ca) * p;

  // LMC Background field: rich star field and faint H-alpha emission
  vec3 col = vec3(0.002, 0.003, 0.007);
  col += starLayer(starUV, 75.0, 0.94, 2.8);
  col += starLayer(starUV + vec2(0.33, 0.71), 160.0, 0.97, 1.6);
  col += starLayer(starUV + vec2(-0.47, 0.19), 320.0, 0.985, 0.9);

  // Soft background H-alpha nebulosity of Large Magellanic Cloud
  float lmcNeb = fbm4(starUV * 1.8 + vec2(1.2, 0.4));
  col += vec3(0.022, 0.008, 0.016) * lmcNeb;

  // Pre-supernova progenitor companion stars (Stars 2 and 3 near the ring)
  vec2 star2Pos = vec2(0.32, 0.24);
  float dStar2 = length(pr - star2Pos);
  float glowStar2 = exp(-dStar2 * dStar2 * 600.0) * 4.5 + exp(-dStar2 * 14.0) * 0.35;
  col += vec3(0.85, 0.92, 1.10) * glowStar2;

  vec2 star3Pos = vec2(-0.28, -0.31);
  float dStar3 = length(pr - star3Pos);
  float glowStar3 = exp(-dStar3 * dStar3 * 650.0) * 3.8 + exp(-dStar3 * 15.0) * 0.30;
  col += vec3(0.95, 0.88, 0.75) * glowStar3;

  // =========================================================================
  // 1. Initial 1987 Core Collapse Flash (Phase 0.00 .. 0.20)
  // =========================================================================
  float flashWeight = (1.0 - smoothstep(0.0, 0.20, phase));
  float flashPulse = exp(-length(p) * 2.8) * 8.0 * flashWeight;
  vec3 flashCol = vec3(1.2, 1.3, 1.5) * flashPulse;
  col += flashCol;

  // =========================================================================
  // 2. Outer Polar Rings (Hourglass Geometry)
  // =========================================================================
  // Inclination y-compression ~ 1.38 (cos 43 deg ~ 0.725 => yStretch ~ 1.38)
  float yStretch = 1.38;

  // Upper polar ring (shifted up in pr.y)
  vec2 prUpper = pr - vec2(0.04, 0.22);
  float distUpper = evalEllipseDist(prUpper, 0.38, yStretch);
  float widthUpper = 0.024;
  float ringUpper = exp(-pow(distUpper / widthUpper, 2.0));
  // Azimuthal brightness variation (brighter on near side)
  float azUpper = atan(prUpper.y * yStretch, prUpper.x);
  float azModUpper = 0.70 + 0.30 * cos(azUpper - 1.2);
  float noiseUpper = 0.85 + 0.30 * vnoise(prUpper * 25.0);
  // Outer rings were flash-ionized in 1987, slowly fading over decades
  float outerRingTimeFade = 0.45 + 0.55 * exp(-phase * 1.8);
  vec3 outerRingCol = vec3(0.40, 0.65, 0.95) * (ringUpper * azModUpper * noiseUpper * outerRingTimeFade * 1.8);

  // Lower polar ring (shifted down in pr.y)
  vec2 prLower = pr - vec2(-0.04, -0.22);
  float distLower = evalEllipseDist(prLower, 0.38, yStretch);
  float widthLower = 0.024;
  float ringLower = exp(-pow(distLower / widthLower, 2.0));
  float azLower = atan(prLower.y * yStretch, prLower.x);
  float azModLower = 0.70 + 0.30 * cos(azLower + 1.2);
  float noiseLower = 0.85 + 0.30 * vnoise(prLower * 25.0 + vec2(4.2, 1.8));
  vec3 lowerRingCol = vec3(0.55, 0.45, 0.90) * (ringLower * azModLower * noiseLower * outerRingTimeFade * 1.7);

  col += outerRingCol + lowerRingCol;

  // =========================================================================
  // 3. Central Equatorial Ring (ER) & "String of Pearls" Shock Knots
  // =========================================================================
  float rER = 0.22; // radius of equatorial ring
  vec2 pER = vec2(pr.x, pr.y * yStretch);
  float dER = length(pER);
  float ringDist = abs(dER - rER);

  // Smooth continuous unshocked gas ring (diffuse red/orange [N II] + H-alpha)
  float ringBase = exp(-pow(ringDist / 0.016, 2.0));
  float gasNoise = 0.8 + 0.4 * fbm4(pr * 35.0);
  vec3 unshockedRing = vec3(0.85, 0.25, 0.12) * ringBase * gasNoise * 1.4;
  col += unshockedRing;

  // Shock wave expansion from center: reach rER at phase ~ 0.35
  // Blast wave radius rShock expands with phase
  float rShock = 0.02 + 0.32 * phase;
  float shockFront = exp(-pow(abs(dER - rShock) / 0.018, 2.0));
  float shockGlow = (1.0 - smoothstep(0.0, rShock, dER)) * exp(-dER * 3.5);
  col += vec3(0.20, 0.45, 0.85) * shockFront * 2.2 * smoothstep(0.15, 0.90, phase);
  col += vec3(0.10, 0.25, 0.50) * shockGlow * 0.8 * smoothstep(0.20, 1.0, phase);

  // Discrete Hot Spot Pearls (~32 clumps around the ring)
  // Each clump has a random angle on the ring and ignites when rShock reaches it
  float az = atan(pER.y, pER.x); // -PI .. PI
  float pearlTotal = 0.0;
  vec3 pearlColAccum = vec3(0.0);

  // Procedural string of pearls: 32 clumps distributed around the ring
  for (int k = 0; k < 32; ++k) {
    float kf = float(k);
    float clumpAngle = (kf / 32.0) * 6.2831853 - 3.14159265;
    // Add realistic jitter to clump positions
    clumpAngle += (hash21(vec2(kf * 3.17, 1.91)) - 0.5) * 0.14;
    float clumpR = rER + (hash21(vec2(kf * 7.43, 8.21)) - 0.5) * 0.016;

    // Position of pearl on screen
    vec2 pearlPos = vec2(cos(clumpAngle) * clumpR, (sin(clumpAngle) * clumpR) / yStretch);
    float distToPearl = length(pr - pearlPos);

    // Ignition trigger: depends on shock arrival time at this angle
    // Spot 1 ignited earliest on the east side (around az ~ 0.8)
    float arrivalOffset = 0.25 + 0.22 * sin(clumpAngle - 0.7) + 0.12 * hash21(vec2(kf, 4.31));
    float ignitePhase = 0.28 + arrivalOffset * 0.40;

    // Intensity curve: rises sharply when shock hits, peaks, then slowly plateaus
    float pearlAge = phase - ignitePhase;
    if (pearlAge > 0.0) {
      // Ignition ramp
      float ramp = smoothstep(0.0, 0.08, pearlAge);
      // Cooling / fading in late phase (JWST era)
      float fade = 1.0 - 0.35 * smoothstep(0.45, 0.75, pearlAge);
      float intensity = ramp * fade;

      // Hotspot core profile
      float core = exp(-pow(distToPearl / 0.0075, 2.0));
      float halo = exp(-distToPearl * 90.0) * 0.45;

      // Color shifts from ultra-hot white-blue (10^6 K) to intense golden peach
      vec3 tint = mix(vec3(0.90, 1.10, 1.40), vec3(1.35, 1.05, 0.70), clamp(pearlAge * 2.0, 0.0, 1.0));
      // Thermal high-energy spike
      float brilliance = core * 6.5 + halo * 2.0;

      pearlColAccum += tint * (brilliance * intensity);
    }
  }
  col += pearlColAccum;

  // =========================================================================
  // 4. Inner Keyhole Ejecta & Asymmetric Radioactive Debris (JWST Signature)
  // =========================================================================
  // Asymmetric inner crescent / peanut shape formed by radioactive decay of 44Ti
  if (phase > 0.30) {
    float ejectaWeight = smoothstep(0.30, 0.95, phase);
    // Expand inner debris over time
    float rEjectaMax = 0.08 + 0.07 * (phase - 0.30);
    // Debris coordinates with tilt and asymmetry
    vec2 pDebris = pr * vec2(1.15, 1.6);
    pDebris.x += 0.02 * sin(pr.y * 18.0);
    float dDebris = length(pDebris);

    // Keyhole cutout effect (characteristic depression / notch)
    float notch = 1.0 - 0.65 * exp(-pow(length(pDebris - vec2(0.025, 0.015)) / 0.035, 2.0));
    float debrisCore = exp(-pow(dDebris / rEjectaMax, 2.0)) * notch;
    float debrisFilaments = fbm4(pDebris * 40.0 + vec2(tE * 0.2, 0.0));

    // Warm peach / amber / crimson emission from cold dust and infrared emission lines (Ar II, S III)
    vec3 debrisCol = mix(vec3(0.95, 0.45, 0.20), vec3(1.20, 0.85, 0.50), debrisFilaments);
    col += debrisCol * (debrisCore * (0.8 + 0.6 * debrisFilaments) * ejectaWeight * 2.4);
  }

  // =========================================================================
  // 5. Central Compact Remnant (Neutron Star / Pulsar Wind Nebula Core)
  // =========================================================================
  // Compact core revealed in late phase as observed by JWST NIRCam/MIRI in 2024
  if (phase > 0.55) {
    float coreReveal = smoothstep(0.55, 0.90, phase);
    float dCore = length(pr - vec2(0.005, -0.004));
    float corePinpoint = exp(-dCore * dCore * 8000.0) * 8.0;
    float coreAura = exp(-dCore * 65.0) * 0.9;
    vec3 pwnCol = vec3(0.70, 0.95, 1.40) * (corePinpoint + coreAura) * coreReveal;
    col += pwnCol;
  }

  // Multiply by scene exposure
  col *= u_Exposure;

  FragColor = vec4(col, 1.0);
}
