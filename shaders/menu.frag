#version 330 core

in vec2 v_TexCoord;
out vec4 fragColor;

uniform sampler2D u_BgTex;
uniform sampler2D u_DiskVideo;
uniform sampler2D u_DiskMask;
uniform sampler2D u_MotionMask;
uniform int u_HasDiskVideo;
uniform int u_AnimateDisk;
uniform int u_ScenicMode;
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

  if (u_AnimateDisk != 0 && diskMask > 0.0) {
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

  if (u_ScenicMode > 0) {
    float motionMask = texture(u_MotionMask, imageUv).r;
    if (motionMask > 0.0) {
      vec2 p = imageUv * vec2(u_BgAspect, 1.0);
      float t = mod(u_Time, 24.0) * (6.28318530718 / 24.0);
      float phase;
      if (u_ScenicMode == 2) {
        phase = p.y * 10.0 - t * 2.0;
      } else if (u_ScenicMode == 3) {
        vec2 d = p - vec2(0.687 * u_BgAspect, 0.459);
        phase = atan(d.y, d.x) * 3.0 + length(d) * 14.0 - t * 2.0;
      } else if (u_ScenicMode == 4) {
        vec2 d = p - vec2(0.631 * u_BgAspect, 0.352);
        phase = atan(d.y, d.x) * 4.0 + length(d) * 10.0 - t * 2.0;
      } else {
        phase = p.x * 8.0 + p.y * 5.0 - t * 2.0;
      }
      float flow = 0.5 + 0.325 * sin(phase) + 0.175 * sin(phase * 0.5 + 1.3);
      color *= 1.0 + (flow - 0.5) * 0.09 * motionMask;
    }
  }

  vec2 edge = v_TexCoord * 2.0 - 1.0;
  fragColor = vec4(color * (1.0 - 0.18 * dot(edge, edge)), 1.0);
}
