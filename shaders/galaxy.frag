#version 330 core
// M31 Andromeda Galaxy impostor sprite (GLSL 330)
// HDR linear workflow with ACES filmic tonemap and gamma 1/2.2.
// Tilted spiral: de-Vaucouleurs-like bulge, exponential disk, two blue arms,
// multiplicative dust lanes, faint warm halo, background starfield.
// Progressive star-resolution layer on approach via u_Zoom.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Zoom;

// Hash for procedural stars and grain
float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

// ACES filmic fit (Narkowicz), input HDR linear, output 0..1
vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  uv.x *= u_Aspect;
  uv /= max(u_Zoom, 0.001);

  // Rotate slightly and tilt for Andromeda's 77 degree inclination
  float angle = 0.55 + 0.005 * sin(u_Time * 0.2);
  float s = sin(angle), c = cos(angle);
  vec2 rot = vec2(c * uv.x - s * uv.y, s * uv.x + c * uv.y);

  // Compress along minor axis for tilted disc
  vec2 p = vec2(rot.x * 1.0, rot.y * 2.4);
  float r = length(p);

  // Galaxy-plane angle and log radius for logarithmic spiral
  float phi = atan(p.y, p.x);
  float lr = log(max(r, 0.01));

  // Linear-space palette (sRGB -> linear via pow 2.2)
  vec3 bulgeC = pow(vec3(0.92, 0.78, 0.60), vec3(2.2));
  vec3 armC = pow(vec3(0.45, 0.60, 0.92), vec3(2.2));
  vec3 dustLin = pow(vec3(0.16, 0.10, 0.06), vec3(2.2));
  vec3 haloC = pow(vec3(0.45, 0.40, 0.35), vec3(2.2)) * 0.06;
  vec3 bgC = vec3(0.004, 0.005, 0.009);

  // Bulge: de-Vaucouleurs-like r^1/4 falloff, HDR peak 5.0
  float bulge = 5.0 * exp(-pow(max(r * 7.0, 0.0), 0.25) * 3.0);
  vec3 bulgeTerm = bulge * bulgeC;

  // Exponential disk, faint vs bulge for 12-20x core-to-arm ratio
  float disk = exp(-r * 2.8) * 0.18;

  // Two blue arms (young stars)
  float spiral = sin(phi * 2.0 - lr * 3.5);
  float armMask = smoothstep(0.1, 0.9, spiral) * exp(-r * 1.8);
  float arm = armMask * 0.22;

  // Dust lanes: multiply disk+arms by ~0.35 with dark brown tint
  float dust = sin(phi * 2.0 - lr * 3.5 + 0.6);
  float dustLane = smoothstep(0.3, 0.8, dust) * exp(-r * 2.0) * smoothstep(0.08, 0.35, r);
  dustLane = clamp(dustLane, 0.0, 1.0);
  vec3 dustMul = (dustLin / max(dustLin.x, 1e-4)) * 0.35;
  vec3 diskArm = (disk + arm) * armC;
  diskArm *= mix(vec3(1.0), dustMul, dustLane);

  // Outer halo: faint warm
  float halo = exp(-r * 1.2);
  vec3 haloTerm = halo * haloC;

  // HDR linear base
  vec3 col = bgC + haloTerm + bulgeTerm + diskArm;

  // Background starfield in screen space (stable under zoom)
  // Sparse subpixel points: 1/(1+d*d*1e6), power-law brightness
  vec2 bq = vec2((v_TexCoord.x * 2.0 - 1.0) * u_Aspect, v_TexCoord.y * 2.0 - 1.0);
  vec2 sg = bq * 24.0;
  vec2 sid = floor(sg);
  vec2 sf = fract(sg) - 0.5;
  float sh = hash21(sid);
  vec2 soff = vec2(hash21(sid + 7.13), hash21(sid + 3.71)) - 0.5;
  soff *= 0.8;
  vec2 sd = sf - soff;
  float spt = 1.0 / (1.0 + dot(sd, sd) * 1.0e6);
  float sb = pow(sh, 9.0) * 8.0;
  float ch = hash21(sid + 17.7);
  vec3 scol = vec3(1.0, 0.96, 0.88);
  if (ch > 0.60 && ch <= 0.80) {
    scol = vec3(0.65, 0.78, 1.0);
  } else if (ch > 0.80) {
    scol = vec3(1.0, 0.55, 0.30);
  }
  col += spt * sb * scol;

  // Progressive detail layer: resolve individual star clouds upon closer approach
  // Adapted to new palette: denser threshold, blue-to-warm mix, mask rescaled
  float resolve = smoothstep(1.4, 3.0, u_Zoom);
  if (resolve > 0.0) {
    vec2 gridP = p * 60.0;
    vec2 id = floor(gridP);
    vec2 f = fract(gridP) - 0.5;
    float h = hash21(id);
    float h2 = hash21(id + 4.7);
    float starHit = step(0.97, h);
    float glow = exp(-dot(f, f) * 18.0);
    vec3 rcol = mix(armC * 2.5 + vec3(0.15), vec3(1.0, 0.85, 0.65), step(0.75, h2));
    float cloudMask = clamp(armMask * 1.5, 0.0, 1.0);
    col += starHit * glow * cloudMask * resolve * rcol * 1.2;
  }

  // ACES filmic tonemap with exposure 1.0, then gamma 1/2.2
  float exposure = 1.0;
  col *= exposure;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));

  // Vignette ~0.3
  vec2 q = v_TexCoord * 2.0 - 1.0;
  float vig = clamp(1.0 - 0.3 * dot(q, q), 0.0, 1.0);
  col *= vig;

  // Post-ACES grain
  float gr = hash21(v_TexCoord * 1021.0 + fract(u_Time) * 7.0) - 0.5;
  col += gr * 0.012;

  FragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
