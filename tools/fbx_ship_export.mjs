#!/usr/bin/env node
// tools/fbx_ship_export.mjs вЂ” Tripo/AI-generated FBX -> game-ready OBJ ship
// plus its PBR texture set (BMP, so the runtime needs no image decoder).
//
// What it does:
//   * reads a binary FBX (7400/7500) with no external dependencies;
//   * takes the first mesh, triangulates it, bakes a deterministic orientation
//     (centered origin, longest axis -> Z, nose -> -Z, uniform scale to
//     --length) and decimates by vertex clustering;
//   * clustering is UV-aware: corners that land in the same spatial cell but
//     belong to different UV islands stay split, so texture seams survive;
//   * recomputes smooth normals, writes an indexed OBJ with v/vt/vn;
//   * pulls the embedded PBR texture clips (Color / NormalGL / ORM_g / ORM_b)
//     out of the FBX and converts them to 24-bit BMP at --tex-size for
//     src/ObjModel.cpp (no stb_image, no zlib: BMP is read directly).
//
// Usage:
//   node tools/fbx_ship_export.mjs <model.fbx> [out_dir]
//        [--cells 384] [--length 2.3] [--name warp_ship]
//        [--tex-size 1024] [--uv-tol 0.01] [--no-tex]
//
// Texture output (out_dir):
//   <name>_albedo.bmp  sRGB base color
//   <name>_normal.bmp  tangent-space normal (OpenGL +Y)
//   <name>_rough.bmp   grayscale roughness (green channel of the ORM map)
//   <name>_metal.bmp   grayscale metallic  (blue channel of the ORM map)
//
// Requires Node >= 18. The FBX side is pure JS; the JPEG/PNG -> BMP step uses
// Windows PowerShell + System.Drawing (--no-tex skips it, e.g. on CI).

import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

// ---------- CLI ----------
const argv = process.argv.slice(2);
const IN = argv[0];
if (!IN || IN.startsWith('--')) {
  console.error(
    'usage: node tools/fbx_ship_export.mjs <model.fbx> [outdir] ' +
    '[--cells 384] [--length 2.3] [--name warp_ship] [--tex-size 1024] [--no-tex]');
  process.exit(2);
}
const flags = {};
for (let i = 1; i < argv.length; i++) {
  if (!argv[i].startsWith('--')) continue;
  const key = argv[i].slice(2);
  const next = argv[i + 1];
  flags[key] = next === undefined || next.startsWith('--') ? '1' : next;
}
const num = (k, d) => (flags[k] === undefined || flags[k] === '1' ? d : Number(flags[k]));
const OUTDIR = argv[1] && !argv[1].startsWith('--') ? argv[1] : '.';
const CELLS = num('cells', 384);
const LENGTH = num('length', 2.3);
const NAME = flags.name && flags.name !== '1' ? flags.name : 'warp_ship';
const TEX_SIZE = num('tex-size', 1024);
const WANT_TEX = flags['no-tex'] === undefined;

fs.mkdirSync(OUTDIR, { recursive: true });
const OUT = path.join(OUTDIR, `${NAME}.obj`);

// ---------- FBX binary parser (handles v7400 & v7500 node records) ----------
const buf = fs.readFileSync(IN);
const MAGIC = Buffer.from('Kaydara FBX Binary  \x00\x1A\x00', 'latin1');
const MAGIC_AT = buf.indexOf(MAGIC);
if (MAGIC_AT < 0) {
  console.error('not a binary FBX file');
  process.exit(1);
}
const VERSION = buf.readUInt32LE(MAGIC_AT + MAGIC.length);
const big = VERSION >= 7500;
const STEP = big ? 8 : 4;
const u = a => Number(big ? buf.readBigUInt64LE(a) : buf.readUInt32LE(a));

function readProp(q) {
  const t = String.fromCharCode(buf[q++]);
  if (t === 'Y') return { v: buf.readInt16LE(q), n: q + 2, t };
  if (t === 'C') return { v: buf[q] !== 0, n: q + 1, t };
  if (t === 'I') return { v: buf.readInt32LE(q), n: q + 4, t };
  if (t === 'F') return { v: buf.readFloatLE(q), n: q + 4, t };
  if (t === 'D') return { v: buf.readDoubleLE(q), n: q + 8, t };
  if (t === 'L') return { v: Number(buf.readBigInt64LE(q)), n: q + 8, t };
  if ('fdlib'.includes(t)) {
    const len = u(q), enc = u(q + 4), comp = u(q + 8);
    q += 12;
    let raw = buf.subarray(q, q + comp);
    if (enc === 1) raw = zlib.inflateSync(raw);
    let v;
    const dv = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    if (t === 'd') { v = new Float32Array(len); for (let i = 0; i < len; i++) v[i] = dv.getFloat64(i * 8, true); }
    if (t === 'f') { v = new Float32Array(len); for (let i = 0; i < len; i++) v[i] = dv.getFloat32(i * 4, true); }
    if (t === 'i') { v = new Int32Array(len); for (let i = 0; i < len; i++) v[i] = dv.getInt32(i * 4, true); }
    if (t === 'l') { v = new Array(len); for (let i = 0; i < len; i++) v[i] = Number(dv.getBigInt64(i * 8, true)); }
    if (t === 'b') { v = []; for (let i = 0; i < len; i++) v.push(raw[i] !== 0); }
    return { v, n: q + comp, t, len, raw };
  }
  if (t === 'S' || t === 'R') {
    const len = buf.readUInt32LE(q); q += 4;
    const v = buf.subarray(q, q + len); q += len;
    return { v, n: q, t };
  }
  throw new Error('bad FBX property type ' + t + ' at ' + (q - 1));
}
function readNode(start) {
  let q = start;
  const endOff = u(q); q += STEP;
  const numProps = u(q); q += STEP;
  q += STEP; // property list byte length
  const nameLen = buf[q]; q += 1;
  const name = buf.subarray(q, q + nameLen).toString('utf8'); q += nameLen;
  const props = [];
  for (let i = 0; i < numProps; i++) { const pr = readProp(q); props.push(pr); q = pr.n; }
  const node = { name, props, children: [], end: endOff };
  if (endOff === 0 || endOff <= q) { node.end = q; return node; }
  while (q < endOff) {
    if (u(q) === 0) break;
    const c = readNode(q);
    node.children.push(c);
    q = c.end;
  }
  node.end = endOff;
  return node;
}
const roots = [];
{
  let q = MAGIC_AT + MAGIC.length + 4;
  while (q < buf.length - STEP && u(q) !== 0) {
    const n = readNode(q);
    roots.push(n);
    q = n.end;
  }
}
const childrenNamed = (n, nm) => n.children.filter(c => c.name === nm);
const firstChild = (n, nm) => n.children.find(c => c.name === nm);
const strOf = n => (n && n.props.length ? n.props[n.props.length - 1].v.toString('utf8') : null);

// ---------- mesh ----------
const objects = roots.find(r => r.name === 'Objects');
if (!objects) { console.error('no Objects section in FBX'); process.exit(1); }
const geometry = childrenNamed(objects, 'Geometry').find(g => firstChild(g, 'Vertices'));
if (!geometry) { console.error('no Geometry/Vertices in FBX'); process.exit(1); }

const arrOf = (n, nm) => {
  const c = n && firstChild(n, nm);
  return c && c.props.length ? c.props[0].v : null;
};
const V = arrOf(geometry, 'Vertices');
const PVI = arrOf(geometry, 'PolygonVertexIndex');
if (!V || !PVI) { console.error('geometry has no Vertices/PolygonVertexIndex'); process.exit(1); }

const uvLayer = firstChild(geometry, 'LayerElementUV');
const uvRef = uvLayer ? (firstChild(uvLayer, 'ReferenceInformationType')?.props[0]?.v?.toString?.() ?? 'Direct') : null;
const uvMapMode = uvLayer ? (firstChild(uvLayer, 'MappingInformationType')?.props[0]?.v?.toString?.() ?? '') : null;
const UV = uvLayer ? arrOf(uvLayer, 'UV') : null;
const UVIDX = uvLayer ? arrOf(uvLayer, 'UVIndex') : null;
const hasUV = !!(UV && UV.length);

// ---------- extract embedded textures ----------
const textureClips = [];
(function walk(n) {
  if (n.name === 'Video') {
    const content = firstChild(n, 'Content');
    const file = firstChild(n, 'RelativeFilename') || firstChild(n, 'Filename');
    if (content && content.props.length) {
      const p = content.props[0];
      const raw = Buffer.isBuffer(p.v) ? p.v : p.v.raw;
      if (raw && raw.length) {
        textureClips.push({ name: file ? file.props[0].v.toString('utf8') : 'clip.bin', raw });
      }
    }
  }
  for (const c of n.children) walk(c);
})({ name: 'root', children: roots });

// ---------- triangulate ----------
const faces = [];
{
  let idx = [];
  for (let i = 0; i < PVI.length; i++) {
    const v = PVI[i];
    if (v < 0) { idx.push(-v - 1); faces.push(idx); idx = []; }
    else idx.push(v);
  }
  if (idx.length >= 3) faces.push(idx);
}

// ---------- UV per source vertex (or per polygon-vertex) ----------
// ReferenceInformationType=IndexToDirect -> UV[UVIndex[c]]; Direct -> UV[c].
const uvAt = (cornerIndex) => {
  if (!hasUV) return [0, 0];
  const k = (uvRef === 'IndexToDirect' && UVIDX) ? UVIDX[cornerIndex] : cornerIndex;
  if (k === undefined || k < 0 || k * 2 + 1 >= UV.length) return [0, 0];
  return [UV[k * 2], UV[k * 2 + 1]];
};

// ---------- bake: center + longest axis -> Z + nose -> -Z + scale ----------
const N = V.length / 3 | 0;
const px = new Float32Array(N), py = new Float32Array(N), pz = new Float32Array(N);
for (let i = 0; i < N; i++) { px[i] = V[i * 3]; py[i] = V[i * 3 + 1]; pz[i] = V[i * 3 + 2]; }
const mn = [Infinity, Infinity, Infinity], mx = [-Infinity, -Infinity, -Infinity];
for (let i = 0; i < N; i++) {
  if (px[i] < mn[0]) mn[0] = px[i]; if (px[i] > mx[0]) mx[0] = px[i];
  if (py[i] < mn[1]) mn[1] = py[i]; if (py[i] > mx[1]) mx[1] = py[i];
  if (pz[i] < mn[2]) mn[2] = pz[i]; if (pz[i] > mx[2]) mx[2] = pz[i];
}
const L = [mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]];
const C = [(mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2, (mn[2] + mx[2]) / 2];
const lenAxis = L[0] > L[1] && L[0] > L[2] ? 0 : L[1] > L[2] ? 1 : 2;
const scale = LENGTH / L[lenAxis];

// Which end is the nose? Mean radial distance from the length axis in the
// outer 9%: the engine cluster (nozzles) has a bigger radius than the spike.
function endRadius(sign) {
  const acc = { n: 0, s: 0 };
  const limit = C[lenAxis] + sign * (L[lenAxis] / 2 - L[lenAxis] * 0.09);
  for (let i = 0; i < N; i++) {
    const a = [px[i], py[i], pz[i]];
    if (sign > 0 ? a[lenAxis] < limit : a[lenAxis] > limit) continue;
    const dv = [a[0] - C[0], a[1] - C[1], a[2] - C[2]];
    dv[lenAxis] = 0;
    acc.s += Math.hypot(dv[0], dv[1], dv[2]);
    acc.n++;
  }
  return acc.n ? acc.s / acc.n : 0;
}
const rPos = endRadius(+1), rNeg = endRadius(-1);
const noseAtMax = rPos < rNeg;
const flipK = noseAtMax ? +1 : -1;
const axIndex = { 0: [1, 2], 1: [0, 2], 2: [0, 1] }[lenAxis];

function bake(i) {
  const a = [px[i] - C[0], py[i] - C[1], pz[i] - C[2]];
  const out = [0, 0, 0];
  out[lenAxis] = -a[lenAxis] * flipK;
  out[axIndex[0]] = a[axIndex[0]];
  out[axIndex[1]] = a[axIndex[1]];
  return [out[0] * scale, out[1] * scale, out[2] * scale];
}

const baked = new Float32Array(N * 3);
for (let i = 0; i < N; i++) {
  const b = bake(i);
  baked[i * 3] = b[0]; baked[i * 3 + 1] = b[1]; baked[i * 3 + 2] = b[2];
}

// ---------- fan-triangulate, keeping each corner's UV ----------
// The UV layer indexes polygon corners (not triangles), so walk polygons in
// order with an explicit corner counter.
const cornerV = [];   // source vertex index
const cornerUV = [];  // uv for that corner
const triStart = [];  // triangle corner offsets into cornerV
{
  let corner = 0;
  for (const poly of faces) {
    const polyCorners = [];
    for (let i = 0; i < poly.length; i++) {
      polyCorners.push({ v: poly[i], uv: uvAt(corner + i) });
    }
    for (let i = 2; i < poly.length; i++) {
      triStart.push(cornerV.length);
      const tri = [polyCorners[0], polyCorners[i - 1], polyCorners[i]];
      for (const t of tri) { cornerV.push(t.v); cornerUV.push(t.uv); }
    }
    corner += poly.length;
  }
}

// ---------- decimate: cluster corners into a uniform grid, keep topology ----------
// A spatial cell alone would merge corners that belong to *different* UV
// islands meeting at a seam вЂ” their averaged UV lands in an atlas gap and the
// hull gets speckled. So each cell keeps a handful of subgroups: a corner
// joins the first subgroup whose running mean UV is within UV_TOL, otherwise
// it opens a new one. Smooth UVs collapse to one vertex per cell, seams get a
// second vertex at (nearly) the same position вЂ” exactly what UV seams need.
const dmax = Math.max(L[0], L[1], L[2]) * scale;
const h = dmax / CELLS;
const UV_TOL = num('uv-tol', 0.01); // atlas units; 1024px texture -> ~10 texels
const cellKeyOf = (i) =>
  (Math.round(baked[i * 3] / h) + 8192) * 134217728 +
  (Math.round(baked[i * 3 + 1] / h) + 8192) * 8192 +
  (Math.round(baked[i * 3 + 2] / h) + 8192);

const cells = new Map(); // cellKey -> [{ id, u, v }] subgroup representatives
const vCount = [];       // corners per id (cluster weight)
const vSum = [];         // position sums (baked space)
const uvSum = [];        // uv sums
const nSum = [];         // accumulated face normals

function idFor(i, uv) {
  const ck = cellKeyOf(i);
  let groups = cells.get(ck);
  if (!groups) { groups = []; cells.set(ck, groups); }
  if (hasUV) {
    for (const g of groups) {
      if (Math.abs(g.u - uv[0]) <= UV_TOL && Math.abs(g.v - uv[1]) <= UV_TOL) {
        const n = vCount[g.id];
        g.u += (uv[0] - g.u) / (n + 1);
        g.v += (uv[1] - g.v) / (n + 1);
        return g.id;
      }
    }
  }
  const id = vCount.length;
  vCount.push(0);
  vSum.push([0, 0, 0]);
  uvSum.push([0, 0]);
  nSum.push([0, 0, 0]);
  groups.push({ id, u: uv[0], v: uv[1] });
  return id;
}

// corner -> cluster id, accumulating position and uv as we go (corner-weighted:
// every corner of a cluster sits inside the same voxel, so the bias is sub-voxel)
const cornerId = new Int32Array(cornerV.length);
for (let c = 0; c < cornerV.length; c++) {
  const i = cornerV[c];
  const uv = cornerUV[c];
  const id = idFor(i, uv);
  cornerId[c] = id;
  vCount[id]++;
  vSum[id][0] += baked[i * 3];
  vSum[id][1] += baked[i * 3 + 1];
  vSum[id][2] += baked[i * 3 + 2];
  if (hasUV) { uvSum[id][0] += uv[0]; uvSum[id][1] += uv[1]; }
}

// average positions up front: face normals AND emitted vertices both need the
// averaged point, not the corner sum (sums carry a per-cluster weight bias).
const avgPos = vSum.map((s, i) => {
  const n = Math.max(vCount[i], 1);
  return [s[0] / n, s[1] / n, s[2] / n];
});

// triangles + face normals per cluster
const tris = [];
const fn = [0, 0, 0];
for (let t = 0; t < triStart.length; t++) {
  const a = cornerId[triStart[t]], b = cornerId[triStart[t] + 1], c = cornerId[triStart[t] + 2];
  if (a === b || b === c || a === c) continue;
  const pa = avgPos[a], pb = avgPos[b], pc = avgPos[c];
  // (sums are unnormalized but scale-invariant for the normal direction)
  const ux = pb[0] - pa[0], uy = pb[1] - pa[1], uz = pb[2] - pa[2];
  const wx = pc[0] - pa[0], wy = pc[1] - pa[1], wz = pc[2] - pa[2];
  fn[0] = uy * wz - uz * wy;
  fn[1] = uz * wx - ux * wz;
  fn[2] = ux * wy - uy * wx;
  for (const id of [a, b, c]) {
    const s = nSum[id];
    // Гейт: противоположно-направленные грани (тонкие пластины: перед+зад)
    // в сумме гасят друг друга — усреднённая нормаль вырождается в
    // случайный фолбэк [0,0,1] и рвёт rim/блики. Доминирующую сторону
    // оставляем: догасившиеся кластеры всё равно редки.
    const cur = s[0] * fn[0] + s[1] * fn[1] + s[2] * fn[2];
    if (cur < 0 && Math.hypot(s[0], s[1], s[2]) > 1e-12) continue;
    s[0] += fn[0]; s[1] += fn[1]; s[2] += fn[2];
  }
  tris.push(a, b, c);
}

// finalize: average positions, normalize normals, drop unused clusters
const remap = new Int32Array(vCount.length).fill(-1);
const outV = [], outVT = [], outN = [];
for (const id of new Set(tris)) {
  if (remap[id] >= 0) continue;
  const n = Math.max(vCount[id], 1);
  const [x, y, z] = avgPos[id];
  remap[id] = outV.length;
  outV.push([x, y, z]);
  const nn = nSum[id];
  const l = Math.hypot(nn[0], nn[1], nn[2]);
  outN.push(l > 1e-9 ? [nn[0] / l, nn[1] / l, nn[2] / l] : [0, 0, 1]);
  outVT.push([uvSum[id][0] / n, uvSum[id][1] / n]);
}

// ---------- emit indexed OBJ ----------
const f = (v) => v.toFixed(5);
const lines = [];
lines.push(`# ${NAME}.obj - converted by tools/fbx_ship_export.mjs`);
lines.push(`# source: ${path.basename(IN)}`);
lines.push(`# baked: centered origin, length axis -> Z, nose -> -Z, length=${LENGTH.toFixed(3)}, grid cells=${CELLS}`);
lines.push(`# layers: position${hasUV ? ' + uv' : ''} + normal`);
for (const v of outV) lines.push(`v ${f(v[0])} ${f(v[1])} ${f(v[2])}`);
if (hasUV) for (const t of outVT) lines.push(`vt ${t[0].toFixed(5)} ${t[1].toFixed(5)}`);
for (const n of outN) lines.push(`vn ${n[0].toFixed(4)} ${n[1].toFixed(4)} ${n[2].toFixed(4)}`);
for (let i = 0; i < tris.length; i += 3) {
  const a = remap[tris[i]] + 1, b = remap[tris[i + 1]] + 1, c = remap[tris[i + 2]] + 1;
  lines.push(hasUV ? `f ${a}/${a}/${a} ${b}/${b}/${b} ${c}/${c}/${c}`
                   : `f ${a}//${a} ${b}//${b} ${c}//${c}`);
}
fs.writeFileSync(OUT, lines.join('\n') + '\n');

// ---------- textures -> BMP ----------
const texReport = {};
if (WANT_TEX && textureClips.length) {
  const kindOf = (filename) => {
    const n = filename.toLowerCase();
    if (n.includes('normal')) return 'normal';
    if (n.includes('orm')) return n.includes('_b') ? 'metal' : 'rough';
    if (n.includes('color') || n.includes('albedo') || n.includes('diffuse') || n.includes('basecolor')) return 'albedo';
    return null;
  };
  const tmp = path.join(OUTDIR, '.fbx_tex');
  fs.mkdirSync(tmp, { recursive: true });
  const here = path.dirname(fileURLToPath(import.meta.url));
  const convScript = path.join(here, 'tex_to_bmp.ps1');
  const shells = ['pwsh', 'powershell'];
  for (const clip of textureClips) {
    const kind = kindOf(clip.name);
    if (!kind) { texReport[clip.name] = 'skipped (unknown role)'; continue; }
    const ext = path.extname(clip.name) || '.bin';
    const rawPath = path.join(tmp, `src_${kind}${ext}`);
    fs.writeFileSync(rawPath, clip.raw);
    const dstPath = path.join(OUTDIR, `${NAME}_${kind}.bmp`);
    let ok = false;
    for (const shell of shells) {
      const r = spawnSync(shell,
        ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', convScript,
         '-Source', rawPath, '-Dest', dstPath, '-Size', String(TEX_SIZE)],
        { stdio: 'inherit' });
      if (!r.error && r.status === 0) { ok = true; break; }
      if (r.error && r.error.code !== 'ENOENT') break;
    }
    texReport[kind] = ok ? path.basename(dstPath) : 'conversion failed (no PowerShell/System.Drawing)';
  }
  fs.rmSync(tmp, { recursive: true, force: true });
}

// ---------- livery: paint the raw clip set into a real-hull look ----------
// The raw Tripo atlas is an unpainted mock. Every decimated face is
// rasterized into its own UV triangle and colored by its ship-space
// position: two-tone hull, graphite belly, ring accent + cyan glow rim,
// charcoal engine block, window strips, amber trim, exhaust glow. Panel
// detail survives: the source albedo sample multiplies the zone tint.
const LIVERY = flags['no-livery'] === undefined;
let liveryReport = null;

if (LIVERY && hasUV && WANT_TEX) {
  const S = TEX_SIZE;

  const readBmp24 = (p) => {
    const b = fs.readFileSync(p);
    let off = b.readUInt32LE(10);
    let w = b.readInt32LE(18);
    let h = b.readInt32LE(22);
    if (b.readUInt16LE(28) !== 24) throw new Error(`not 24bpp: ${p}`);
    const topDown = h < 0;
    if (topDown) h = -h;
    const stride = ((w * 3 + 3) >> 2) << 2;
    const rgb = Buffer.alloc(w * h * 3);
    for (let y = 0; y < h; y++) {
      const srcRow = topDown ? y : (h - 1 - y);
      b.copy(rgb, y * w * 3, off + srcRow * stride, off + srcRow * stride + w * 3);
    }
    return { w, h, rgb };
  };
  const writeBmp24 = (p, cv) => {
    const { w, h, rgb } = cv;
    const stride = ((w * 3 + 3) >> 2) << 2;
    const size = 54 + stride * h;
    const b = Buffer.alloc(size);
    b.write('BM', 0, 'latin1');
    b.writeUInt32LE(size, 2);
    b.writeUInt32LE(54, 10);
    b.writeUInt32LE(40, 14);
    b.writeInt32LE(w, 18);
    b.writeInt32LE(h, 22);
    b.writeUInt16LE(1, 26);
    b.writeUInt16LE(24, 28);
    b.writeUInt32LE(stride * h, 34);
    for (let y = 0; y < h; y++) {
      const d = 54 + (h - 1 - y) * stride;
      for (let x = 0; x < w; x++) {
        const si = (y * w + x) * 3;
        const di = d + x * 3;
        b[di] = rgb[si + 2];      // BMP хранит BGR
        b[di + 1] = rgb[si + 1];
        b[di + 2] = rgb[si];
      }
    }
    fs.writeFileSync(p, b);
  };

  const pAlbedo = path.join(OUTDIR, `${NAME}_albedo.bmp`);
  const pRough = path.join(OUTDIR, `${NAME}_rough.bmp`);
  const pMetal = path.join(OUTDIR, `${NAME}_metal.bmp`);
  if (!fs.existsSync(pAlbedo)) {
    console.error('livery: no albedo BMP to paint over, skipped');
  } else {
    const alb = readBmp24(pAlbedo);
    const rgh = fs.existsSync(pRough) ? readBmp24(pRough) : null;
    const mtl = fs.existsSync(pMetal) ? readBmp24(pMetal) : null;
    const emis = { w: S, h: S, rgb: Buffer.alloc(S * S * 3) };
    const mask = new Uint8Array(S * S);

    const sample = (cv, u, v, out) => {
      const fx = Math.min(Math.max(u, 0), 1) * (cv.w - 1);
      const fy = (1 - Math.min(Math.max(v, 0), 1)) * (cv.h - 1);
      const x0 = Math.floor(fx), y0 = Math.floor(fy);
      const x1 = Math.min(x0 + 1, cv.w - 1), y1 = Math.min(y0 + 1, cv.h - 1);
      const tx = fx - x0, ty = fy - y0;
      for (let c = 0; c < 3; c++) {
        const p00 = cv.rgb[(y0 * cv.w + x0) * 3 + c];
        const p10 = cv.rgb[(y0 * cv.w + x1) * 3 + c];
        const p01 = cv.rgb[(y1 * cv.w + x0) * 3 + c];
        const p11 = cv.rgb[(y1 * cv.w + x1) * 3 + c];
        out[c] = (p00 * (1 - tx) + p10 * tx) * (1 - ty) +
                 (p01 * (1 - tx) + p11 * tx) * ty;
      }
    };

    // Face-space stats for the ring threshold: hull radius vs ring radius.
    let rMax = 0;
    for (let i = 0; i < outV.length; i++) {
      const r = Math.hypot(outV[i][0], outV[i][1]);
      if (r > rMax) rMax = r;
    }
    const RING_R = Math.max(0.19, rMax * 0.62);   // rings + pylons
    const RIM_IN = rMax * 0.86, RIM_OUT = rMax * 0.985;

    const hash01 = (x, y, z) => {
      const s = Math.sin(x * 127.1 + y * 311.7 + z * 74.7) * 43758.5453;
      return s - Math.floor(s);
    };

    // Per-pixel zone decision in baked ship space: thin window chains,
    // ring neon channel, amber trim, belly, nose, exhaust. armorMix
    // (block breakup) stays a per-face argument.
    const zoneAt = (x, y, z, armorMix) => {
      const r = Math.hypot(x, y);
      let zr, zg, zb, rough, metal, er = 0, eg = 0, eb = 0;
      const isRing = r > RING_R;
      if (isRing) {
        if (r < RING_R + 0.055) {
          zr = 10; zg = 14; zb = 26;
          rough = 0.25; metal = 0.70;
          er = 42; eg = 150; eb = 255;
        } else {
          zr = 62; zg = 68; zb = 80;
          if (armorMix) { zr = 96; zg = 104; zb = 118; }
          rough = 0.94; metal = 0.08;
          const ang = Math.atan2(y, x) / (2 * Math.PI) + 0.5;
          const zz = z * 3.5 % 1;
          if ((ang * 44 % 1) < 0.50 && zz > 0.18 && zz < 0.82) {
            zr = 9; zg = 11; zb = 18;
            rough = 0.18; metal = 0.60;
            er = 24; eg = 120; eb = 250;
          }
        }
      } else if (z > 0.98) {
        zr = 40; zg = 42; zb = 48;
        rough = 0.94; metal = 0.08;
        if (z > 1.10) { er = 220; eg = 66; eb = 18; }
      } else if (z > 0.60) {
        zr = 48; zg = 52; zb = 62;
        if (armorMix) { zr = 78; zg = 85; zb = 100; }
        rough = 0.94; metal = 0.08;
      } else if (z > 0.50 && z <= 0.565) {
        zr = 217; zg = 158; zb = 64;
        rough = 0.65; metal = 0.20;
        er = 40; eg = 26; eb = 10;
      } else if (y < -0.045) {
        zr = 48; zg = 53; zb = 62;
        rough = 0.92; metal = 0.10;
      } else if (z < -0.72) {
        zr = 150; zg = 158; zb = 170;
        rough = 0.80; metal = 0.20;
      } else {
        zr = 62; zg = 68; zb = 80;
        if (armorMix) { zr = 98; zg = 106; zb = 122; }
        rough = 0.92; metal = 0.10;
        if (y > -0.045 && y < 0.04 && z > -0.55 && z < 0.35 &&
            (z * 15.0 % 1) < 0.55) {
          zr = 9; zg = 12; zb = 20;
          rough = 0.18; metal = 0.60;
          er = 26; eg = 130; eb = 255;
        }
      }
      return { zr, zg, zb, rough, metal, er, eg, eb };
    };

    let painted = 0;
    for (let t = 0; t < tris.length; t += 3) {
      const ia = remap[tris[t]], ib = remap[tris[t + 1]], ic = remap[tris[t + 2]];
      const pa = outV[ia], pb = outV[ib], pc = outV[ic];
      const ta = outVT[ia], tb = outVT[ib], tc = outVT[ic];
      const cx = (pa[0] + pb[0] + pc[0]) / 3;
      const cy = (pa[1] + pb[1] + pc[1]) / 3;
      const cz = (pa[2] + pb[2] + pc[2]) / 3;
      const jit = hash01(cx * 53, cy * 53, cz * 53) - 0.5;
      const armorMix = hash01(cx * 31, cy * 31, cz * 31) > 0.7;

      // Panel tone: quantized luminance sampled from the source atlas at
      // the face centroid — grizzly terraces survive, Tripo copper does
      // not leak. Glowing pixels bypass the panel factor.
      const su = (ta[0] + tb[0] + tc[0]) / 3;
      const sv = (ta[1] + tb[1] + tc[1]) / 3;
      const src = [0, 0, 0];
      sample(alb, su, sv, src);
      const luma = (src[0] * 0.299 + src[1] * 0.587 + src[2] * 0.114) / 255;
      const q = Math.round(luma * 4) / 4;
      const panel = 0.70 + 0.55 * q;                    // 0.70..1.25
      const vj = 0.985 + hash01(cz * 97, cy * 97, cx * 97) * 0.03;
      const kArmor = panel * vj;

      // ---- rasterize the face's UV triangle ----
      const px = (v) => v * (S - 1);
      const x0 = Math.max(0, Math.floor(Math.min(ta[0], tb[0], tc[0]) * (S - 1)) - 1);
      const x1 = Math.min(S - 1, Math.ceil(Math.max(ta[0], tb[0], tc[0]) * (S - 1)) + 1);
      const y0 = Math.max(0, Math.floor(Math.min(1 - ta[1], 1 - tb[1], 1 - tc[1]) * (S - 1)) - 1);
      const y1 = Math.min(S - 1, Math.ceil(Math.max(1 - ta[1], 1 - tb[1], 1 - tc[1]) * (S - 1)) + 1);
      const axp = px(ta[0]), ayp = px(1 - ta[1]);
      const bxp = px(tb[0]), byp = px(1 - tb[1]);
      const cxp = px(tc[0]), cyp = px(1 - tc[1]);
      // sign-agnostic edge functions: inside = all signs agree
      const e00 = (byp - ayp), e01 = (axp - bxp);
      const e10 = (cyp - byp), e11 = (bxp - cxp);
      const e20 = (ayp - cyp), e21 = (cxp - axp);
      const EPS = -1e-6;
      // barycentric normalization: weight of the opposite vertex is
      // edge function / edge function at that vertex
      const l1a = e10 * (axp - bxp) + e11 * (ayp - byp);
      const l2b = e20 * (bxp - cxp) + e21 * (byp - cyp);
      const l0c = e00 * (cxp - axp) + e01 * (cyp - ayp);
      const degenerate = Math.abs(l1a) < 1e-9 || Math.abs(l2b) < 1e-9 ||
                         Math.abs(l0c) < 1e-9;
      for (let py = y0; py <= y1; py++) {
        const fy = py + 0.5;
        for (let ppx = x0; ppx <= x1; ppx++) {
          const fx = ppx + 0.5;
          const l0 = e00 * (fx - axp) + e01 * (fy - ayp);
          const l1 = e10 * (fx - bxp) + e11 * (fy - byp);
          const l2 = e20 * (fx - cxp) + e21 * (fy - cyp);
          if (!((l0 >= EPS && l1 >= EPS && l2 >= EPS) ||
                (l0 <= -EPS && l1 <= -EPS && l2 <= -EPS))) continue;
          let wa, wb, wc;
          if (degenerate) {
            wa = wb = wc = 1 / 3;
          } else {
            // sign-agnostic flavor: numerator and denominator share the
            // winding sign, so the ratio is already a positive weight
            wa = l1 / l1a; wb = l2 / l2b; wc = l0 / l0c;
          }
          const pxs = wa * pa[0] + wb * pb[0] + wc * pc[0];
          const pys = wa * pa[1] + wb * pb[1] + wc * pc[1];
          const pzs = wa * pa[2] + wb * pb[2] + wc * pc[2];
          const zo = zoneAt(pxs, pys, pzs, armorMix);
          const usesEm = zo.er !== 0 || zo.eg !== 0 || zo.eb !== 0;
          const k = usesEm ? 1.0 : kArmor;
          const rough = Math.min(1, Math.max(0.05, zo.rough + jit * 0.08));
          const metal = Math.min(1, Math.max(0, zo.metal + jit * 0.04));
          const idx = (py * S + ppx) * 3;
          alb.rgb[idx] = Math.min(255, zo.zr * k);
          alb.rgb[idx + 1] = Math.min(255, zo.zg * k);
          alb.rgb[idx + 2] = Math.min(255, zo.zb * k);
          if (rgh) {
            const rr = Math.round(rough * 255);
            rgh.rgb[idx] = rr; rgh.rgb[idx + 1] = rr; rgh.rgb[idx + 2] = rr;
          }
          if (mtl) {
            const rm = Math.round(metal * 255);
            mtl.rgb[idx] = rm; mtl.rgb[idx + 1] = rm; mtl.rgb[idx + 2] = rm;
          }
          emis.rgb[idx] = zo.er;
          emis.rgb[idx + 1] = zo.eg;
          emis.rgb[idx + 2] = zo.eb;
          mask[py * S + ppx] = 1;
          painted++;
        }
      }
    }
    // 1px dilate: kill rasterization cracks between adjacent face splats
    const dil = Buffer.from(alb.rgb), dilR = rgh ? Buffer.from(rgh.rgb) : null;
    const dilM = mtl ? Buffer.from(mtl.rgb) : null, dilE = Buffer.from(emis.rgb);
    const mask2 = new Uint8Array(mask);
    for (let y = 1; y < S - 1; y++) {
      for (let x = 1; x < S - 1; x++) {
        const i = y * S + x;
        if (mask2[i]) continue;
        const srcI = mask2[i - 1] ? i - 1 : mask2[i + 1] ? i + 1 :
                     mask2[i - S] ? i - S : mask2[i + S] ? i + S : -1;
        if (srcI < 0) continue;
        for (let c = 0; c < 3; c++) {
          dil[i * 3 + c] = alb.rgb[srcI * 3 + c];
          if (dilR) dilR[i * 3 + c] = rgh.rgb[srcI * 3 + c];
          if (dilM) dilM[i * 3 + c] = mtl.rgb[srcI * 3 + c];
          dilE[i * 3 + c] = emis.rgb[srcI * 3 + c];
        }
        mask[i] = 1;
      }
    }
    alb.rgb = dil; if (rgh) rgh.rgb = dilR; if (mtl) mtl.rgb = dilM; emis.rgb = dilE;

    writeBmp24(pAlbedo, alb);
    if (rgh) writeBmp24(pRough, rgh);
    if (mtl) writeBmp24(pMetal, mtl);
    writeBmp24(path.join(OUTDIR, `${NAME}_emissive.bmp`), emis);
    liveryReport = { ringRadius: +RING_R.toFixed(3), facePixels: painted };
  }
}

console.log(JSON.stringify({
  fbx: path.basename(IN), fbxVersion: VERSION,
  srcVerts: N, srcTris: triStart.length,
  outVerts: outV.length, outTris: tris.length / 3,
  out: OUT, sizeMB: +(fs.statSync(OUT).size / 1024 / 1024).toFixed(1),
  uv: hasUV ? `${UV.length / 2} texcoords (${uvRef}/${uvMapMode})` : 'none',
  axis: ['x', 'y', 'z'][lenAxis], flipped: !!noseAtMax,
  livery: liveryReport,
  textures: texReport,
}, null, 2));
