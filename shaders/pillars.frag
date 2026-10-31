#version 330 core
// M16 Pillars of Creation вЂ” Eagle Nebula (GLSL 330 core)
// Physical simulation:
// 1. Towering molecular dust elephant trunks (Pillar I, II, III).
// 2. Photoevaporative ionization fronts sculpted by UV radiation from NGC 6611.
// 3. Narrow-band emission: [S II] (672 nm), H-alpha (656 nm), [O III] (501 nm).
// 4. Evaporating Gaseous Globules (EGGs) harboring newborn protostars.
// 5. Dual-spectral blend: Hubble Optical (opaque silhouettes) <-> JWST Infrared (translucent glowing dust + embedded young stellar objects).
// Linear HDR output (tonemapping and bloom in post_comp.frag).

out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;
uniform vec2 u_ObjNdc;
uniform float u_ObjZoom;
uniform vec3 u_CamPar;

// Spectral mode: 0.0 = Hubble Optical (HST), 1.0 = JWST Infrared (NIRCam/MIRI).
uniform float u_InfraredBlend;
// Photoevaporation rate: stellar wind and UV ablation intensity (default 1.0).
uniform float u_PhotoEvapRate;

// Noise utilities
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

float fbm(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  mat2 rot = mat2(0.8, -0.6, 0.6, 0.8);
  for (int i = 0; i < 5; ++i) {
    v += a * valueNoise(p);
    p = rot * p * 2.02 + vec2(17.3, 9.1);
    a *= 0.5;
  }
  return v;
}

float ridgedFbm(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  mat2 rot = mat2(0.8, -0.6, 0.6, 0.8);
  for (int i = 0; i < 5; ++i) {
    float n = valueNoise(p);
    float ridge = 1.0 - abs(2.0 * n - 1.0);
    v += a * ridge;
    p = rot * p * 2.04 + vec2(11.7, 5.3);
    a *= 0.5;
  }
  return v;
}

// Background starfield
vec3 starLayer(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  float h0 = hash2(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash2(id + vec2(7.13, 1.37)), hash2(id + vec2(3.71, 9.41))) - vec2(0.5);
  float d = length(f - off * 0.9);
  float glow = exp(-d * d * 48.0);
  float h1 = hash2(id + vec2(9.17, 2.53));
  float bright = pow(h1, 14.0) * gain;
  vec3 tint = mix(vec3(0.7, 0.85, 1.0), vec3(1.0, 0.85, 0.6), hash2(id + vec2(5.31, 0.71)));
  return mask * glow * bright * tint;
}

// Massive cluster stars with diffraction spikes
vec3 clusterStar(vec2 p, vec2 center, float size, vec3 color, float brightness) {
  vec2 d = p - center;
  float r = length(d);
  if (r > size * 14.0) return vec3(0.0);
  float core = exp(-pow(r / size, 1.8) * 1.5);
  float halo = exp(-r / (size * 3.5)) * 0.25;

  // 6-pointed diffraction spikes (JWST / HST hybrid)
  float ang = atan(d.y, d.x);
  float spike1 = exp(-pow(abs(sin(ang * 3.0)), 0.15) * r / (size * 0.15));
  float spike2 = exp(-pow(abs(cos(ang * 3.0)), 0.15) * r / (size * 0.15));
  float spikes = (spike1 + spike2) * 0.35 * exp(-r / (size * 6.0));

  return (core + halo + spikes) * color * brightness;
}

// Evaluates the density of the 3 iconic dust pillars at coordinate p
float pillarDensity(vec2 p) {
  float evapTime = u_Time * 0.012 * max(u_PhotoEvapRate, 0.2);

  // Pillar I (Left - majestic giant column, ~4-5 light years)
  // Base at bottom left (-0.25, -0.9), reaches up to (-0.08, 0.45)
  float p1_t = clamp((p.y + 0.85) / 1.35, 0.0, 1.0);
  float p1_centerX = mix(-0.28, -0.06, p1_t) + sin(p1_t * 5.0) * 0.035;
  float p1_width = mix(0.18, 0.08, p1_t) + fbm(vec2(p.y * 3.0, 0.0)) * 0.05;
  if (p1_t > 0.85) {
    float topF = sin(p.x * 35.0) * 0.04;
    p1_width += topF;
  }
  float dist1 = abs(p.x - p1_centerX) / max(p1_width, 0.01);
  float cap1 = 1.0 - smoothstep(0.40, 0.52, p.y);
  float base1 = smoothstep(-0.95, -0.80, p.y);
  float body1 = (1.0 - smoothstep(0.7, 1.25, dist1)) * cap1 * base1;

  // Pillar II (Center spire, intermediate height)
  // Base at (-0.02, -0.9), reaches up to (0.12, 0.18)
  float p2_t = clamp((p.y + 0.85) / 1.05, 0.0, 1.0);
  float p2_centerX = mix(-0.02, 0.14, p2_t) + sin(p2_t * 6.0) * 0.025;
  float p2_width = mix(0.14, 0.055, p2_t) + fbm(vec2(p.y * 4.0, 3.2)) * 0.04;
  float dist2 = abs(p.x - p2_centerX) / max(p2_width, 0.01);
  float cap2 = 1.0 - smoothstep(0.12, 0.24, p.y);
  float base2 = smoothstep(-0.95, -0.75, p.y);
  float body2 = (1.0 - smoothstep(0.7, 1.3, dist2)) * cap2 * base2;

  // Pillar III (Right column, stout spire)
  // Base at (0.22, -0.9), reaches up to (0.34, -0.08)
  float p3_t = clamp((p.y + 0.85) / 0.80, 0.0, 1.0);
  float p3_centerX = mix(0.24, 0.36, p3_t) + cos(p3_t * 4.0) * 0.02;
  float p3_width = mix(0.13, 0.05, p3_t) + fbm(vec2(p.y * 4.5, 7.1)) * 0.035;
  float dist3 = abs(p.x - p3_centerX) / max(p3_width, 0.01);
  float cap3 = 1.0 - smoothstep(-0.14, -0.02, p.y);
  float base3 = smoothstep(-0.95, -0.70, p.y);
  float body3 = (1.0 - smoothstep(0.7, 1.3, dist3)) * cap3 * base3;

  // Combine pillar silhouettes
  float combined = max(body1, max(body2, body3));

  // Add internal molecular cloud turbulence (ridged noise)
  float turb = ridgedFbm(p * 5.5 - vec2(0.0, evapTime));
  float fineTurb = fbm(p * 14.0 + vec2(evapTime * 0.5, 0.0));
  float internalDensity = combined * (0.65 + 0.35 * turb) * (0.8 + 0.2 * fineTurb);

  return clamp(internalDensity, 0.0, 1.0);
}

void main() {
  vec2 ndc = v_TexCoord * 2.0 - 1.0;
  vec2 starUV = ndc;

  // World-anchored framing
  float nz = (u_ObjZoom <= 0.0) ? 1.0 : u_ObjZoom;
  vec2 uv = (ndc - u_ObjNdc) / nz;
  uv.x *= u_Aspect;

  // Offset pillars so they rise majestically in the frame
  vec2 p = uv * 1.35 + vec2(0.05, 0.25);

  // Background sky and stellar nurseries
  vec3 col = vec3(0.003, 0.004, 0.008);
  col += starLayer(starUV, 110.0, 0.991, 1.2);
  col += starLayer(starUV + vec2(27.4, 15.8), 210.0, 0.982, 0.65);

  // Background emission nebula (Eagle Nebula diffuse envelope)
  // Ionized cavity illuminated by massive O/B stars in NGC 6611 (upper-right)
  vec2 clusterLoc = vec2(0.55, 0.70);
  float distToCluster = length(p - clusterLoc);
  float cavityGlow = exp(-distToCluster * 1.6);
  float nebFbm = fbm(p * 2.2 + vec2(u_Time * 0.003, 0.0));

  // Hubble optical emission colors:
  // Emerald-teal [O III] high ionization near the cluster
  vec3 o3Col = vec3(0.20, 0.75, 0.85);
  // Warm golden-coral H-alpha (656 nm) / [S II] (672 nm) diffuse veil
  vec3 haCol = vec3(0.85, 0.35, 0.22);
  vec3 nebColor = mix(haCol, o3Col, clamp(cavityGlow * 1.4, 0.0, 1.0));
  col += nebColor * (0.28 + 0.32 * nebFbm) * cavityGlow * 1.4;

  // Background cluster stars of NGC 6611 (ionizing source)
  col += clusterStar(p, clusterLoc, 0.022, vec3(0.75, 0.88, 1.0), 3.5);
  col += clusterStar(p, clusterLoc + vec2(-0.15, 0.12), 0.016, vec3(0.80, 0.92, 1.0), 2.2);
  col += clusterStar(p, clusterLoc + vec2(0.18, -0.09), 0.018, vec3(0.85, 0.95, 1.0), 2.8);
  col += clusterStar(p, clusterLoc + vec2(-0.25, -0.15), 0.014, vec3(0.90, 0.85, 0.7), 1.8);
  col += clusterStar(p, clusterLoc + vec2(0.08, 0.20), 0.015, vec3(0.70, 0.85, 1.0), 2.0);

  // Compute pillar molecular dust density
  float rho = pillarDensity(p);

  // Normal / gradient estimation to find the illuminated photoevaporative rim
  float eps = 0.012;
  float rhoR = pillarDensity(p + vec2(eps, 0.0));
  float rhoU = pillarDensity(p + vec2(0.0, eps));
  vec2 grad = vec2(rhoR - rho, rhoU - rho) / eps;
  float gradLen = length(grad);
  vec2 normal = (gradLen > 1e-4) ? grad / gradLen : vec2(0.0);

  // Incident UV radiation vector from NGC 6611 cluster (top-right)
  vec2 uvDir = normalize(clusterLoc - p);
  float facing = clamp(dot(normal, uvDir), 0.0, 1.0);

  // Brilliant photoevaporative ionization boundary layer (Hubble bright rim)
  float rim = smoothstep(0.15, 0.75, rho) * (1.0 - smoothstep(0.75, 0.98, rho));
  float ionizationRim = rim * pow(facing, 1.4) * (1.2 + 0.8 * fbm(p * 18.0));

  // Ablation streamer wind (photoevaporated gas flowing away toward lower-left)
  vec2 streamDir = -uvDir;
  float streamerNoise = fbm((p + streamDir * (u_Time * 0.04 * max(u_PhotoEvapRate, 0.2))) * 7.0);
  float streamers = smoothstep(0.4, 0.8, streamerNoise) * rim * (1.0 - facing * 0.7) * 0.45;

  // === DUAL-SPECTRAL RENDERING: HUBBLE OPTICAL vs JWST INFRARED ===
  float irK = clamp(u_InfraredBlend, 0.0, 1.0);

  // 1. HUBBLE OPTICAL (HST) PALETTE:
  // Opaque dense chocolate-brown/charcoal silhouette pillars with glowing rim
  vec3 hstRimColor = mix(vec3(0.95, 0.45, 0.18), vec3(0.25, 0.85, 0.95), facing);
  vec3 hstDustColor = vec3(0.04, 0.025, 0.015);
  vec3 hstColor = mix(col, hstDustColor, smoothstep(0.05, 0.70, rho));
  hstColor += hstRimColor * ionizationRim * 2.2;
  hstColor += vec3(0.75, 0.35, 0.20) * streamers * 1.2;

  // 2. JWST INFRARED (NIRCam / MIRI) PALETTE:
  // Infrared penetrates cold dust: glowing translucent russet-orange dust sheets,
  // delicate gossamer wisps and thousands of embedded newborn protostars
  vec3 jwstDustColor = mix(vec3(0.85, 0.32, 0.08), vec3(0.95, 0.65, 0.20), fbm(p * 8.0)) * (rho * 0.75);
  vec3 jwstRimColor = vec3(1.0, 0.88, 0.55) * ionizationRim * 1.8;
  vec3 jwstColor = mix(col, col * 0.35 + jwstDustColor, smoothstep(0.02, 0.55, rho));
  jwstColor += jwstRimColor;

  // Embedded newborn protostars inside the pillars (revealed in JWST Infrared!)
  vec2 eg1 = vec2(-0.07, 0.42);  // Tip of Pillar I
  vec2 eg2 = vec2(-0.11, 0.38);  // EGG 2 on Pillar I
  vec2 eg3 = vec2(-0.06, 0.26);  // Spire knot
  vec2 eg4 = vec2(0.13, 0.19);   // Tip of Pillar II
  vec2 eg5 = vec2(0.35, -0.06);  // Tip of Pillar III
  vec2 eg6 = vec2(-0.18, 0.05);  // Deep embedded YSO in Pillar I
  vec2 eg7 = vec2(0.05, -0.15);  // Deep embedded YSO in Pillar II

  vec3 embeddedStars = vec3(0.0);
  embeddedStars += clusterStar(p, eg1, 0.009, vec3(1.0, 0.75, 0.4), 2.2);
  embeddedStars += clusterStar(p, eg2, 0.006, vec3(1.0, 0.55, 0.25), 1.6);
  embeddedStars += clusterStar(p, eg3, 0.007, vec3(0.9, 0.8, 1.0), 1.8);
  embeddedStars += clusterStar(p, eg4, 0.008, vec3(1.0, 0.85, 0.5), 2.0);
  embeddedStars += clusterStar(p, eg5, 0.007, vec3(1.0, 0.60, 0.3), 1.7);
  embeddedStars += clusterStar(p, eg6, 0.011, vec3(1.0, 0.45, 0.15), 3.0) * mix(0.15, 1.0, irK);
  embeddedStars += clusterStar(p, eg7, 0.010, vec3(1.0, 0.50, 0.20), 2.5) * mix(0.10, 1.0, irK);

  vec3 denseInfraredStars = starLayer(starUV * 1.5 + vec2(43.1, 88.2), 160.0, 0.965, 0.9) *
                            vec3(1.0, 0.65, 0.35) * (rho * 1.5 + 0.2);
  jwstColor += denseInfraredStars * irK;

  // Final blend between Hubble Optical and JWST Infrared
  vec3 finalPillars = mix(hstColor, jwstColor, irK);
  finalPillars += embeddedStars;

  // Gentle vignette
  float vig = 1.0 - 0.28 * clamp(dot(starUV * 0.72, starUV * 0.72), 0.0, 1.0);
  finalPillars *= vig;

  // Linear exposure gain (ACES tonemapper + gamma applied in post_comp.frag)
  finalPillars *= 1.35 * u_Exposure;

  FragColor = vec4(max(finalPillars, vec3(0.0)), 1.0);
}