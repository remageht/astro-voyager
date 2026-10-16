#version 330 core
// Saturn-like ring: radial stripes + deep Cassini gap, faint radial spokes
// (azimuth from u_Center, rotating with u_Time), subtle shimmer.
// Normal blending. Screenshots stay deterministic at fixed u_Time.
in float v_R01;
in vec3 v_WorldPos;
out vec4 FragColor;
uniform float u_Seed;
uniform vec3 u_ColA;
uniform float u_Time;
uniform vec3 u_Center;
// v3.2.0: planet radius (world units), 0 = shadow off. The sun sits at the
// world origin: ring fragments whose sightline to the sun passes through
// the planet sphere (centre u_Center) are eclipsed.
uniform float u_PlanetR;
float hash12(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}
float vnoise2(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = hash12(i);
  float b = hash12(i + vec2(1.0, 0.0));
  float c = hash12(i + vec2(0.0, 1.0));
  float d = hash12(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float fbm3(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  float s = 0.0;
  for (int i = 0; i < 3; ++i) {
    v += a * vnoise2(p);
    s += a;
    p *= 2.03;
    a *= 0.5;
  }
  return v / max(s, 0.0001);
}
void main() {
  float stripes = fbm3(vec2(v_R01 * 28.0, u_Seed));
  float fine = vnoise2(vec2(v_R01 * 90.0, u_Seed * 1.7));
  float dens = stripes * (0.75 + 0.25 * fine);
  float edge = (1.0 - smoothstep(0.0, 0.06, v_R01)) * (1.0 - smoothstep(0.94, 1.0, v_R01));
  // Cassini Division: deep narrow evacuated gap at 0.65.
  float cass = 1.0 - smoothstep(0.012, 0.035, abs(v_R01 - 0.65));
  // Encke hairline at 0.875.
  float encke = 1.0 - smoothstep(0.004, 0.012, abs(v_R01 - 0.875));
  float alpha = dens * edge * 0.9;
  alpha *= 1.0 - 0.96 * cass;
  alpha *= 1.0 - 0.70 * encke;
  // Faint radial spokes: 5-fold pattern in the mid ring, slowly revolving.
  vec3 rel = v_WorldPos - u_Center;
  float ang = atan(rel.z, rel.x);
  float spokePat = 0.5 + 0.5 * sin(ang * 5.0 + u_Time * 0.15 + sin(ang * 2.0 + u_Seed) * 0.7);
  float spokeMask = smoothstep(0.25, 0.45, v_R01) * (1.0 - smoothstep(0.55, 0.75, v_R01));
  float spokes = smoothstep(0.75, 0.95, spokePat) * spokeMask * 0.25;
  // Subtle icy shimmer.
  float shim = 0.95 + 0.05 * sin(u_Time * 2.0 + v_R01 * 25.0);
  vec3 col = u_ColA * (0.5 + 0.5 * dens) * shim;
  col += vec3(0.9, 0.9, 0.95) * spokes;
  // Evacuated gaps are darker as well as transparent.
  col *= 1.0 - 0.6 * cass;
  col *= 1.0 - 0.35 * encke;
  alpha = clamp(alpha + spokes * 0.3, 0.0, 1.0);
  // v3.2.0 planet shadow: the sun is at the world origin, so the planet
  // sphere at u_Center occludes the ring on the far side: dim + thin out.
  if (u_PlanetR > 0.0) {
    vec3 sd = normalize(-v_WorldPos);
    vec3 L = u_Center - v_WorldPos;
    float tP = dot(L, sd);
    if (tP > 0.0) {
      vec3 closest = v_WorldPos + tP * sd;
      float dP = length(closest - u_Center);
      float sh = 1.0 - smoothstep(u_PlanetR * 0.90, u_PlanetR * 1.08, dP);
      col *= 1.0 - 0.80 * sh;
      alpha *= 1.0 - 0.45 * sh;
    }
  }
  // v3.0.0: ring albedo art is display-referred; decode to linear HDR.
  FragColor = vec4(pow(max(col, vec3(0.0)), vec3(2.2)), alpha);
}
