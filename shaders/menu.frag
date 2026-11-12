#version 330 core
// menu.frag - Dynamic AAA Sci-Fi Main Menu Background (v5.0.9)
// Real-time 60 FPS GLSL simulation:
// - High-fidelity base texture sampling with Ken Burns camera drift
// - Differential Keplerian swirl & Doppler brightening on Kerr black hole accretion disk
// - Alcubierre Tri-Ring warp drive rotation and pulsing electric cyan energy coils
// - Spacetime metric compression & expansion ripple waves along the Alcubierre bubble
// - Ion thruster plasma exhaust pulsation
// - Twinkling parallax starfield and relativistic cosmic dust

in vec2 v_TexCoord;
out vec4 fragColor;

uniform sampler2D u_BgTex;
uniform float u_Time;
uniform vec2 u_Res;

float hash2(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
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

float fbm(vec2 p) {
  float v = 0.0;
  float amp = 0.55;
  for (int i = 0; i < 4; ++i) {
    v += amp * valueNoise(p);
    p = p * 2.03 + vec2(11.0, 7.0);
    amp *= 0.5;
  }
  return v;
}

// Relativistic star particles with harmonic twinkle
vec3 starLayer(vec2 uv, float scale, float thresh, float gain, float t) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  float h0 = hash2(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash2(id + vec2(7.13, 1.37)), hash2(id + vec2(3.71, 9.41))) - vec2(0.5);
  float d = length(f - off * 0.9);
  float core = exp(-d * d * 320.0);
  float halo = exp(-d * d * 38.0) * 0.06;
  float h1 = hash2(id + vec2(9.17, 2.53));
  float tw = 0.75 + 0.25 * sin(t * (1.8 + 2.2 * h1) + h1 * 43.0);
  vec3 tint = mix(vec3(0.65, 0.85, 1.0), vec3(1.0, 0.88, 0.65), hash2(id + vec2(5.31, 0.71)));
  return tint * mask * tw * gain * (core * 1.2 + halo);
}

void main() {
  float aspect = u_Res.x / max(u_Res.y, 1.0);
  float t = u_Time;

  // OpenGL quad V=0 is bottom, V=1 is top. Texture row 0 is top.
  vec2 screenUv = vec2(v_TexCoord.x, 1.0 - v_TexCoord.y);

  // Subtle Ken Burns slow orbital camera breathing
  vec2 drift = vec2(sin(t * 0.07) * 0.005, cos(t * 0.05) * 0.0035);
  vec2 baseUv = clamp(screenUv + drift, 0.001, 0.999);

  // === 1. Black Hole Accretion Disk Physics ===
  // Black hole singularity center in normalized texture space
  vec2 bhCenter = vec2(0.495, 0.445);
  vec2 bhDelta = (baseUv - bhCenter) * vec2(aspect, 1.0);
  float bhDist = length(bhDelta);
  float bhAngle = atan(bhDelta.y, bhDelta.x);

  // Differential Keplerian swirl: inner disk orbits faster than outer rim
  float keplerSpeed = 0.35 / max(bhDist * 1.5, 0.18);
  float diskNoise = fbm(vec2(bhAngle * 2.5 + t * keplerSpeed, bhDist * 12.0 - t * 0.12));
  
  // Tangential velocity vector for plasma stream
  vec2 tangentDir = vec2(-sin(bhAngle), cos(bhAngle));
  float diskMask = smoothstep(0.12, 0.22, bhDist) * smoothstep(0.68, 0.38, bhDist);
  vec2 accDistort = tangentDir * (diskNoise - 0.5) * 0.007 * diskMask;

  // === 2. Alcubierre Warp Drive Metric Distortions ===
  // Ship warp rings centroid in normalized texture space
  vec2 warpCenter = vec2(0.758, 0.672);
  vec2 shipDir = normalize(vec2(-0.88, -0.47)); // Bow direction
  vec2 shipNorm = vec2(-shipDir.y, shipDir.x);  // Ring plane normal

  vec2 relShip = (baseUv - warpCenter);
  float sAxis = dot(relShip, shipDir);
  float sPerp = dot(relShip, shipNorm);

  // Metric ripple waves propagating along the Alcubierre warp bubble
  float bubbleFalloff = exp(-pow(sAxis * 4.2, 2.0) - pow(sPerp * 7.5, 2.0));
  float metricRipple = sin(sAxis * 48.0 - t * 7.5) * 0.0045 * bubbleFalloff;
  vec2 warpDistort = shipDir * metricRipple;

  // Composite distortion onto base texture sampling
  vec2 finalSampleUv = clamp(baseUv + accDistort + warpDistort, 0.001, 0.999);
  vec4 color = texture(u_BgTex, finalSampleUv);

  // === 3. Relativistic Doppler Brightening on Accretion Disk ===
  // Gas rotating towards observer on the left side is blueshifted and brightened
  float dopplerBoost = 1.0 + 0.18 * sin(bhAngle - 0.35) * diskMask;
  color.rgb *= dopplerBoost;

  // Photon ring inner horizon shimmer
  float photonRing = exp(-pow((bhDist - 0.190) * 55.0, 2.0));
  float photonPulse = 0.85 + 0.25 * sin(t * 3.5 + bhAngle * 3.0);
  vec3 photonCol = vec3(1.0, 0.82, 0.52) * photonRing * photonPulse * 0.65;
  color.rgb += photonCol;

  // === 4. Warp Drive Tri-Ring Active Energy Coils ===
  // Three rotating Alcubierre warp coils
  float totalRingEnergy = 0.0;
  for (int r = 0; r < 3; ++r) {
    float rPos = -0.095 + float(r) * 0.095;
    float distToRingAxis = sAxis - rPos;
    float ringRadius = length(vec2(distToRingAxis * 2.8, sPerp));
    
    // Toroidal ring geometry
    float ringCore = exp(-pow((ringRadius - 0.088) * 42.0, 2.0));
    float ringAngle = atan(sPerp, distToRingAxis * 2.8);
    
    // Rotating energy pulses along the coil circumference (alternating direction)
    float spinDir = (r % 2 == 0) ? 1.0 : -1.0;
    float coilWave = sin(ringAngle * 8.0 - t * 4.2 * spinDir);
    float coilPulse = pow(max(coilWave, 0.0), 2.5);
    
    totalRingEnergy += ringCore * (0.45 + 0.75 * coilPulse);
  }
  vec3 warpEnergyCol = vec3(0.05, 0.85, 1.0) * totalRingEnergy * 0.75;
  vec3 warpCoreCol = vec3(0.70, 0.95, 1.0) * pow(totalRingEnergy, 2.0) * 0.40;
  color.rgb += warpEnergyCol + warpCoreCol;

  // === 5. Ion Thruster Plume Glow ===
  // Plasma flicker at the stern of the vessel
  vec2 sternPos = warpCenter - shipDir * 0.22;
  vec2 sternDelta = baseUv - sternPos;
  float sternDist = length(vec2(dot(sternDelta, shipDir) * 3.5, dot(sternDelta, shipNorm) * 8.0));
  float engineFlicker = (0.80 + 0.20 * sin(t * 24.0) + 0.10 * cos(t * 38.0));
  vec3 enginePlume = vec3(1.0, 0.68, 0.30) * exp(-sternDist * sternDist * 12.0) * engineFlicker * 0.55;
  color.rgb += enginePlume;

  // === 6. Drifting Cosmic Particles & Space Dust ===
  vec2 dustUv = screenUv + vec2(t * 0.003, t * 0.001);
  color.rgb += starLayer(dustUv, 85.0, 0.96, 0.35, t);

  // Soft cinematic corner vignette
  vec2 normScreen = v_TexCoord * 2.0 - 1.0;
  float vignette = 1.0 - 0.18 * dot(normScreen, normScreen);
  color.rgb *= vignette;

  fragColor = vec4(color.rgb, 1.0);
}
