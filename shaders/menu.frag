#version 330 core

in vec2 v_TexCoord;
out vec4 fragColor;

uniform sampler2D u_BgTex;
uniform sampler2D u_DiskVideo;
uniform sampler2D u_DiskMask;
uniform int u_HasDiskVideo;
uniform float u_Time;
uniform vec2 u_Res;
uniform float u_BgAspect;

void main() {
  float aspect = u_Res.x / max(u_Res.y, 1.0);
  vec2 screenUv = vec2(v_TexCoord.x, 1.0 - v_TexCoord.y);
  vec2 coverScale = vec2(min(aspect / u_BgAspect, 1.0),
                         min(u_BgAspect / aspect, 1.0));
  vec2 imageUv = (screenUv - 0.5) * coverScale + 0.5;
  vec3 color = texture(u_BgTex, imageUv).rgb;
  float diskMask = texture(u_DiskMask, imageUv).r;

  if (diskMask > 0.0) {
    if (u_HasDiskVideo != 0) {
      color = mix(color, texture(u_DiskVideo, imageUv).rgb, diskMask);
    } else {
      vec2 delta = (imageUv - vec2(0.505, 0.505)) * vec2(u_BgAspect, 1.0);
      float radius = length(delta);
      float angle = atan(delta.y, delta.x);
      float timePhase = mod(u_Time, 24.0) * (6.28318530718 / 24.0);
      float flowPhase = angle * 7.0 + radius * 54.0 - timePhase * 5.0;
      float plasma = pow(0.5 + 0.5 * sin(flowPhase), 2.2);
      color *= 1.0 + (plasma - 0.35) * 0.22 * diskMask;
      color += vec3(1.0, 0.52, 0.16) * (0.075 * plasma * diskMask);
    }
  }

  vec2 edge = v_TexCoord * 2.0 - 1.0;
  fragColor = vec4(color * (1.0 - 0.18 * dot(edge, edge)), 1.0);
}
