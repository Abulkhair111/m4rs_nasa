"""Crop the LOLA LDEM_875S_5M strip and write (1) a glTF terrain mesh and (2) a 16-bit RAW heightmap.
Usage: python dem_to_glb.py <strip.bin> <col0> <row0> <size> <out_dir>
Heights: DN * 0.5 m (relative to the 1737.4 km reference sphere)."""
import sys, array, struct, json, math, os

path, c0, r0, N, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
W, ROWS, PX = 30336, 1024, 5.0
data = array.array('h')
with open(path, 'rb') as f:
    data.frombytes(f.read())
H = [[data[(r0 + r) * W + (c0 + c)] * 0.5 for c in range(N)] for r in range(N)]
# DN is quantised to 0.5 m -> terraces under grazing light. Smooth with a few 3x3 box passes.
PASSES = int(os.environ.get('DEM_SMOOTH', '3'))
for _ in range(PASSES):
    S = [[0.0] * N for _ in range(N)]
    for r in range(N):
        rs = (max(r - 1, 0), r, min(r + 1, N - 1))
        for c in range(N):
            cs = (max(c - 1, 0), c, min(c + 1, N - 1))
            S[r][c] = sum(H[a][b] for a in rs for b in cs) / 9.0
    H = S
SUFFIX = os.environ.get('DEM_SUFFIX', '')
zmin = min(min(row) for row in H); zmax = max(max(row) for row in H)
zc = H[N // 2][N // 2]
half = (N - 1) * PX / 2.0

# --- glTF (meters, Y-up, right-handed): x = column (east-ish), z = row, y = height
pos = array.array('f'); nrm = array.array('f'); uv = array.array('f')
for r in range(N):
    for c in range(N):
        x = c * PX - half; z = r * PX - half; y = H[r][c] - zc
        pos.extend((x, y, z))
        hl = H[r][max(c - 1, 0)]; hr = H[r][min(c + 1, N - 1)]
        hu = H[max(r - 1, 0)][c]; hd = H[min(r + 1, N - 1)][c]
        nx = (hl - hr) / (2 * PX); nz = (hu - hd) / (2 * PX); ny = 1.0
        l = math.sqrt(nx * nx + ny * ny + nz * nz)
        nrm.extend((nx / l, ny / l, nz / l))
        uv.extend((c / 8.0, r / 8.0))
idx = array.array('I')
for r in range(N - 1):
    for c in range(N - 1):
        v00 = r * N + c; v10 = v00 + 1; v01 = v00 + N; v11 = v01 + 1
        idx.extend((v00, v01, v10, v10, v01, v11))

blobs = [pos.tobytes(), nrm.tobytes(), uv.tobytes(), idx.tobytes()]
offsets = []; buf = b''
for b in blobs:
    offsets.append(len(buf)); buf += b
    buf += b'\x00' * ((4 - len(buf) % 4) % 4)
xs = pos[0::3]; ys = pos[1::3]; zs = pos[2::3]
gltf = {
    "asset": {"version": "2.0", "generator": "MissionCrew dem_to_glb (NASA LRO LOLA LDEM_875S_5M)"},
    "scene": 0, "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0, "name": "LunarTerrain"}],
    "materials": [{"name": "M_LunarTerrain", "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.5, 0.5, 1], "metallicFactor": 0, "roughnessFactor": 1}}],
    "meshes": [{"name": "LunarTerrain", "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3, "material": 0}]}],
    "buffers": [{"byteLength": len(buf)}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": offsets[0], "byteLength": len(blobs[0]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[1], "byteLength": len(blobs[1]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[2], "byteLength": len(blobs[2]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[3], "byteLength": len(blobs[3]), "target": 34963}],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": N * N, "type": "VEC3", "min": [min(xs), min(ys), min(zs)], "max": [max(xs), max(ys), max(zs)]},
        {"bufferView": 1, "componentType": 5126, "count": N * N, "type": "VEC3"},
        {"bufferView": 2, "componentType": 5126, "count": N * N, "type": "VEC2"},
        {"bufferView": 3, "componentType": 5125, "count": len(idx), "type": "SCALAR"}],
}
js = json.dumps(gltf).encode(); js += b' ' * ((4 - len(js) % 4) % 4)
glb = struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(buf))
glb += struct.pack('<II', len(js), 0x4E4F534A) + js + struct.pack('<II', len(buf), 0x004E4942) + buf
os.makedirs(out, exist_ok=True)
gpath = os.path.join(out, 'LunarTerrain_LOLA_%d_%d%s.glb' % (c0, r0, SUFFIX))
with open(gpath, 'wb') as f: f.write(glb)

# --- RAW 16-bit heightmap (little endian) for Landscape import, 0..65535 over [zmin, zmax]
rpath = os.path.join(out, 'LunarTerrain_LOLA_%d_%d_%dx%d%s.r16' % (c0, r0, N, N, SUFFIX))
rng = max(zmax - zmin, 1e-3)
raw = array.array('H', (int(round((H[r][c] - zmin) / rng * 65535)) for r in range(N) for c in range(N)))
with open(rpath, 'wb') as f: f.write(raw.tobytes())
print(json.dumps({"glb": gpath, "r16": rpath, "zmin": zmin, "zmax": zmax, "relief_m": zmax - zmin, "center_h": zc,
                  "size_m": (N - 1) * PX, "verts": N * N, "tris": len(idx) // 3,
                  "landscape_z_scale_for_r16": rng * 100.0 / 512.0}))
