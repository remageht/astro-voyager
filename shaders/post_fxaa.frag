#version 330 core
// FXAA 3.11 (quality, LDR). Runs after the composite and before the HUD,
// so ImGui text stays untouched. Deterministic: no frame history.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Color;
uniform vec2 u_TexelSize;  // 1 / framebuffer resolution

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
  vec2 uv = v_TexCoord;
  vec2 t = u_TexelSize;
  vec3 rgbM = texture(u_Color, uv).rgb;
  vec3 rgbNW = texture(u_Color, uv + vec2(-1.0, -1.0) * t).rgb;
  vec3 rgbNE = texture(u_Color, uv + vec2(1.0, -1.0) * t).rgb;
  vec3 rgbSW = texture(u_Color, uv + vec2(-1.0, 1.0) * t).rgb;
  vec3 rgbSE = texture(u_Color, uv + vec2(1.0, 1.0) * t).rgb;
  float lumaM = luma(rgbM);
  float lumaNW = luma(rgbNW);
  float lumaNE = luma(rgbNE);
  float lumaSW = luma(rgbSW);
  float lumaSE = luma(rgbSE);
  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
  // Flat area: keep the center pixel (still filtered by GL_LINEAR).
  if (lumaMax - lumaMin < max(0.0312, lumaMax * 0.125)) {
    FragColor = vec4(rgbM, 1.0);
    return;
  }
  // Edge direction + reduction damped by the four diagonal lumas.
  vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                  ((lumaNW + lumaSW) - (lumaNE + lumaSE)));
  float dirReduce =
      max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.03125, 0.0078125);
  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
  dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * t;
  // Two candidate blends: 3-tap (sharp) and 5-tap (soft).
  vec3 rgbA = 0.5 * (texture(u_Color, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
                     texture(u_Color, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
  vec3 rgbB = rgbA * 0.5 +
              0.25 * (texture(u_Color, uv + dir * -0.5).rgb +
                      texture(u_Color, uv + dir * 0.5).rgb);
  float lumaB = luma(rgbB);
  FragColor = vec4((lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB, 1.0);
}
