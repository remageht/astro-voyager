#version 330 core
// PostFX composite: scene (HDR-linear float16) + bloom mip pyramid (level 0
// already accumulates all levels) + anamorphic streak, then ACES + gamma +
// grain; relativistic Doppler tint when flying. Output feeds FXAA (LDR).
// u_BloomStrength == 0.0 => tonemapped scene only (stage 2a no longer
// pixel-exact: tonemap moved here from the station shaders in v3.0.0).
// u_Beta == 0.0 (harness / standing still) => tint block skipped.
out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom;   // post_down13/post_up9 pyramid, level 0
uniform sampler2D u_Streak;  // post_streak.frag (level-2 horizontal smear)
uniform float u_StreakK;     // streak share, cool tint below
uniform float u_BloomStrength;
uniform float u_Time;  // sim time for grain (fixed by the harness => PNG stable)

vec3 acesNarkowicz(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
float compHash(vec2 p) {
  return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// === Relativity === observer velocity: Doppler tint per view ray.
// Ahead of the motion -> blue + brighter, behind -> red + dimmer
// (relativistic beaming). Zero when the camera stands still.
uniform vec3 u_CamFwd;
uniform vec3 u_CamRight;
uniform vec3 u_CamUp;
uniform float u_FovY;
uniform float u_Aspect;
uniform vec3 u_VelDir;
uniform float u_Beta;

// === Ship survival === (v3.3.0): low-shield screen effects (0 = exact pass).
uniform float u_ChromaK;  // chromatic aberration amount 0..1
uniform float u_StaticK;  // interference: noise + vignette 0..1
// === Speed FX === (v3.3.0): motion streaks, 0 = exact pass (harness).
uniform float u_SpeedK;  // 0..1 speed amount
// === Warp field === (v3.6.0): искривление пространства за кораблём —
// локальный «пузырь» у экранной позиции корабля: радиальное линзирование
// фона, анизотропный смаз вдоль курса (звёзды тянутся шлейфом) и бегущая
// рябь; всё анимировано через u_Time. u_WarpK == 0 => точный проход.
uniform vec2 u_WarpUv;   // экранная позиция корабля (0..1, y вверх)
uniform vec2 u_WarpDir;  // экранное направление курса (не нормировано)
uniform float u_WarpK;   // 0..1 сила варп-поля
// === Ship survival === (v3.3.0): shield impact flash, 0 = exact pass.
uniform float u_ShieldK;  // 0..1 impact amount (cold blue edge glow)
// === Horizon capture === (v3.6.0): катсцена захвата горизонта —
// приливная линза + красное смещение + белая вспышка (0 => exact pass).
uniform float u_HorizonK;      // прилив + красное смещение 0..1
uniform float u_HorizonFlash;  // плавная белая вспышка 0..1

// === Cinematography Director & Filmic Grading === (v4.4.0)
uniform int u_ColorGrade;       // 0=Neutral, 1=Interstellar Amber, 2=Cyberpunk Neon, 3=NASA Mono Tri-X, 4=Technicolor Vintage
uniform float u_FilmicVignette; // 0.0..1.0 smooth lens vignette

vec3 applyColorGrading(vec3 c, int grade) {
  if (grade == 1) {
    // Interstellar Amber: warm sci-fi golden highlights, deep amber contrast
    vec3 amber = vec3(
        c.r * 1.12 + 0.02,
        c.g * 1.01 + 0.005,
        c.b * 0.82
    );
    amber = clamp(amber, 0.0, 1.0);
    amber = pow(amber, vec3(0.95, 1.0, 1.10));
    return clamp(mix(c, amber, 0.85), 0.0, 1.0);
  } else if (grade == 2) {
    // Cyberpunk Neon: cool cyan shadows, electric magenta/amber highlights
    vec3 shadows = vec3(0.0, 0.12, 0.18);
    vec3 highlights = vec3(1.15, 0.85, 1.10);
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 neon = mix(c + shadows * (1.0 - luma), c * highlights, luma);
    neon = clamp(neon, 0.0, 1.0);
    neon = neon * neon * (3.0 - 2.0 * neon);
    return clamp(mix(c, neon, 0.75), 0.0, 1.0);
  } else if (grade == 3) {
    // NASA Monochromatic: Kodak Tri-X 400 panchromatic scientific film
    float luma = dot(c, vec3(0.299, 0.587, 0.114));
    float triX = smoothstep(0.02, 0.98, luma);
    return vec3(clamp(triX, 0.0, 1.0));
  } else if (grade == 4) {
    // Technicolor Vintage: 3-strip color saturation and warmth
    vec3 tech;
    tech.r = c.r * 1.25 - c.g * 0.12 - c.b * 0.10;
    tech.g = c.g * 1.20 - c.r * 0.08 - c.b * 0.08;
    tech.b = c.b * 1.15 - c.r * 0.05 - c.g * 0.05;
    tech = clamp(tech, 0.0, 1.0);
    float luma = dot(tech, vec3(0.299, 0.587, 0.114));
    tech = mix(vec3(luma), tech, 1.25);
    return clamp(tech, 0.0, 1.0);
  }
  return c;
}

void main() {
  vec4 sceneT = texelFetch(u_Scene, ivec2(gl_FragCoord.xy), 0);
  vec3 bloom = texture(u_Bloom, v_TexCoord).rgb +
               texture(u_Streak, v_TexCoord).rgb *
                   (u_StreakK * vec3(0.55, 0.75, 1.0));
  vec3 col = sceneT.rgb + bloom * u_BloomStrength;
  if (u_WarpK > 0.001) {
    // Warp field: локальная деформация пространства вокруг корабля.
    // texture() только внутри гейта => u_WarpK==0 остаётся texelFetch-точным.
    // v3.6.0: амплитуды снижены (мягкая линза), рябь медленнее и мельче —
    // комфорт для глаз; явление визуально не похоже на спид-стрики.
    vec2 wp = v_TexCoord - u_WarpUv;
    float wr = length(wp);
    const float WR = 0.40;  // радиус пузыря по экрану
    if (wr < WR + 0.30) {
      float fall = 1.0 - smoothstep(0.0, WR, wr);
      float k = u_WarpK * fall * fall;
      vec2 wdir = u_WarpDir;
      float wl = length(wdir);
      wdir = (wl > 1e-4) ? (wdir / wl) : vec2(0.0, 1.0);
      float along = dot(wp, wdir);
      // Шлейф позади корабля (против курса): смаз тянет звёзды вдоль оси.
      float wake = 1.0 - smoothstep(0.0, 0.60, max(-along, 0.0));
      vec2 off = -wp * (0.18 * k) - wdir * (0.06 * k * wake);
      // Анимированная рябь: бежит от корабля вдоль шлейфа (мягкая, редкая).
      float rip = sin(wr * 18.0 - u_Time * 4.5);
      off += wdir * (0.014 * k * wake * rip) +
             (wp / (wr + 1e-5)) * (0.007 * k * rip);
      // Лёгкий хроматический разлёт по краям пузыря.
      vec3 warped = vec3(texture(u_Scene, v_TexCoord + off * 1.02).r,
                         texture(u_Scene, v_TexCoord + off).g,
                         texture(u_Scene, v_TexCoord + off * 0.98).b);
      if (k * wake > 0.01) {
        // Дуплет вдоль курса: звёзды в шлейфе превращаются в лучи.
        for (int i = 1; i <= 2; ++i) {
          float t = 0.016 * float(i) * k * wake;
          warped = max(warped,
                       texture(u_Scene, v_TexCoord + off + wdir * t).rgb);
        }
      }
      col = mix(col, warped + bloom * u_BloomStrength,
                clamp(k * 1.1, 0.0, 1.0));
    }
  }
  if (u_ChromaK > 0.0001) {
    // texture() lives only inside this branch: the amount==0 path must stay
    // texelFetch-exact (no filtering, no extra samples).
    vec2 cabOff = (v_TexCoord - 0.5) * 0.012 * u_ChromaK;
    col = vec3(texture(u_Scene, v_TexCoord + cabOff).r, col.g,
               texture(u_Scene, v_TexCoord - cabOff).b);
  }
  if (u_SpeedK > 0.001) {
    // Motion streaks: bright scene pixels smear backwards along the velocity
    // (3 taps, texture() only inside this gate => amount==0 stays texel-exact).
    // v3.6.0: смаз короче и мягче (комфортный «разгон», без стробоскопа).
    vec2 sp = v_TexCoord - 0.5;
    vec2 vd = vec2(dot(u_VelDir, u_CamRight), dot(u_VelDir, u_CamUp));
    float vl = length(vd);
    if (vl > 1e-4) {
      vd /= vl;
      float align = max(dot(normalize(sp + vec2(1e-4)), vd), 0.0);
      float env = smoothstep(0.12, 0.62, length(sp)) * align * u_SpeedK;
      vec3 acc = col;
      for (int i = 1; i <= 3; ++i) {
        float t = 0.010 * float(i) * u_SpeedK;
        acc = max(acc, texture(u_Scene, v_TexCoord - vd * t).rgb +
                          bloom * u_BloomStrength);
      }
      col = mix(col, acc, clamp(env * 0.26, 0.0, 0.32));
    }
  }
  if (u_ShieldK > 0.001) {
    // Shield hit: cold blue edge glow + soft wash (0 => exact pass).
    float sEdge = smoothstep(0.30, 1.0, length(v_TexCoord - 0.5));
    col += vec3(0.30, 0.55, 1.10) * u_ShieldK * (0.18 + 0.82 * sEdge);
  }
  if (u_HorizonK > 0.001 || u_HorizonFlash > 0.0001) {
    // v3.6.0: приливная линза — тянет кадр к центру, красное смещение и
    // тёмное кольцо «в колодец»; белая вспышка на пике провала.
    vec2 ndc = v_TexCoord * 2.0 - 1.0;
    float d = length(ndc);
    float pull = 1.0 - smoothstep(0.05, 0.55, d);
    vec2 off = -ndc * (0.22 * u_HorizonK * pull);
    vec3 hcol = vec3(texture(u_Scene, v_TexCoord + off * 1.02).r,
                     texture(u_Scene, v_TexCoord + off).g,
                     texture(u_Scene, v_TexCoord + off * 0.98).b);
    vec3 redshift = vec3(1.0 + 0.65 * u_HorizonK,
                         1.0 - 0.12 * u_HorizonK,
                         1.0 - 0.42 * u_HorizonK);
    hcol *= redshift;
    hcol *= 1.0 - 0.35 * u_HorizonK * (1.0 - smoothstep(0.05, 0.55, d));
    col = mix(col, hcol, u_HorizonK);
    if (u_HorizonFlash > 0.0001) {
      col = mix(col, vec3(1.05, 1.02, 0.98), u_HorizonFlash * 0.9);
    }
  }
  // === Relativity === (I1): the CPU invariant is
  //   g = 1 - beta * (n . dir), n = photon propagation, dir = observer motion.
  // This pixel's ray points camera -> scene, so n = -ray and the code below
  // computes s = g - 1 = beta * dot(ray, u_VelDir): blue ahead, red behind.
  if (u_Beta > 0.001) {
    vec2 ndc = v_TexCoord * 2.0 - 1.0;
    float t = tan(u_FovY * 0.5);
    vec3 ray = normalize(u_CamFwd + ndc.x * u_Aspect * t * u_CamRight +
                         ndc.y * t * u_CamUp);
    // s in (-beta, +beta): > 0 towards the motion (blueshift),
    // < 0 behind the motion (redshift).
    float s = u_Beta * dot(ray, u_VelDir);
    vec3 tint = s > 0.0 ? mix(vec3(1.0), vec3(0.72, 0.88, 1.45), s)
                        : mix(vec3(1.0), vec3(1.55, 0.62, 0.45), -s);
    float gain = 1.0 + 0.6 * s;  // beaming: front brighter, rear dimmer
    col *= tint * gain;
  }
  // Single tonemap for the whole frame (moved here from station shaders).
  col = acesNarkowicz(col);
  col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));

  // Filmic color grading & LUT presets (v4.4.0)
  if (u_ColorGrade > 0) {
    col = applyColorGrading(col, u_ColorGrade);
  }

  // Smooth lens vignette (v4.4.0)
  if (u_FilmicVignette > 0.001) {
    vec2 vUv = v_TexCoord - 0.5;
    float dist = length(vUv * vec2(1.0, 0.9));
    float vig = smoothstep(0.35, 0.85, dist);
    col *= 1.0 - vig * clamp(u_FilmicVignette, 0.0, 1.0) * 0.85;
  }

  // Single post-tonemap grain (replaces the per-shader grains).
  col += (compHash(gl_FragCoord.xy + fract(u_Time) * 100.0) - 0.5) * 0.012;
  if (u_StaticK > 0.0001) {
    // Interference wash + edge vignette (loops in the existing compHash).
    float sN = compHash(gl_FragCoord.xy * 0.371 + fract(u_Time) * 97.0);
    float sVig = smoothstep(0.35, 0.85, length(v_TexCoord - 0.5));
    col = mix(col, vec3(sN) * 0.5, u_StaticK * 0.5);
    col *= 1.0 - 0.65 * sVig * u_StaticK;
  }
  FragColor = vec4(col, sceneT.a);
}
