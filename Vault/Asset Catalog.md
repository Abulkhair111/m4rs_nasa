# Asset Catalog

## Maps (`/Game/MissionCrew/Maps`)
| Map | Content |
|---|---|
| `L_MainMenu` | Menu backdrop: the camera stands on the west rim of a **real 120 m LOLA crater** looking east (+X). Actors: `MenuCamera` (0,0,60, pitch -7, AutoActivate Player0), `LunarTerrain` (StaticMeshActor_5, terrain offset so the ground at the origin is z = -250), `Rover` (StaticMeshActor_3 at 1700,520, yaw -125, pitch/roll fitted to the slope by ground traces), `Starfield` (sphere, scale 100000), `PP_Space` (unbound, manual exposure, bias -1, physical camera exposure off), DirectionalLight (pitch -25, yaw 25, 12 lux), SkyLight 0.08 (no realtime capture). WorldSettings GameMode = BP_MenuGameMode. |
| `L_Lobby` | Lobby/role select (other agent). |
| `L_Mission` | Template_Default copy, not built yet. Contains `MissionVideoEvent` (BP_MissionVideoEvent at 0,0,200), which plays the intro video when the mission starts. |
| `L_AssemblyBay` | **S1 Design & Assembly bay** (menu/art session). Hall 27x27 m (walls at +-1350), 40 m tall, NASA `Gantry` (MI_Gantry_*) in the NE corner (1050,900); walls `MI_Hall_*`; lighting by agent A: volumetric fog, PostProcess (manual exposure), hero spot, 24 cyan accents, 6 gantry spots (see Status). Center: `AssemblyStand` (lander placeholder at 0,0,0), `PowerStrip`, 13 `Sock_*` sockets (incl. `Sock_Tank_A/B` at -90,±130,170, type 8). Stations: `Shelf_PowerComms` (0,1150, dept 1, cable reel first), `Shelf_Propulsion` (0,-1150, dept 3, engines + lidar + 2 tanks) + `FuelPump`/`FuelDrain`, `Shelf_Science` (1150,0, dept 4), `Desk_Assembly` (-1150,0, dept 2, wrench) + `Lever_Falcon9/Vulcan/FalconHeavy` (1200/2500/4000 kg, $70/110/150M) + `LaunchButton`. Signs on shelves/READY buttons name the S1 roles. `Ready_*` ×4, `WallBoard_W/E`, `MassScale` (0,-470), `ConstructionManager` (-1400,1400), `BayStart_0..3` (PlayerStart, tag `Bay`). WorldSettings GameMode = BP_MC_GameMode. Press Play here to test solo. |

## Art
| Asset | Notes |
|---|---|
| `Art/Meshes/Rover/Perseverance` | StaticMesh, ~2.7×4.1×2.2 m, origin at wheel bottom, 10k tris, **Nanite OFF** (project has no SM6), 48 material slots. The folder also holds 48 materials and 37 textures from the GLB. Source: `SourceArt/Perseverance.glb` (user-supplied). |
| `Art/Meshes/Terrain/LunarTerrain_LOLA_26576_256_smooth` | **Real lunar south-pole terrain** from NASA LRO LOLA LDEM_875S_5M: 512×512 samples at 5 m = 2555 m square, relief 198 m, 522k tris, Nanite OFF, smoothed (3× box) to remove 0.5 m quantisation. Built by `Tools/LOLA` (see its README). A matching `.r16` heightmap for a future Landscape is in `SourceArt/LOLA`. |
| `Art/Meshes/NASA/<Name>/<Name>` | 17 static meshes from NASA 3D Resources (ApolloLM, SatWings1/3, SatBody1, SatRadio1, ORU, VoyagerAntenna, ShuttleRCS, ShuttleEng, LLCD, ExternalTank, InfraredCamera, TOMS, COS, WFPC, Wrench, Tether1), with their materials. Nanite OFF. Source GLBs in `SourceArt/NASA3D`; import with `Tools/NASA3D/import_nasa_parts.py`. Assigned in the `DA_Part_*` assets (see [[S1 Design & Assembly - Handoff]]). **Agent B 2026-11-14, extra imports (not yet assigned to parts, free to use for props/rockets/lander):** `SaturnV`, `Ares1B`, `Atlas9`, `JupiterC` (rockets, ok for launch-vehicle visuals), `SRB` (Shuttle solid booster), `SuperLightTank` (Shuttle super-lightweight tank, same family as `ExternalTank`), `VikingLander` (lander-like body with RTG housings, 1.8 MB), `Pioneer10` (probe with RTG booms, textured), `HabitatHDU`, `BaseStation`, `LaunchPad`, `Crawler`, `VAB` (exterior only), plus duplicates `Gantry`, `MobileLauncher` (agent A has its own copies in `Construction/Env`). `RASSOR` (6 MB regolith robot) was NOT imported: the build needs >3 GB RAM and hung the commandlet; the GLB is parked in `SourceArt/NASA3D/deferred`. Sources: all files from github.com/nasa/NASA-3D-Resources (public domain, NASA media guidelines). No web source for an RTG or a drill was usable without login/JS; CGTrader/Sketchfab candidates (e.g. 'Radio Thermal Generator NASA Style RTG') are third-party with unclear licence and were not imported. RTG and Drill remain engine-cylinder placeholders. |
| `Art/Materials/M_MoonSurface` | World-aligned (top-down) regolith: T_MoonDusted D/N/R. Params: `DetailTileCm` 350, `MacroTileCm` 4200 (brightness variation), `Desaturate` 0.85, `Tint`. |
| `Art/Materials/M_Starfield` | Unlit two-sided. Voronoi noise on CameraVectorWS × 300, power 120 × 40. |
| `Art/Textures/Moon/T_MoonDusted_{D,N,R}` | Poly Haven "Moon Dusted 01" 2K, **CC0**. N = TC_Normalmap, R = TC_Masks (sampler Masks). Source PNGs in `SourceArt/Textures/moon_dusted_01`. |

## UI
`UI/WBP_MainMenu`, `UI/WBP_LanSessionRow`, `UI/WBP_Lobby`, `UI/WBP_RoleCard` (other agent). All text is English (translated 2026-11-14).
`UI/WBP_MissionVideo`: fullscreen video (black Backdrop + VideoImage brush = MT_MissionVideo) and a SKIP button. Construct → Delay 0 → `StartVideo` (binds OnEndReached/OnMediaOpenFailed → `FinishVideo`, then OpenSource; if OpenSource returns false → FinishVideo). `FinishVideo` (runs once) → Close → `OnVideoFinished` dispatcher → RemoveFromParent.

`WBP_MainMenu` styling (menu/art): `LeftShade` navy 0.86, `EdgeLine` (cyan, x=800), `StatusBar`, `MissionCard` (Border → `MissionCardBox`: CardTag/CardTitle/CardBody), buttons restyled (Host cyan, Join dark blue, Quit dark with red hover).

## Construction (`/Game/MissionCrew/Construction`), all placeholders are engine BasicShapes
| Asset | Notes |
|---|---|
| `Data/PDA_PartDef` | PrimaryDataAsset BP: DisplayName, RealAnalog, Description, Department, SocketType, MassKg, CostM, PowerW (+gen/−use), StorageWh, DataMbps, BandwidthMbps, ScienceValue, Reliability, bHeavy, **Mesh**, MeshScale, Color (alpha 0 = keep the mesh's own materials), **ThrustN, FuelCapKg, bIceCapable, ToolType** (0 part, 1 wrench, 2 cable). |
| `Data/Parts/DA_Part_*` (18) | Power & Comms: SolarSmall, SolarLarge, RTG (heavy), BatterySmall, BatteryLarge (heavy), AntennaLow, AntennaHigh. Propulsion: EngineBasic (4500 N), EngineThrottle (7000 N), Lidar, **TankSmall** (600 kg cap, 35 kg, $6M), **TankLarge** (1100 kg cap, 60 kg, $10M, heavy). Science: NIRVSS, NSS, MSolo, Drill, Rover (all ice-capable), Camera (not ice-capable). All numbers are placeholders (TBD). **To use real models, set `Mesh`/`MeshScale`, and set Color alpha to 0.** **2026-11-14 (agent B):** every part got a bigger `MeshScale` (x1.15-2.0, so parts read from ~2 m), a display name that states what the model really is (e.g. "Solar array 300 W (satellite wings)", "Thruster 4.5 kN (shuttle RCS)"), and a one-line plain-English `Description` with key numbers ("Solar array: +300 W power. ..."). DA has no pivot/rotation field, only `MeshScale`. |
| `Data/Parts/DA_Tool_Wrench`, `DA_Tool_Cable` | Tools (SocketType 99, never installed). Wrench: dept 2, ToolType 1. Cable reel: dept 1, ToolType 2. Placeholder cylinders. |
| `Blueprints/*` | See [[Architecture]] → Construction phase. Placeholder geometry lives in BP components: `BP_AssemblyStand` (lander body/legs/deck/cradle), `BP_RocketLever` (RocketBody/RocketNose), shelves, pumps, buttons, scale, power ring. |
| `Materials/M_Proto` | Params Color, Emissive, Roughness, Metallic. 13 instances `MI_Proto_{Wall,Floor,Metal,Foil,Power,Propulsion,Science,Mission,LightPanel,Red,Green,Yellow,Dark}`. |
| `Materials/M_Ghost` | Unlit translucent, pulsing (Color, Opacity), for socket previews. |
| `Input/IA_Bay_*`, `IMC_Bay` | Forward/Back/Left/Right (bool, WASD), Look (Mouse2D), Interact (E), Jump (Space). |
| `UI/WBP_BayHUD` | Crosshair, PromptText, MessageText (7 s toast), SummaryText (time/mass/budget/power/fuel/rocket), HelpText (S1 roles + controls, 3 lines). |
| `UI/WBP_WallBoard` | 1600×900 board on a WidgetComponent (scale 0.25, Masked, transparent background over the dark Frame mesh: an opaque widget background showed up washed-out blue). Timer, budget, stats, 10 colored checklist rows `Check0..9` (`SetRow`; row 9 via `UpdateRow9`), report (header with thrust/T/W/tanks/ice count, failing checks, accuracy) + last message. |

## Media (`/Game/MissionCrew/Media`)
| Asset | Notes |
|---|---|
| `MP_MissionVideo` | MediaPlayer, PlayOnOpen on, Loop off. |
| `MT_MissionVideo` | MediaTexture bound to MP_MissionVideo. |
| `FMS_MissionIntro` | FileMediaSource → `./Content/Movies/MissionIntro.mp4` (in UE 5.8, `./` is relative to the **project** folder). **The video file isn't supplied yet.** Drop an H.264 .mp4 there. |
Created by `Tools/Media/create_mission_video_assets.py` (commandlet). `Config/DefaultGame.ini` stages `Content/Movies` as NonUFS for packaged builds.

## Core (mission video)
`Core/BP_MissionVideoEvent` (Actor): BeginPlay → if `bPlayOnBeginPlay` → **`PlayMissionVideo`** event: AddComponentByClass MediaSoundComponent (audio) → set cursor visible → CreateWidget WBP_MissionVideo for PlayerController 0 → AddToViewport (Z 100) → bind `OnVideoFinished` → `HandleVideoFinished` (restores the cursor, calls dispatcher **`OnMissionVideoFinished`**, which mission logic can hook into).

## Source files (outside Content, tracked via LFS)
`SourceArt/` — glb, png. **Never put source files in Content/** (auto-import is unreliable; use the import tools, see [[Unreal MCP Workflow]]).

## Construction/Env (agent A)
| Asset | Notes |
|---|---|
| `Construction/Env/MI_Hall_Wall`, `MI_Hall_Floor`, `MI_Hall_Ceiling` | Dark navy MIs of `M_Proto` for the bay shell. |
| `Construction/Env/Meshes/Gantry/Gantry` | NASA Gantry (static, ~20x14x18 m at scale 1), scale 0.75 in `L_AssemblyBay` NE corner; override mats `MI_Gantry_Steel`, `MI_Gantry_BlueGrey`, `MI_Gantry_Hazard`. |
| `Construction/Env/Meshes/MobileLauncherPad/MobileLauncherPad` | NASA Mobile Launcher pad (skin stripped), unused; 1 uu = 1 m. |
Source GLBs: `SourceArt/NASA3D_Env/`. Import script: `Tools/Env/import_env_models.py`.

| `UI/WBP_ResourceGraphs` (agent D) | Live dashboard widget, 1920x1080, transparent. T/W gauge with 1.3/2.0 markers, 9 coloured bars (`Bar0..8`/`Val0..8`), `CriticalText`, `FootText`. Tick-driven `Refresh`/`SetBar`. Meant for a 3D `BP_GraphBoard` (not yet created) on the west wall. |
