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

float hash11(float p) {
  p = fract(p * 0.1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}

// One band of radial streaks: angular hash picks sparse "star lines",
// the fract() ramp slides them outward over time (outward flow:
// constant phase needs r growing with u_Time).
float streakBand(float ang01, float r, float density, float seed) {
  float id = floor(ang01 * density + seed);
  float h = hash11(id * 1.37 + seed * 7.0);
  float rr = fract(r * 1.7 - u_Time * (0.6 + 1.8 * h) + h * 11.0);
  float pulse = smoothstep(0.0, 0.10, rr) * (1.0 - smoothstep(0.10, 0.55, rr));
  return pulse * step(0.30, h);
}

// Burst-layer twin of streakBand with explicit flow time (агент J).
// The original above is untouched: at u_Burst == 0 the bang block below
// is skipped and the frame stays pixel-identical.
float streakBandB(float ang01, float r, float density, float seed, float t) {
  float id = floor(ang01 * density + seed);
  float h = hash11(id * 1.37 + seed * 7.0);
  float rr = fract(r * 1.7 - t * (0.6 + 1.8 * h) + h * 11.0);
  float pulse = smoothstep(0.0, 0.10, rr) * (1.0 - smoothstep(0.10, 0.55, rr));
  return pulse * step(0.30, h);
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
  float bR = streakBand(ang01, rs * 0.994, 170.0, 1.0);
  float bG = streakBand(ang01, rs * 1.000, 170.0, 1.0);
  float bB = streakBand(ang01, rs * 1.006, 170.0, 1.0);
  vec3 streak = vec3(bR, bG, bB) * env;

  // Finer second layer: density grows visibly with warp intensity.
  float fine = streakBand(ang01 + 0.003, rs * 1.002, 340.0, 5.0);
  streak += vec3(0.55, 0.70, 1.0) * fine * env * k;

  // Cool rim fringe + hot core glow (tunnel feel at high k).
  float rim = smoothstep(0.45, 0.95, r);
  vec3 fringe = vec3(0.30, 0.55, 1.00) * rim * 0.30;
  float core = 1.0 - smoothstep(0.0, 0.55, r);
  vec3 glow = vec3(0.65, 0.80, 1.00) * core * core * 0.55;

  vec3 col = (streak * vec3(0.75, 0.85, 1.0) * 0.85 + fringe + glow) * k;

  // Own exposure lift (SceneManager m_exposure is never touched).
  col *= 1.0 + 0.25 * k;

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
    float chromaK = 1.0 + 3.0 * burst;
    float flowT = u_Time * (1.0 + 4.0 * burst);
    float cR = streakBandB(ang01, rs * (1.0 + (0.994 - 1.0) * chromaK), 170.0, 1.0, flowT);
    float cG = streakBandB(ang01, rs * 1.000, 170.0, 1.0, flowT);
    float cB = streakBandB(ang01, rs * (1.0 + (1.006 - 1.0) * chromaK), 170.0, 1.0, flowT);
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
