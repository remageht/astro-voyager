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
// Wrapped absolute angular distance in [-pi, pi].
float angDist(float a, float b) {
  float d = mod(a - b + 3.14159265, 6.2831853) - 3.14159265;
  return abs(d);
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
      // MARS: basalt + wind-blown dust streaks, seasonal tint, polar caps.
      float continents = fbm4(sp * 3.0 + u_Seed) * 0.9677 + 0.0323 * vnoise3((sp * 3.0 + u_Seed) * 16.9818);
      land = smoothstep(0.48, 0.56, continents);
      albedo = mix(u_ColA, u_ColB, land);
      float dust = fbm3(vec3(sp.x * 3.0, sp.y * 10.0, sp.z * 3.0) + u_Seed);
      albedo = mix(albedo, vec3(0.85, 0.55, 0.35), dust * 0.45);
      float season = 0.5 + 0.5 * sin(u_Time * 0.07);
      albedo = mix(albedo, albedo * vec3(1.06, 0.92, 0.86), season * 0.5);
      float cap = smoothstep(0.86, 0.93, abs(spN.y));
      albedo = mix(albedo, vec3(0.92, 0.90, 0.88), cap);
    } else if (u_Body == 5) {
      // MOON: dark maria plains + highlands + two-scale craters.
      float mar = smoothstep(0.52, 0.64, fbm4(sp * 2.0 + u_Seed));
      albedo = mix(vec3(0.62, 0.62, 0.63), vec3(0.30, 0.30, 0.32), mar);
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
    } else {
      // EARTH + generic rocky: continents, craters, caps, clouds.
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
      if (u_Body == 4) {
        vec3 cp = sp * 5.0 + u_Seed + vec3(u_Time * 0.03, 0.0, 0.0);
        float cl = fbm4(cp);
        float clm = smoothstep(0.55, 0.75, cl) * u_Clouds;
        cloudShadow = clm * 0.35;
        albedo *= 1.0 - cloudShadow * (0.2 + diff);
        albedo = mix(albedo, vec3(1.0), clm * 0.85);
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
  vec3 col = albedo * (0.03 + diff) + nightAmb;
  if (u_Body == 4) {
    // EARTH: soft terminator with warm twilight band.
    float day = smoothstep(-0.06, 0.22, diff);
    vec3 dayCol = albedo * (0.03 + diff);
    float twi = smoothstep(-0.08, 0.02, diff) * (1.0 - smoothstep(0.02, 0.25, diff));
    dayCol += albedo * vec3(0.9, 0.4, 0.2) * twi * 0.35;
    // Seasonal shine on the polar caps.
    float capM = smoothstep(0.80, 0.88, abs(spN.y));
    vec3 rdir = reflect(-sunDir, n);
    float spec = pow(max(dot(rdir, viewDir), 0.0), 24.0);
    float season = 0.5 + 0.5 * sin(u_Time * 0.1);
    dayCol += vec3(0.9, 0.95, 1.0) * spec * capM * (0.4 + 0.6 * season) * day;
    col = dayCol + nightAmb;
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
    }
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
