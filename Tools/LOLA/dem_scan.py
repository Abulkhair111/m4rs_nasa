import sys, array, math
path = sys.argv[1]
W = 30336; ROWS = 1024; WIN = 512; STEP = 128; PX = 5.0
data = array.array('h')
with open(path, 'rb') as f:
    data.frombytes(f.read())
assert len(data) == W * ROWS
def h(r, c): return data[r * W + c] * 0.5
res = []
for c0 in range(2000, W - 2000 - WIN, STEP):
    for r0 in range(0, ROWS - WIN + 1, 256):
        # coarse stats on every 4th pixel
        hs = []; slopes = []; rough = []
        for r in range(r0, r0 + WIN, 8):
            for c in range(c0, c0 + WIN, 8):
                z = h(r, c); hs.append(z)
                dzx = (h(r, c + 4) - h(r, c)) / (4 * PX)
                dzy = (h(r + 4 if r + 4 < ROWS else r - 4, c) - z) / (4 * PX)
                slopes.append(math.degrees(math.atan(math.hypot(dzx, dzy))))
                # small-scale roughness: laplacian over 4 px (20 m)
                if 4 <= r < ROWS - 4 and c >= 4:
                    lap = h(r - 4, c) + h(r + 4, c) + h(r, c - 4) + h(r, c + 4) - 4 * z
                    rough.append(abs(lap))
        relief = max(hs) - min(hs)
        ms = sum(slopes) / len(slopes); mx = sorted(slopes)[int(len(slopes) * 0.98)]
        rg = sum(rough) / max(1, len(rough))
        res.append((c0, r0, relief, ms, mx, rg))
# want: moderate relief, mean slope 4-12 deg, p98 slope < 28, high roughness (craters)
cand = [x for x in res if 30 < x[2] < 250 and 4 < x[3] < 12 and x[4] < 28]
cand.sort(key=lambda x: -x[5])
print("windows", len(res), "candidates", len(cand))
for x in cand[:10]:
    print("col %d row %d relief %.0fm meanSlope %.1f p98 %.1f rough %.2f" % x)
