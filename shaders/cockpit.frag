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

// v4.6.0 Realism Overhaul uniforms
uniform vec2  u_Prograde;
uniform float u_ProgradeOn;
uniform vec2  u_Retrograde;
uniform float u_RetrogradeOn;
uniform float u_GForce;
uniform float u_Blackout;
uniform float u_Redout;
uniform float u_RadarAlt;
uniform float u_VerticalSpeed;
uniform float u_TerrainAlert;
uniform float u_EntryPlasma;

// v4.6.1 Planetary Landing uniforms
uniform float u_GearState;
uniform float u_GearProgress;
uniform float u_IsLanded;
uniform float u_SurfaceDustK;

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
    // Cockpit interior material: White aerospace composite for Egor's Ring Ship (style 1),
    // dark carbon-composite for Dart (style 0) / other ships
    vec3 frameBase;
    float bevelHighlight = 0.0;

    // Edge proximity for bevels
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
    bevelHighlight = (1.0 - smoothstep(0.0, 0.02, edgeProx)) * 0.18;

    if (u_ShipStyle == 1) {
      // Egor's White Ship (Astro-Voyager / IXS Alcubierre): matte white aerospace ceramic
      frameBase = vec3(0.82, 0.84, 0.86);
      vec2 tileUV = headP * 35.0;
      float tileSeam = step(0.965, fract(tileUV.x)) + step(0.965, fract(tileUV.y));
      frameBase = mix(frameBase, vec3(0.20, 0.22, 0.24), clamp(tileSeam, 0.0, 1.0));
      frameBase -= vec3(bevelHighlight * 0.45); // Dark bevel lines on white panels
    } else {
      // Carbon-composite / titanium dark cockpit interior material
      frameBase = vec3(0.055, 0.058, 0.065);
      vec2 weaveUV = headP * 120.0;
      float weave = sin(weaveUV.x) * cos(weaveUV.y);
      frameBase += vec3(weave * 0.012);
      frameBase += vec3(bevelHighlight * 1.2);
    }

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

    // Atmospheric hypersonic entry plasma ionization glow (v4.6.0)
    if (u_EntryPlasma > 0.005) {
      vec2 flowP = headP * vec2(1.5, 3.5) + vec2(0.0, u_Time * 14.0);
      float plasmaNoise = noise2D(flowP) * 0.6 + noise2D(flowP * 2.2 + vec2(u_Time * 8.0, 0.0)) * 0.4;
      float streak = pow(plasmaNoise, 3.0) * 1.8;
      float shockMask = smoothstep(-0.6, 0.4, headP.y);
      vec3 plasmaCol = mix(vec3(1.0, 0.38, 0.06), vec3(1.0, 0.88, 0.55), streak);
      float plasmaAlpha = clamp((streak * 0.7 + shockMask * 0.3) * u_EntryPlasma, 0.0, 0.85);
      glassRgb += plasmaCol * plasmaAlpha * 2.2;
      glassAlpha = max(glassAlpha, plasmaAlpha * 0.8);
    }

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

    // G. Tactical Compass Heading Tape (v4.3.0)
    vec2 compassP = hudP - vec2(0.0, 0.38);
    if (abs(compassP.x) < 0.32 && abs(compassP.y) < 0.035) {
      // Horizontal compass baseline
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, sdSegment(compassP, vec2(-0.30, 0.0), vec2(0.30, 0.0)))) * 0.7;
      // Heading lubber line (center indicator triangle)
      hudGlow += (1.0 - smoothstep(0.001, 0.003, sdSegment(compassP, vec2(0.0, 0.0), vec2(0.0, -0.018)))) * 1.2;
      // Sliding heading ticks based on u_Yaw
      float tickSpacing = 0.05;
      for (int t = -6; t <= 6; ++t) {
        float tickX = float(t) * tickSpacing - (fract(u_Yaw / 30.0) - 0.5) * tickSpacing;
        if (abs(tickX) < 0.28) {
          float tickH = (mod(float(t), 3.0) == 0.0) ? 0.014 : 0.007;
          hudGlow += (1.0 - smoothstep(0.001, 0.002, sdSegment(compassP - vec2(tickX, 0.0), vec2(0.0, 0.0), vec2(0.0, tickH)))) * 0.6;
        }
      }
    }

    // H. Flight Path Vector: Prograde Marker (v4.6.0)
    if (u_ProgradeOn > 0.5) {
      vec2 progP = hudP - u_Prograde;
      float dCirc = abs(length(progP) - 0.026);
      hudGlow += (1.0 - smoothstep(0.001, 0.0028, dCirc)) * 1.25;
      // 3 Wings at 12, 9, 3 o'clock
      float topWing = sdSegment(progP, vec2(0.0, 0.026), vec2(0.0, 0.044));
      float leftWing = sdSegment(progP, vec2(-0.044, 0.0), vec2(-0.026, 0.0));
      float rightWing = sdSegment(progP, vec2(0.026, 0.0), vec2(0.044, 0.0));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, min(topWing, min(leftWing, rightWing)))) * 1.1;
    }

    // I. Flight Path Vector: Retrograde Marker (v4.6.0)
    if (u_RetrogradeOn > 0.5) {
      vec2 retroP = hudP - u_Retrograde;
      float dCirc = abs(length(retroP) - 0.026);
      hudGlow += (1.0 - smoothstep(0.001, 0.0028, dCirc)) * 1.1;
      // Internal X cross
      float dCross1 = sdSegment(retroP, vec2(-0.015, -0.015), vec2(0.015, 0.015));
      float dCross2 = sdSegment(retroP, vec2(-0.015, 0.015), vec2(0.015, -0.015));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, min(dCross1, dCross2))) * 1.0;
    }

    // J. Digital G-Meter & High-G warning (v4.6.0)
    vec2 gP = hudP - vec2(0.48, -0.12);
    if (abs(gP.x) < 0.025 && abs(gP.y) < 0.12) {
      float gOutline = sdBox(gP, vec2(0.012, 0.10));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, abs(gOutline))) * 0.6;
      float gNorm = clamp((u_GForce - 1.0) / 6.0, 0.0, 1.0);
      float gFill = -0.10 + 0.20 * gNorm;
      if (gP.y < gFill) {
        hudGlow += (u_GForce > 4.0) ? 0.95 : 0.55;
      }
    }

    // K. Radar Altimeter Tape (ALT AGL) & Vertical Speed (v4.6.0)
    if (u_RadarAlt >= 0.0) {
      vec2 altP = hudP - vec2(-0.48, -0.12);
      if (abs(altP.x) < 0.025 && abs(altP.y) < 0.12) {
        float altOutline = sdBox(altP, vec2(0.012, 0.10));
        hudGlow += (1.0 - smoothstep(0.001, 0.0025, abs(altOutline))) * 0.7;
        float altNorm = clamp(u_RadarAlt / 50.0, 0.0, 1.0);
        float altFill = -0.10 + 0.20 * altNorm;
        if (altP.y < altFill) {
          hudGlow += 0.55;
        }
      }
    }

    // L. Terrain Proximity Warning Banner (v4.6.0)
    if (u_TerrainAlert > 0.5) {
      float blink = step(0.0, sin(u_Time * 14.0));
      vec2 alertP = hudP - vec2(0.0, -0.22);
      if (blink > 0.5 && abs(alertP.x) < 0.22 && abs(alertP.y) < 0.045) {
        float boxD = abs(sdBox(alertP, vec2(0.20, 0.038)));
        float warnGlow = (1.0 - smoothstep(0.001, 0.003, boxD)) * 1.5;
        float stripes = sin((alertP.x + alertP.y) * 120.0);
        warnGlow += step(0.2, stripes) * 0.45;
        col.rgb += vec3(1.0, 0.25, 0.08) * warnGlow * 1.8;
        col.a = max(col.a, 0.9);
      }
    }

    // M. Planetary Landing Gear Annunciator (v4.6.1)
    // 3-point tricycle landing gear status indicators (Nose, Left, Right)
    vec2 gearBoxP = hudP - vec2(0.48, 0.08);
    if (abs(gearBoxP.x) < 0.045 && abs(gearBoxP.y) < 0.045) {
      float frameD = abs(sdBox(gearBoxP, vec2(0.040, 0.038)));
      hudGlow += (1.0 - smoothstep(0.001, 0.0025, frameD)) * 0.65;

      // 3 Gear Pods: Nose at top center, Main Left, Main Right
      vec2 noseP = gearBoxP - vec2(0.0, 0.016);
      vec2 leftP = gearBoxP - vec2(-0.022, -0.016);
      vec2 rightP = gearBoxP - vec2(0.022, -0.016);
      float podD = min(length(noseP), min(length(leftP), length(rightP))) - 0.0065;

      if (u_GearState > 1.5 && u_GearState < 2.5) {
        // Gear Deployed & Locked (Green solid)
        if (podD < 0.0) {
          col.rgb += vec3(0.2, 1.0, 0.3) * 1.5;
          col.a = max(col.a, 0.95);
        }
      } else if (u_GearState > 0.5 && u_GearState < 3.5) {
        // Gear in transit (Amber flashing)
        float amberBlink = step(0.0, sin(u_Time * 12.0));
        if (podD < 0.0 && amberBlink > 0.5) {
          col.rgb += vec3(1.0, 0.75, 0.1) * 1.6;
          col.a = max(col.a, 0.95);
        }
      } else {
        // Gear retracted: faint pod outline
        hudGlow += (1.0 - smoothstep(0.001, 0.002, abs(podD))) * 0.35;
      }
    }

    // N. Low Altitude Gear Warning (v4.6.1)
    if (u_RadarAlt >= 0.0 && u_RadarAlt < 0.8 && u_GearState < 1.5) {
      float gWarnBlink = step(0.0, sin(u_Time * 16.0));
      vec2 gAlertP = hudP - vec2(0.0, -0.16);
      if (gWarnBlink > 0.5 && abs(gAlertP.x) < 0.18 && abs(gAlertP.y) < 0.030) {
        float boxD = abs(sdBox(gAlertP, vec2(0.16, 0.024)));
        float warnG = (1.0 - smoothstep(0.001, 0.0025, boxD)) * 1.4;
        col.rgb += vec3(1.0, 0.15, 0.05) * warnG * 1.7;
        col.a = max(col.a, 0.9);
      }
    }

    // Additive HUD projection on the glass
    if (hudGlow > 0.001) {
      col.rgb += hudCol * hudGlow * 1.35;
      col.a = max(col.a, clamp(hudGlow, 0.0, 0.95));
    }
  }

  // 4. PILOT PHYSIOLOGICAL RESPONSE (v4.6.0)
  // Blackout (tunnel vision under high-G)
  if (u_Blackout > 0.001) {
    float distCenter = length(p * vec2(0.75, 1.0));
    float vignette = smoothstep(0.55 - u_Blackout * 0.35, 1.25 - u_Blackout * 0.45, distCenter);
    col.rgb *= (1.0 - vignette * u_Blackout * 0.95);
  }
  // Redout (negative G crimson wash)
  if (u_Redout > 0.001) {
    col.rgb = mix(col.rgb, vec3(col.r * 1.35 + 0.22, col.g * 0.25, col.b * 0.20), u_Redout * 0.65);
  }

  // 5. PLANETARY SURFACE DUST KICKUP (v4.6.1)
  // Billowing particle vortices near ground when engines fire
  if (u_SurfaceDustK > 0.001) {
    vec2 dustUV = p * 1.8 + vec2(u_Time * 0.45, -u_Time * 0.75);
    float dNoise1 = noise2D(dustUV * 4.0);
    float dNoise2 = noise2D(dustUV * 9.0 + vec2(1.7, 3.2));
    float dustPattern = dNoise1 * 0.65 + dNoise2 * 0.35;
    float lowerGradient = smoothstep(0.4, -0.85, p.y);
    float dustAlpha = clamp(dustPattern * lowerGradient * u_SurfaceDustK * 0.85, 0.0, 0.80);
    vec3 dustTint = vec3(0.78, 0.64, 0.48); // warm regolith dust
    col.rgb = mix(col.rgb, dustTint, dustAlpha);
    col.a = max(col.a, dustAlpha);
  }

  FragColor = col;
}
