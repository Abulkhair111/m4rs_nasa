# LOLA terrain pipeline

Real lunar south-pole terrain from NASA LRO LOLA (public domain), product **LDEM_875S_5M**
(5 m/px polar stereographic, int16 LSB, height = DN × 0.5 m). Label:
https://pds-geosciences.wustl.edu/lro/lro-l-lola-3-rdr-v1/lrolol_1xxx/data/lola_gdr/polar/img/ldem_875s_5m.lbl

Use the engine Python: `A:/epic games/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe -I`

1. Download a strip of rows with an HTTP range request (the full file is 1.8 GB, record = 60672 bytes/row):
   `curl -r $((13500*60672))-$(((13500+1024)*60672-1)) -o SourceArt/LOLA/ldem_875s_5m_rows13500_1024.bin <img url>`
   (the strip itself is git-ignored).
2. `dem_scan.py <strip>` ranks 512×512 windows by relief, slope and crater roughness.
3. `DEM_SMOOTH=3 DEM_SUFFIX=_smooth dem_to_glb.py <strip> 26576 256 512 SourceArt/LOLA` writes the glb mesh plus a .r16 heightmap.
   Smoothing removes the 0.5 m quantisation terraces.
4. `dem_craters.py <strip> 26576 256 512` lists crater positions (glTF x → UE X, glTF z → UE Y, metres).
5. Import the glb with `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=import_glb_commandlet.py`
   (the MCP import tool only accepts fbx/obj). Then turn Nanite off (the project has no SM6).

Current patch: strip col 26576, row 256 (absolute row 13756), 2555 m square, relief 198 m.
For a Landscape in L_Mission, use the `.r16` (512×512). Z scale = relief_m × 100 / 512 ≈ 38.73; XY scale = 500.
