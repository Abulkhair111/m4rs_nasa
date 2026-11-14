"""Make a GLB import as a StaticMesh: drop skins, animations and joint weights.

Usage: python strip_glb_skins.py in.glb out.glb
Only the JSON chunk changes; the binary buffer is copied as is (unused accessors stay).
"""
import json, struct, sys

src, dst = sys.argv[1], sys.argv[2]
data = open(src, "rb").read()
magic, version, _ = struct.unpack_from("<III", data, 0)
assert magic == 0x46546C67, "not a GLB"
off = 12
chunks = []
while off < len(data):
    ln, typ = struct.unpack_from("<II", data, off)
    chunks.append([typ, data[off + 8: off + 8 + ln]])
    off += 8 + ln
js = json.loads(chunks[0][1].decode("utf8"))
js.pop("skins", None)
js.pop("animations", None)
for n in js.get("nodes", []):
    n.pop("skin", None)
for m in js.get("meshes", []):
    for p in m.get("primitives", []):
        for k in [k for k in p.get("attributes", {}) if k.startswith(("JOINTS_", "WEIGHTS_"))]:
            del p["attributes"][k]
raw = json.dumps(js, separators=(",", ":")).encode("utf8")
raw += b" " * ((4 - len(raw) % 4) % 4)
chunks[0][1] = raw
body = b"".join(struct.pack("<II", len(c), t) + c for t, c in chunks)
open(dst, "wb").write(struct.pack("<III", magic, version, 12 + len(body)) + body)
print("ok", dst)
