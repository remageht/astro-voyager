#version 330 core
// PBR-lite warp-ship hull: GGX/Smith/Schlick direct key light driven by the
// scene's dominant source (station star/disk), cool anti-key rim, and sky
// IBL (mip-filtered local cubemap): ambient diffuse + roughness-blurred
// reflections. Base colors stay display-referred; decode to linear here.
// Output is linear HDR (ACES + bloom live in the composite).
//
// Two shading paths:
//   u_TexOn == 1 — Model hull (style 3): albedo / tangent-space normal /
//     roughness / metallic come from the Tripo PBR set baked to BMP by
//     tools/fbx_ship_export.mjs. Panel detailing lives in those maps.
//   u_TexOn == 0 — procedural parts: flat u_BaseColor + procedural panel
//     seams on the metal hull (S7).
in vec3 v_Normal;
in vec3 v_WorldPos;
in vec3 v_PartPos;
in vec3 v_ObjPos;   // ship-space позиция: скин Ring Ship
in vec2 v_UV;
out vec4 FragColor;
uniform vec3 u_CamPos;
uniform vec3 u_BaseColor;
uniform vec3 u_KeyDir;    // surface -> dominant scene light
uniform vec3 u_KeyColor;  // color * intensity
uniform float u_Metal;
uniform float u_Rough;
uniform vec3 u_Emissive;  // линейное HDR-свечение (сопла, навогни, кабина)
uniform samplerCube u_Env;
uniform float u_EnvMaxLod;
uniform float u_ReflK;  // v3.6.0: 0 = зеркальные отражения выключены (настройки)
uniform int u_TexOn;    // 1 = PBR-набор стиля Model подключён
uniform int u_EmissiveOn;  // 1 = есть emissive-карта стиля Model (юнит 5)
uniform int u_SkinOn;   // 1 = панельный скин Ring Ship (стиль 1)
uniform mat4 u_ShipInv; // inverse(ship world) — скин живёт в пространстве корабля
uniform sampler2D u_SkinAlbedo;  // тайловый скин Ring Ship (albedo)
uniform sampler2D u_SkinRough;   // он же, roughness
uniform sampler2D u_Albedo;
uniform sampler2D u_NormalTex;
uniform sampler2D u_RoughTex;
uniform sampler2D u_MetalTex;
uniform sampler2D u_EmissiveTex;  // запечённые наклейки/окна/кольцевой неон

const float kPi = 3.14159265;

float dGGX(float NoH, float a) {
  float a2 = a * a;
  float d = NoH * NoH * (a2 - 1.0) + 1.0;
  return a2 / (kPi * d * d + 1e-7);
}
float vSmith(float NoV, float NoL, float a) {
  float a2 = a * a;
  float lv = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
  float ll = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
  return 0.5 / max(lv + ll, 1e-5);
}
vec3 fSchlick(float u, vec3 f0) { return f0 + (1.0 - f0) * pow(1.0 - u, 5.0); }

void main() {
  vec3 n = normalize(v_Normal);
  vec3 geomN = n;  // rim/силуэт считают по ГЕОМЕТРИИ: нормаль-карта Model
                   // рваная (кластерные vt), rim по ней заливает корпус
  vec3 V = normalize(u_CamPos - v_WorldPos);
  float NoV = max(dot(n, V), 1e-4);
  vec3 albedo;
  float rough;
  float metal;
  vec3 mapEm = vec3(0.0);  // emissive из карты стиля Model

  if (u_TexOn == 1) {
    // sRGB albedo -> linear; roughness/metallic maps are linear grayscale.
    // LOD 0 принудительно: vt у Model — кластерные средние, производные UV
    // скачут между кластерами, mip-фильтр вырождается в средний цвет атласа.
    albedo = pow(max(textureLod(u_Albedo, v_UV, 0.0).rgb, vec3(0.0)), vec3(2.2));
    rough = clamp(textureLod(u_RoughTex, v_UV, 0.0).r, 0.05, 1.0);
    metal = clamp(textureLod(u_MetalTex, v_UV, 0.0).r, 0.0, 1.0);
    if (u_EmissiveOn == 1)
      mapEm = pow(textureLod(u_EmissiveTex, v_UV, 0.0).rgb, vec3(2.2));
    // Tangent-space normal without a tangent stream: build the cotangent
    // frame from screen-space derivatives of world position and uv.
    vec3 nmap = textureLod(u_NormalTex, v_UV, 0.0).xyz * 2.0 - 1.0;
    vec3 dp1 = dFdx(v_WorldPos);
    vec3 dp2 = dFdy(v_WorldPos);
    vec2 duv1 = dFdx(v_UV);
    vec2 duv2 = dFdy(v_UV);
    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float im = inversesqrt(max(dot(T, T), dot(B, B)) + 1e-12);
    n = normalize(T * (im * nmap.x) + B * (im * nmap.y) + n * nmap.z);
  } else {
    albedo = pow(max(u_BaseColor, vec3(0.0)), vec3(2.2));
    rough = clamp(u_Rough, 0.05, 1.0);
    metal = clamp(u_Metal, 0.0, 1.0);
    // S7: панельные линии и вариация плит — только металлический корпус
    // (u_Metal >= 0.85; стекло/крылья/свечения остаются гладкими). Сетка в
    // объектном пространстве части: узкие швы + лёгкий разброс тона плит.
    if (u_SkinOn == 1) {
      // === Ring Ship skin ===: тайловая текстура (tools/make_ring_skin.mjs)
      // выбирается трипланарно в пространстве КОРАБЛЯ (v_ObjPos), поэтому
      // панели, заклёпки и потёртости идут непрерывно сквозь все эллипсоиды и
      // кольца и не требуют UV (у процедурных мешей их нет). Mipmaps снимают
      // мерцание панельной сетки на дистанции.
      vec3 tp = v_ObjPos * 4.0;
      vec3 an = abs(normalize(v_Normal));
      an /= (an.x + an.y + an.z + 1e-5);
      vec3 ca = texture(u_SkinAlbedo, tp.yz).rgb * an.x +
                texture(u_SkinAlbedo, tp.xz).rgb * an.y +
                texture(u_SkinAlbedo, tp.xy).rgb * an.z;
      float cr = texture(u_SkinRough, tp.yz).r * an.x +
                 texture(u_SkinRough, tp.xz).r * an.y +
                 texture(u_SkinRough, tp.xy).r * an.z;
      albedo = pow(max(ca, vec3(0.0)), vec3(2.2));
      rough = clamp(mix(rough, cr, 0.90), 0.08, 1.0);
      // Гарь у сопел: корма темнее и «металличнее».
      float heat = smoothstep(0.55, 1.15, v_ObjPos.z);
      albedo *= mix(1.0, 0.72, heat);
      metal = clamp(metal + 0.08 * heat, 0.0, 1.0);
    } else if (metal >= 0.85) {
      vec3 pp = (v_PartPos + vec3(0.13, -0.07, 0.21)) * 5.0;
      vec3 g = abs(fract(pp) - 0.5);
      float seam = min(min(g.x, g.y), g.z);
      float panel = smoothstep(0.02, 0.06, seam);
      float cell = dot(floor(pp), vec3(12.9898, 78.233, 37.719));
      float h = fract(sin(cell) * 43758.5453);
      albedo *= mix(0.80, 1.0, panel) * (0.95 + 0.09 * h);
      rough = clamp(rough + (1.0 - panel) * 0.12, 0.05, 1.0);
    }
  }

  vec3 f0 = mix(vec3(0.04), albedo, metal);
  vec3 kd = (vec3(1.0) - f0) * (1.0 - metal);

  // Key light.
  vec3 L = normalize(u_KeyDir);
  float NoL = max(dot(n, L), 0.0);
  vec3 H = normalize(L + V);
  float NoH = max(dot(n, H), 0.0);
  float VoH = max(dot(V, H), 0.0);
  vec3 spec = fSchlick(VoH, f0) * dGGX(NoH, rough) * vSmith(NoV, NoL, rough);
  vec3 col = (kd * albedo / kPi + spec) * u_KeyColor * NoL;

  // Cool rim from the anti-key direction — silhouette readability.
  if (u_TexOn != 1) {
    vec3 rimDir = normalize(-L + vec3(0.0, 0.45, 0.0));
    float rimNoL = max(dot(geomN, rimDir), 0.0);
    float rimFres = pow(1.0 - max(dot(geomN, V), 1e-4), 3.0);
    col += vec3(0.45, 0.62, 0.95) * rimFres * (0.30 + 0.22 * rimNoL);
  }

  // Sky IBL: diffuse from the coarsest mip, reflections by roughness lod.
  vec3 R = reflect(-V, n);
  vec3 envSpec = textureLod(u_Env, R, rough * u_EnvMaxLod).rgb;
  vec3 envDiff = textureLod(u_Env, n, u_EnvMaxLod).rgb;
  vec3 fEnv = f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NoV, 5.0);
  col += envDiff * albedo * kd * 2.6;  // ambient gain: sky is dim HDR
  col += envSpec * fEnv * (1.0 - 0.55 * rough) * u_ReflK;
  col += u_Emissive + mapEm;  // S5/S8: сопла, навогни, кабина + Model эмиссив
  FragColor = vec4(col, 1.0);
}
