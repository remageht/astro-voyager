#version 330 core
// Варп-кокон корабля: эллипсоидный ореол из бегущих струй, носовой веер и
// кормовой красный хвост. Формы запечены в shell-local координатах
// (полюса вдоль z: нос = -z); анимация дрейфа целиком в шейдере: угол theta
// восстанавливается из позиции и сдвигается u_Drift*u_Time по меридиану.
// v4.0: режим 3 — полупрозрачная «хризалидная» оболочка (заполненный
// эллипсоид из сферической сетки): френель к краям силуэта + мягкие
// малярные пятна и потёки энергии; аддитивный блендинг делает лёгкую
// муть, а кромки читаются как водянистый кокон (см. референс).
layout(location = 0) in vec3 position;
uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform float u_Time;
uniform float u_Drift;   // рад/с дрейфа струй (тяга + варп)
uniform float u_Elong;   // вытяжение кокона по z (варп)
uniform float u_Energy;  // общий множитель яркости (0.3 idle .. 1+ warp)
uniform int u_Mode;      // 0 струи-кокон; 1 носовой веер; 2 кормовой хвост;
                         // 3 оболочка-эллипсоид (filled, френель)
uniform vec3 u_CyanCol;  // переднее-циановая ветвь (база палитры)
uniform vec3 u_RedCol;   // кормовая тёплая ветвь
uniform vec3 u_CamShell; // камера в shell-local (для френеля режима 3)
out vec3 v_Color;

const float A_LAT = 0.55f;     // поперечный радиус кокона
const float C_LEN = 1.25f;     // полудлина кокона (запечена)
const float C_CENTER = 0.15f;  // сдвиг центра кокона (нос прижат)

float hash12(vec2 p) {
  vec3 p3 = fract(vec3(p.x, p.y, p.x) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

void main() {
  if (u_Mode == 3) {
    // Оболочка-хризалида: сферическая сетка единичного радиуса ->
    // эллипсоид ореола (та же раскладка, что у струй режима 0).
    float lenH = max(length(position), 1e-4);
    float cosT = clamp(position.z / lenH, -0.999, 0.999);
    float theta = acos(cosT);
    float phi = atan(position.y, position.x);
    float lat = sin(theta);
    float zz = cosT;
    vec3 p = vec3(A_LAT * lat * cos(phi), A_LAT * lat * sin(phi),
                  zz * C_LEN * u_Elong + C_CENTER);
    gl_Position = u_MVP * u_Model * vec4(p, 1.0);
    // Нормаль эллипсоида: grad(x^2/a^2 + y^2/a^2 + z^2/c^2).
    vec3 nrm = normalize(vec3(p.x / (A_LAT * A_LAT),
                              p.y / (A_LAT * A_LAT),
                              (p.z - C_CENTER) /
                                  ((C_LEN * u_Elong) * (C_LEN * u_Elong))));
    vec3 V = normalize(u_CamShell - p);
    float ndv = clamp(abs(dot(nrm, V)), 0.0, 1.0);
    // Мягкий и эфирный френель: прозрачный центр (не забивает силуэт корабля),
    // деликатное и тонкое свечение по кромке силуэта.
    float fres = pow(1.0 - ndv, 3.2);
    // «Водянистость»: мягкие энергетические переливы и бегущие потоки.
    float blotch = hash12(floor(vec2(phi * 3.5, theta * 3.5)) + 4.1);
    float flows = 0.65 +
                  0.35 * sin(theta * 5.0 - u_Drift * u_Time * (0.8 + 0.3 * zz) +
                             blotch * 6.28);
    // Цвет: у носа циановый, к корме тёплый; мягкое свечение по френелю.
    float mixF = smoothstep(0.35, -0.30, zz);
    vec3 col = mix(u_RedCol, u_CyanCol, mixF);
    col *= (0.015 + 0.35 * fres) * flows * (0.75 + 0.25 * blotch);
    // Деликатный ободок у переднего полюса без избыточной засветки.
    col += vec3(0.90, 0.96, 1.0) * exp(-max(-zz, 0.0) * 2.2) * fres * 0.40;
    col *= u_Energy * 0.70;
    v_Color = col;
    return;
  }
  if (u_Mode == 0) {
    // Меридианы сфероида: phi = atan2(y, x); theta восстанавливаем из z.
    float phi = atan(position.y, position.x);
    float cosT = clamp(position.z / C_LEN, -0.999, 0.999);
    float theta = acos(cosT);
    // id струи: азимут квантуем в 512 бинов (концы отрезка дают почти
    // одинаковый atan2 -> попадают в один бин, дрейф когерентный).
    float id = hash12(vec2(floor(phi * 81.5), 19.7));
    float drift = mod(theta + u_Drift * u_Time * (0.55 + 0.90 * id) +
                          id * 2.9, 6.28318);
    // Складываем назад в [0, pi] (переход через полюс продолжает меридиан).
    float tt = 3.14159265 - abs(drift - 3.14159265);
    float lat = sin(tt);
    float zz = cos(tt);
    vec3 p = vec3(A_LAT * lat * cos(phi), A_LAT * lat * sin(phi),
                  zz * C_LEN * u_Elong + C_CENTER);
    gl_Position = u_MVP * u_Model * vec4(p, 1.0);
    // Тонкая деликатная огибающая меридианов.
    float env = smoothstep(0.03, 0.22, lat) * (0.70 + 0.30 * lat);
    // Цвет: нос (zz < 0) — циан, корма (zz > 0.2) — тёплый красный; спад к
    // экватору ореола.
    float mixF = smoothstep(0.35, -0.30, zz);
    vec3 col = mix(u_RedCol, u_CyanCol, mixF);
    // Мягкий полупрозрачный блик у носового полюса.
    col += vec3(1.0, 0.97, 0.92) * exp(-max(-zz, 0.0) * 2.6) * 0.40 * mixF;
    // Снижаем яркость в ~2.8 раза (0.36), чтобы корпус и звёзды читались сквозь линии.
    col *= env * (0.85 + 0.45 * id) * u_Energy * 0.36;
    v_Color = col;
    return;
  }
  if (u_Mode == 1) {
    // Носовой веер: лучи от точки у носа наружу, затухают к кончикам.
    float f = clamp((-position.z - 1.05) / 0.70, 0.0, 1.0);
    float glow = 1.0 - f * f;
    float fl = 0.75 + 0.25 * sin(u_Time * 7.3 + position.y * 31.0);
    vec3 col = mix(u_CyanCol, vec3(1.0, 0.98, 0.95), 0.50) * glow * fl;
    col *= u_Energy * (0.40 + 0.60 * glow) * 0.36;
    v_Color = col;
    gl_Position = u_MVP * u_Model * vec4(position, 1.0);
    return;
  }
  // Режим 2: кормовой хвост — тёплые струи, лёгкое покачивание, затухание.
  vec3 p = position;
  p.x += 0.05 * sin(u_Time * 2.1 + p.z * 2.4);
  p.y += 0.03 * sin(u_Time * 1.7 + p.z * 1.9);
  gl_Position = u_MVP * u_Model * vec4(p, 1.0);
  float fade = 1.0 - clamp((p.z - 1.05) / 1.45, 0.0, 1.0);
  float fl = 0.80 + 0.20 * sin(u_Time * 3.1 + position.x * 23.0);
  vec3 col = u_RedCol * (0.55 + 0.45 * fade) * fade * fl;
  col *= u_Energy * 0.36;
  v_Color = col;
}
