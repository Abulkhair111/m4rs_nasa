# Asset Catalog

## Maps (`/Game/MissionCrew/Maps`)
| Map | Content |
|---|---|
| `L_MainMenu` | Menu backdrop: the camera stands on the west rim of a **real 120 m LOLA crater** looking east (+X). Actors: `MenuCamera` (0,0,60, pitch -7, AutoActivate Player0), `LunarTerrain` (StaticMeshActor_5, terrain offset so the ground at the origin is z = -250), `Rover` (StaticMeshActor_3 at 1700,520, yaw -125, pitch/roll fitted to the slope by ground traces), `Starfield` (sphere, scale 100000), `PP_Space` (unbound, manual exposure, bias -1, physical camera exposure off), DirectionalLight (pitch -25, yaw 25, 12 lux), SkyLight 0.08 (no realtime capture). WorldSettings GameMode = BP_MenuGameMode. |
| `L_Lobby`, `L_Mission` | Copies of the Template_Default map, not built yet. |

## Art
| Asset | Notes |
|---|---|
| `Art/Meshes/Rover/Perseverance` | StaticMesh, ~2.7×4.1×2.2 m, origin at wheel bottom, 10k tris, **Nanite OFF** (project has no SM6), 48 material slots. The folder also holds 48 materials and 37 textures from the GLB. Source: `SourceArt/Perseverance.glb` (user-supplied). |
| `Art/Meshes/Terrain/LunarTerrain_LOLA_26576_256_smooth` | **Real lunar south-pole terrain** from NASA LRO LOLA LDEM_875S_5M: 512×512 samples at 5 m = 2555 m square, relief 198 m, 522k tris, Nanite OFF, smoothed (3× box) to remove 0.5 m quantisation. Built by `Tools/LOLA` (see its README). A matching `.r16` heightmap for a future Landscape is in `SourceArt/LOLA`. |
| `Art/Materials/M_MoonSurface` | World-aligned (top-down) regolith: T_MoonDusted D/N/R. Params: `DetailTileCm` 350, `MacroTileCm` 4200 (brightness variation), `Desaturate` 0.85, `Tint`. |
| `Art/Materials/M_Starfield` | Unlit two-sided. Voronoi noise on CameraVectorWS × 300, power 120 × 40. |
| `Art/Textures/Moon/T_MoonDusted_{D,N,R}` | Poly Haven "Moon Dusted 01" 2K, **CC0**. N = TC_Normalmap, R = TC_Masks (sampler Masks). Source PNGs in `SourceArt/Textures/moon_dusted_01`. |

## UI
`UI/WBP_MainMenu`, `UI/WBP_LanSessionRow` (other agent).

## Source files (outside Content, tracked via LFS)
`SourceArt/` — glb, png. **Never put source files in Content/** (auto-import is unreliable; use the import tools, see [[Unreal MCP Workflow]]).
