#version 330 core
// 3C 273 quasar impostor: HDR core + thin blue jet + faint host + starfield.
// ACES + vignette + grain. Diffraction spikes removed (cheap look).
out vec4 FragColor;
in vec2 v_TexCoord;
uniform vec3 u_CoreColor;
uniform float u_Time;
uniform float u_Exposure;
uniform float u_QCoreK;  // quasar core gain (default 1.0; qso 2.2): core + inner glow up, host down
uniform float u_QZoom;  // close-up magnification from camera distance (neutral 1.0)

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float hash11(float p) {
  p = fract(p * 443.8975);
  p *= p + 19.19;
  return fract(p * p);
}

float noise1D(float p) {
  float i = floor(p);
  float f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  return mix(hash11(i), hash11(i + 1.0), f);
}

vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 starLayer(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  float h0 = hash21(id);
  float mask = step(thresh, h0);
  vec2 off = vec2(hash21(id + vec2(7.13)), hash21(id + vec2(3.71))) - vec2(0.5);
  float d = length(f - off * 0.8);
  float glow = exp(-d * d * 42.0);
  float h1 = hash21(id + vec2(9.17));
  float bright = pow(h1, 12.0) * gain;
  vec3 tint = mix(vec3(0.65, 0.75, 1.0), vec3(1.0, 0.85, 0.65), hash21(id + vec2(5.31)));
  return mask * glow * bright * tint;
}

void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  // Close-up zoom driven by camera distance (SceneManager sets u_QZoom from
  // spawn/dist, clamped >= 1). Far away qz == 1.0: pixel-identical framing.
  float qz = (u_QZoom <= 0.0) ? 1.0 : u_QZoom;
  uv /= qz;
  // Up-close detail smoothing: 0 at qz == 1 (reference frames unchanged).
  float qCloseK = smoothstep(2.0, 6.0, qz);
  float r = length(uv);

  // Central black-hole shadow absorbs core/host/disk light inside r < 0.015,
  // at any zoom: the shadow stays dark and hard-edged, no rings inside it.
  float shadow = smoothstep(0.010, 0.015, r);

  // Near-black sky.
  vec3 col = vec3(0.004, 0.005, 0.009);

  // Background starfield: two depth layers, power-law brightness.
  col += starLayer(uv, 90.0, 0.992, 1.4);
  col += starLayer(uv + vec2(13.1), 190.0, 0.985, 0.55);

  // Faint elliptical host galaxy under the core. Dimmed as the core gain
  // rises (x0.7 at u_QCoreK == 2.2); exactly neutral at u_QCoreK == 1.0.
  float hostK = mix(1.0, 0.7, clamp((u_QCoreK - 1.0) / 1.2, 0.0, 1.0));
  vec2 euv = vec2(uv.x, uv.y * 1.6);
  float re = length(euv);
  vec3 hostCol = vec3(0.55, 0.47, 0.38) * 0.030 * exp(-pow(re / 0.2, 0.25));
  hostCol *= 1.0 - smoothstep(0.3, 1.0, re);
  // Faint two-arm spiral hint in the host (m=2, slowly rotating, fades out).
  float hostAng = atan(euv.y, euv.x) - u_Time * 0.05;
  float hostSpiral = 0.5 + 0.5 * sin(hostAng * 2.0 + re * 9.0);
  hostCol *= 1.0 + 0.60 * (hostSpiral - 0.5) * exp(-re * 4.0);
  hostCol *= hostK;
  hostCol *= shadow;
  col += hostCol;

  // Rotating accretion disk swirl, face-on spiral with differential
  // (Kepler-like) shear: inner pattern laps the outer one, visibly winding
  // live. Anchored to the harness epoch (u_Time == 1.0): the reference frame
  // is bit-identical to the legacy rigid rotation (baseAngle - 0.25).
  // Up close the spiral relaxes to its mean (no moire rings near the core).
  float baseAngle = atan(uv.y, uv.x);
  float diskR = length(uv * vec2(1.0, 2.2));
  float qOm = 0.004 * pow(max(diskR, 1e-3), -1.5);
  float patternAngle = baseAngle - (0.25 * 1.0 + qOm * (u_Time - 1.0));
  float spiral = sin(patternAngle * 3.0 + diskR * 18.0);
  float spiralSm = mix(spiral, 0.0, qCloseK);
  float diskMask = smoothstep(0.004, 0.015, diskR) * (1.0 - smoothstep(0.015, 0.12, diskR));
  float diskBright = (0.5 + 0.5 * spiralSm) * diskMask * exp(-diskR * 6.0);
  diskBright *= shadow;
  col += vec3(0.65, 0.75, 1.0) * diskBright * 0.6;

  // Thin blue jet along x axis, one side longer (+x).
  float d = abs(uv.y + 0.12 * uv.x + sin(u_Time * 0.3) * 0.008);
  float width = 1.0 - smoothstep(0.004, 0.012, d);
  float s = uv.x;
  // PH x2 outflow (was 0.15): streaks visibly ride outward live; anchored
  // to the harness epoch so the reference frame is bit-identical.
  float sAdv = s - (0.15 * 1.0 + 0.30 * (u_Time - 1.0));
  float n = noise1D(sAdv * 8.0 + 1.7);
  float knots = pow(n, 8.0) * 0.8;
  knots *= exp(-abs(s) * 2.0) + 0.25;
  float jetBright = 0.08 + knots * 0.34;
  float envP = exp(-max(s, 0.0) * 2.2);
  float envN = exp(-max(-s, 0.0) * 5.5);
  float env = mix(envN, envP, step(0.0, s));
  float axial = smoothstep(0.008, 0.03, abs(s));
  float jetMask = width * env * axial * (1.0 - smoothstep(0.2, 0.9, r));
  vec3 jetCol = vec3(0.60, 0.72, 1.0) * jetBright;
  jetMask *= 0.9 + 0.1 * sin(u_Time * 1.5 + s * 20.0);
  col += jetMask * jetCol;
  // Discrete blobs riding outward along +x, fading with travel distance.
  // Radial-in-|s| + time only; gated to +x so the de-beamed counter-jet
  // keeps its asymmetry. PH x2 outflow (was 0.12): clumps visibly stream
  // live; harness-epoch anchored, reference frame bit-identical.
  float bR0 = 0.03 + fract(0.12 * 1.0 + 0.24 * (u_Time - 1.0) + 0.00) * 0.45;
  float bR1 = 0.03 + fract(0.12 * 1.0 + 0.24 * (u_Time - 1.0) + 0.37) * 0.45;
  float bR2 = 0.03 + fract(0.12 * 1.0 + 0.24 * (u_Time - 1.0) + 0.71) * 0.45;
  float bSig = 0.0;
  bSig += exp(-pow((abs(s) - bR0) * 14.0, 2.0)) * exp(-bR0 * 3.0);
  bSig += exp(-pow((abs(s) - bR1) * 14.0, 2.0)) * exp(-bR1 * 3.0);
  bSig += exp(-pow((abs(s) - bR2) * 14.0, 2.0)) * exp(-bR2 * 3.0);
  col += vec3(0.62, 0.74, 1.0) * bSig * width * axial * 0.5 * step(0.0, s);

  // Faint receding counter-jet (-x), de-beamed: mirrored s, wider, shorter, dimmer.
  float sC = -s;
  float sCAdv = sC - (0.15 * 1.0 + 0.30 * (u_Time - 1.0));
  float nC = noise1D(sCAdv * 8.0 + 1.7);
  float knotsC = pow(nC, 8.0) * 0.8;
  knotsC *= exp(-abs(sC) * 2.0) + 0.25;
  float jetBrightC = (0.08 + knotsC * 0.34) * 0.15;
  float envCP = exp(-max(sC, 0.0) * 2.2);
  float envCN = exp(-max(-sC, 0.0) * 5.5);
  float envC = mix(envCN, envCP, step(0.0, sC));
  float dC = abs(uv.y + 0.12 * uv.x + sin(u_Time * 0.3) * 0.008);
  float widthC = min((1.0 - smoothstep(0.004, 0.012, dC)) * 1.6, 1.0);
  float axialC = smoothstep(0.008, 0.03, abs(sC));
  float cjShort = 1.0 - smoothstep(0.30, 0.54, abs(sC));
  float sideC = 1.0 - step(0.0, s);
  float jetMaskC = widthC * envC * axialC * cjShort * sideC * (1.0 - smoothstep(0.2, 0.9, r));
  vec3 jetColC = vec3(0.60, 0.72, 1.0) * jetBrightC;
  jetMaskC *= 0.9 + 0.1 * sin(u_Time * 1.5 + sC * 20.0);
  col += jetMaskC * jetColC;

  // HDR core with tight falloff, 2-3 px feel. No cross spikes.
  // BH vs quasar look: a black hole shows a dark shadow + lensed thin disk
  // with a dark core; a quasar core outshines everything (u_QCoreK) + jets.
  // Central black-hole shadow absorbs core light inside r < 0.015.
  float coreFall = 1.0 / (1.0 + r * r * 12000.0);
  vec3 coreBase = vec3(1.0, 0.98, 0.94) * 10.0;
  vec3 tint = u_CoreColor / vec3(1.0, 0.85, 0.65);
  float flick = 0.94 + 0.05 * sin(u_Time * 2.3) + 0.02 * sin(u_Time * 5.7);
  // Proximity dimming: blinding from afar, compact structured core/ring up
  // close (no milky blowout at qso_close). Narrower falloff + deeper dimming.
  float proxDim = mix(1.0, 0.12, qCloseK);
  col += coreBase * tint * coreFall * flick * shadow * u_QCoreK * proxDim;

  // Compact white glow ring hugging the shadow (closer in, narrower, whiter).
  float ringQ = (r - 0.026) * 120.0;
  float ringBright = exp(-ringQ * ringQ);
  float ang = atan(uv.y, uv.x) - u_Time * 1.5;
  float asym = 0.55 + 0.45 * sin(ang * 2.0);
  col += vec3(1.0, 0.93, 0.82) * ringBright * asym * (3.0 * u_QCoreK) * proxDim;

  // PH core-touch hint: a soft white flash when the camera pushes into the
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

  // ACES filmic, exposure 1.0, then gamma.
  col *= u_Exposure;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));

  // Post-ACES grain.
  float gr = hash21(v_TexCoord * 913.7 + vec2(fract(u_Time) * 7.31)) - 0.5;
  col += gr * 0.012;

  FragColor = vec4(col, 1.0);
}
