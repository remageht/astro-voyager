#version 330 core
// Tidal Disruption Event AT2019qiz (2MASX J04463790-1013349, 215 Mly).
// Physical & visual phenomena:
//   - Supermassive Black Hole (M_BH ~ 10^6 M☉) with event horizon and photon sphere
//   - Main-sequence star shredded past tidal radius r_t ~ 24 r_s
//   - Relativistic spaghettification: elongated curved stream wrapping the horizon
//   - Doppler beaming: approaching arm blue-shifted (v ~ 0.3c), receding arm red-shifted
//   - Apsidal precession causing stream-stream self-intersection shock knots
//   - Peak super-Eddington accretion flare with radiation-driven relativistic debris wind
//   - Fallback power-law decay and disk cooling following Rees (1988) t^(-5/3) law
//
// Evolution controlled by u_Phase (0.0 .. 1.0):
//   0.00 .. 0.25 - Tidal Capture & Peeling (star penetrates r_t)
//   0.25 .. 0.50 - Spaghettification & Stream Collision Shock Knots
//   0.50 .. 0.75 - Peak Super-Eddington Accretion Flare & Relativistic Outflow Wind
//   0.75 .. 1.00 - Fallback Decay (t^-5/3) & Accretion Disk Cooling
// Output: Linear HDR (ACES tonemapped downstream by post_comp.frag).

out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;
uniform float u_Phase;      // 0.0 .. 1.0 (evolution timeline)
uniform vec2  u_ObjNdc;     // object anchor in NDC
uniform float u_ObjZoom;    // magnification
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

// Background stars of host galaxy 2MASX J04463790-1013349
vec3 hostGalaxyStars(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  float h0 = hash2(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash2(id + vec2(5.17, 3.41)), hash2(id + vec2(8.23, 1.77))) - vec2(0.5);
  float d = length(f - off * 0.85);
  float glow = exp(-d * d * 52.0);
  float h1 = hash2(id + vec2(2.13, 9.61));
  float bright = pow(h1, 12.0) * gain;
  vec3 tint = mix(vec3(0.65, 0.85, 1.00), vec3(1.00, 0.92, 0.75), hash2(id + vec2(4.19, 7.33)));
  return mask * glow * bright * tint;
}

void main() {
  vec2 p = v_TexCoord * 2.0 - 1.0;
  vec2 starUV = p;
  p -= u_ObjNdc;
  p.x *= u_Aspect;
  p /= max(u_ObjZoom, 0.1);
  p += u_CamPar.xy * 0.15;

  float phase = clamp(u_Phase, 0.0, 1.0);
  float tE = u_Time * 0.28;

  // Host galaxy subtle background core glow
  float rRaw = length(p);
  vec3 col = vec3(0.0);
  float hostCore = exp(-rRaw * rRaw * 2.2) * 0.07;
  col += vec3(0.55, 0.65, 0.95) * hostCore;

  // Background stars with gravitational deflection around SMBH
  float rS = 0.14; // Event horizon radius
  float gravDeflect = 0.22 * rS / max(rRaw - rS * 0.85, 0.015);
  vec2 lensedStarUV = starUV - normalize(p + vec2(1e-5)) * gravDeflect * 0.12;
  col += hostGalaxyStars(lensedStarUV, 26.0, 0.965, 1.6);
  col += hostGalaxyStars(lensedStarUV, 64.0, 0.982, 2.2);

  // Black hole metric polar coordinates
  float r = max(rRaw, 1e-4);
  float theta = atan(p.y, p.x);

  // Doppler boosting coordinate factor (approaching left side v ~ 0.3c)
  float dopplerFactor = 1.0 + 0.55 * (-p.x / max(r, 0.08));

  // =========================================================================
  // 1. Spaghettified Stellar Stream (Phase 0.00 .. 0.55)
  // =========================================================================
  // Curved spiral geodesic wrapping the SMBH horizon
  float streamPhaseK = 1.0 - smoothstep(0.45, 0.80, phase);
  if (streamPhaseK > 0.001) {
    // Incoming intact star before disruption (phase 0.00 .. 0.22)
    vec2 intactPos = vec2(0.52 - phase * 1.4, 0.36 - phase * 0.7);
    float dStar = length(p - intactPos);
    float starCore = exp(-dStar * dStar * 850.0);
    float starCorona = exp(-dStar * 18.0) * 0.45;
    vec3 intactCol = vec3(1.4, 1.25, 0.85) * (starCore * 3.5 + starCorona);
    float intactFade = 1.0 - smoothstep(0.18, 0.32, phase);
    col += intactCol * intactFade;

    // Relativistic tidal tail (spiral wrapper)
    float spiralA = theta + 2.85 * log(max(r / 0.16, 0.005)) - tE * 1.5;
    float streamEnvelope = abs(sin(spiralA * 0.5));
    // Stream narrows near periapsis, widens into debris fan outward
    float streamWidth = mix(0.018, 0.065, clamp(r / 0.7, 0.0, 1.0));
    float streamDist = streamEnvelope * r;
    float streamMask = 1.0 - smoothstep(0.0, streamWidth, streamDist);

    // Radial extent: stream stretches from rS*1.2 out to tidal radius rt ~ 0.85
    float radialGate = smoothstep(rS * 1.15, rS * 1.6, r) * (1.0 - smoothstep(0.65, 0.95, r));

    // Internal turbulence and clumping
    vec2 turbUV = vec2(spiralA * 2.0, r * 8.0 - tE * 2.5);
    float turb = fbm4(turbUV) * 0.7 + 0.65;

    // Color gradient: hot blue/white at periapsis and approaching arm, golden-red on trailing arm
    vec3 streamBlue = vec3(0.55, 0.90, 1.40) * 2.4;
    vec3 streamGold = vec3(1.35, 0.85, 0.35) * 1.8;
    vec3 streamColor = mix(streamGold, streamBlue, clamp(dopplerFactor - 0.7, 0.0, 1.0));

    float streamIntensity = streamMask * radialGate * turb * streamPhaseK * dopplerFactor;
    col += streamColor * streamIntensity;
  }

  // =========================================================================
  // 2. Stream-Stream Self-Intersection Shock Knots (Phase 0.20 .. 0.60)
  // =========================================================================
  // Apsidal precession causes returning stream to crash into incoming stream
  float shockK = smoothstep(0.20, 0.35, phase) * (1.0 - smoothstep(0.55, 0.75, phase));
  if (shockK > 0.001) {
    // Collision point near periapsis at r ~ 0.28, angle ~ 2.1 rad
    vec2 shockPos = vec2(cos(2.1), sin(2.1)) * 0.28;
    float dShock = length(p - shockPos);
    float shockCore = exp(-dShock * dShock * 1200.0) * 4.5;
    float shockGlow = exp(-dShock * 24.0) * 0.9;
    // High-temperature X-ray/UV shock radiation (blinding blue-white)
    vec3 shockColor = vec3(0.85, 1.15, 1.60) * (shockCore + shockGlow) * shockK;
    col += shockColor;

    // Secondary collision ripples / bow shock
    float shockWave = sin(dShock * 45.0 - tE * 8.0) * 0.5 + 0.5;
    float shockWaveGlow = shockWave * exp(-dShock * 14.0) * 0.35 * shockK;
    col += vec3(0.40, 0.75, 1.25) * shockWaveGlow;
  }

  // =========================================================================
  // 3. Peak Accretion Flare & Relativistic Outflow Wind (Phase 0.40 .. 1.00)
  // =========================================================================
  // Super-Eddington transient circularized accretion disk
  float flareBirth = smoothstep(0.35, 0.55, phase);
  if (flareBirth > 0.001) {
    // Fallback decay rate follows Rees (1988) t^-5/3 law
    float decayArg = 1.0 + 3.2 * max(phase - 0.55, 0.0);
    float fallbackRate = pow(decayArg, -1.6667); // (t - t0)^(-5/3)

    // Elliptical inclined accretion disk (inclination ~ 52 deg)
    float cosInc = 0.61;
    vec2 diskCoord = vec2(p.x, p.y / cosInc);
    float rDisk = length(diskCoord);
    float thetaDisk = atan(diskCoord.y, diskCoord.x);

    // Inner disk rim at ISCO ~ 3 rS, outer radius ~ 0.52
    float isco = rS * 2.2;
    float rDiskMax = 0.52;
    float diskRingMask = smoothstep(isco * 0.95, isco * 1.3, rDisk) * (1.0 - smoothstep(rDiskMax * 0.75, rDiskMax, rDisk));

    // Relativistic disk rotation and spiral density arms
    float diskSpiral = thetaDisk * 2.0 - 4.5 * log(max(rDisk / isco, 0.01)) + tE * 4.0;
    float armPattern = sin(diskSpiral) * 0.35 + 0.65;
    float diskTurb = fbm4(vec2(thetaDisk * 1.8, rDisk * 12.0 - tE * 2.0)) * 0.45 + 0.8;

    // Thermal spectrum shifts: 40,000 K cyan/UV at peak, cools to 8,000 K amber
    vec3 hotPeakColor = vec3(0.70, 1.10, 1.85) * 3.6;
    vec3 coolDecayColor = vec3(1.35, 0.82, 0.38) * 1.6;
    float coolMix = smoothstep(0.55, 0.95, phase);
    vec3 diskBaseColor = mix(hotPeakColor, coolDecayColor, coolMix);

    // Disk Doppler boosting
    float diskDoppler = 1.0 + 0.65 * (-diskCoord.x / max(rDisk, 0.05));
    float diskIntensity = diskRingMask * armPattern * diskTurb * diskDoppler * flareBirth * fallbackRate;
    col += diskBaseColor * diskIntensity;

    // High-velocity radiation-driven outflow wind (v_wind ~ 10,000 km/s)
    // Spherical/bipolar expanding envelope shielding the central engine
    float windRadius = 0.25 + (phase - 0.35) * 0.75;
    float windShellDist = abs(r - windRadius);
    float windShell = exp(-windShellDist * windShellDist * 38.0);
    float windNoise = fbm4(vec2(theta * 3.0, r * 6.0 - tE)) * 0.6 + 0.7;
    float windAlpha = smoothstep(0.40, 0.60, phase) * (1.0 - smoothstep(0.75, 1.00, phase));
    vec3 windColor = vec3(0.45, 0.75, 1.15) * windShell * windNoise * windAlpha * 1.25;
    col += windColor;

    // Core radiation flash (diffuse illumination around SMBH)
    float coreFlash = exp(-r * 7.5) * flareBirth * fallbackRate * 2.8;
    col += diskBaseColor * coreFlash;
  }

  // =========================================================================
  // 4. Photon Sphere & Black Hole Event Horizon Shadow
  // =========================================================================
  // Photon sphere at r = 1.5 * rS
  float rPhoton = rS * 1.5;
  float photonDist = abs(r - rPhoton);
  float photonRing = exp(-photonDist * photonDist * 2200.0) * 1.85;
  // Dynamic photon ring intensity boosts during peak flare
  float ringBoost = 1.0 + smoothstep(0.35, 0.55, phase) * 1.4;
  col += vec3(0.85, 1.10, 1.50) * photonRing * ringBoost;

  // Event horizon shadow: perfect absorption inside rS
  float shadowMask = smoothstep(rS * 0.98, rS * 1.04, r);
  col *= shadowMask;

  // Exposure tone adjustment
  col *= u_Exposure;

  FragColor = vec4(col, 1.0);
}
