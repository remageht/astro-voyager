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
uniform sampler2D u_EarthMap;
uniform int u_HasEarthMap;
// v5.1.0: Stage 6 Exoplanetary Biomes & Ocean Hydrodynamics
uniform float u_Terraform;
uniform vec4 u_Wake;  // xyz = spin-local unit vector under skimming craft, w = intensity 0..1

float oceanGlintGGX(vec3 n, vec3 sunDir, vec3 viewDir, float rough) {
  vec3 H = normalize(sunDir + viewDir);
  float NdotH = max(dot(n, H), 0.0);
  float NdotV = max(dot(n, viewDir), 0.001);
  float NdotL = max(dot(n, sunDir), 0.001);
  float a = max(rough, 0.06);
  float a2 = a * a;
  float alpha2 = a2 * a2;
  float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
  float D = alpha2 / (3.14159265 * max(denom * denom, 1e-5));
  float F = 0.02 + 0.98 * pow(1.0 - max(dot(viewDir, H), 0.0), 5.0);
  float k = (a + 1.0);
  k = (k * k) / 8.0;
  float G = (NdotL / (NdotL * (1.0 - k) + k)) * (NdotV / (NdotV * (1.0 - k) + k));
  return (D * F * G) / max(4.0 * NdotV * NdotL, 0.001);
}

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
float fbm5(vec3 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 5; ++i) {
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
vec2 earthUv(vec3 p) {
  return vec2(atan(-p.z, p.x) / 6.28318530718 + 0.5,
              0.5 - asin(clamp(p.y, -1.0, 1.0)) / 3.14159265359);
}

float planetElevation(vec3 sp, int body, int type, float seed) {
  if (type == 0 || type == 1) return 0.0;
  float h = 0.0;
  if (body == 4) {
    return 0.0;
  } else if (body == 6) {
    // Mars: Tharsis volcanoes, crater rims & Valles Marineris canyon
    float baseM = fbm4(sp * 3.2 + seed * 2.1);
    float craters = abs(fbm4(sp * 12.0 + seed * 3.5) - 0.5) * 2.0;
    float fine = fbm3(sp * 34.0 + seed * 9.2);
    h = 0.018 * baseM + 0.014 * craters + 0.005 * fine;
  } else if (body == 12) {
    // Hercules-b: basalt ridges & emerald plateaus above sea level
    float cont = fbm4(sp * 3.8 + seed);
    float detail = fbm3(sp * 22.0 + seed * 4.1);
    h = smoothstep(0.47, 0.85, cont) * (0.016 + 0.008 * detail);
  } else if (body == 13 || body == 14) {
    float ridge = 1.0 - abs(fbm4(sp * 6.0 + seed * 1.9) - 0.5) * 2.0;
    h = 0.012 * pow(ridge, 3.0) + 0.004 * fbm3(sp * 24.0 + seed);
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
  if (u_PlanetRelief == 1 && u_Body != 4 && (u_Type == 2 || u_Type == 3)) {
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
    vec3 gp = sp * 9.0 + u_Seed;
    float g1 = fbm5(gp + vec3(u_Time * 0.06, u_Time * 0.03, 0.0));
    float g2 = fbm4(gp * 2.3 + vec3(0.0, -u_Time * 0.05, u_Time * 0.04));
    float network = 1.0 - abs(fbm4(gp * 0.6) - 0.5) * 2.0;
    float gran = g1 * 0.55 + g2 * 0.30 + network * 0.15;
    gran = clamp((gran - 0.5) * 3.4 + 0.5, 0.0, 1.0);
    vec3 ecol = mix(u_ColB * vec3(1.0, 0.55, 0.30), u_ColA, gran) * 4.2;
    ecol *= 0.50 + 0.95 * gran;
    float facN = fbm4(sp * 6.0 - u_Seed + vec3(0.0, u_Time * 0.03, 0.0));
    float facMask = smoothstep(0.55, 0.80, facN);
    ecol += vec3(1.0, 0.95, 0.85) * facMask * 1.8;
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
    float vort = fbm3(sp * 16.0 + u_Seed * 2.7);
    bands += (vort - 0.5) * 0.18;
    if (u_Body == 8) {
      bands = mix(bands, 0.5, 0.5);
    }
    albedo = mix(u_ColA, u_ColB, bands);
    if (u_Body == 7) {
      albedo = mix(albedo, albedo * vec3(1.06, 1.02, 0.94), 0.35);
    } else if (u_Body == 8) {
      albedo = mix(albedo, albedo * vec3(1.05, 0.98, 0.85), 0.4);
    }
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
      float collar = smoothstep(0.30, 0.36, d) * (1.0 - smoothstep(0.36, 0.50, d));
      albedo = mix(albedo, vec3(0.95, 0.93, 0.88), collar * 0.5);
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
      albedo = mix(albedo, vec3(0.80, 0.42, 0.24), dust * 0.55);
      float basalt = smoothstep(0.55, 0.70, fbm4(sp * 5.0 + u_Seed * 2.9));
      albedo = mix(albedo, vec3(0.35, 0.22, 0.16), basalt * 0.5);
      float rift = smoothstep(0.60, 0.75, fbm3(vec3(sp.x * 8.0, sp.y * 2.0, sp.z * 8.0) + u_Seed * 4.4));
      albedo *= 1.0 - rift * 0.3;
      float season = 0.5 + 0.5 * sin(u_Time * 0.07);
      albedo = mix(albedo, albedo * vec3(1.06, 0.92, 0.86), season * 0.5);
      // Craters and chasms
      float crBig = smoothstep(0.66, 0.76, fbm3(sp * 16.0 + u_Seed * 2.3));
      albedo *= 1.0 - crBig * 0.35;
      float crSmall = smoothstep(0.72, 0.77, fbm3(sp * 38.0 + u_Seed * 5.1));
      albedo *= 1.0 - crSmall * 0.22;
      // Organic fractured polar ice cap
      float capNoise = fbm4(sp * 7.5 + u_Seed * 3.7) * 0.06;
      float cap = smoothstep(0.82, 0.90, abs(spN.y) + capNoise);
      albedo = mix(albedo, vec3(0.94, 0.92, 0.90), cap);
      // v5.1.0: Terraformed Martian oceans, shoreline froth, basalt canyons & green lowlands
      if (u_Terraform > 0.01) {
        float seaLevel = mix(0.28, 0.48, clamp(u_Terraform, 0.0, 1.0));
        float oceanMask = 1.0 - smoothstep(seaLevel - 0.02, seaLevel + 0.02, continents);
        float shelfMask = (1.0 - smoothstep(seaLevel - 0.09, seaLevel, continents)) * oceanMask;
        float foamWave = 0.5 + 0.5 * sin(continents * 160.0 - u_Time * 2.2 + fbm3(sp * 24.0) * 6.0);
        float shoreBand = smoothstep(seaLevel - 0.025, seaLevel, continents) * (1.0 - smoothstep(seaLevel, seaLevel + 0.015, continents));
        vec3 deepSea = vec3(0.03, 0.16, 0.38);
        vec3 turquoiseShelf = vec3(0.12, 0.64, 0.72);
        vec3 waterCol = mix(deepSea, turquoiseShelf, shelfMask * 0.78);
        waterCol = mix(waterCol, vec3(0.88, 0.95, 0.98), shoreBand * foamWave * 0.75 * u_Terraform);
        float vegZone = smoothstep(seaLevel, seaLevel + 0.14, continents) * (1.0 - cap) * (1.0 - smoothstep(0.55, 0.85, abs(spN.y)));
        vec3 floraCol = mix(vec3(0.15, 0.44, 0.22), vec3(0.28, 0.55, 0.26), fbm3(sp * 11.0 + u_Seed));
        albedo = mix(albedo, floraCol, vegZone * u_Terraform * 0.85);
        albedo = mix(albedo, waterCol, oceanMask * smoothstep(0.15, 0.55, u_Terraform));
        land = mix(land, 1.0 - oceanMask, smoothstep(0.15, 0.55, u_Terraform));
      }
    } else if (u_Body == 12) {
      // HERCULES-B: Turquoise coral archipelagos, basalt canyons, emerald continental lowlands & silicate peaks
      float continents = fbm5(sp * 3.8 + u_Seed) * 0.85 + 0.15 * fbm3(sp * 11.5 + u_Seed * 2.1);
      float seaLevel = 0.49;
      land = smoothstep(seaLevel - 0.015, seaLevel + 0.018, continents);
      float shelf = smoothstep(seaLevel - 0.085, seaLevel, continents) * (1.0 - land);
      float abyss = 1.0 - smoothstep(seaLevel - 0.16, seaLevel - 0.05, continents);
      vec3 oceanCol = mix(vec3(0.06, 0.36, 0.54), vec3(0.02, 0.12, 0.30), abyss);
      oceanCol = mix(oceanCol, vec3(0.10, 0.74, 0.78), shelf * 0.85);
      // Shoreline surf froth
      float shoreBand = smoothstep(seaLevel - 0.022, seaLevel, continents) * (1.0 - land);
      float surfCrest = 0.5 + 0.5 * sin(continents * 180.0 - u_Time * 2.4 + fbm3(sp * 28.0) * 5.0);
      oceanCol = mix(oceanCol, vec3(0.90, 0.97, 0.99), shoreBand * surfCrest * 0.82);
      // Continental strata: coastal sand -> emerald vegetation -> basalt highlands -> quartz peaks
      float latCool = smoothstep(0.55, 0.85, abs(spN.y));
      vec3 beachCol = vec3(0.76, 0.72, 0.56);
      vec3 forestCol = mix(vec3(0.12, 0.44, 0.24), vec3(0.24, 0.56, 0.30), fbm3(sp * 14.0 + u_Seed));
      vec3 canyonCol = vec3(0.32, 0.34, 0.40);
      vec3 peakCol = vec3(0.84, 0.88, 0.92);
      vec3 landCol = mix(beachCol, forestCol, smoothstep(seaLevel + 0.015, seaLevel + 0.06, continents));
      landCol = mix(landCol, canyonCol, smoothstep(0.60, 0.74, continents));
      landCol = mix(landCol, peakCol, clamp(smoothstep(0.74, 0.86, continents) + latCool * 0.65, 0.0, 1.0));
      albedo = mix(oceanCol, landCol, land);
      if (u_Clouds > 0.05) {
        vec3 sunDirLocal = sunDir;
        if (u_SpinAngle != 0.0) {
          float spinC = cos(u_SpinAngle);
          float spinS = sin(u_SpinAngle);
          sunDirLocal = vec3(spinC * sunDir.x + spinS * sunDir.z, sunDir.y,
                             -spinS * sunDir.x + spinC * sunDir.z);
        }
        vec3 cp = sp * 4.5 + u_Seed + vec3(u_Time * 0.025, 0.0, 0.0);
        float cl = fbm4(cp);
        float clm = smoothstep(0.50, 0.72, cl) * u_Clouds;
        vec3 sSp = sp - sunDirLocal * 0.024;
        float sh = smoothstep(0.51, 0.73, fbm4(sSp * 4.5 + u_Seed + vec3(u_Time * 0.025, 0.0, 0.0))) * u_Clouds * 0.38;
        albedo *= 1.0 - sh * (0.2 + 0.8 * diff);
        albedo = mix(albedo, vec3(0.95, 0.98, 1.0), clm * 0.82);
      }
    } else if (u_Body == 13) {
      // TITAN: Tholin orange dunes, cryovolcanic plains & dark hydrocarbon methane lakes
      float basin = fbm4(sp * 3.4 + u_Seed);
      float duneNoise = fbm3(vec3(sp.x * 18.0, sp.y * 4.0, sp.z * 18.0) + u_Seed * 2.1);
      float cryo = smoothstep(0.66, 0.78, fbm4(sp * 8.5 + u_Seed * 3.3));
      float seaLevel = 0.39;
      land = smoothstep(seaLevel - 0.015, seaLevel + 0.015, basin);
      vec3 duneCol = mix(u_ColB, u_ColA, duneNoise);
      duneCol = mix(duneCol, vec3(0.76, 0.72, 0.66), cryo * 0.45);
      vec3 methaneMare = vec3(0.04, 0.05, 0.07);
      float froth = 0.5 + 0.5 * sin(basin * 140.0 - u_Time * 1.4);
      float mareShore = smoothstep(seaLevel - 0.018, seaLevel, basin) * (1.0 - land);
      methaneMare = mix(methaneMare, vec3(0.45, 0.38, 0.28), mareShore * froth * 0.45);
      albedo = mix(methaneMare, duneCol, land);
    } else if (u_Body == 14) {
      // EUROPA: Fractured glacial shell with reddish-ochre magnesium-sulfate lineae & chaos terrain
      float iceBase = fbm4(sp * 3.2 + u_Seed);
      float lineae1 = 1.0 - abs(fbm4(vec3(sp.x * 5.0 + sp.y * 2.0, sp.y * 6.0, sp.z * 5.0) + u_Seed * 1.9) - 0.5) * 2.0;
      float lineae2 = 1.0 - abs(fbm4(vec3(sp.z * 7.0, sp.x * 3.0 - sp.y * 5.0, sp.y * 4.0) + u_Seed * 3.1) - 0.5) * 2.0;
      float ridge = pow(max(lineae1, lineae2), 6.0);
      float chaos = smoothstep(0.60, 0.74, fbm3(sp * 11.0 + u_Seed * 4.7));
      vec3 glacierCol = mix(vec3(0.76, 0.84, 0.90), vec3(0.95, 0.97, 0.99), iceBase);
      vec3 saltRidge = vec3(0.56, 0.32, 0.22);
      albedo = mix(glacierCol, saltRidge, clamp(ridge * 0.85 + chaos * 0.35, 0.0, 0.90));
      land = 1.0;
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
      albedo *= 1.0 - micro * 0.35;
    } else if (u_Body == 4) {
      vec4 surface = u_HasEarthMap == 1 ? texture(u_EarthMap, earthUv(spN)) : vec4(0.08, 0.08, 0.08, 0.0);
      land = surface.a;
      albedo = surface.rgb;

      // --- Two-Layer Cyclonic Cloud System with Directional Ground Shadows & Coriolis Drift ---
      vec3 sunDirLocal = sunDir;
      if (u_SpinAngle != 0.0) {
        float spinC = cos(u_SpinAngle);
        float spinS = sin(u_SpinAngle);
        sunDirLocal = vec3(spinC * sunDir.x + spinS * sunDir.z, sunDir.y,
                           -spinS * sunDir.x + spinC * sunDir.z);
      }
      float latWeight = cos(sp.y * 3.14159265 * 0.5);
      float coriolisSpeed = mix(0.012, 0.038, latWeight);
      vec3 cp = sp * 4.8 + u_Seed + vec3(u_Time * coriolisSpeed, 0.0, 0.0);
      float cl1 = fbm4(cp);
      float cl2 = fbm3(cp * 2.4 + vec3(u_Time * (coriolisSpeed * 0.65), 0.0, 0.0));
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
      // Directional cloud shadow projected on exoplanet terrain
      if (u_Clouds > 0.05) {
        vec3 sunDirLocal = sunDir;
        if (u_SpinAngle != 0.0) {
          float spinC = cos(u_SpinAngle);
          float spinS = sin(u_SpinAngle);
          sunDirLocal = vec3(spinC * sunDir.x + spinS * sunDir.z, sunDir.y,
                             -spinS * sunDir.x + spinC * sunDir.z);
        }
        vec3 shadowSp = sp - sunDirLocal * 0.026;
        float shadowVal = fbm3(shadowSp * 4.2 + u_Seed);
        float cloudShadowExo = smoothstep(0.52, 0.74, shadowVal) * u_Clouds * 0.38;
        albedo *= 1.0 - cloudShadowExo * (0.2 + 0.8 * diff);
      }
      // Shallow coastal shelf tint around continent edges.
      float shelf = smoothstep(0.42, 0.48, continents) * (1.0 - land);
      albedo = mix(albedo, vec3(0.25, 0.55, 0.75), shelf * 0.5);
      float cr = smoothstep(0.68, 0.8, fbm3(sp * 22.0 + u_Seed));
      albedo *= 1.0 - cr * 0.45;
      float cap = smoothstep(0.80, 0.88, abs(spN.y));
      albedo = mix(albedo, vec3(0.9, 0.93, 0.95), cap);
      if (u_Terraform > 0.20) {
        float foam = 0.5 + 0.5 * sin(continents * 150.0 - u_Time * 2.1);
        float shore = smoothstep(0.45, 0.48, continents) * (1.0 - land);
        albedo = mix(albedo, vec3(0.86, 0.95, 0.98), shore * foam * 0.70 * clamp(u_Terraform, 0.0, 1.0));
      }
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
        float g1 = fbm5(vec3(sp.x, sp.y * 4.0, sp.z) * 42.0 + u_Seed * 5.0);
        float g2 = fbm5(vec3(sp.x, sp.y * 6.0, sp.z) * 110.0 + u_Seed * 9.0);
        float g3 = fbm5(vec3(sp.x, sp.y * 8.0, sp.z) * 220.0 + u_Seed * 13.0);
        albedo *= 1.0 + (0.16 * (g1 - 0.5) + 0.10 * (g2 - 0.5) + 0.06 * (g3 - 0.5)) * nearK;
      } else if (u_Type == 2 && u_Body != 4) {
        float r1 = fbm5(sp * 70.0 + u_Seed * 3.1);
        float r2 = fbm5(sp * 180.0 + u_Seed * 7.7);
        float r3 = fbm5(sp * 340.0 + u_Seed * 11.3);
        albedo *= 1.0 + (0.20 * (r1 - 0.5) + 0.12 * (r2 - 0.5) + 0.07 * (r3 - 0.5)) * nearK;
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
    vec3 dayCol = albedo * (0.03 + rsF * diff);
    float twi = smoothstep(-0.08, 0.02, diff) * (1.0 - smoothstep(0.02, 0.25, diff));
    dayCol += albedo * vec3(0.9, 0.4, 0.2) * twi * 0.35;
    // Seasonal shine on the polar caps.
    vec3 iceMap = u_HasEarthMap == 1 ? texture(u_EarthMap, earthUv(spN)).rgb : vec3(0.0);
    float capM = smoothstep(0.45, 0.80, min(iceMap.r, min(iceMap.g, iceMap.b))) * land;
    vec3 rdir = reflect(-sunDir, n);
    float spec = pow(max(dot(rdir, viewDir), 0.0), 24.0);
    float season = 0.5 + 0.5 * sin(u_Time * 0.1);
    dayCol += vec3(0.9, 0.95, 1.0) * spec * capM * (0.4 + 0.6 * season) * day;
    // v3.2.0 / v4.5.0: PBR Cook-Torrance GGX ocean sun glint on water surfaces (F0=0.02)
    float ggx = oceanGlintGGX(n, sunDir, viewDir, 0.14);
    float glint = pow(max(dot(rdir, viewDir), 0.0), 90.0);
    dayCol += vec3(0.85, 0.94, 1.0) * (ggx * 2.2 + glint * 0.5) * (1.0 - land) * (1.0 - capM) * day;
    // v5.1.0: Low-altitude ship hydro wake froth on liquid oceans
    if (u_Wake.w > 0.01) {
      float dWake = length(spN - normalize(u_Wake.xyz));
      float wakeRing = exp(-dWake * 18.0) * (0.55 + 0.45 * sin(dWake * 180.0 - u_Time * 10.0));
      dayCol += vec3(0.82, 0.95, 1.0) * wakeRing * (1.0 - land) * (1.0 - capM) * u_Wake.w * 0.85 * day;
    }
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

    // Aurora Borealis / Australis ribbons at polar latitudes (Birkeland currents)
    float polarLat = abs(sp.y);
    if (polarLat > 0.65) {
      float auroraOval = smoothstep(0.65, 0.78, polarLat) * (1.0 - smoothstep(0.85, 0.95, polarLat));
      float auroraWave = sin(atan(sp.z, sp.x) * 5.0 + u_Time * 0.8) * 0.5 + 0.5;
      float auroraCurtain = fbm3(sp * 14.0 + vec3(0.0, u_Time * 0.35, u_Time * 0.15));
      float auroraIntensity = auroraOval * (0.4 + 0.6 * auroraWave) * auroraCurtain * (1.0 - smoothstep(0.0, 0.15, diff)) * 1.5;
      vec3 auroraCol = mix(vec3(0.12, 0.95, 0.48), vec3(0.72, 0.18, 0.95), smoothstep(0.74, 0.85, polarLat));
      col += auroraCol * auroraIntensity;
    }
  } else if (u_Type == 2 && u_Body != 3) {
    float night = 1.0 - smoothstep(0.0, 0.08, diff);
    float cc = fbm3(sp * 14.0 + u_Seed);
    float cities = smoothstep(0.62, 0.8, cc) * land;
    col += vec3(1.0, 0.75, 0.42) * cities * night * u_Clouds * 2.2;
    // Exoplanet / Titan hydrocarbon / terraformed ocean GGX glint & low-alt hydro wake
    if (u_Clouds > 0.05 || u_Body == 12 || u_Body == 13 || u_Terraform > 0.20) {
      float seaRough = (u_Body == 13) ? 0.10 : 0.16;
      float ggx = oceanGlintGGX(n, sunDir, viewDir, seaRough);
      vec3 glintTint = (u_Body == 13) ? vec3(0.95, 0.72, 0.42) : vec3(0.78, 0.94, 1.0);
      col += glintTint * ggx * 1.8 * (1.0 - land) * diff;
      if (u_Wake.w > 0.01) {
        float dWake = length(spN - normalize(u_Wake.xyz));
        float wakeRing = exp(-dWake * 18.0) * (0.55 + 0.45 * sin(dWake * 180.0 - u_Time * 10.0));
        col += vec3(0.84, 0.96, 1.0) * wakeRing * (1.0 - land) * u_Wake.w * (0.25 + 0.75 * diff);
      }
    }
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
    } else if (u_Body == 12) {
      rimColor = vec3(0.18, 0.72, 1.05);
      rimGain = 1.40;
    } else if (u_Body == 13) {
      // Titan: dense orange photochemical nitrogen-methane haze
      rimColor = vec3(1.0, 0.62, 0.24);
      rimGain = 1.95;
    }
    // v4.3.0 / v4.5.0: Physical Rayleigh & Mie Atmospheric Scattering
    if (u_PlanetAtmosphere == 1) {
      float cosTheta = dot(-viewDir, sunDir);
      float rayleighPhase = 0.75 * (1.0 + cosTheta * cosTheta);
      float g = 0.80;
      float miePhase = (1.0 - g * g) / (4.0 * 3.14159265 * pow(max(1.0 + g * g - 2.0 * g * cosTheta, 0.0001), 1.5));
      float opticalDepth = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
      float termTwi = smoothstep(-0.16, 0.08, dot(n, sunDir)) * (1.0 - smoothstep(0.08, 0.32, dot(n, sunDir)));
      vec3 sunsetColor = (u_Body == 6) ? vec3(0.35, 0.65, 0.95) : vec3(1.0, 0.46, 0.16); // Mars blue sunsets, Earth golden
      vec3 rayleighScatt = rimColor * opticalDepth * rayleighPhase * (0.2 + diff);
      vec3 mieScatt = vec3(1.0, 0.96, 0.88) * opticalDepth * (miePhase * 12.5) * 0.18 * diff;
      vec3 sunsetScatt = sunsetColor * opticalDepth * termTwi * (0.85 + 0.45 * clamp(u_Clouds, 0.0, 1.0));
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
