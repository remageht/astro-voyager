#version 330 core
// M31 Andromeda Galaxy impostor sprite (GLSL 330)
// Realistic tilted spiral galaxy with bright bulge, spiral arms, dust lanes, and halo.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform float u_Time;
uniform float u_Aspect;

void main() {
  vec2 uv = v_TexCoord * 2.0 - 1.0;
  uv.x *= u_Aspect;

  // Rotate slightly and tilt for Andromeda's 77 degree inclination
  float angle = 0.55 + 0.005 * sin(u_Time * 0.2);
  float s = sin(angle), c = cos(angle);
  vec2 rot = vec2(c * uv.x - s * uv.y, s * uv.x + c * uv.y);
  
  // Compress along minor axis for tilted disc
  vec2 p = vec2(rot.x * 1.0, rot.y * 2.4);
  float r = length(p);

  // Core bulge (bright yellowish-white)
  float core = exp(-r * 6.0) * 2.5;

  // Spiral arms
  float phi = atan(p.y, p.x);
  float spiral = sin(phi * 2.0 - log(max(r, 0.01)) * 3.5);
  float armMask = smoothstep(0.1, 0.9, spiral) * exp(-r * 1.8);

  // Dust lanes
  float dust = sin(phi * 2.0 - log(max(r, 0.01)) * 3.5 + 0.6);
  float dustLane = smoothstep(0.3, 0.8, dust) * exp(-r * 2.0) * 0.4;

  // Disk glow
  float disk = exp(-r * 2.2) * 0.9;

  // Outer halo
  float halo = exp(-r * 1.2) * 0.2;

  // Colors
  vec3 coreColor = vec3(1.0, 0.92, 0.78);   // Warm stellar population II
  vec3 armColor = vec3(0.65, 0.82, 1.0);    // Young blue stars & H II regions
  vec3 dustColor = vec3(0.12, 0.08, 0.06);   // Dark dust absorption

  vec3 col = core * coreColor + (disk + armMask * 0.8) * armColor + halo * vec3(0.5, 0.6, 0.8);
  col = mix(col, dustColor, dustLane * smoothstep(0.15, 0.6, r));

  FragColor = vec4(col, 1.0);
}
