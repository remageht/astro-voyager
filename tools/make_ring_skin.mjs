#!/usr/bin/env node
// tools/make_ring_skin.mjs — generate the tileable Ring Ship skin.
//
// The procedural Ring Ship (style 1) is built from unit spheres and rings with
// no UVs, so the skin is sampled triplanar in ship space (see shaders/ship.frag,
// u_SkinOn). That needs a *tileable* texture: this tool paints one 4x4 panel
// tile with seams, rivets, per-panel tone, streaks and grime, and writes it as
// 24-bit BMP (the format src/ObjModel.cpp already reads) plus a matching
// roughness map.
//
// Usage:
//   node tools/make_ring_skin.mjs [out_dir] [--size 512] [--name ring_skin]
//
// Output: <out_dir>/<name>_albedo.bmp, <out_dir>/<name>_rough.bmp
// No dependencies: everything is periodic, so the tile wraps seamlessly.

import fs from 'node:fs';
import path from 'node:path';

const argv = process.argv.slice(2);
const flags = {};
for (let i = 0; i < argv.length; i++) {
  if (!argv[i].startsWith('--')) continue;
  const k = argv[i].slice(2);
  const v = argv[i + 1];
  flags[k] = v === undefined || v.startsWith('--') ? '1' : v;
}
const OUTDIR = argv[0] && !argv[0].startsWith('--') ? argv[0] : '.';
const SIZE = flags.size && flags.size !== '1' ? Number(flags.size) : 512;
const NAME = flags.name && flags.name !== '1' ? flags.name : 'ring_skin';

const PANELS = 4;              // panelling per tile edge
const f = (x) => x - Math.floor(x);
const clamp01 = (x) => (x < 0 ? 0 : x > 1 ? 1 : x);
const mix = (a, b, t) => a + (b - a) * t;
const smooth = (a, b, x) => {
  const t = clamp01((x - a) / (b - a));
  return t * t * (3 - 2 * t);
};
// Deterministic hash -> [0,1), periodic because it only ever sees cell indices.
const hash2 = (x, y) => {
  const s = Math.sin(x * 127.1 + y * 311.7) * 43758.5453123;
  return f(s);
};

// Panel grid: returns { cellX, cellY, seam } with seam in [0,1] (1 = panel body).
function panelGrid(u, v, panels) {
  const pu = u * panels, pv = v * panels;
  const fu = f(pu), fv = f(pv);
  const du = Math.min(fu, 1 - fu), dv = Math.min(fv, 1 - fv);
  const d = Math.min(du, dv);                        // distance to seam, tiles
  const seam = smooth(0.006, 0.030, d);              // thin groove
  return { cellX: Math.floor(pu), cellY: Math.floor(pv), seam, du, dv };
}

const albedo = Buffer.alloc(SIZE * SIZE * 3);
const rough = Buffer.alloc(SIZE * SIZE * 3);

for (let y = 0; y < SIZE; y++) {
  for (let x = 0; x < SIZE; x++) {
    const u = (x + 0.5) / SIZE;
    const v = (y + 0.5) / SIZE;

    // --- primary panelling ---
    const g = panelGrid(u, v, PANELS);
    const tone = hash2(g.cellX, g.cellY);            // per-panel paint tone
    const seam = g.seam;

    // --- secondary sub-panelling (hatches), lower contrast ---
    const g2 = panelGrid(u * 2 + 0.37, v * 2 + 0.11, PANELS);
    const seam2 = g2.seam;

    // --- rivets near the panel corners ---
    const pu = u * PANELS, pv = v * PANELS;
    const cu = f(pu), cv = f(pv);
    const cornerD = Math.hypot(cu - 0.5, cv - 0.5);
    const rivet = 1 - smooth(0.012, 0.045, Math.abs(cornerD - 0.46));

    // --- vertical grime streaks (wrap on x) ---
    const streakSeed = hash2(Math.floor(f(u) * 64), 0);
    const streak = smooth(0.55, 0.95,
      streakSeed * 0.6 + 0.4 * Math.sin((v + streakSeed) * 6.283 * 2));
    const dirt = 1 - 0.22 * streak;

    // --- fine noise / scratches ---
    const n = hash2(Math.floor(u * SIZE / 2), Math.floor(v * SIZE / 2)) * 0.06 +
              hash2(Math.floor(u * SIZE / 6), Math.floor(v * SIZE / 6)) * 0.04;

    // --- albedo (display-referred, sRGB-ish) ---
    let r = 0.56, gg = 0.59, b = 0.64;               // painted steel
    const toneK = 0.94 + 0.12 * tone;
    r *= toneK; gg *= toneK; b *= toneK;
    const seamK = mix(0.50, 1.0, seam) * mix(0.86, 1.0, seam2);
    r *= seamK; gg *= seamK; b *= seamK;
    const rivetK = 1 - 0.55 * rivet;
    r *= rivetK; gg *= rivetK; b *= rivetK;
    r *= dirt; gg *= dirt; b *= dirt;
    r += n; gg += n; b += n;
    // navy decal stripe along one panel row (gives the skin a "livery")
    const stripe = (Math.floor(pv) % PANELS === 1) && seam > 0.5
      ? smooth(0.42, 0.46, f(pv)) * (1 - smooth(0.54, 0.58, f(pv)))
      : 0;
    r = mix(r, 0.14, stripe * 0.85);
    gg = mix(gg, 0.18, stripe * 0.85);
    b = mix(b, 0.26, stripe * 0.85);

    const i3 = (y * SIZE + x) * 3;
    albedo[i3] = Math.round(clamp01(r) * 255);
    albedo[i3 + 1] = Math.round(clamp01(gg) * 255);
    albedo[i3 + 2] = Math.round(clamp01(b) * 255);

    // --- roughness: seams and dirt rougher, rivets slightly smoother ---
    let ro = 0.42;
    ro += (1 - seam) * 0.30;
    ro += (1 - seam2) * 0.08;
    ro += 0.16 * streak;
    ro -= 0.10 * rivet;
    ro += n * 0.8;
    ro = clamp01(ro);
    const rv = Math.round(ro * 255);
    rough[i3] = rv; rough[i3 + 1] = rv; rough[i3 + 2] = rv;
  }
}

// 24-bit bottom-up BMP (what src/ObjModel.cpp's loader expects).
function writeBmp(file, rgb) {
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
    const src = (SIZE - 1 - y) * SIZE * 3;   // BMP rows are bottom-up
    for (let x = 0; x < SIZE; x++) {
      row[x * 3] = rgb[src + x * 3 + 2];     // BGR
      row[x * 3 + 1] = rgb[src + x * 3 + 1];
      row[x * 3 + 2] = rgb[src + x * 3];
    }
    row.copy(body, y * rowSize);
  }
  fs.writeFileSync(file, Buffer.concat([header, body]));
}

fs.mkdirSync(OUTDIR, { recursive: true });
const albedoPath = path.join(OUTDIR, `${NAME}_albedo.bmp`);
const roughPath = path.join(OUTDIR, `${NAME}_rough.bmp`);
writeBmp(albedoPath, albedo);
writeBmp(roughPath, rough);
console.log(JSON.stringify({
  size: SIZE, panelsPerTile: PANELS,
  albedo: albedoPath, rough: roughPath,
  kb: Math.round(fs.statSync(albedoPath).size / 1024),
}, null, 2));
