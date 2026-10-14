#version 330 core
// Vela-like pulsar: hot neutron star + tilted-dipole lighthouse beams.
// Spin axis slightly leaned in screen; magnetic axis tilted ~22 deg and
// precessing with spin. Pulse = beam sweeps the camera line of sight.
// Dipole magnetosphere arcs, pulsed radio halo, background stars.
// Harness epoch: u_Time == 1.0 (tE = u_Time - 1.0 keeps spin phase stable).
out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;

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

// Dipole magnetosphere silhouette in the magnetic meridian, projected to
// screen. Loops pinch at the poles along the magnetic axis (classic dipole).
float dipoleArcs(vec2 m, float bLen, int count) {
  float acc = 0.0;
  for (int i = 1; i <= count; ++i) {
    float L = 0.10 * float(i);  // loop scale grows outward
    // angle from magnetic axis (both poles) in the projected plane
    float th = atan(m.y, abs(m.x) + 1e-5);
    float lineR = L * sin(th) * sin(th);
    float d = abs(length(m) - lineR);
    // thin bright line, fade outer loops
    float line = exp(-d * d * 9000.0) * exp(-L * 2.4);
    acc += line;
  }
  return acc;
}

void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  vec2 starUV = uv;
  uv.x *= u_Aspect;
  float r = length(uv);
  float tE = u_Time - 1.0;  // epoch-stable spin for the harness

  // Near-black sky plus two star layers.
  vec3 col = vec3(0.004, 0.005, 0.009);
  col += starLayer(starUV, 90.0, 0.992, 1.4);
  col += starLayer(starUV + vec2(13.1), 190.0, 0.985, 0.55);

  // --- 3D frame ----------------------------------------------------------
  // Spin axis: mostly up, slight lean in screen. Magnetic axis tilted ~22
  // deg and precessing: period ~2.5s (artistic; real Vela is 89 ms).
  float spin = tE * 2.5;
  float axisTilt = 0.22;
  vec3 S = normalize(vec3(sin(axisTilt), cos(axisTilt), 0.0));
  vec3 e1 = normalize(cross(S, vec3(0.0, 0.0, 1.0)));
  vec3 e2 = cross(S, e1);
  float magTilt = 0.38;
  vec3 B = normalize(S * cos(magTilt) +
                      (e1 * cos(spin) + e2 * sin(spin)) * sin(magTilt));

  // Pulse: how sharply the beam (or its opposite) points at the camera
  // (camera looks down -Z, so "toward camera" is +Z).
  float towardCam = max(B.z, 0.0);
  float awayCam = max(-B.z, 0.0);
  float pulse = pow(towardCam, 14.0) + 0.6 * pow(awayCam, 14.0);

  // --- Lighthouse beams --------------------------------------------------
  vec2 bScreen = B.xy;
  float bLen = length(bScreen);
  if (bLen > 0.02) {
    vec2 bdir = bScreen / bLen;
    float along = dot(uv, bdir);
    float perp = abs(dot(uv, vec2(-bdir.y, bdir.x)));
    // Cone: width grows with distance from the star along the axis.
    float coneW = 0.012 + abs(along) * 0.30;
    float beam = exp(-(perp * perp) / (coneW * coneW));
    // Hollow-ish core: brighter at the cone edge for a radio-loud look.
    float edge = 0.55 + 0.45 * smoothstep(0.0, coneW * 0.7, perp);
    float outward = smoothstep(0.02, 0.07, abs(along));
    float distFade = exp(-r * 2.0);
    // This physical beam's facing factor (near pole along +B, far along -B).
    float face = (along > 0.0) ? towardCam : awayCam;
    float gain = 0.12 + 3.2 * pow(max(face, 0.0), 10.0);
    vec3 beamCol = mix(vec3(0.55, 0.72, 1.0), vec3(0.90, 0.95, 1.0),
                       pow(max(face, 0.0), 6.0));
    col += beamCol * beam * edge * outward * distFade * gain * 0.55;
  }

  // --- Pulsed radio halo -------------------------------------------------
  // Soft synchrotron halo brightening with the pulse (the "on" state).
  float halo = exp(-r * 3.2) * (0.20 + 1.6 * pulse);
  col += vec3(0.40, 0.58, 1.0) * halo * 0.35;

  // --- Dipole magnetosphere arcs ----------------------------------------
  // Rotate screen coords into the magnetic frame (B projected on screen).
  if (bLen > 0.02) {
    vec2 bdir = bScreen / bLen;
    vec2 m = vec2(dot(uv, bdir), dot(uv, vec2(-bdir.y, bdir.x)));
    float arcs = dipoleArcs(m, bLen, 5);
    // Foreshorten: arcs dim when the magnetic pole points along the view
    // (they'd be seen edge-on / face-on in 3D).
    float arcFace = 0.35 + 0.65 * (1.0 - abs(B.z));
    vec3 arcCol = vec3(0.50, 0.70, 1.0);
    col += arcCol * arcs * arcFace * 0.5;
  }

  // --- Neutron star surface ---------------------------------------------
  // Tiny hot disk (~2% of screen). Blue-white photosphere; magnetic hot
  // poles brighter when they rotate toward the camera.
  float nsR = 0.020;
  float nsMask = smoothstep(nsR, nsR * 0.6, r);
  if (nsMask > 0.0) {
    vec2 sdir = (r > 1e-5) ? uv / r : vec2(0.0, 1.0);
    vec2 bdirN = normalize(B.xy + vec2(1e-6));
    float poleAlign = abs(dot(sdir, bdirN));
    float spot = pow(poleAlign, 10.0);
    float facing = 0.4 + 0.6 * (towardCam + awayCam);
    vec3 surf = vec3(0.72, 0.84, 1.0) * 1.1;
    vec3 hotSpot = vec3(1.0, 0.96, 0.90) * 4.0;
    col += nsMask * mix(surf, hotSpot, spot * facing);
  }

  // Central PSF glare, flares with the pulse.
  col += vec3(0.85, 0.92, 1.0) * exp(-r * r * 700.0) * (0.6 + 4.0 * pulse);

  // --- Finish ------------------------------------------------------------
  float vig = 1.0 - 0.3 * clamp(dot(starUV * 0.7, starUV * 0.7), 0.0, 1.0);
  col *= vig;

  col *= 1.25 * u_Exposure;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));

  float gr = hash21(starUV * 913.7 + vec2(fract(u_Time) * 7.31)) - 0.5;
  col += gr * 0.012;

  FragColor = vec4(col, 1.0);
}
