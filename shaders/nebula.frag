#version 330 core
// M1 Crab Nebula Supernova Remnant & Pulsar Wind Nebula (PWN) (GLSL 330)
// Physical simulation:
// 1. Central engine: Crab Pulsar with 33 ms spin and relativistic wind.
// 2. Relativistic termination shock ring (R_ts ~ 0.1 pc) where the pair wind shocks.
// 3. Tilted equatorial synchrotron torus with Doppler-boosted expanding wisps (v ~ 0.5c).
// 4. Polar synchrotron jets collimated by magnetic hoop stresses.
// 5. Thermal Rayleigh-Taylor filamentary cage ([O III], H-alpha, [S II]) encasing the non-thermal magnetosphere.
// 6. Multi-wavelength blend: u_PwnMode (0.0 = Optical Filaments, 0.5 = Composite, 1.0 = X-Ray Synchrotron).
// Linear HDR output (ACES tonemapper + gamma applied in post_comp.frag).

out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;
uniform vec2 u_ObjNdc;
uniform float u_ObjZoom;
uniform vec3 u_CamPar;

// PWN mode: 0.0 = Optical Filaments, 0.5 = Composite (Chandra+Hubble), 1.0 = X-ray Synchrotron core.
uniform float u_PwnMode;
// Relativistic termination shock radius (default ~0.12).
uniform float u_TerminationShock;

float hash2(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float valueNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float a = hash2(i);
  float b = hash2(i + vec2(1.0, 0.0));
  float c = hash2(i + vec2(0.0, 1.0));
  float d = hash2(i + vec2(1.0, 1.0));
  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float ridgedFbm(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  mat2 rot = mat2(0.8, -0.6, 0.6, 0.8);
  for (int i = 0; i < 4; ++i) {
    float n = valueNoise(p);
    float ridge = 1.0 - abs(2.0 * n - 1.0);
    v += a * ridge;
    p = rot * p * 2.03 + vec2(11.7);
    a *= 0.5;
  }
  return v;
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
  vec2 ndc = v_TexCoord * 2.0 - 1.0;
  vec2 starUV = ndc;

  float nz = (u_ObjZoom <= 0.0) ? 1.0 : u_ObjZoom;
  vec2 uv = (ndc - u_ObjNdc) / nz;
  uv.x *= u_Aspect;

  vec2 p = vec2(uv.x * 1.25, uv.y * 0.95);
  float r = length(p);

  // Near-black sky plus starfield
  vec3 col = vec3(0.004, 0.005, 0.009);
  col += starLayer(starUV, 90.0, 0.992, 1.4);
  col += starLayer(starUV + vec2(13.1), 190.0, 0.985, 0.55);

  // Default mode balance: 0.5 = composite
  float pwnK = clamp(u_PwnMode <= 0.0 ? 0.5 : u_PwnMode, 0.0, 1.0);
  float rShock = (u_TerminationShock <= 0.0) ? 0.12 : u_TerminationShock;

  // Rotation frame aligned with Crab Pulsar spin axis (~32 degrees inclination)
  const float tiltAng = 0.558; // ~32 deg
  mat2 rotPulsar = mat2(cos(tiltAng), -sin(tiltAng), sin(tiltAng), cos(tiltAng));
  vec2 pRot = rotPulsar * p;
  // Equatorial plane coordinates (foreshortened by inclination)
  vec2 pEq = vec2(pRot.x, pRot.y * 2.2);
  float rEq = length(pEq);

  // === 1. THERMAL FILAMENTARY CAGE (Hubble / Optical [S II], H-alpha, [O III]) ===
  vec2 q = p * (1.0 + 0.9 * exp(-max(r, 1e-3) * 1.5));
  float fil = ridgedFbm(q * 5.0 + vec2(u_Time * 0.015, 0.0));
  fil = pow(fil, 3.0);
  float radialFalloff = 1.0 - smoothstep(0.15, 0.9, r);
  float filMask = smoothstep(0.45, 0.75, fil) * radialFalloff;
  float fil2 = ridgedFbm(q * 9.0 + vec2(0.0, u_Time * 0.01));
  float fiberMask = smoothstep(0.5, 0.8, fil2) * (1.0 - smoothstep(0.2, 0.85, r));

  // Multi-band filament palette: outer [S II] ruby, middle H-alpha orange, inner [O III] cyan-green
  vec3 rubyCol = vec3(0.85, 0.18, 0.10);
  vec3 tealCol = vec3(0.20, 0.75, 0.70);
  float ionizationGrad = clamp(1.0 - r / 0.75, 0.0, 1.0);
  vec3 filamentCol = mix(rubyCol, tealCol, ionizationGrad * 0.45);

  float thread = pow(fil2, 12.0);
  vec3 thermalFilaments = filMask * filamentCol + fiberMask * filamentCol * 0.6 +
                          thread * filamentCol * 0.5 * radialFalloff;

  // === 2. RELATIVISTIC TERMINATION SHOCK & EQUATORIAL TORUS (PWN) ===
  // Cold pair wind shocks at R_ts into an ultra-bright synchrotron ring
  float shockProfile = exp(-pow((rEq - rShock) * 35.0, 2.0));
  // Doppler boosting on approaching side (pEq.x > 0)
  float dopplerBoost = 1.0 + 0.65 * clamp(pEq.x / max(rShock, 0.01), -1.0, 1.0);
  vec3 shockCol = vec3(0.70, 0.88, 1.0) * shockProfile * dopplerBoost * 2.8;

  // Expanding relativistic synchrotron wisps (v ~ 0.5c)
  float wPhase0 = fract(u_Time * 0.12);
  float wPhase1 = fract(u_Time * 0.12 + 0.50);
  float wR0 = rShock + wPhase0 * 0.35;
  float wR1 = rShock + wPhase1 * 0.35;
  float wisp0 = exp(-pow((rEq - wR0) * 28.0, 2.0)) * (1.0 - wPhase0);
  float wisp1 = exp(-pow((rEq - wR1) * 28.0, 2.0)) * (1.0 - wPhase1);
  float torusMask = 1.0 - smoothstep(0.35, 0.65, rEq);
  vec3 wispsCol = vec3(0.45, 0.72, 1.0) * (wisp0 + wisp1) * dopplerBoost * torusMask * 1.8;

  // === 3. POLAR RELATIVISTIC SYNCHROTRON JETS ===
  // Magnetically collimated along pRot.y
  float jetDistY = abs(pRot.y);
  float jetDistX = abs(pRot.x);
  float jetMask = exp(-pow(jetDistX / (0.04 + jetDistY * 0.12), 2.0)) *
                  smoothstep(0.04, 0.15, jetDistY) * (1.0 - smoothstep(0.45, 0.85, jetDistY));
  // Helical instability kinks along the jet
  float jetKinks = sin(jetDistY * 30.0 - u_Time * 2.5) * 0.02;
  float kinkMask = exp(-pow((pRot.x - jetKinks) / 0.05, 2.0));
  vec3 jetCol = vec3(0.55, 0.80, 1.0) * jetMask * (0.8 + 0.4 * kinkMask) * 1.6;

  // === 4. DIFFUSE SYNCHROTRON BLUE MAGNETOSPHERE ===
  float syncFall = exp(-r / 0.38);
  float syncAmp = mix(1.3, 0.45, clamp(r / 0.75, 0.0, 1.0));
  vec3 syncCol = vec3(0.38, 0.60, 1.0) * (syncFall * syncAmp);

  // Outer wispy envelope
  float envLow = ridgedFbm(p * 2.2 + vec2(u_Time * 0.008, 0.0));
  float envShell = smoothstep(0.7, 0.95, r) * (1.0 - smoothstep(1.0, 1.3, r));
  float envMask = smoothstep(0.35, 0.75, envLow) * envShell;
  vec3 envelopeCol = filamentCol * 0.25 + vec3(0.42, 0.62, 1.0) * 0.2;

  // Pulsar 33 ms strobe: flashes illuminate the high-energy plasma
  float strobe = pow(0.5 + 0.5 * sin(u_Time * 4.0), 8.0);

  // Combine components weighted by PWN multi-wavelength mode:
  // Optical mode (pwnK -> 0.0): filaments dominant
  // X-Ray Synchrotron mode (pwnK -> 1.0): termination shock, torus, wisps and jets dominant
  vec3 pwnHighEnergy = shockCol + wispsCol + jetCol + syncCol * 1.5;
  vec3 pwnOptical = thermalFilaments * 1.4 + envelopeCol * envMask * 0.25;

  vec3 diffuse = mix(pwnOptical + pwnHighEnergy * 0.5,
                     pwnHighEnergy * 1.6 + pwnOptical * 0.35,
                     pwnK);

  // Strobe pulse lifts the nebula synchronous with the beams
  diffuse = min(diffuse, vec3(2.5));
  col += diffuse;
  col += diffuse * (0.45 * strobe);

  // Tiny central pulsar point source (33 ms rotator)
  float psr = exp(-r * r * 60000.0) * 3.5 * (1.0 + 2.8 * strobe);
  col += vec3(0.92, 0.96, 1.0) * psr;

  // Outer filament knots
  vec2 knotGrid = p * 26.0;
  vec2 knotId = floor(knotGrid);
  vec2 knotF = fract(knotGrid) - 0.5;
  float knotH = hash21(knotId);
  float knotHit = step(0.978, knotH);
  vec2 knotOff = vec2(hash21(knotId + 7.13), hash21(knotId + 3.71)) - 0.5;
  knotOff *= 0.8;
  vec2 knotD = knotF - knotOff;
  float knotProf = exp(-dot(knotD, knotD) * 60.0);
  float ringGate = smoothstep(0.25, 0.4, r) * (1.0 - smoothstep(0.8, 1.0, r));
  vec3 knotCol = vec3(1.0, 0.62, 0.3) * 2.0;
  col += knotHit * knotProf * filMask * ringGate * knotCol * (1.2 - 0.5 * pwnK);

  // Pulsar lighthouse beams (dual rotating dipole lobes)
  float beamAng = u_Time * 2.0;
  float ang = atan(p.y, p.x);
  float rel = mod(ang - beamAng, 3.14159265);
  float angDiff = min(rel, 3.14159265 - rel);
  float beam = exp(-pow(angDiff * 8.0, 2.0)) * exp(-r * 3.0);
  beam *= 1.0 - smoothstep(0.2, 0.35, r);
  col += vec3(0.7, 0.85, 1.0) * beam * 0.25;

  // Screen vignette
  float vig = 1.0 - 0.3 * clamp(dot(starUV * 0.7, starUV * 0.7), 0.0, 1.0);
  col *= vig;

  // Linear exposure gain (ACES tonemapper + gamma applied in post_comp.frag)
  col *= 1.35 * u_Exposure;

  FragColor = vec4(max(col, vec3(0.0)), 1.0);
}