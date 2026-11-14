# Asset Catalog

## Maps (`/Game/MissionCrew/Maps`)
| Map | Content |
|---|---|
| `L_MainMenu` | Menu backdrop. Actors: `MenuCamera` (AutoActivate Player0, pitch -4 at origin), `Rover` (Perseverance at 1000,290,-250, yaw -125), `MoonGround` (engine Plane, scale 10000, z = -250, M_MoonSurface), `Starfield` (sphere, scale 100000, M_Starfield), `PP_Space` (unbound, manual exposure, bias -1, physical camera exposure off), DirectionalLight (pitch -38, yaw 35, 8 lux), SkyLight 0.08 (no realtime capture). WorldSettings GameMode = BP_MenuGameMode. |
| `L_Lobby`, `L_Mission` | Copies of the Template_Default map, not built yet. |

## Art
| Asset | Notes |
|---|---|
| `Art/Meshes/Rover/Perseverance` | StaticMesh, ~2.7×4.1×2.2 m, origin at wheel bottom, 10k tris, **Nanite OFF** (project has no SM6), 48 material slots. The folder also holds 48 materials and 37 textures from the GLB. Source: `SourceArt/Perseverance.glb` (user-supplied). |
| `Art/Materials/M_MoonSurface` | World-aligned (top-down) regolith: T_MoonDusted D/N/R. Params: `DetailTileCm` 350, `MacroTileCm` 4200 (brightness variation), `Desaturate` 0.85, `Tint`. |
| `Art/Materials/M_Starfield` | Unlit two-sided. Voronoi noise on CameraVectorWS × 300, power 120 × 40. |
| `Art/Textures/Moon/T_MoonDusted_{D,N,R}` | Poly Haven "Moon Dusted 01" 2K, **CC0**. N = TC_Normalmap, R = TC_Masks (sampler Masks). Source PNGs in `SourceArt/Textures/moon_dusted_01`. |

## UI
`UI/WBP_MainMenu`, `UI/WBP_LanSessionRow` (other agent).

## Source files (outside Content, tracked via LFS)
`SourceArt/` — glb, png. **Never put source files in Content/** (auto-import is unreliable; use the import tools, see [[Unreal MCP Workflow]]).
