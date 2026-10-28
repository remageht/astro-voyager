#version 330 core
// Solid warp-ship hull: ellipsoid parts with lambert + fresnel rim.
// Model hull (style 3) adds a uv stream for the PBR texture set; procedural
// parts leave attribute 2 disabled, so a_UV reads as (0,0) and ship.frag's
// u_TexOn branch stays off.
//
// v_ObjPos is the ship-space position (inverse of the ship's world matrix):
// the procedural Ring Ship skin draws its panel grid in that space, so the
// pattern is continuous across all hull parts and does not slide with the ship.
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 a_Aux;  // sphere: normal; ring: a_R (unused)
layout(location = 2) in vec2 a_UV;   // Model hull: texture coordinates
uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform mat4 u_ShipInv;    // inverse of the ship world matrix (skin space)
uniform vec3 u_InvScale;   // inverse part scale for correct ellipsoid normals
uniform int u_FlatNormal;  // 1 = ring: local normal +Y
uniform float u_RingSpin;  // угол кольца Obj (rad); процедурным ставится 0
out vec3 v_Normal;
out vec3 v_WorldPos;
out vec3 v_PartPos;  // object-space позиция: панельные линии корпуса (S7)
out vec3 v_ObjPos;   // ship-space позиция: скин Ring Ship
out vec2 v_UV;
void main() {
  // Спин колец Ring Ship (style 1): кольца — отдельные торы в XY-плоскости,
  // пилоны к корпусу удалены из ring_ship.obj, поэтому можно вращать только
  // обод (r>0.35) в его трёх z-срезах, не деформируя крепления.
  float k = 0.0;
  {
    float rxy = length(position.xy);
    float radial = smoothstep(0.350, 0.370, rxy);
    float z = position.z;
    float slab = max(step(-0.36, z) * step(z, -0.10),
                     max(step(0.14, z) * step(z, 0.31),
                         step(0.57, z) * step(z, 0.76)));
    k = radial * slab;
  }
  float ang = u_RingSpin * k;
  float ca = cos(ang);
  float sa = sin(ang);
  vec4 wp = u_Model * vec4(position.x * ca - position.y * sa,
                           position.x * sa + position.y * ca,
                           position.z, 1.0);
  v_WorldPos = wp.xyz;
  v_PartPos = position;
  // Скин сэмплируется в НЕКРУТЁНОЙ позиции: окраска приклеена к точке
  // материала — неон и окна едут вместе с кольцом, панели корпуса стоят.
  vec4 wpSkin = u_Model * vec4(position, 1.0);
  v_ObjPos = (u_ShipInv * wpSkin).xyz;
  v_UV = a_UV;
  vec3 n = (u_FlatNormal == 1) ? vec3(0.0, 1.0, 0.0)
                                : a_Aux * u_InvScale * u_InvScale;
  // Нормали кольцевой зоны крутятся вместе с геометрией.
  n = vec3(n.x * ca - n.y * sa, n.x * sa + n.y * ca, n.z);
  v_Normal = mat3(u_Model) * n;
  gl_Position = u_MVP * wp;
}
