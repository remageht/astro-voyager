#version 330 core
// Kilonova GW170817 / AT 2017gfo: Binary Neutron Star Merger.
// Stages controlled by u_Phase (0.0 .. 1.0):
//   0.00 .. 0.25 - Relativistic inspiral (two orbiting neutron stars, tidal distortion, GW wakes)
//   0.25 .. 0.50 - Hypermassive merger flash, expanding spacetime gravitational-wave ripples (h+, hx)
//   0.50 .. 0.75 - Relativistic polar jets (short GRB 170817A, spine-sheath, shock diamonds)
//   0.75 .. 1.00 - Expanding r-process nucleosynthesis fireball (polar blue wind + equatorial gold/crimson lanthanides)
// Output: Linear HDR (ACES tonemapped by post_comp.frag).

out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;
uniform float u_Phase;      // 0.0 .. 1.0 explosion scrub
uniform vec2  u_ObjNdc;     // object anchor in NDC
uniform float u_ObjZoom;    // close-up magnification
uniform vec3  u_CamPar;     // parallax from camera angle

// Hash & Noise functions
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

vec3 starLayer(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
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

void main() {
  vec2 p = v_TexCoord * 2.0 - 1.0;
  vec2 starUV = p;
  p -= u_ObjNdc;
  p.x *= u_Aspect;
  p /= max(u_ObjZoom, 0.1);
  p += u_CamPar.xy * 0.15;

  float phase = clamp(u_Phase, 0.0, 1.0);
  float tE = u_Time - 1.0;

  // Background deep space & host galaxy NGC 4993 (lenticular/elliptical host)
  vec3 col = vec3(0.003, 0.004, 0.008);
  col += starLayer(starUV, 80.0, 0.993, 1.3);
  col += starLayer(starUV + vec2(17.3, 8.4), 180.0, 0.986, 0.5);

  // NGC 4993 host galaxy: soft de Vaucouleurs profile, offset to the upper-left
  vec2 hostPos = p - vec2(-0.45, 0.28);
  float rHost = length(vec2(hostPos.x * 0.9 + hostPos.y * 0.4, hostPos.y * 0.9 - hostPos.x * 0.4));
  float hostL = exp(-pow(max(rHost, 0.001) * 2.8, 0.35)) * 0.28;
  vec3 hostCol = mix(vec3(1.0, 0.90, 0.72), vec3(0.9, 0.78, 0.60), clamp(rHost * 2.0, 0.0, 1.0));
  col += hostCol * hostL;

  float r = length(p);
  float phi = atan(p.y, p.x);

  // =========================================================================
  // STAGE 1: Relativistic Binary Inspiral (phase 0.0 .. 0.28)
  // =========================================================================
  float inspiralFade = 1.0 - smoothstep(0.22, 0.27, phase);
  if (inspiralFade > 0.001) {
    float subPhase = clamp(phase / 0.25, 0.0, 1.0);
    // Orbital separation shrinks as merger nears: a ~ (1 - subPhase)^0.25
    float sep = 0.28 * pow(max(1.0 - subPhase * 0.96, 0.01), 0.25);
    // Chirping orbital speed: frequency accelerates sharply near merger
    float chirpSpeed = (subPhase * subPhase * 16.0 + 3.0);
    float orbAngle = tE * chirpSpeed + subPhase * 24.0;

    // Binary mass ratio: M1 = 1.48 Msun, M2 = 1.26 Msun (GW170817)
    float q = 1.26 / 1.48;
    float r1 = sep * (q / (1.0 + q));
    float r2 = sep * (1.0 / (1.0 + q));

    // Inclination tilt (~25 degrees)
    float incCos = 0.90;
    vec2 pos1 = vec2(cos(orbAngle), sin(orbAngle) * incCos) * r1;
    vec2 pos2 = vec2(-cos(orbAngle), -sin(orbAngle) * incCos) * r2;

    // Neutron Star 1
    float d1 = length(p - pos1);
    float nsR1 = 0.024;
    float nsCore1 = 1.0 - smoothstep(nsR1 * 0.4, nsR1, d1);
    float nsGlow1 = exp(-d1 * 40.0) * 1.8;
    // Relativistic Doppler beaming on NS1: brighter along orbital velocity
    vec2 velDir1 = normalize(vec2(-sin(orbAngle), cos(orbAngle) * incCos));
    float doppler1 = 1.0 + 0.6 * dot(normalize(p - pos1 + vec2(1e-4)), velDir1);
    vec3 nsCol1 = mix(vec3(0.65, 0.85, 1.4), vec3(0.9, 0.95, 1.0), nsCore1) * doppler1;
    col += (nsCore1 * 4.5 + nsGlow1) * nsCol1 * inspiralFade;

    // Neutron Star 2
    float d2 = length(p - pos2);
    float nsR2 = 0.021;
    float nsCore2 = 1.0 - smoothstep(nsR2 * 0.4, nsR2, d2);
    float nsGlow2 = exp(-d2 * 45.0) * 1.6;
    vec2 velDir2 = -velDir1;
    float doppler2 = 1.0 + 0.6 * dot(normalize(p - pos2 + vec2(1e-4)), velDir2);
    vec3 nsCol2 = mix(vec3(0.55, 0.78, 1.3), vec3(0.9, 0.95, 1.0), nsCore2) * doppler2;
    col += (nsCore2 * 4.0 + nsGlow2) * nsCol2 * inspiralFade;

    // Spiral gravitational wave wakes trailing the binary inspiral
    float gwWake = sin(2.0 * phi - 14.0 * r + orbAngle * 2.0);
    float gwEnvelope = exp(-r * 8.0) * smoothstep(0.04, 0.12, r);
    vec3 gwWakeCol = vec3(0.25, 0.55, 1.0) * max(gwWake, 0.0) * gwEnvelope * (0.8 + 1.5 * subPhase);
    col += gwWakeCol * inspiralFade;

    // Gravitational lensing micro-ring around binary barycenter
    float rEin = 0.045;
    float lensedRing = exp(-pow(abs(r - rEin) * 120.0, 2.0)) * 0.8;
    col += vec3(0.7, 0.85, 1.1) * lensedRing * inspiralFade;
  }

  // =========================================================================
  // STAGE 2: Merger Flash & Expanding Spacetime GW Ripples (phase >= 0.22)
  // =========================================================================
  if (phase > 0.20) {
    // Peak merger flash (around phase 0.25)
    float flashDelta = abs(phase - 0.25);
    float flashEnvelope = exp(-(flashDelta * flashDelta) / 0.002);
    float flashCore = exp(-r * r * 450.0) * 22.0 * flashEnvelope;
    float flashHalo = exp(-r * 9.0) * 3.5 * flashEnvelope;
    vec3 flashCol = mix(vec3(0.85, 0.95, 1.2), vec3(1.0, 1.0, 1.0), clamp(flashCore * 0.2, 0.0, 1.0));
    col += flashCol * (flashCore + flashHalo);

    // Gravitational wave spacetime metric ripples: concentric rings expanding at c
    float gwTime = max(phase - 0.24, 0.0);
    if (gwTime > 0.001) {
      float gwFront = gwTime * 3.2;  // wavefront radius
      float distToFront = abs(r - gwFront);
      // Quadrupole angular factor: cos(2*phi)
      float quad = 0.65 + 0.35 * cos(2.0 * phi - 0.4);
      // High-frequency chirp metric ripples behind the front
      float metricOsc = sin(48.0 * (r - gwFront) + tE * 8.0);
      float metricDecay = 1.0 / sqrt(r + 0.08);
      float gwWave = (1.0 - smoothstep(0.0, 0.40, distToFront)) * abs(metricOsc) * metricDecay * quad;

      // Graviton wave packet tint: coherent cyan-violet metric energy
      vec3 gwPacketCol = mix(vec3(0.20, 0.65, 1.10), vec3(0.70, 0.35, 1.00), clamp(r * 1.5, 0.0, 1.0));
      col += gwPacketCol * gwWave * 1.4;

      // Secondary metric echo ring
      float echoFront = gwFront * 0.62;
      float echoDist = abs(r - echoFront);
      float echoWave = (1.0 - smoothstep(0.0, 0.25, echoDist)) * metricDecay * 0.6 * quad;
      col += vec3(0.35, 0.80, 1.25) * echoWave;
    }
  }

  // =========================================================================
  // STAGE 3: Relativistic Polar Jets (Short GRB 170817A) (phase 0.26 .. 0.85)
  // =========================================================================
  float jetLife = smoothstep(0.25, 0.32, phase) * (1.0 - smoothstep(0.75, 0.90, phase));
  if (jetLife > 0.001) {
    // Polar axis with realistic 20-degree viewing tilt
    float jetTilt = 0.38;
    vec2 jetDir = vec2(sin(jetTilt), cos(jetTilt));
    vec2 jetPerp = vec2(-jetDir.y, jetDir.x);

    float alongJet = dot(p, jetDir);
    float perpJet = abs(dot(p, jetPerp));

    float jetProgress = clamp((phase - 0.26) / 0.45, 0.0, 1.0);
    float jetLength = 0.15 + jetProgress * 0.95;

    // Both polar jets (near lobe brighter due to relativistic Doppler boosting)
    float isNorth = step(0.0, alongJet);
    float jetSign = isNorth * 2.0 - 1.0;
    float absAlong = abs(alongJet);

    if (absAlong < jetLength) {
      // Collimated opening angle: cone radius grows slowly outward
      float coneRadius = 0.012 + absAlong * 0.085;
      float inCone = 1.0 - smoothstep(coneRadius * 0.3, coneRadius, perpJet);

      // Jet spine-sheath structure: ultra-relativistic core + slower cocoon
      float spine = exp(-pow(perpJet / (coneRadius * 0.45), 2.0));
      // Shock diamonds along the jet axis
      float shocks = 1.0 + 0.45 * sin(absAlong * 55.0 - tE * 12.0);

      // Forward bow shock at the jet head
      float headDist = abs(absAlong - jetLength);
      float bowShock = exp(-headDist * 40.0) * smoothstep(0.0, coneRadius * 2.2, perpJet);

      // Relativistic forward-boosting factor for the approaching jet
      float boost = (alongJet > 0.0) ? 1.45 : 0.45;

      vec3 spineCol = vec3(0.70, 0.88, 1.40) * 3.8;
      vec3 sheathCol = vec3(0.85, 0.30, 0.95) * 1.6;
      vec3 jetCol = mix(sheathCol, spineCol, spine) * shocks * boost;

      col += jetCol * inCone * jetLife;
      col += vec3(0.5, 0.85, 1.3) * bowShock * jetLife * boost * 2.2;
    }
  }

  // =========================================================================
  // STAGE 4: Expanding r-Process Nucleosynthesis Fireball (AT 2017gfo)
  // =========================================================================
  float kilonovaLife = smoothstep(0.26, 0.35, phase);
  if (kilonovaLife > 0.001) {
    float kTime = phase - 0.26;
    // Ejecta expands at v ~ 0.1c - 0.3c
    float rBlueMax = 0.12 + kTime * 1.10;   // fast polar wind
    float rRedMax = 0.08 + kTime * 0.75;    // slower dense equatorial torus

    // Polar angle relative to jet axis
    float jetTilt = 0.38;
    vec2 jetDir = vec2(sin(jetTilt), cos(jetTilt));
    vec2 jetPerp = vec2(-jetDir.y, jetDir.x);
    float polCos = abs(dot(p, jetDir)) / max(r, 1e-4);
    float eqSin = abs(dot(p, jetPerp)) / max(r, 1e-4);

    // Turbulent fractal noise for expanding nucleosynthetic debris clouds
    float noise1 = fbm4(p * (6.0 + kTime * 8.0) - vec2(tE * 0.15));
    float noise2 = fbm4(p * 14.0 + vec2(tE * 0.22, -tE * 0.18));
    float turb = mix(noise1, noise2, 0.45);

    // --- Component A: Polar "Blue Kilonova" (Lanthanide-poor, fast wind) ---
    // High-temperature blue-white photosphere, iron-peak & nickel decay
    float blueRadius = rBlueMax * (0.85 + 0.30 * turb) * (0.5 + 0.5 * polCos);
    float blueMask = 1.0 - smoothstep(blueRadius * 0.25, blueRadius, r);
    float blueDecay = 1.0 / (1.0 + kTime * 3.5);
    vec3 bluePhotosphere = mix(vec3(0.45, 0.72, 1.30), vec3(0.85, 0.92, 1.25), turb);
    col += blueMask * bluePhotosphere * blueDecay * 2.2 * kilonovaLife;

    // --- Component B: Equatorial "Red Kilonova" (Lanthanide-rich, r-process) ---
    // Massive neutron capture synthesizing Gold, Platinum, Uranium, Neodymium.
    // Extremely high atomic opacity -> deep crimson, amber, radioactive gold glow!
    float redRadius = rRedMax * (0.80 + 0.35 * turb) * (0.4 + 0.6 * eqSin);
    float redMask = 1.0 - smoothstep(redRadius * 0.20, redRadius, r);
    // Lanthanide dust absorption curtain on the outer rim
    float dustRim = smoothstep(redRadius * 0.6, redRadius, r) * (1.0 - smoothstep(redRadius, redRadius * 1.15, r));

    // Radioactive heating power-law decay ~ t^-1.3
    float rProcessDecay = 1.0 / pow(1.0 + kTime * 4.0, 1.3);
    vec3 goldCore = vec3(1.35, 0.85, 0.25);
    vec3 crimsonDebris = vec3(1.10, 0.22, 0.08);
    vec3 rProcessCol = mix(crimsonDebris, goldCore, turb * eqSin);
    col += redMask * rProcessCol * rProcessDecay * 3.0 * kilonovaLife;
    // Dark silhouette of cold expanding heavy element dust
    col += vec3(0.45, 0.15, 0.05) * dustRim * rProcessDecay * 1.5 * kilonovaLife;

    // --- Remnant Core: Newly born compact object (Kerr Black Hole) ---
    // Central accretion torus and nascent photon ring
    if (phase > 0.45) {
      float bhR = 0.016;
      float bhMask = 1.0 - smoothstep(bhR * 0.5, bhR * 1.2, r);
      float bhGlow = exp(-pow(abs(r - bhR * 1.3) * 180.0, 2.0)) * 1.8;
      vec3 coreCol = mix(vec3(0.0), vec3(1.2, 0.7, 0.2), bhGlow);
      col = mix(col, coreCol, bhMask * (1.0 - exp(-kTime * 4.0)));
    }
  }

  // Linear HDR exposure gain + subtle vignette
  float vig = 1.0 - 0.28 * clamp(dot(starUV * 0.65, starUV * 0.65), 0.0, 1.0);
  col *= vig;
  col *= 1.35 * u_Exposure;

  FragColor = vec4(max(col, vec3(0.0)), 1.0);
}
