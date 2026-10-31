#version 330 core
// Diegetic 3D Cockpit & Collimated HUD (v4.2.0 Alpha)
// Renders structural canopy struts, carbon-titanium dashboard, glass Fresnel & scratches,
// photochromic stellar auto-tint, and infinite-focus holographic HUD.

in vec2 v_UV;
out vec4 FragColor;

uniform vec2  u_Resolution;
uniform float u_Time;
uniform vec3  u_HeadOffset;
uniform vec3  u_HeadTilt;
uniform float u_Rumble;
uniform float u_Pitch;
uniform float u_Roll;
uniform float u_Yaw;
uniform float u_Speed;
uniform float u_Throttle;
uniform float u_WarpActive;
uniform float u_WarpCharge;
uniform float u_Hull;
uniform float u_Shield;
uniform float u_AutoTint;
uniform float u_TargetAngle;
uniform float u_TargetLocked;
uniform int   u_ShipStyle;

// Hash functions for procedural carbon fiber and glass scratches
float hash21(vec2 p) {
  p = fract(p * vec2(234.34, 435.345));
  p += dot(p, p + 34.23);
  return fract(p.x * p.y);
}

float noise2D(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = hash21(i + vec2(0.0, 0.0));
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// 2D Rotation
vec2 rotate2D(vec2 v, float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

// SDF Line segment
float sdSegment(vec2 p, vec2 a, vec2 b) {
  vec2 pa = p - a, ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

// SDF Box
float sdBox(vec2 p, vec2 b) {
  vec2 d = abs(p) - b;
  return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

void main() {
  vec2 uv = v_UV;
  float aspect = u_Resolution.x / max(u_Resolution.y, 1.0);
  vec2 p = (uv - 0.5) * 2.0;
  p.x *= aspect;

  // Pilot head vibration & tilt
  vec2 rumbleJitter = vec2(sin(u_Time * 55.0), cos(u_Time * 47.0)) * u_Rumble * 0.005;
  vec2 headP = p - u_HeadOffset.xy * 2.2 + rumbleJitter;
  headP = rotate2D(headP, u_HeadTilt.z * 0.85);

  // Ship style color scheme:
  // 0: Dart = Electric Cyan
  // 1: Ring Ship = Golden Amber / Solar
  // 2: Shuttle = Aerospace Emerald Green
  // 3: Cruiser = Arctic Blue
  vec3 hudCol = vec3(0.18, 0.82, 1.0);
  if (u_ShipStyle == 1) hudCol = vec3(1.0, 0.72, 0.22);
  else if (u_ShipStyle == 2) hudCol = vec3(0.25, 0.95, 0.45);
  else if (u_ShipStyle == 3) hudCol = vec3(0.40, 0.75, 1.0);

  // -------------------------------------------------------------
  // 1. CANOPY STRUCTURE (Struts & Lower Dashboard Console)
  // -------------------------------------------------------------
  // Lower dashboard edge
  float dashCurve = -0.66 + 0.14 * (headP.x * headP.x * 0.5);
  bool isDashboard = (headP.y < dashCurve);

  // Left & Right A-Pillars (canopy frame arches)
  float absX = abs(headP.x);
  // Strut line from (0.58, 1.1) down to (1.35, -0.7)
  float strutSlope = 1.8 / 0.77;
  float strutX = 0.58 + (1.1 - headP.y) / strutSlope;
  float strutDist = abs(absX - strutX);
  bool isStrut = (headP.y > dashCurve) && (strutDist < 0.075);

  // Overhead upper spar
  bool isOverhead = (headP.y > 0.86);

  bool isCanopySolid = isDashboard || isStrut || isOverhead;

  vec4 col = vec4(0.0);

  if (isCanopySolid) {
    // Carbon-composite / titanium cockpit interior material
    vec3 frameBase = vec3(0.055, 0.058, 0.065);
    
    // Carbon weave micro-pattern
    vec2 weaveUV = headP * 120.0;
    float weave = sin(weaveUV.x) * cos(weaveUV.y);
    frameBase += vec3(weave * 0.012);

    // Subtle edge bevel highlight
    float edgeProx = 1.0;
    if (isDashboard) {
      edgeProx = min(edgeProx, abs(headP.y - dashCurve));
    }
    if (isStrut) {
      edgeProx = min(edgeProx, 0.075 - strutDist);
    }
    if (isOverhead) {
      edgeProx = min(edgeProx, headP.y - 0.86);
    }
    float bevelHighlight = (1.0 - smoothstep(0.0, 0.02, edgeProx)) * 0.18;
    frameBase += vec3(bevelHighlight * 1.2);

    // Dashboard instrument panel glows
    if (isDashboard) {
      // Left MFD: Shield & Hull status bar
      vec2 leftMfdP = headP - vec2(-0.62 * aspect, -0.76);
      if (abs(leftMfdP.x) < 0.28 && abs(leftMfdP.y) < 0.08) {
        frameBase = vec3(0.02, 0.03, 0.04);
        // Hull bar (horizontal)
        float hBar = step(-0.03, leftMfdP.y) * step(leftMfdP.y, -0.01) *
                     step(-0.24, leftMfdP.x) * step(leftMfdP.x, -0.24 + 0.48 * u_Hull);
        // Shield bar (horizontal)
        float sBar = step(0.01, leftMfdP.y) * step(leftMfdP.y, 0.03) *
                     step(-0.24, leftMfdP.x) * step(leftMfdP.x, -0.24 + 0.48 * u_Shield);
        vec3 barCol = mix(vec3(1.0, 0.2, 0.1), vec3(0.2, 0.9, 0.4), u_Hull) * hBar +
                      vec3(0.2, 0.7, 1.0) * sBar;
        frameBase += barCol * 1.5;
      }

      // Right MFD: Warp & Velocity status
      vec2 rightMfdP = headP - vec2(0.62 * aspect, -0.76);
      if (abs(rightMfdP.x) < 0.28 && abs(rightMfdP.y) < 0.08) {
        frameBase = vec3(0.02, 0.03, 0.04);
        float wBar = step(-0.01, rightMfdP.y) * step(rightMfdP.y, 0.02) *
                     step(-0.24, rightMfdP.x) * step(rightMfdP.x, -0.24 + 0.48 * u_WarpCharge);
        vec3 wCol = (u_WarpActive > 0.5) ? vec3(0.3, 0.9, 1.0) : vec3(0.8, 0.5, 0.1);
        frameBase += wCol * wBar * (1.2 + 0.3 * sin(u_Time * 8.0));
      }

      // Warning stencils / indicator LEDs on central cowling
      vec2 centerCowlingP = headP - vec2(0.0, -0.67);
      float ledSpacing = 0.04;
      for (int i = -3; i <= 3; ++i) {
        float dLed = length(centerCowlingP - vec2(float(i) * ledSpacing, 0.0));
        float ledGlow = 1.0 - smoothstep(0.003, 0.010, dLed);
        vec3 ledC = (i == 0 && u_WarpActive > 0.5) ? vec3(0.2, 1.0, 0.9) :
                    (abs(float(i)) > 2) ? vec3(0.1, 0.9, 0.3) : vec3(0.08, 0.5, 0.9);
        frameBase += ledC * ledGlow * 1.2;
      }
    }

    col = vec4(frameBase, 1.0);
  } else {
    // -------------------------------------------------------------
    // 2. CANOPY GLASS (Fresnel, Micro-scratches & Auto-Tint)
    // -------------------------------------------------------------
    // Subtle glass curvature vignette and Fresnel edge reflection
    float r = length(headP * vec2(0.8, 1.1));
    float glassFresnel = pow(clamp(r * 0.75, 0.0, 1.0), 4.0) * 0.22;

    // Micro-scratches on glass caught by light
    float scratchNoise = noise2D(headP * 180.0);
    float scratch = pow(scratchNoise, 12.0) * 0.07;

    // Photochromic stellar auto-tint: darkening against blinding light
    vec3 tintColor = vec3(0.08, 0.14, 0.22) * u_AutoTint * 0.65;
    float glassAlpha = clamp(glassFresnel + scratch + u_AutoTint * 0.40, 0.0, 0.85);

    vec3 glassRgb = vec3(0.05, 0.12, 0.20) * glassFresnel + vec3(scratch) + tintColor;
    col = vec4(glassRgb, glassAlpha);

    // -------------------------------------------------------------
    // 3. COLLIMATED HOLOGRAPHIC HUD (Infinite focus projection)
    // -------------------------------------------------------------
    // Collimated HUD moves with slight parallax but stays centered
    vec2 hudP = p - u_HeadOffset.xy * 0.5;

    float hudGlow = 0.0;

    // A. Boresight Reticle (Center crosshair)
    float reticleDist = length(hudP);
    // Outer circle
    hudGlow += 1.0 - smoothstep(0.001, 0.004, abs(reticleDist - 0.060));
    // Center dot
    hudGlow += (1.0 - smoothstep(0.002, 0.005, reticleDist)) * 1.5;
    // Crosshair ticks (top, bottom, left, right)
    hudGlow += 1.0 - smoothstep(0.001, 0.003, sdSegment(hudP, vec2(-0.085, 0.0), vec2(-0.065, 0.0)));
    hudGlow += 1.0 - smoothstep(0.001, 0.003, sdSegment(hudP, vec2( 0.065, 0.0), vec2( 0.085, 0.0)));
    hudGlow += 1.0 - smoothstep(0.001, 0.003, sdSegment(hudP, vec2(0.0, -0.085), vec2(0.0, -0.065)));
    hudGlow += 1.0 - smoothstep(0.001, 0.003, sdSegment(hudP, vec2(0.0,  0.065), vec2(0.0,  0.085)));

    // B. Pitch Ladder & Horizon
    vec2 pitchP = rotate2D(hudP, -radians(u_Roll));
    float pitchOffset = radians(u_Pitch) * 0.45;
    vec2 ladderP = pitchP + vec2(0.0, pitchOffset);

    // Horizon line (dashed center line with gap for reticle)
    float horizD1 = sdSegment(ladderP, vec2(-0.35, 0.0), vec2(-0.09, 0.0));
    float horizD2 = sdSegment(ladderP, vec2( 0.09, 0.0), vec2( 0.35, 0.0));
    hudGlow += (1.0 - smoothstep(0.001, 0.003, min(horizD1, horizD2))) * 0.85;

    // Pitch rungs (-20, -10, +10, +20 degrees)
    float rungSpacing = radians(10.0) * 0.45;
    for (int k = -2; k <= 2; ++k) {
      if (k == 0) continue;
      float rungY = float(k) * rungSpacing;
      vec2 rungPos = ladderP - vec2(0.0, rungY);
      float rungL = (abs(float(k)) == 1) ? 0.14 : 0.20;
      float dL = sdSegment(rungPos, vec2(-rungL, 0.0), vec2(-0.07, 0.0));
      float dR = sdSegment(rungPos, vec2( 0.07, 0.0), vec2( rungL, 0.0));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, min(dL, dR))) * 0.65;
    }

    // C. Throttle Bar (Left tape: x ~ -0.42)
    vec2 throtP = hudP - vec2(-0.42, 0.0);
    if (abs(throtP.x) < 0.025 && abs(throtP.y) < 0.25) {
      float tapeOutline = sdBox(throtP, vec2(0.015, 0.22));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, abs(tapeOutline))) * 0.7;
      float throtFill = -0.22 + 0.44 * clamp(u_Throttle, 0.0, 1.2);
      if (throtP.y < throtFill) {
        hudGlow += 0.55;
      }
    }

    // D. Velocity Tape (Right tape: x ~ +0.42)
    vec2 velP = hudP - vec2(0.42, 0.0);
    if (abs(velP.x) < 0.025 && abs(velP.y) < 0.25) {
      float velOutline = sdBox(velP, vec2(0.015, 0.22));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, abs(velOutline))) * 0.7;
      float speedNorm = clamp(u_Speed / 120.0, 0.0, 1.0);
      float velFill = -0.22 + 0.44 * speedNorm;
      if (velP.y < velFill) {
        hudGlow += 0.55;
      }
    }

    // E. Target Box / Lock Brackets
    if (u_TargetAngle < 40.0) {
      float tgtRadius = 0.12;
      float tgtBox = sdBox(hudP, vec2(tgtRadius));
      float tgtEdge = abs(tgtBox);
      // Segmented corner brackets
      float cornerMask = step(0.07, abs(hudP.x)) * step(0.07, abs(hudP.y));
      float lockPulse = (u_TargetLocked > 0.5) ? (1.0 + 0.35 * sin(u_Time * 12.0)) : 0.75;
      hudGlow += (1.0 - smoothstep(0.001, 0.003, tgtEdge)) * cornerMask * lockPulse;
    }

    // F. Warp Drive Holographic Banner
    if (u_WarpActive > 0.5) {
      vec2 warpBadgeP = hudP - vec2(0.0, 0.32);
      float badgeBox = sdBox(warpBadgeP, vec2(0.18, 0.035));
      hudGlow += (1.0 - smoothstep(0.001, 0.003, abs(badgeBox))) * (0.8 + 0.4 * sin(u_Time * 6.0));
    }

    // Additive HUD projection on the glass
    if (hudGlow > 0.001) {
      col.rgb += hudCol * hudGlow * 1.35;
      col.a = max(col.a, clamp(hudGlow, 0.0, 0.95));
    }
  }

  FragColor = col;
}
