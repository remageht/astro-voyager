#!/usr/bin/env node
// tools/generate_space_probe.mjs — Deep Space Science & Relay Probe generator
// Generates:
//   res/models/probe.obj
//   res/models/probe_albedo.bmp
//   res/models/probe_normal.bmp
//   res/models/probe_rough.bmp
//   res/models/probe_metal.bmp
//   res/models/probe_emissive.bmp

import fs from 'node:fs';
import path from 'node:path';

const OUTDIR = process.argv[2] || 'res/models';
const SIZE = 512; // 512x512 PBR texture atlas
fs.mkdirSync(OUTDIR, { recursive: true });

// =========================================================================
// 1. Texture Atlas Generation (Albedo, Normal, Roughness, Metal, Emissive)
// =========================================================================
const albedo = Buffer.alloc(SIZE * SIZE * 3);
const normal = Buffer.alloc(SIZE * SIZE * 3);
const rough = Buffer.alloc(SIZE * SIZE * 3);
const metal = Buffer.alloc(SIZE * SIZE * 3);
const emissive = Buffer.alloc(SIZE * SIZE * 3);

const clamp01 = (x) => (x < 0 ? 0 : x > 1 ? 1 : x);

for (let y = 0; y < SIZE; y++) {
  for (let x = 0; x < SIZE; x++) {
    const u = (x + 0.5) / SIZE;
    const v = (y + 0.5) / SIZE;
    const idx = (y * SIZE + x) * 3;

    // Default neutral normal (+Z in tangent space -> RGB 128, 128, 255)
    let nx = 0.0, ny = 0.0, nz = 1.0;
    let r = 0.5, g = 0.5, b = 0.5;
    let ro = 0.5, me = 0.5;
    let er = 0.0, eg = 0.0, eb = 0.0;

    // Sub-quadrant layout:
    // Q1: u < 0.5, v >= 0.5 -> Main Avionics Bus (Gold MLI foil + radiators)
    // Q2: u >= 0.5, v >= 0.5 -> High-gain Reflector Dish (Off-white / titanium / gold feed)
    // Q3: u < 0.5, v < 0.5 -> Solar Arrays (Navy blue silicon cells + busbars)
    // Q4: u >= 0.5, v < 0.5 -> Trusses, booms, thrusters & beacons

    if (u < 0.5 && v >= 0.5) {
      // Avionics body: Gold foil texture with subtle wrinkled crinkles
      const su = (u - 0.0) / 0.5;
      const sv = (v - 0.5) / 0.5;
      const crinkle = Math.sin(su * 45.0) * Math.cos(sv * 45.0) * 0.15 +
                      Math.sin(su * 95.0 + sv * 65.0) * 0.08;
      // Gold MLI foil color
      r = 0.95 + crinkle * 0.1;
      g = 0.72 + crinkle * 0.08;
      b = 0.22 + crinkle * 0.05;
      ro = 0.28 + Math.abs(crinkle) * 0.35;
      me = 0.92;

      // Silver radiator patch in part of the texture
      if (su > 0.65 && sv < 0.45) {
        r = 0.88; g = 0.90; b = 0.93;
        ro = 0.18; me = 0.85;
      }
      // Electronic hatch lines
      if ((su * 6) % 1.0 < 0.04 || (sv * 6) % 1.0 < 0.04) {
        r *= 0.6; g *= 0.6; b *= 0.6;
        ro = 0.7;
        nx = 0.2; ny = 0.2;
      }
    } else if (u >= 0.5 && v >= 0.5) {
      // High-gain dish: Ceramic off-white dish with radial ribbing
      const su = (u - 0.5) / 0.5;
      const sv = (v - 0.5) / 0.5;
      const cu = su - 0.5;
      const cv = sv - 0.5;
      const dist = Math.hypot(cu, cv) * 2.0; // 0 at center, 1 at edge
      const angle = Math.atan2(cv, cu);

      if (dist < 0.15) {
        // Gold feed horn at dish center
        r = 0.96; g = 0.75; b = 0.25;
        ro = 0.22; me = 0.95;
      } else if (dist <= 0.92) {
        // Dish dish surface: off-white thermal coat
        const rib = Math.cos(angle * 16.0) > 0.85 ? 0.08 : 0.0;
        const ring = Math.abs(Math.sin(dist * 20.0)) > 0.92 ? 0.05 : 0.0;
        const cVal = 0.88 + rib + ring;
        r = cVal; g = cVal * 0.98; b = cVal * 0.95;
        ro = 0.45 - rib * 0.2;
        me = 0.12;
      } else {
        // Dish outer rim: dark titanium alloy
        r = 0.32; g = 0.34; b = 0.38;
        ro = 0.35; me = 0.75;
      }
    } else if (u < 0.5 && v < 0.5) {
      // Solar arrays: deep space blue photovoltaic cells
      const su = (u - 0.0) / 0.5;
      const sv = (v - 0.0) / 0.5;
      const cellX = (su * 8.0) % 1.0;
      const cellY = (sv * 12.0) % 1.0;
      const isGrid = (cellX < 0.06) || (cellY < 0.05);
      const isBusbar = (cellX > 0.48 && cellX < 0.52);

      if (isGrid) {
        // Metallic silver/gold interconnect grid
        r = 0.75; g = 0.75; b = 0.78;
        ro = 0.25; me = 0.80;
      } else if (isBusbar) {
        // Gold busbar
        r = 0.90; g = 0.75; b = 0.25;
        ro = 0.20; me = 0.90;
      } else {
        // Silicon photovoltaic cell: deep rich blue/cyan specular
        r = 0.04; g = 0.10; b = 0.28;
        ro = 0.15; me = 0.05;
      }
    } else {
      // Trusses, booms, thruster quads and emissive telemetry beacons
      const su = (u - 0.5) / 0.5;
      const sv = (v - 0.0) / 0.5;

      if (su < 0.4 && sv < 0.4) {
        // Strobe beacon (green/cyan telemetry indicator)
        r = 0.1; g = 0.95; b = 0.8;
        er = 0.3; eg = 1.8; eb = 1.4; // HDR emissive
        ro = 0.1; me = 0.1;
      } else if (su < 0.4 && sv >= 0.4 && sv < 0.8) {
        // Amber warning / star tracker beacon
        r = 0.95; g = 0.55; b = 0.1;
        er = 1.9; eg = 0.9; eb = 0.15; // HDR emissive
        ro = 0.1; me = 0.1;
      } else {
        // Titanium / Carbon truss struts
        const stripe = ((su + sv) * 20.0) % 1.0 > 0.5 ? 0.05 : 0.0;
        r = 0.26 + stripe; g = 0.28 + stripe; b = 0.32 + stripe;
        ro = 0.40; me = 0.80;
      }
    }

    // Write BGR albedo
    albedo[idx] = Math.round(clamp01(r) * 255);
    albedo[idx + 1] = Math.round(clamp01(g) * 255);
    albedo[idx + 2] = Math.round(clamp01(b) * 255);

    // Tangent normal map (0.5 + 0.5*n)
    const nLen = Math.hypot(nx, ny, nz) || 1.0;
    normal[idx] = Math.round(((nx / nLen) * 0.5 + 0.5) * 255);
    normal[idx + 1] = Math.round(((ny / nLen) * 0.5 + 0.5) * 255);
    normal[idx + 2] = Math.round(((nz / nLen) * 0.5 + 0.5) * 255);

    // Roughness & Metallic
    const roVal = Math.round(clamp01(ro) * 255);
    rough[idx] = roVal; rough[idx + 1] = roVal; rough[idx + 2] = roVal;

    const meVal = Math.round(clamp01(me) * 255);
    metal[idx] = meVal; metal[idx + 1] = meVal; metal[idx + 2] = meVal;

    // Emissive
    emissive[idx] = Math.round(clamp01(er) * 255);
    emissive[idx + 1] = Math.round(clamp01(eg) * 255);
    emissive[idx + 2] = Math.round(clamp01(eb) * 255);
  }
}

// 24-bit bottom-up BMP writer
function writeBmp(filepath, rgbBuffer) {
  const rowSize = (SIZE * 3 + 3) & ~3;
  const dataSize = rowSize * SIZE;
  const header = Buffer.alloc(54);
  header.write('BM', 0, 'latin1');
  header.writeUInt32LE(54 + dataSize, 2);
  header.writeUInt32LE(54, 10);
  header.writeUInt32LE(40, 14);
  header.writeInt32LE(SIZE, 18);
  header.writeInt32LE(SIZE, 22);
  header.writeUInt16LE(1, 26);
  header.writeUInt16LE(24, 28);
  header.writeUInt32LE(dataSize, 34);

  const body = Buffer.alloc(dataSize);
  const row = Buffer.alloc(rowSize);
  for (let y = 0; y < SIZE; y++) {
    const src = (SIZE - 1 - y) * SIZE * 3;
    for (let x = 0; x < SIZE; x++) {
      row[x * 3] = rgbBuffer[src + x * 3 + 2];     // B
      row[x * 3 + 1] = rgbBuffer[src + x * 3 + 1]; // G
      row[x * 3 + 2] = rgbBuffer[src + x * 3];     // R
    }
    row.copy(body, y * rowSize);
  }
  fs.writeFileSync(filepath, Buffer.concat([header, body]));
}

writeBmp(path.join(OUTDIR, 'probe_albedo.bmp'), albedo);
writeBmp(path.join(OUTDIR, 'probe_normal.bmp'), normal);
writeBmp(path.join(OUTDIR, 'probe_rough.bmp'), rough);
writeBmp(path.join(OUTDIR, 'probe_metal.bmp'), metal);
writeBmp(path.join(OUTDIR, 'probe_emissive.bmp'), emissive);

// =========================================================================
// 2. 3D Mesh Generation (Indexed Wavefront OBJ)
// =========================================================================
const verts = []; // [x, y, z]
const uvs = [];   // [u, v]
const norms = []; // [nx, ny, nz]
const faces = []; // array of [[v, vt, vn], [v, vt, vn], [v, vt, vn]] (1-indexed)

function addVertex(x, y, z, u, v, nx, ny, nz) {
  verts.push([x, y, z]);
  uvs.push([u, v]);
  norms.push([nx, ny, nz]);
  return verts.length; // 1-based index
}

function addTri(i1, i2, i3) {
  faces.push([[i1, i1, i1], [i2, i2, i2], [i3, i3, i3]]);
}

function addQuad(i1, i2, i3, i4) {
  addTri(i1, i2, i3);
  addTri(i1, i3, i4);
}

// A. Central Bus: Octagonal prism (z from -0.35 to +0.35, r = 0.25)
const octN = 8;
const busR = 0.24;
const busZ0 = -0.32, busZ1 = 0.32;

// Bus side faces
for (let i = 0; i < octN; i++) {
  const a0 = (i / octN) * Math.PI * 2;
  const a1 = ((i + 1) / octN) * Math.PI * 2;
  const x0 = Math.cos(a0) * busR, y0 = Math.sin(a0) * busR;
  const x1 = Math.cos(a1) * busR, y1 = Math.sin(a1) * busR;
  const nx0 = Math.cos(a0), ny0 = Math.sin(a0);
  const nx1 = Math.cos(a1), ny1 = Math.sin(a1);

  const u0 = (i / octN) * 0.48;
  const u1 = ((i + 1) / octN) * 0.48;

  const v1 = addVertex(x0, y0, busZ0, u0, 0.52, nx0, ny0, 0);
  const v2 = addVertex(x1, y1, busZ0, u1, 0.52, nx1, ny1, 0);
  const v3 = addVertex(x1, y1, busZ1, u1, 0.98, nx1, ny1, 0);
  const v4 = addVertex(x0, y0, busZ1, u0, 0.98, nx0, ny0, 0);
  addQuad(v1, v2, v3, v4);
}

// Bus front/back endcaps
const frontCenter = addVertex(0, 0, busZ0, 0.25, 0.75, 0, 0, -1);
const backCenter = addVertex(0, 0, busZ1, 0.25, 0.75, 0, 0, 1);
for (let i = 0; i < octN; i++) {
  const a0 = (i / octN) * Math.PI * 2;
  const a1 = ((i + 1) / octN) * Math.PI * 2;
  const x0 = Math.cos(a0) * busR, y0 = Math.sin(a0) * busR;
  const x1 = Math.cos(a1) * busR, y1 = Math.sin(a1) * busR;

  const vf0 = addVertex(x0, y0, busZ0, 0.25 + Math.cos(a0)*0.2, 0.75 + Math.sin(a0)*0.2, 0, 0, -1);
  const vf1 = addVertex(x1, y1, busZ0, 0.25 + Math.cos(a1)*0.2, 0.75 + Math.sin(a1)*0.2, 0, 0, -1);
  addTri(frontCenter, vf0, vf1);

  const vb0 = addVertex(x0, y0, busZ1, 0.25 + Math.cos(a0)*0.2, 0.75 + Math.sin(a0)*0.2, 0, 0, 1);
  const vb1 = addVertex(x1, y1, busZ1, 0.25 + Math.cos(a1)*0.2, 0.75 + Math.sin(a1)*0.2, 0, 0, 1);
  addTri(backCenter, vb1, vb0);
}

// B. Parabolic High-Gain Dish Antenna:
// Center at (0, 0.15, -0.45), facing forward/up (-Z and +Y)
const dishCenter = [0.0, 0.18, -0.42];
const dishSegs = 24;
const dishR = 0.52;
const dishDepth = 0.14;

// Dish rings
const dishVertRings = [];
for (let rStep = 0; rStep <= 3; rStep++) {
  const ringFrac = rStep / 3.0;
  const curR = ringFrac * dishR;
  const curZ = ringFrac * ringFrac * dishDepth;
  const ring = [];
  for (let s = 0; s < dishSegs; s++) {
    const a = (s / dishSegs) * Math.PI * 2;
    const px = dishCenter[0] + Math.cos(a) * curR;
    const py = dishCenter[1] + Math.sin(a) * curR;
    const pz = dishCenter[2] + curZ;

    // Normal on parabola
    const nx = -Math.cos(a) * ringFrac * 0.8;
    const ny = -Math.sin(a) * ringFrac * 0.8;
    const nz = -1.0;
    const nLen = Math.hypot(nx, ny, nz);

    const u = 0.75 + Math.cos(a) * ringFrac * 0.22;
    const v = 0.75 + Math.sin(a) * ringFrac * 0.22;
    ring.push(addVertex(px, py, pz, u, v, nx/nLen, ny/nLen, nz/nLen));
  }
  dishVertRings.push(ring);
}
// Quads between dish rings
for (let r = 0; r < 3; r++) {
  for (let s = 0; s < dishSegs; s++) {
    const sNext = (s + 1) % dishSegs;
    addQuad(dishVertRings[r][s], dishVertRings[r][sNext], dishVertRings[r+1][sNext], dishVertRings[r+1][s]);
  }
}

// Sub-reflector feed horn (gold cone at dish focal point)
const hornTip = addVertex(dishCenter[0], dishCenter[1], dishCenter[2] - 0.22, 0.75, 0.75, 0, 0, 1);
for (let s = 0; s < 8; s++) {
  const a0 = (s / 8) * Math.PI * 2;
  const a1 = ((s + 1) / 8) * Math.PI * 2;
  const v0 = addVertex(dishCenter[0] + Math.cos(a0)*0.04, dishCenter[1] + Math.sin(a0)*0.04, dishCenter[2] - 0.12, 0.75, 0.75, Math.cos(a0), Math.sin(a0), 0.5);
  const v1 = addVertex(dishCenter[0] + Math.cos(a1)*0.04, dishCenter[1] + Math.sin(a1)*0.04, dishCenter[2] - 0.12, 0.75, 0.75, Math.cos(a1), Math.sin(a1), 0.5);
  addTri(hornTip, v0, v1);
}

// C. Solar Array Wings:
// Left wing (X = -0.32 to -1.45) & Right wing (X = +0.32 to +1.45)
for (const dir of [-1, 1]) {
  const xInner = 0.30 * dir;
  const xOuter = 1.45 * dir;
  const y = -0.04;
  const z0 = -0.20;
  const z1 = 0.28;

  // Front face (solar cells)
  const normY = 1.0;
  const f1 = addVertex(xInner, y + 0.008, z0, 0.02, 0.02, 0, normY, 0);
  const f2 = addVertex(xOuter, y + 0.008, z0, 0.48, 0.02, 0, normY, 0);
  const f3 = addVertex(xOuter, y + 0.008, z1, 0.48, 0.48, 0, normY, 0);
  const f4 = addVertex(xInner, y + 0.008, z1, 0.02, 0.48, 0, normY, 0);
  if (dir > 0) addQuad(f1, f2, f3, f4);
  else addQuad(f2, f1, f4, f3);

  // Back face (substrate)
  const b1 = addVertex(xInner, y - 0.008, z0, 0.75, 0.25, 0, -normY, 0);
  const b2 = addVertex(xOuter, y - 0.008, z0, 0.95, 0.25, 0, -normY, 0);
  const b3 = addVertex(xOuter, y - 0.008, z1, 0.95, 0.45, 0, -normY, 0);
  const b4 = addVertex(xInner, y - 0.008, z1, 0.75, 0.45, 0, -normY, 0);
  if (dir > 0) addQuad(b4, b3, b2, b1);
  else addQuad(b3, b4, b1, b2);

  // Wingtip beacon strobe
  const bTip1 = addVertex(xOuter, y + 0.03, z1 + 0.02, 0.60, 0.15, dir, 1, 0);
  const bTip2 = addVertex(xOuter, y - 0.03, z1 + 0.02, 0.60, 0.15, dir, -1, 0);
  const bTip3 = addVertex(xOuter + 0.04 * dir, y, z1 + 0.02, 0.60, 0.15, dir, 0, 1);
  addTri(bTip1, bTip2, bTip3);
}

// D. Science Magnetometer Boom extending backwards:
const boomZ0 = busZ1;
const boomZ1 = 1.15;
const bTruss1 = addVertex(-0.02, -0.02, boomZ0, 0.7, 0.3, -1, -1, 0);
const bTruss2 = addVertex( 0.02, -0.02, boomZ0, 0.7, 0.3,  1, -1, 0);
const bTruss3 = addVertex( 0.02,  0.02, boomZ0, 0.7, 0.3,  1,  1, 0);
const bTruss4 = addVertex(-0.02,  0.02, boomZ0, 0.7, 0.3, -1,  1, 0);

const bTipV1 = addVertex(-0.015, -0.015, boomZ1, 0.7, 0.4, -1, -1, 0);
const bTipV2 = addVertex( 0.015, -0.015, boomZ1, 0.7, 0.4,  1, -1, 0);
const bTipV3 = addVertex( 0.015,  0.015, boomZ1, 0.7, 0.4,  1,  1, 0);
const bTipV4 = addVertex(-0.015,  0.015, boomZ1, 0.7, 0.4, -1,  1, 0);
addQuad(bTruss1, bTruss2, bTipV2, bTipV1);
addQuad(bTruss2, bTruss3, bTipV3, bTipV2);
addQuad(bTruss3, bTruss4, bTipV4, bTipV3);
addQuad(bTruss4, bTruss1, bTipV1, bTipV4);

// Magnetometer Sensor canister at boom tip
const magTipCenter = addVertex(0, 0, boomZ1 + 0.08, 0.6, 0.6, 0, 0, 1);
for (let s = 0; s < 6; s++) {
  const a0 = (s / 6) * Math.PI * 2;
  const a1 = ((s + 1) / 6) * Math.PI * 2;
  const mv0 = addVertex(Math.cos(a0)*0.04, Math.sin(a0)*0.04, boomZ1, 0.6, 0.6, Math.cos(a0), Math.sin(a0), 0);
  const mv1 = addVertex(Math.cos(a1)*0.04, Math.sin(a1)*0.04, boomZ1, 0.6, 0.6, Math.cos(a1), Math.sin(a1), 0);
  addTri(magTipCenter, mv0, mv1);
}

// Write Wavefront OBJ file
const objLines = [];
objLines.push('# Deep Space Relay & Science Probe');
objLines.push('mtllib probe.mtl');
objLines.push('o SpaceProbe');

for (const v of verts) {
  objLines.push(`v ${v[0].toFixed(5)} ${v[1].toFixed(5)} ${v[2].toFixed(5)}`);
}
for (const vt of uvs) {
  objLines.push(`vt ${vt[0].toFixed(5)} ${vt[1].toFixed(5)}`);
}
for (const vn of norms) {
  objLines.push(`vn ${vn[0].toFixed(5)} ${vn[1].toFixed(5)} ${vn[2].toFixed(5)}`);
}

objLines.push('usemtl ProbeMat');
objLines.push('s 1');
for (const f of faces) {
  objLines.push(`f ${f[0][0]}/${f[0][1]}/${f[0][2]} ${f[1][0]}/${f[1][1]}/${f[1][2]} ${f[2][0]}/${f[2][1]}/${f[2][2]}`);
}

const objPath = path.join(OUTDIR, 'probe.obj');
fs.writeFileSync(objPath, objLines.join('\n') + '\n');

console.log(JSON.stringify({
  status: 'OK',
  vertices: verts.length,
  faces: faces.length,
  obj: objPath,
  albedo: path.join(OUTDIR, 'probe_albedo.bmp'),
  normal: path.join(OUTDIR, 'probe_normal.bmp'),
  rough: path.join(OUTDIR, 'probe_rough.bmp'),
  metal: path.join(OUTDIR, 'probe_metal.bmp'),
  emissive: path.join(OUTDIR, 'probe_emissive.bmp')
}, null, 2));
