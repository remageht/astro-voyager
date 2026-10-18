#version 330 core
// M1 Crab Nebula Supernova Remnant impostor sprite (GLSL 330)
// True Crab palette: red-orange ridged filaments + blue synchrotron core.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;
uniform float u_Exposure;
// v3.6.0: мир-привязка импостера — NDC-центр объекта, зум от дистанции и
// ракурс камеры (0/1/0 на спавне — нейтрально для эталонах).
uniform vec2 u_ObjNdc;
uniform float u_ObjZoom;
uniform vec3 u_CamPar;

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

// v3.0.0: ACES + gamma moved to post_comp.frag; this shader outputs linear HDR.

vec3 starLayer(vec2 uv, float scale, float thresh, float gain) {
  vec2 g = uv * scale;
  vec2 id = floor(g);
  vec2 f = fract(g) - vec2(0.5);
  // v3.5.0: sin-based lattice hash (hash2) instead of hash21: the latter
  // clusters on integer lattices, which aligned the star offsets into
  // visible dotted columns. Jitter is near-full-cell so the field reads
  // Poisson-like.
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
  // Фон звёзд — экранный (не привязан к объекту).
  vec2 starUV = ndc;
  // Объект живёт в мире: центр из NDC, зум от дистанции спавна.
  float nz = (u_ObjZoom <= 0.0) ? 1.0 : u_ObjZoom;
  vec2 uv = (ndc - u_ObjNdc) / nz;
  uv.x *= u_Aspect;

  vec2 p = vec2(uv.x * 1.25, uv.y * 0.95);
  float r = length(p);

  // Near-black sky plus starfield.
  vec3 col = vec3(0.004, 0.005, 0.009);
  col += starLayer(starUV, 90.0, 0.992, 1.4);
  col += starLayer(starUV + vec2(13.1), 190.0, 0.985, 0.55);

  // Red-orange filaments, ridged fbm 4 octaves.
  vec2 q = p * (1.0 + 0.9 * exp(-max(r, 1e-3) * 1.5));
  float fil = ridgedFbm(q * 5.0 + vec2(u_Time * 0.015, 0.0));
  fil = pow(fil, 3.0);
  float radialFalloff = 1.0 - smoothstep(0.15, 0.9, r);
  float filMask = smoothstep(0.45, 0.75, fil) * radialFalloff;
  float fil2 = ridgedFbm(q * 9.0 + vec2(0.0, u_Time * 0.01));
  float fiberMask = smoothstep(0.5, 0.8, fil2) * (1.0 - smoothstep(0.2, 0.85, r));
  vec3 filamentCol = vec3(0.82, 0.22, 0.10) * 0.9;
  float thread = pow(fil2, 12.0);
  vec3 diffuse = filMask * filamentCol + fiberMask * filamentCol * 0.6 + thread * filamentCol *
                  0.5 * radialFalloff;

  // Synchrotron blue center with exp falloff.
  float syncFall = exp(-r / 0.4);
  float syncAmp = mix(1.2, 0.5, clamp(r / 0.8, 0.0, 1.0));
  vec3 syncCol = vec3(0.42, 0.62, 1.0) * (syncFall * syncAmp);
  diffuse += syncCol;

  // Outer wispy envelope outside main body: low-freq 2-octave fbm shell.
  // Outer wisps: same ridged filaments, larger scale, thin shell outside body.
  float envLow = ridgedFbm(p * 2.2 + vec2(u_Time * 0.008, 0.0));
  float envShell = smoothstep(0.7, 0.95, r) * (1.0 - smoothstep(1.0, 1.3, r));
  float envMask = smoothstep(0.35, 0.75, envLow) * envShell;
  vec3 envelopeCol = filamentCol * 0.25 + vec3(0.42, 0.62, 1.0) * 0.2;
  diffuse += envelopeCol * envMask * 0.25;

  // Expanding wisps: arcs drifting outward from the pulsar, fading as they
  // travel. Radial + time; arc modulation breaks full rings into wisps.
  float wAng = atan(p.y, p.x);
  float arcMod = 0.5 + 0.5 * sin(wAng * 3.0 + 1.0);
  float wR0 = fract(u_Time * 0.07 + 0.00) * 0.65 + 0.05;
  float wR1 = fract(u_Time * 0.07 + 0.45) * 0.65 + 0.05;
  float wispSig = 0.0;
  wispSig += exp(-pow((r - wR0) * 22.0, 2.0)) * (1.0 - wR0);
  wispSig += exp(-pow((r - wR1) * 22.0, 2.0)) * (1.0 - wR1);
  diffuse += vec3(0.65, 0.78, 1.0) * wispSig * (0.25 + 0.75 * arcMod) * radialFalloff * 0.5;

  // Pulsar strobe, synchronous with the lighthouse beams (2x spin rate):
  // each flash briefly lifts the whole nebula (global lift below).
  float strobe = pow(0.5 + 0.5 * sin(u_Time * 4.0), 8.0);

  // Keep diffuse dim, no white clip.
  diffuse = min(diffuse, vec3(2.0));
  col += diffuse;
  // Strobe flash lights the mist: brief global lift, synchronous with beams.
  col += diffuse * (0.55 * strobe);

  // Tiny pulsar dot, 1-2 px. Flares with the strobe.
  float psr = exp(-r * r * 60000.0) * 3.0 * (1.0 + 2.5 * strobe);
  col += vec3(1.0) * psr;

  // Filament knots: bright approachable objects on filaments.
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
  col += knotHit * knotProf * filMask * ringGate * knotCol;

  // Pulsar lighthouse beams: two thin opposite rotating beams.
  float beamAng = u_Time * 2.0;
  float ang = atan(p.y, p.x);
  float rel = mod(ang - beamAng, 3.14159265);
  float angDiff = min(rel, 3.14159265 - rel);
  float beam = exp(-pow(angDiff * 8.0, 2.0)) * exp(-r * 3.0);
  beam *= 1.0 - smoothstep(0.2, 0.35, r);
  col += vec3(0.7, 0.85, 1.0) * beam * 0.25;

  // Vignette about 0.3 in screen space.
  float vig = 1.0 - 0.3 * clamp(dot(starUV * 0.7, starUV * 0.7), 0.0, 1.0);
  col *= vig;

  // Linear exposure gain (tonemap + gamma + grain: post_comp.frag).
  col *= 1.3 * u_Exposure;

  FragColor = vec4(max(col, vec3(0.0)), 1.0);
}
