import sys, array, math, json
path, c0, r0, N = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
W, PX = 30336, 5.0
data = array.array('h')
with open(path, 'rb') as f: data.frombytes(f.read())
H = [[data[(r0 + r) * W + (c0 + c)] * 0.5 for c in range(N)] for r in range(N)]
for _ in range(3):
    S = [[0.0] * N for _ in range(N)]
    for r in range(N):
        rs = (max(r-1,0), r, min(r+1,N-1))
        for c in range(N):
            cs = (max(c-1,0), c, min(c+1,N-1))
            S[r][c] = sum(H[a][b] for a in rs for b in cs) / 9.0
    H = S
zc = H[N//2][N//2]; half = (N-1)*PX/2
def g(x, z):  # glTF metres -> height rel. centre
    c = int(round((x + half)/PX)); r = int(round((z + half)/PX))
    return H[r][c] - zc
probe = {k: g(*v) for k, v in {"x+1000": (1000,0), "x-1000": (-1000,0), "z+1000": (0,1000), "z-1000": (0,-1000), "x+600z+600": (600,600)}.items()}
# crater search: depth = ring mean (radius R) - centre; diameter 60..300 m
best = []
for R in (8, 12, 18, 25):          # px radius (40..125 m)
    for r in range(64, N-64, 4):
        for c in range(64, N-64, 4):
            ring = [H[int(r + R*math.sin(t))][int(c + R*math.cos(t))] for t in [i*math.pi/8 for i in range(16)]]
            d = sum(ring)/16 - H[r][c]
            if d > 0: best.append((d / (2*R*PX), d, R, r, c))
best.sort(reverse=True)
picked = []
for ratio, d, R, r, c in best:
    if all(math.hypot(r-pr, c-pc) > 40 for _,_,_,pr,pc in picked):
        picked.append((ratio, d, R, r, c))
    if len(picked) >= 8: break
out = [{"depth_m": round(d,1), "diam_m": 2*R*PX, "gltf_x": c*PX-half, "gltf_z": r*PX-half, "dist_from_centre_m": round(math.hypot(c*PX-half, r*PX-half))} for _, d, R, r, c in picked]
print(json.dumps({"probe": probe, "craters": out}, indent=0))
