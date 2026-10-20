#version 330 core
// Warp-drive tunnel overlay (agent W, SHIP-ONLY file).
// Transparent fullscreen layer drawn by SceneManager over every station:
// radial streak lines + cool edge fringe + tunnel glow + entry/exit flash.
// Blended with (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) by the host. The host
// SKIPS the draw call entirely at u_WarpK == 0 and u_Flash == 0, so the
// screenshot harness stays byte-identical (no GL state is touched).
// NOTE: all smoothstep() edges are ascending (edge0 < edge1);
// reversed edges are undefined behaviour and break the static gate.
in vec2 v_TexCoord;
out vec4 FragColor;

uniform float u_WarpK;   // 0..1 warp intensity (0 = neutral)
uniform float u_Flash;   // 0..1 white flash envelope (0 = neutral)
uniform float u_Burst;   // 0..1 shockwave envelope, агент J (0 = neutral)
uniform float u_Aspect;  // viewport aspect (width / height)
uniform float u_Time;    // wall seconds (streak flow only)
// v3.3.0: streak stability pass (long tails, capped burst flow, softened rim).

float hash11(float p) {
  p = fract(p * 0.1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}

// One band of radial streaks: angular hash picks sparse "star lines",
// the fract() ramp slides them outward over time (outward flow:
// constant phase needs r growing with u_Time).
// v3.3.0: long comet tail (was a short dash) => no temporal strobing.
float streakBand(float ang01, float r, float density, float seed) {
  float id = floor(ang01 * density + seed);
  float h = hash11(id * 1.37 + seed * 7.0);
  float rr = fract(r * 1.7 - u_Time * (0.6 + 1.8 * h) + h * 11.0);
  float pulse = smoothstep(0.0, 0.06, rr) * (1.0 - smoothstep(0.06, 0.85, rr));
  return pulse * step(0.30, h);
}

// Burst-layer twin of streakBand with explicit flow time (агент J).
// The original above is untouched: at u_Burst == 0 the bang block below
// is skipped and the frame stays pixel-identical.
float streakBandB(float ang01, float r, float density, float seed, float t) {
  float id = floor(ang01 * density + seed);
  float h = hash11(id * 1.37 + seed * 7.0);
  float rr = fract(r * 1.7 - t * (0.6 + 1.8 * h) + h * 11.0);
  float pulse = smoothstep(0.0, 0.06, rr) * (1.0 - smoothstep(0.06, 0.85, rr));
  return pulse * step(0.30, h);
}

// v4.0 (reference pass): value-noise fbm for the tunnel walls — soft
// chrysalis curtains wrapping the flight line, plus the red entry
// nebula ring. All terms are multiplied by k downstream, so neutral
// frames (warpK == 0) stay byte-identical.
float hash12(vec2 q) {
  vec3 p3 = fract(vec3(q.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}
float vnoise(vec2 q) {
  vec2 i = floor(q);
  vec2 f = fract(q);
  vec2 u = f * f * (3.0 - 2.0 * f);  // smooth interp (ascending edges ok)
  float a = hash12(i);
  float b = hash12(i + vec2(1.0, 0.0));
  float c = hash12(i + vec2(0.0, 1.0));
  float d = hash12(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float fbm(vec2 q) {
  float s = 0.0;
  float amp = 0.55;
  for (int i = 0; i < 4; ++i) {
    s += amp * vnoise(q);
    q = q * 2.13 + vec2(17.3, 9.1);
    amp *= 0.5;
  }
  return s;
}

void main() {
  vec2 p = vec2((v_TexCoord.x - 0.5) * u_Aspect, v_TexCoord.y - 0.5);
  float r = length(p);
  float rs = max(r, 0.0001);
  float ang01 = atan(p.y, p.x) * 0.15915494 + 0.5;
  float k = clamp(u_WarpK, 0.0, 1.0);
  float flash = clamp(u_Flash, 0.0, 1.0);

  // Radial envelope: streaks fade in away from the centre, full at rim.
  float env = smoothstep(0.02, 0.38, r);

  // Per-channel radial scales fake chromatic aberration at the rim
  // (no scene texture here, so the fringe is synthesized on the lines).
  float bR = streakBand(ang01, rs * 0.9965, 170.0, 1.0);
  float bG = streakBand(ang01, rs * 1.000, 170.0, 1.0);
  float bB = streakBand(ang01, rs * 1.0035, 170.0, 1.0);
  vec3 streak = vec3(bR, bG, bB) * env;
  streak *= 1.0 - 0.30 * smoothstep(0.55, 1.0, r);  // v3.3.0: soften the rim

  // Finer second layer: density grows visibly with warp intensity.
  float fine = streakBand(ang01 + 0.003, rs * 1.002, 220.0, 5.0);
  streak += vec3(0.45, 0.70, 1.0) * fine * env * k * 0.65;

  // Cool rim fringe + hot core glow (tunnel feel at high k).
  float rim = smoothstep(0.45, 0.95, r);
  vec3 fringe = vec3(0.30, 0.55, 1.00) * rim * 0.22;
  float core = 1.0 - smoothstep(0.0, 0.55, r);
  vec3 glow = vec3(0.65, 0.80, 1.00) * core * core * 0.28;

  // v4.0 tunnel walls: two fbm curtains with a slow angular twirl; the
  // wall annulus sits between the starburst and the outer rim. The slow
  // radial slide (-0.05*t) reads as "flying through folded space".
  float wallR = smoothstep(0.10, 0.30, r) * (1.0 - smoothstep(0.66, 1.05, r));
  float tw = ang01 + 0.10 / rs;  // vortex swirl (stronger near the core)
  vec2 wq1 = vec2(tw * 5.0, r * 3.2 - u_Time * 0.05);
  vec2 wq2 = vec2((ang01 - 0.04 / rs) * 7.0, r * 5.1 - u_Time * 0.08);
  float f1 = fbm(wq1);
  float f2 = fbm(wq2);
  float fil1 = smoothstep(0.42, 0.80, f1);   // ascending edges (gate rule)
  float fil2 = smoothstep(0.55, 0.95, f2);
  vec3 walls = (vec3(0.14, 0.36, 0.88) * fil1 * 0.40 +
                vec3(0.22, 0.52, 1.00) * fil2 * 0.24) * wallR;

  // Red entry nebula: warm dust annulus at the outermost band.
  float redBand = smoothstep(0.72, 0.88, r) * (1.0 - smoothstep(0.95, 1.12, r));
  float redN = fbm(vec2(ang01 * 9.0, r * 4.0 - u_Time * 0.03));
  vec3 redDust = vec3(0.85, 0.18, 0.12) * smoothstep(0.50, 0.90, redN) * redBand * 0.32;

  // White-hot exit core: compact shimmering mouth at the flight centre.
  float mouth = exp(-r * r * 160.0);
  vec3 hotCore = vec3(1.0, 0.985, 0.94) * mouth * (0.55 + 0.15 * sin(u_Time * 7.3));

  // Prismatic rim accent: faint rainbow ring on the wall silhouette edge.
  float prEdge = exp(-pow((r - 0.68) * 9.0, 2.0));
  vec3 prism = vec3(sin(6.28318 * (ang01 + u_Time * 0.05)),
                    sin(6.28318 * (ang01 + 0.33)),
                    sin(6.28318 * (ang01 - 0.33))) * 0.5 + 0.5;
  prism *= prEdge * 0.10;

  // v3.6.0: общая интенсивность оверлея снижена (комфорт для глаз).
  // v4.0: добавлены стены туннеля, красный вход, white-hot ядро и
  // призматический ободок — всё внутри одного множителя k.
  vec3 col = (streak * vec3(0.75, 0.85, 1.0) * 0.62 + fringe + glow +
              walls + redDust + hotCore + prism) * k;

  // Own exposure lift (SceneManager m_exposure is never touched).
  col *= 1.0 + 0.12 * k;

  // WarpJump bang (агент J): shockwave ring + burst chromaticism + flash peak.
  // Gated by u_Burst > 0 (default 0 = this block is skipped, frame below
  // stays pixel-identical to HEAD). All smoothstep edges ascending.
  float burst = clamp(u_Burst, 0.0, 1.0);
  if (burst > 0.0005) {
    // Shockwave ring: R rides 0 -> 1.2 over the Burst; the (1-burst)
    // envelope lets the ring fly past the camera and fade out.
    float ringR = 1.2 * burst;
    float ringG = (r - ringR) * 6.0;
    float ring = exp(-ringG * ringG) * (1.0 - burst);
    col += vec3(0.70, 0.85, 1.00) * ring * 2.0;
    // Chromatic aberration x(1+3B): radial channel scales spread apart;
    // streak drift x(1+4B) via explicit flow time.
    // v3.3.0: capped flow/chroma spread => lines stay coherent at max speed.
    float chromaK = 1.0 + 1.5 * burst;
    float flowT = u_Time * (1.0 + 1.8 * burst);
    float cR = streakBandB(ang01, rs * (1.0 + (0.9965 - 1.0) * chromaK), 170.0, 1.0, flowT);
    float cG = streakBandB(ang01, rs * 1.000, 170.0, 1.0, flowT);
    float cB = streakBandB(ang01, rs * (1.0 + (1.0035 - 1.0) * chromaK), 170.0, 1.0, flowT);
    col += vec3(cR, cG, cB) * env * burst;
    // Flash peak 2.2*B*(1-r): hot centre over the regular u_Flash below.
    col += vec3(1.0, 0.97, 0.92) * (2.2 * burst * (1.0 - smoothstep(0.0, 1.0, r)));
  }

  // Entry / exit / arrival flash (warm white).
  col += vec3(1.0, 0.97, 0.92) * flash;

  float alpha = clamp(dot(col, vec3(0.3333)) * 1.4 + flash, 0.0, 1.0);
  // v3.0.0: tunnel art is display-referred; decode to linear HDR.
  FragColor = vec4(pow(max(col, vec3(0.0)), vec3(2.2)), alpha);
}
