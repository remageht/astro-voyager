#version 330 core
// Planet surface: sun=0 gas=1 rocky=2 ice=3 coma=4. HDR linear + ACES 1.1 + gamma.
// u_Body detail id: 0 generic, 1 sun, 2 mercury, 3 venus, 4 earth, 5 moon,
// 6 mars, 7 jupiter, 8 saturn, 9 uranus, 10 neptune, 11 halley head.
// All procedural detail is sampled AFTER the axial-spin rotation (sp/spN),
// so patterns ride with the body while the sun terminator stays fixed.
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Local;
out vec4 FragColor;
uniform int u_Type;
uniform int u_Body;
uniform float u_Seed;
uniform vec3 u_ColA;
uniform vec3 u_ColB;
uniform float u_Time;
uniform vec3 u_CamPos;
uniform float u_Spot;
uniform float u_Clouds;
uniform float u_Exposure;
// Axial spin: surface pattern angle (radians) about the Y axis,
// driven by sim time on the CPU (0 in the screenshot harness).
uniform float u_SpinAngle;
// v3.2.0 ring-shadow set (Saturn): body radius in world units, ring-plane
// tilt (same literal as the SceneManager ring model) and annulus bounds.
// The sun sits at the world origin; u_Model[3] is the planet centre.
uniform float u_Radius;
uniform int u_RingOn;
uniform float u_RingInner;
uniform float u_RingOuter;
uniform float u_RingTilt;
uniform mat4 u_Model;
// v3.2.0 moon transit shadows: up to 5 occluder spheres (Jupiter/Saturn
// moon families). xyz = moon centre relative to the planet centre (world
// units), w = moon radius; u_MoonOccN = number of active entries.
uniform vec4 u_MoonOcc[5];
uniform int u_MoonOccN;
// v4.3.0: Planet surface relief & Rayleigh-Mie atmospheric scattering
uniform int u_PlanetRelief;
uniform int u_PlanetAtmosphere;

float hash11(float p) {
  return fract(sin(p) * 43758.5453);
}
float hash13(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.11, 0.17, 0.13));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float vnoise3(vec3 p) {
  vec3 i = floor(p);
  vec3 f = fract(p);
  vec3 u = f * f * (3.0 - 2.0 * f);
  float n000 = hash13(i + vec3(0.0, 0.0, 0.0));
  float n100 = hash13(i + vec3(1.0, 0.0, 0.0));
  float n010 = hash13(i + vec3(0.0, 1.0, 0.0));
  float n110 = hash13(i + vec3(1.0, 1.0, 0.0));
  float n001 = hash13(i + vec3(0.0, 0.0, 1.0));
  float n101 = hash13(i + vec3(1.0, 0.0, 1.0));
  float n011 = hash13(i + vec3(0.0, 1.0, 1.0));
  float n111 = hash13(i + vec3(1.0, 1.0, 1.0));
  float nx00 = mix(n000, n100, u.x);
  float nx10 = mix(n010, n110, u.x);
  float nx01 = mix(n001, n101, u.x);
  float nx11 = mix(n011, n111, u.x);
  float nxy0 = mix(nx00, nx10, u.y);
  float nxy1 = mix(nx01, nx11, u.y);
  return mix(nxy0, nxy1, u.z);
}
float fbm4(vec3 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 4; ++i) {
    v += a * vnoise3(p);
    s += a;
    p *= 2.03;
    a *= 0.5;
  }
  return v / max(s, 0.0001);
}
float fbm3(vec3 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 3; ++i) {
    v += a * vnoise3(p);
    s += a;
    p *= 2.03;
    a *= 0.5;
  }
  return v / max(s, 0.0001);
}
// v4.3.0: Procedural elevation field for normal bump mapping
float planetElevation(vec3 sp, int body, int type, float seed) {
  if (type == 0 || type == 1) return 0.0;
  float h = 0.0;
  if (body == 4) {
    // Earth: continents, mountains & peaks
    float cont = fbm4(sp * 2.8 + seed * 1.3);
    float mountains = fbm4(sp * 14.0 + seed * 4.1);
    float peaks = fbm3(sp * 38.0 + seed * 8.7);
    float landMask = smoothstep(0.48, 0.54, cont);
    h = landMask * (0.014 * mountains + 0.006 * peaks);
  } else if (body == 6) {
    // Mars: Tharsis volcanoes, crater rims & Valles Marineris canyon
    float baseM = fbm4(sp * 3.2 + seed * 2.1);
    float craters = abs(fbm4(sp * 12.0 + seed * 3.5) - 0.5) * 2.0;
    float fine = fbm3(sp * 34.0 + seed * 9.2);
    h = 0.018 * baseM + 0.014 * craters + 0.005 * fine;
  } else if (body == 5 || body == 2) {
    // Moon / Mercury: crater basins and highlands
    float basin = fbm4(sp * 2.5 + seed * 1.7);
    float craterRims = pow(abs(fbm4(sp * 13.0 + seed * 4.3) - 0.5) * 2.0, 1.4);
    float micro = fbm3(sp * 40.0 + seed * 7.1);
    h = 0.014 * basin + 0.016 * craterRims + 0.006 * micro;
  } else if (type == 2 || type == 3) {
    float r1 = fbm4(sp * 4.0 + seed * 2.3);
    float r2 = fbm3(sp * 20.0 + seed * 5.6);
    h = 0.015 * r1 + 0.007 * r2;
  }
  return h;
}
// Wrapped absolute angular distance in [-pi, pi].
float angDist(float a, float b) {
  float d = mod(a - b + 3.14159265, 6.2831853) - 3.14159265;
  return abs(d);
}
// v3.2.0: Saturn ring shadow on the planet. The ring plane passes through
// the planet centre with normal (0, cos tilt, sin tilt); the sun is at the
// world origin. Returns 1 outside the shadow; inside the annulus a stripe
// comb (plus the deep Cassini gap at the same 0.65 span position as
// ring.frag) dims the direct sunlight. Soft annulus edges avoid aliasing.
float ringShadow(vec3 pr, vec3 sd) {
  if (u_RingOn == 0) return 1.0;
  vec3 n = vec3(0.0, cos(u_RingTilt), sin(u_RingTilt));
  float dn = dot(sd, n);
  if (abs(dn) < 1e-4) return 1.0;
  float tR = -dot(pr, n) / dn;
  if (tR <= 0.0) return 1.0;
  vec3 c = pr + tR * sd;
  float rho = length(c - dot(c, n) * n);
  float band = smoothstep(u_RingInner - 0.06, u_RingInner + 0.06, rho) *
               (1.0 - smoothstep(u_RingOuter - 0.06, u_RingOuter + 0.06, rho));
  float span = max(u_RingOuter - u_RingInner, 0.01);
  float r01 = (rho - u_RingInner) / span;
  float cass = smoothstep(0.60, 0.65, r01) * (1.0 - smoothstep(0.65, 0.71, r01));
  float comb = 0.55 + 0.45 * sin(r01 * 42.0 + 1.3);
  float depth = (0.60 - 0.34 * cass) * comb;
  return 1.0 - band * depth;
}
// v3.2.0 moon transit shadows: a moon disc blocks the direct sunlight on
// the planet (tiny dark dots sweeping across Jupiter/Saturn). Same
// sun-at-origin convention as ringShadow.
float moonShadow(vec3 pr, vec3 sd) {
  float s = 1.0;
  for (int i = 0; i < 5; ++i) {
    if (i >= u_MoonOccN) break;
    vec3 L = u_MoonOcc[i].xyz - pr;
    float tM = dot(L, sd);
    if (tM > 0.0) {
      vec3 cl = pr + tM * sd;
      float dM = length(cl - u_MoonOcc[i].xyz);
      s *= 1.0 - 0.92 * (1.0 - smoothstep(u_MoonOcc[i].w * 0.9,
                                          u_MoonOcc[i].w * 1.15, dM));
    }
  }
  return s;
}
// v3.0.0: ACES + gamma moved to post_comp.frag; this shader outputs linear HDR.
void main() {
  vec3 n = normalize(v_Normal);
  vec3 viewDir = normalize(u_CamPos - v_WorldPos);
  vec3 ln = normalize(v_Local);
  // Spin the procedural-noise sample position about Y so continents,
  // bands and granulation travel under a fixed sun terminator.
  // Zero angle keeps the identity path (bit-exact screenshots).
  vec3 sp = v_Local;
  vec3 spN = ln;
  if (u_SpinAngle != 0.0) {
    float spinC = cos(u_SpinAngle);
    float spinS = sin(u_SpinAngle);
    sp = vec3(spinC * v_Local.x + spinS * v_Local.z, v_Local.y,
              -spinS * v_Local.x + spinC * v_Local.z);
    spN = vec3(spinC * ln.x + spinS * ln.z, ln.y,
               -spinS * ln.x + spinC * ln.z);
  }
  vec3 sunDir = normalize(-v_WorldPos);
  // v4.3.0: Procedural normal bump mapping (micro-relief on rocky bodies)
  if (u_PlanetRelief == 1 && (u_Type == 2 || u_Type == 3)) {
    float dCam = length(v_WorldPos - u_CamPos);
    float nearK = 1.0 - smoothstep(u_Radius * 12.0, u_Radius * 36.0, dCam);
    if (nearK > 0.001) {
      vec3 t1 = abs(spN.y) < 0.98 ? normalize(cross(spN, vec3(0.0, 1.0, 0.0))) : normalize(cross(spN, vec3(1.0, 0.0, 0.0)));
      vec3 t2 = cross(spN, t1);
      float eps = 0.0035;
      float h0 = planetElevation(spN, u_Body, u_Type, u_Seed);
      float h1 = planetElevation(spN + t1 * eps, u_Body, u_Type, u_Seed);
      float h2 = planetElevation(spN + t2 * eps, u_Body, u_Type, u_Seed);
      vec3 dH_local = ((h1 - h0) * t1 + (h2 - h0) * t2) / eps;
      vec3 dH_world = dH_local;
      if (u_SpinAngle != 0.0) {
        float spinC = cos(-u_SpinAngle);
        float spinS = sin(-u_SpinAngle);
        dH_world = vec3(spinC * dH_local.x + spinS * dH_local.z, dH_local.y,
                        -spinS * dH_local.x + spinC * dH_local.z);
      }
      vec3 gradW = mat3(u_Model) * dH_world;
      n = normalize(n - gradW * (0.85 * nearK));
    }
  }
  float diff = max(dot(n, sunDir), 0.0);
  float mu = clamp(dot(n, viewDir), 0.0, 1.0);
  vec3 nightAmb = vec3(0.10, 0.16, 0.30) * (0.015 * (1.0 - diff));
  if (u_Type == 0) {
    // SUN: boiling two-scale granulation (time-advected, spin-riding).
    vec3 gp = sp * 5.0 + u_Seed;
    float g1 = fbm4(gp + vec3(u_Time * 0.06, u_Time * 0.03, 0.0));
    float g2 = fbm3(gp * 2.3 + vec3(0.0, -u_Time * 0.05, u_Time * 0.04));
    float gran = g1 * 0.65 + g2 * 0.35;
    gran = clamp((gran - 0.5) * 3.0 + 0.5, 0.0, 1.0);
    vec3 ecol = mix(u_ColB * vec3(1.0, 0.75, 0.55), u_ColA, gran) * 4.2;
    ecol *= 0.55 + 0.90 * gran;
    float facN = fbm4(sp * 5.0 - u_Seed + vec3(0.0, u_Time * 0.03, 0.0));
    float facMask = smoothstep(0.60, 0.85, facN);
    ecol += vec3(1.0, 0.95, 0.85) * facMask * 1.5;
    float tintN = fbm3(sp * 1.5 + u_Seed * 0.7 + vec3(0.0, u_Time * 0.02, 0.0));
    ecol = mix(ecol, vec3(1.0, 0.55, 0.2) * 4.0, clamp(tintN, 0.0, 1.0) * 0.3);
    float flareId = floor(u_Time / 9.0);
    float flareFrac = fract(u_Time / 9.0);
    float gate = smoothstep(0.70, 0.9, hash11(flareId)) * (1.0 - smoothstep(0.9, 1.0, flareFrac));
    float fl = smoothstep(0.72, 0.92, fbm3(sp * 6.0 + flareId + vec3(0.0, u_Time * 0.15, 0.0)));
    vec2 flareCenter = (vec2(hash11(flareId + 17.0), hash11(flareId + 41.0)) - 0.5) * 1.2;
    float rc = length(sp.xy - flareCenter);
    float rd = (rc - flareFrac * 1.5) * 6.0;
    float ring = exp(-rd * rd);
    ecol += (vec3(1.0, 0.6, 0.25) * fl * 2.0 + vec3(1.0, 0.85, 0.6) * ring * 1.2) * gate;
    // Limb darkening: disc centre bright, limb falls to ~0.35.
    float limb = 0.35 + 0.65 * pow(mu, 0.55);
    vec3 scol = ecol * limb;
    // Chromosphere: thin warm rim just inside the limb.
    float chrom = pow(1.0 - mu, 3.0) * smoothstep(0.0, 0.25, mu);
    scol += vec3(1.0, 0.45, 0.15) * chrom * 2.0;
    // Prominence feet: 3 plasma tongues on the limb, slowly breathing.
    float pang = atan(spN.z, spN.x);
    float prom = exp(-pow(angDist(pang, 0.7) * 2.2, 2.0));
    prom += exp(-pow(angDist(pang, -1.9) * 2.2, 2.0));
    prom += exp(-pow(angDist(pang, 2.6) * 2.2, 2.0));
    float breath = 0.65 + 0.35 * sin(u_Time * 0.7 + pang * 2.0);
    float limbBand = (1.0 - smoothstep(0.04, 0.40, mu)) * smoothstep(0.0, 0.02, mu);
    scol += vec3(1.0, 0.35, 0.10) * prom * limbBand * breath * 4.0;
    scol *= 1.1;
    scol = min(scol, vec3(8.0));
    FragColor = vec4(max(scol, vec3(0.0)), 1.0);
    return;
  }
  vec3 albedo = vec3(0.5);
  float land = 0.0;
  float cloudShadow = 0.0;
  if (u_Type == 1) {
    // GAS GIANTS: turbulent band swirls, domain-warped.
    vec3 lp = spN;
    float warp = fbm3(vec3(sp.zz * 3.0, u_Seed)) * 0.8;
    float bandScale = (u_Body == 8) ? 6.0 : 9.0;
    vec3 bp = vec3(lp.x * 2.0, lp.y * bandScale + warp * 1.5, lp.z * 2.0) + u_Seed;
    bp += (fbm3(sp * 4.0 + u_Seed) - 0.5) * 1.2;
    float bands = fbm4(bp) * 0.9677 + 0.0323 * vnoise3(bp * 16.9818);
    if (u_Body == 8) {
      bands = mix(bands, 0.5, 0.35);
    }
    albedo = mix(u_ColA, u_ColB, bands);
    if (u_Spot > 0.5) {
      // Great Red Spot: oval vortex, drifts in longitude with wall time
      // (fixed offset at fixed u_Time, so screenshots stay deterministic).
      float drift = u_Time * 0.05;
      float dc = cos(drift);
      float ds = sin(drift);
      vec3 spotDir = normalize(vec3(0.3, -0.25, 0.9));
      vec3 sd = vec3(dc * spotDir.x + ds * spotDir.z, spotDir.y,
                     -ds * spotDir.x + dc * spotDir.z);
      vec3 dvec = (spN - sd) * vec3(1.0, 1.7, 1.0);
      float d = length(dvec);
      float turb = (fbm3(spN * 12.0 + u_Seed) - 0.5) * 0.10;
      float spot = 1.0 - smoothstep(0.12, 0.30, d + turb);
      albedo = mix(albedo, vec3(0.85, 0.32, 0.18), spot * 0.9);
      float core = 1.0 - smoothstep(0.05, 0.14, d + turb * 0.5);
      albedo = mix(albedo, vec3(0.95, 0.55, 0.35), core * 0.7);
      float rim = smoothstep(0.24, 0.30, d) * (1.0 - smoothstep(0.30, 0.42, d));
      albedo *= 1.0 - rim * 0.25;
    }
    float pol = smoothstep(0.55, 0.9, abs(spN.y));
    albedo = mix(albedo, albedo * vec3(0.75, 0.8, 0.95), pol * 0.5);
  } else if (u_Type == 2) {
    if (u_Body == 3) {
      // VENUS: milky opaque atmosphere, surface hidden; faint chevrons.
      float chev = fbm3(vec3(spN.x * 1.5, spN.y * 6.0, spN.z * 1.5) + u_Seed);
      chev = chev * 0.8 + 0.2 * fbm3(sp * 3.0 + u_Seed * 1.3);
      albedo = mix(vec3(0.96, 0.88, 0.68), vec3(0.80, 0.66, 0.44), chev * 0.55);
    } else if (u_Body == 6) {
      // MARS: basalt + wind-blown dust streaks, seasonal tint, polar caps, rift canyons.
      float continents = fbm4(sp * 3.0 + u_Seed) * 0.9677 + 0.0323 * vnoise3((sp * 3.0 + u_Seed) * 16.9818);
      land = smoothstep(0.48, 0.56, continents);
      albedo = mix(u_ColA, u_ColB, land);
      float dust = fbm3(vec3(sp.x * 3.0, sp.y * 10.0, sp.z * 3.0) + u_Seed);
      albedo = mix(albedo, vec3(0.85, 0.55, 0.35), dust * 0.45);
      float season = 0.5 + 0.5 * sin(u_Time * 0.07);
      albedo = mix(albedo, albedo * vec3(1.06, 0.92, 0.86), season * 0.5);
      // Craters and chasms
      float crBig = smoothstep(0.66, 0.76, fbm3(sp * 16.0 + u_Seed * 2.3));
      albedo *= 1.0 - crBig * 0.35;
      float crSmall = smoothstep(0.72, 0.77, fbm3(sp * 38.0 + u_Seed * 5.1));
      albedo *= 1.0 - crSmall * 0.22;
      // Organic fractured polar ice cap
      float capNoise = fbm4(sp * 7.5 + u_Seed * 3.7) * 0.06;
      float cap = smoothstep(0.86, 0.93, abs(spN.y) + capNoise);
      albedo = mix(albedo, vec3(0.94, 0.92, 0.90), cap);
    } else if (u_Body == 5) {
      // MOON family (Earth's Moon + Galileans + Titan): maria plains,
      // highlands, two-scale craters, tinted by u_ColA/u_ColB. The Moon def
      // carries the classic 0.62/0.30 greys so it renders as before.
      float mar = smoothstep(0.52, 0.64, fbm4(sp * 2.0 + u_Seed));
      albedo = mix(u_ColA, u_ColB, mar);
      float crBig = smoothstep(0.64, 0.75, fbm3(sp * 14.0 + u_Seed));
      albedo *= 1.0 - crBig * 0.40;
      float crSmall = smoothstep(0.70, 0.75, fbm3(sp * 40.0 + u_Seed * 1.7));
      albedo *= 1.0 - crSmall * 0.35;
    } else if (u_Body == 2) {
      // MERCURY: dense sharp craters on grey regolith.
      float base = fbm4(sp * 4.0 + u_Seed);
      albedo = mix(u_ColA, u_ColB, base);
      float cr = smoothstep(0.60, 0.66, fbm3(sp * 26.0 + u_Seed));
      albedo *= 1.0 - cr * 0.50;
      float micro = smoothstep(0.70, 0.74, fbm3(sp * 55.0 + u_Seed * 1.7));
      albedo *= 1.0 - micro * 0.28;
    } else if (u_Body == 4) {
      // EARTH: Multi-biome procedural Earth with bathymetry, organic ice caps, and directional cloud shadows.
      float continents = fbm4(sp * 3.0 + u_Seed) * 0.9677 + 0.0323 * vnoise3((sp * 3.0 + u_Seed) * 16.9818);
      land = smoothstep(0.48, 0.56, continents);

      // --- Ocean Bathymetry (Depth Gradients) ---
      float oceanDepth = smoothstep(0.20, 0.48, continents);
      vec3 deepAbyss = vec3(0.015, 0.045, 0.14);
      vec3 openOcean = mix(vec3(0.030, 0.115, 0.29), u_ColA * 0.35, 0.3);
      vec3 shelfCol  = vec3(0.075, 0.460, 0.58);
      vec3 lagoonCol = vec3(0.120, 0.580, 0.65);

      vec3 oceanCol = mix(deepAbyss, openOcean, oceanDepth);
      float shelf = smoothstep(0.40, 0.48, continents);
      oceanCol = mix(oceanCol, shelfCol, shelf * 0.75);
      float lagoon = smoothstep(0.46, 0.485, continents);
      oceanCol = mix(oceanCol, lagoonCol, lagoon * 0.65);

      // --- Multi-Biome Land Terrain ---
      float lat = abs(spN.y);
      float moisture = fbm4(sp * 2.5 + u_Seed * 1.7 + vec3(1.2, 3.4, 5.6));
      float elevation = fbm4(sp * 6.5 + u_Seed * 3.3);

      vec3 desertSand    = vec3(0.74, 0.60, 0.38); // Arid desert sand
      vec3 desertDunes   = vec3(0.66, 0.48, 0.28); // Weathered reddish dunes
      vec3 rainforest    = vec3(0.07, 0.28, 0.10); // Lush tropical canopy
      vec3 savannaGrass  = vec3(0.38, 0.46, 0.20); // Subtropical grasslands
      vec3 temperateWood = vec3(0.15, 0.36, 0.13); // Temperate broadleaf forests
      vec3 plainsGreen   = vec3(0.25, 0.44, 0.18); // Rolling temperate plains
      vec3 borealTaiga   = vec3(0.11, 0.23, 0.14); // Subpolar coniferous taiga
      vec3 tundraMoss    = vec3(0.38, 0.38, 0.28); // High-latitude tundra
      vec3 mountainRock  = vec3(0.34, 0.30, 0.26); // Craggy highland peaks
      vec3 mountainSnow  = vec3(0.92, 0.95, 0.98); // High-altitude alpine snow

      vec3 tropicalCol = mix(savannaGrass, rainforest, smoothstep(0.35, 0.65, moisture));
      vec3 aridCol = mix(desertDunes, desertSand, smoothstep(0.30, 0.60, elevation));
      vec3 subTropCol = mix(aridCol, savannaGrass, smoothstep(0.48, 0.70, moisture));
      vec3 temperateCol = mix(plainsGreen, temperateWood, smoothstep(0.38, 0.65, moisture));
      vec3 polarBorderCol = mix(tundraMoss, borealTaiga, smoothstep(0.40, 0.65, moisture));

      vec3 landCol = tropicalCol;
      landCol = mix(landCol, subTropCol, smoothstep(0.18, 0.30, lat));
      landCol = mix(landCol, temperateCol, smoothstep(0.36, 0.48, lat));
      landCol = mix(landCol, polarBorderCol, smoothstep(0.58, 0.70, lat));

      // Mountain ridges and snow-capped peaks
      float mountainMask = smoothstep(0.55, 0.75, elevation) * smoothstep(0.50, 0.58, continents);
      landCol = mix(landCol, mountainRock, mountainMask * 0.85);
      float peakSnowMask = smoothstep(0.70, 0.85, elevation + lat * 0.35) * smoothstep(0.51, 0.58, continents);
      landCol = mix(landCol, mountainSnow, peakSnowMask * 0.95);

      // Coastline sandy beach
      landCol = mix(vec3(0.70, 0.64, 0.50), landCol, smoothstep(0.50, 0.53, continents));

      // Blend land base color with u_ColB
      landCol = mix(landCol, mix(landCol, u_ColB, 0.25), 0.5);

      albedo = mix(oceanCol, landCol, land);

      // --- Organic Fractured Polar Ice Caps & Sea Ice Floes ---
      float iceNoise = fbm4(sp * 6.5 + u_Seed * 4.1) * 0.12 + fbm3(sp * 16.0 + vec3(2.1, 0.4, 1.8)) * 0.05;
      float capLat = lat + iceNoise;
      float cap = smoothstep(0.78, 0.87, capLat);
      float seaIce = smoothstep(0.72, 0.80, capLat) * (1.0 - cap) * (1.0 - land);
      vec3 glacialIce = mix(vec3(0.92, 0.95, 0.99), vec3(0.82, 0.91, 0.96), fbm3(sp * 12.0));
      albedo = mix(albedo, glacialIce, cap);
      albedo = mix(albedo, vec3(0.72, 0.86, 0.93), seaIce * 0.65);

      // --- Two-Layer Cyclonic Cloud System with Directional Ground Shadows ---
      vec3 sunDirLocal = sunDir;
      if (u_SpinAngle != 0.0) {
        float spinC = cos(u_SpinAngle);
        float spinS = sin(u_SpinAngle);
        sunDirLocal = vec3(spinC * sunDir.x + spinS * sunDir.z, sunDir.y,
                           -spinS * sunDir.x + spinC * sunDir.z);
      }
      vec3 cp = sp * 4.8 + u_Seed + vec3(u_Time * 0.03, 0.0, 0.0);
      float cl1 = fbm4(cp);
      float cl2 = fbm3(cp * 2.4 + vec3(u_Time * 0.015, 0.0, 0.0));
      float cloudVal = cl1 * 0.72 + cl2 * 0.28;
      float clm = smoothstep(0.50, 0.72, cloudVal) * u_Clouds;

      // Directional cloud shadow projected onto surface along local sun vector
      vec3 shadowSp = sp - sunDirLocal * 0.026;
      vec3 shadowCp = shadowSp * 4.8 + u_Seed + vec3(u_Time * 0.03, 0.0, 0.0);
      float shadowCl1 = fbm4(shadowCp);
      float shadowCl2 = fbm3(shadowCp * 2.4 + vec3(u_Time * 0.015, 0.0, 0.0));
      float shadowVal = shadowCl1 * 0.72 + shadowCl2 * 0.28;
      cloudShadow = smoothstep(0.51, 0.73, shadowVal) * u_Clouds * 0.42;

      // Ground shadow dims diffuse direct sunlight on land & ocean
      albedo *= 1.0 - cloudShadow * (0.2 + 0.8 * diff);

      // Volumetric cloud self-shadowing and sun-side rim lighting
      vec3 cloudIllumSp = cp + sunDirLocal * 0.12;
      float cloudSunDiff = clamp(fbm4(cloudIllumSp) - cl1 + 0.65, 0.45, 1.15);
      vec3 cloudCol = vec3(0.96, 0.98, 1.0) * cloudSunDiff;
      albedo = mix(albedo, cloudCol, clm * 0.90);
    } else {
      // Generic rocky: continents, craters, caps, clouds.
      float continents = fbm4(sp * 3.0 + u_Seed) * 0.9677 + 0.0323 * vnoise3((sp * 3.0 + u_Seed) * 16.9818);
      land = smoothstep(0.48, 0.56, continents);
      albedo = mix(u_ColA, u_ColB, land);
      // Shallow coastal shelf tint around continent edges.
      float shelf = smoothstep(0.42, 0.48, continents) * (1.0 - land);
      albedo = mix(albedo, vec3(0.25, 0.55, 0.75), shelf * 0.5);
      float cr = smoothstep(0.68, 0.8, fbm3(sp * 22.0 + u_Seed));
      albedo *= 1.0 - cr * 0.45;
      float cap = smoothstep(0.80, 0.88, abs(spN.y));
      albedo = mix(albedo, vec3(0.9, 0.93, 0.95), cap);
    }
  } else if (u_Type == 4) {
    albedo = vec3(0.75, 1.0, 0.85);
  } else {
    // ICE GIANTS: cold thin bands; Neptune gets streaks + dark spot.
    float bandScale = (u_Body == 10) ? 7.0 : 5.0;
    float b3 = fbm3(vec3(spN.x * 1.0, spN.y * bandScale, spN.z * 1.0) + u_Seed);
    float iceK = b3 * 0.35 + 0.15 * spN.y;
    if (u_Body == 10) {
      float streak = fbm3(sp * 6.0 + u_Seed * 1.3);
      iceK += (streak - 0.5) * 0.25;
    }
    iceK = clamp(iceK, 0.0, 1.0);
    albedo = mix(u_ColA, u_ColB, iceK);
    if (u_Body == 10) {
      float drift = u_Time * 0.03;
      float dc = cos(drift);
      float ds = sin(drift);
      vec3 spotDir = normalize(vec3(-0.4, 0.3, 0.85));
      vec3 sd = vec3(dc * spotDir.x + ds * spotDir.z, spotDir.y,
                     -ds * spotDir.x + dc * spotDir.z);
      float d = length((spN - sd) * vec3(1.0, 1.5, 1.0));
      float spot = 1.0 - smoothstep(0.10, 0.24, d);
      albedo = mix(albedo, vec3(0.08, 0.15, 0.35), spot * 0.8);
    }
  }
  // v3.2.0 close-range surface detail: two extra noise octaves that fade in
  // only when the camera is near the body (nearK == 0 at reference
  // distances, so the far screenshots are unchanged). Deterministic.
  {
    float dCam = length(v_WorldPos - u_CamPos);
    float nearK = 1.0 - smoothstep(u_Radius * 14.0, u_Radius * 32.0, dCam);
    if (nearK > 0.001) {
      if (u_Type == 1 || u_Type == 3) {
        // Gas/ice giants: azimuthally stretched jet-stream streak detail.
        float g1 = fbm4(vec3(sp.x, sp.y * 4.0, sp.z) * 42.0 + u_Seed * 5.0);
        float g2 = fbm4(vec3(sp.x, sp.y * 6.0, sp.z) * 110.0 + u_Seed * 9.0);
        albedo *= 1.0 + (0.16 * (g1 - 0.5) + 0.10 * (g2 - 0.5)) * nearK;
      } else if (u_Type == 2) {
        // Rocky/moon surfaces: two-scale granular relief. Earth keeps its
        // matte ocean look (relief mostly on land, reduced on water).
        float r1 = fbm4(sp * 55.0 + u_Seed * 3.1);
        float r2 = fbm4(sp * 140.0 + u_Seed * 7.7);
        float rockK = (u_Body == 4) ? mix(0.2, 0.7, land) : 1.0;
        albedo *= 1.0 + (0.20 * (r1 - 0.5) + 0.12 * (r2 - 0.5)) * nearK * rockK;
      }
    }
  }
  // Ring + moon-transit shadows: dim only the direct sunlight term, leave
  // the ambient floor and the night side untouched.
  vec3 prW = v_WorldPos - u_Model[3].xyz;
  float rsF = 1.0;
  if (u_RingOn == 1) {
    rsF = ringShadow(prW, sunDir);
  }
  if (u_MoonOccN > 0) {
    rsF *= moonShadow(prW, sunDir);
  }
  vec3 col = albedo * (0.03 + rsF * diff) + nightAmb;
  if (u_Body == 4) {
    // EARTH: soft terminator with warm twilight band.
    float day = smoothstep(-0.06, 0.22, diff);
    vec3 dayCol = albedo * (0.03 + diff);
    float twi = smoothstep(-0.08, 0.02, diff) * (1.0 - smoothstep(0.02, 0.25, diff));
    dayCol += albedo * vec3(0.9, 0.4, 0.2) * twi * 0.35;
    // Seasonal shine on the polar caps.
    float capM = smoothstep(0.78, 0.87, abs(spN.y) + fbm4(sp * 6.5 + u_Seed * 4.1) * 0.12);
    vec3 rdir = reflect(-sunDir, n);
    float spec = pow(max(dot(rdir, viewDir), 0.0), 24.0);
    float season = 0.5 + 0.5 * sin(u_Time * 0.1);
    dayCol += vec3(0.9, 0.95, 1.0) * spec * capM * (0.4 + 0.6 * season) * day;
    // v3.2.0 ocean sun glint: tight specular on water only (land keeps its
    // matte look), gated to the day side and away from the ice caps.
    float glint = pow(max(dot(rdir, viewDir), 0.0), 90.0);
    dayCol += vec3(0.85, 0.92, 1.0) * glint * (1.0 - land) * (1.0 - capM) * day * 1.7;
    // v3.2.0 night city lights: static warm speckles on the land of the
    // night side (deterministic pattern, no time dependence).
    float cityN = fbm4(sp * 26.0 + u_Seed * 2.0);
    float city = smoothstep(0.62, 0.78, cityN) * land * (1.0 - day);
    col = dayCol + nightAmb + vec3(1.0, 0.82, 0.55) * city * 1.3;
    // Night city lights: street network with per-cell flicker, night only.
    float night = 1.0 - smoothstep(0.0, 0.12, diff);
    float cc = fbm3(sp * 14.0 + u_Seed);
    float cities = smoothstep(0.62, 0.8, cc) * land;
    float net = smoothstep(0.6, 0.9, fbm3(sp * 30.0 + u_Seed * 1.7));
    float cellFlick = 0.8 + 0.2 * sin(u_Time * 2.5 + hash13(floor(sp * 14.0)) * 40.0);
    col += vec3(1.0, 0.75, 0.42) * cities * (0.7 + 0.5 * net) * night * cellFlick * 2.0;
  } else if (u_Type == 2 && u_Body != 3) {
    float night = 1.0 - smoothstep(0.0, 0.08, diff);
    float cc = fbm3(sp * 14.0 + u_Seed);
    float cities = smoothstep(0.62, 0.8, cc) * land;
    col += vec3(1.0, 0.75, 0.42) * cities * night * u_Clouds * 2.2;
  }
  if (u_Type == 3) {
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.0);
    col += u_ColB * rim * 0.55;
    float spIce = step(0.985, hash13(vec3(floor(sp.xz * 40.0), u_Seed))) * 0.6;
    col += vec3(1.0) * spIce * diff;
  }
  if (u_Type == 1 || u_Type == 2 || u_Type == 3) {
    vec3 rimColor = vec3(0.45, 0.65, 1.0);
    float rimGain = 1.0;
    if (u_Type == 1) {
      rimColor = vec3(0.9, 0.85, 0.75) * 0.7;
    } else if (u_Type == 3) {
      rimColor = vec3(0.5, 0.85, 1.0);
      rimGain = 1.6;
    }
    if (u_Body == 3) {
      // Venus: bright milky limb haze, surface stays hidden.
      rimColor = vec3(1.0, 0.92, 0.72);
      rimGain = 2.2;
    } else if (u_Body == 4) {
      // Earth: luminous sapphire/cyan Rayleigh limb veil.
      rimColor = vec3(0.22, 0.58, 1.15);
      rimGain = 1.45;
    }
    // v4.3.0: Physical Rayleigh & Mie Atmospheric Scattering
    if (u_PlanetAtmosphere == 1) {
      float cosTheta = dot(-viewDir, sunDir);
      float rayleighPhase = 0.75 * (1.0 + cosTheta * cosTheta);
      float g = 0.76;
      float miePhase = (1.0 - g * g) / pow(max(1.0 + g * g - 2.0 * g * cosTheta, 0.001), 1.5);
      float opticalDepth = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
      float termTwi = smoothstep(-0.15, 0.10, dot(n, sunDir)) * (1.0 - smoothstep(0.10, 0.35, dot(n, sunDir)));
      vec3 sunsetColor = (u_Body == 6) ? vec3(0.35, 0.65, 0.95) : vec3(1.0, 0.45, 0.15); // Mars blue sunsets, Earth golden
      vec3 rayleighScatt = rimColor * opticalDepth * rayleighPhase * (0.2 + diff);
      vec3 mieScatt = vec3(1.0, 0.95, 0.85) * opticalDepth * miePhase * 0.15 * diff;
      vec3 sunsetScatt = sunsetColor * opticalDepth * termTwi * 0.8;
      col += (rayleighScatt + mieScatt + sunsetScatt) * rimGain;
    } else {
      float atmRim = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
      col += rimColor * atmRim * (0.25 + diff) * rimGain;
      float fwdAmt = pow(max(dot(-viewDir, sunDir), 0.0), 16.0);
      col += rimColor * fwdAmt * atmRim * 1.5 * rimGain;
      float limbRed = pow(1.0 - max(dot(n, viewDir), 0.0), 2.0) * 0.35 * diff;
      col = mix(col, col * vec3(1.05, 0.85, 0.7), limbRed);
      if (u_Body == 6) {
        // Mars: thin dusty rose rim.
        float dustRim = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
        col += vec3(0.9, 0.55, 0.35) * dustRim * (0.15 + diff * 0.5);
      }
    }
  }
  if (u_Type == 4) {
    vec3 comaColor = vec3(0.75, 1.0, 0.85) * 3.0;
    float ndotl = max(dot(normalize(v_Normal), normalize(u_CamPos - v_WorldPos)), 0.0);
    col = comaColor * (0.25 + 2.75 * pow(ndotl, 1.5));
    // Coma flicker: alive shimmer, fixed phase at fixed u_Time.
    col *= 0.88 + 0.09 * sin(u_Time * 6.0 + u_Seed * 3.0) + 0.03 * sin(u_Time * 17.0);
  }
  col *= 1.1 * u_Exposure;
  FragColor = vec4(max(col, vec3(0.0)), 1.0);
}
