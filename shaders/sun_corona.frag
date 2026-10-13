#version 330 core
// Sun corona shell: soft outer glow + 3 breathing prominence tongues.
// Additive pass over a 1.6x sun-radius sphere (planet.vert varyings).
// Centre of the shell stays transparent so the photosphere shows through.
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Local;
out vec4 FragColor;
uniform float u_Time;
uniform vec3 u_CamPos;
uniform float u_Exposure;
uniform float u_Seed;
float angDist(float a, float b) {
  float d = mod(a - b + 3.14159265, 6.2831853) - 3.14159265;
  return abs(d);
}
vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
  vec3 n = normalize(v_Normal);
  vec3 viewDir = normalize(u_CamPos - v_WorldPos);
  float mu = abs(dot(n, viewDir));
  // Halo hugs the solar limb (shell sightline mu~0.8) and falls off outward;
  // the shell centre stays shut so the photosphere shows through.
  float core = exp(-pow((mu - 0.82) * 3.5, 2.0));
  float outer = pow(1.0 - mu, 2.0) * smoothstep(0.0, 0.25, mu) * 0.5;
  float centerGate = 1.0 - smoothstep(0.88, 0.995, mu);
  float glow = (core + outer) * centerGate;
  // Streaky coronal rays, slowly turning.
  float ang = atan(v_Local.z, v_Local.x);
  float rays = 0.85 + 0.15 * sin(ang * 9.0 + u_Time * 0.4 + sin(ang * 3.0 + u_Seed) * 1.2);
  // Prominence tongues: 3 lobes hugging the shell limb, breathing.
  float tongues = exp(-pow(angDist(ang, 0.7) * 1.8, 2.0));
  tongues += exp(-pow(angDist(ang, -1.9) * 1.8, 2.0));
  tongues += exp(-pow(angDist(ang, 2.6) * 1.8, 2.0));
  float breath = 0.6 + 0.4 * sin(u_Time * 0.7 + ang * 2.0);
  float limbBand = exp(-pow((mu - 0.80) * 4.0, 2.0));
  vec3 col = vec3(1.0, 0.55, 0.20) * glow * rays * 1.1;
  col += vec3(1.0, 0.35, 0.10) * tongues * limbBand * breath * 2.0;
  float alpha = clamp(glow * rays + tongues * limbBand * breath, 0.0, 1.0) * 0.6;
  col *= 1.1 * u_Exposure;
  col = acesNarkowicz(col);
  col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
  FragColor = vec4(col, alpha);
}
