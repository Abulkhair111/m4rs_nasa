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

### Hall textures (agent A)
Source: ambientCG (https://ambientcg.com), licence CC0 1.0. `T_Hall_Wall_*` = Concrete034, `T_Hall_Floor_*` = MetalPlates006, `T_Hall_Ceiling_*` = Metal038 (1K JPG, Color + Roughness) in `Construction/Env/Textures/`; originals in `SourceArt/AmbientCG/`. Master material `Construction/Env/M_HallTiled` (world-aligned, so stretched cube walls tile correctly); instances `MI_Hall_Wall/Floor/Ceiling`.

## Web models (agent H, 2026-11-14) - `Art/Meshes/Web/<Name>/`
Source files and `SOURCES.txt` (URL, author, licence) in `SourceArt/WebModels/<Name>/`. All three are NASA 3D Resources (github.com/nasa/NASA-3D-Resources, public domain per NASA media guidelines). `StaticMeshTools.import_file` accepts only **FBX/OBJ** (not GLB), so models were converted with Blender 5.2 (`C:\Program Files\Blender Foundation\Blender 5.2`, FBX export `global_scale=1`, `FBX_SCALE_NONE`, axes -Y/Z gives 1 m = 100 uu). Nanite not enabled.
| Asset | Real size | Notes |
|---|---|---|
| `NASA_SLS_Block1/SLS_Block1` | 22.4 x 11.3 x 98.1 m (uu: 2236 x 1134 x 9812), origin bottom-centre, Z up | SLS Block 1 with both boosters, 274k tris, from the NASA 3D-print STL (untextured; one light-grey material `M_Space_Launch_System_`). Launcher visual for agent A. Heavy-ish, use LODs or distance only. |
| `NASA_InSight_Lander_T/InSight_Lander` | 6.1 x 2.8 x 1.6 m (panels deployed), 147k tris, 17 materials + 4 textures | Textured NASA JPL InSight lander (legs, deck, two round solar arrays, arm). Best-detail lander available; a Mars lander, not a Moon one. Origin near deck level (z -64..+95 cm). Folder has a `_T` suffix, ignore. |
| `NASA_NewHorizons_RTG/NH_RTG` | 0.35 x 0.35 x 1.13 m, origin bottom-centre, 4k tris | New Horizons GPHS-RTG, dark metal material. **Assigned to `DA_Part_RTG`** (MeshScale 1.8, colour alpha 0). |
No CC0/CC-BY Falcon 9, Vulcan, Falcon Heavy, Artemis HLS or textured Apollo LM could be downloaded without login (Smithsonian 3D has no LM/rocket; Sketchfab/CGTrader/TurboSquid need login or are paid). Not imported: SLS multi-GB 7z, Gateway (66 MB GLB), Space Exploration Vehicle (22 MB).

## Zone markings (agent I, 2026-11-14) - `Construction/Env/`
`M_ZoneDecal` (Deferred Decal, translucent, emissive; params Color/Opacity/Glow; exposure is manual bias 9.5 so keep Glow <= 1) and MIs `MI_Zone_Lander`, `MI_Zone_Ascent`, `MI_Zone_Lander_Line`, `MI_Zone_Ascent_Line`, `MI_Zone_Divider`. `MI_Proto_Mission` is now dark steel instead of pink.

## Rocket parts (agent J, 2026-11-14) - `Art/Meshes/Rocket/<Part>/<Part>` (26 static meshes)
For KSP-style stackable rockets. Every mesh: origin at the **bottom centre**, +Z up, nose/payload end up and engine end down, 1 uu = 1 cm (FBX scale 1), Nanite OFF, no geometry edited. Stack by placing the next part's origin at the previous part's top (z = height). Sources, SOURCES.txt and the FBX files are in `SourceArt/WebModels/<Pack>/` (`NASA_SaturnV_Modular`, `NASA_Orion`, `NASA_ShuttleGemini`, `Kenney_SpaceKit`). Each part has its own folder with materials/textures. Thumbnails checked with CaptureAssetImage (orientation OK). Sizes are metres; "r bot/top" = attach radius at the bottom/top end (m). Saturn V scale is approximate (uniform 0.1117 so the S-IC is 10.06 m wide).
Licences: NASA = NASA 3D Resources, public domain per NASA media guidelines (no NASA logo use, no endorsement). Kenney = CC0.

| Part | Kind | Height | Diameter | r bot / top | Notes | Licence |
|---|---|---|---|---|---|---|
| `SaturnV_CommandModule` | capsule | 2.24 | 3.8 | 1.5 / 0.9 | Apollo CM, 3.6k tris | NASA PD |
| `SaturnV_ServiceModule` | service module (under capsule) | 9.09 | 4.8 | 1.5 / 1.8 | Apollo SM with engine bell at the bottom, 46k tris | NASA PD |
| `SaturnV_EscapeTower` | nose / LES | 12.39 | 4.3 | 2.2 / 0.2 | Launch escape tower with its own boost cone, 16k tris | NASA PD |
| `SaturnV_LemShroud` | fairing / adapter | 7.11 | 6.7 | 3.3 / 2.2 | Conical LM adapter | NASA PD |
| `SaturnV_S3` | tank + engine (upper stage) | 20.3 | 7.4 | 1.3 (nozzle) / 2.6 | S-IVB with J-2 | NASA PD |
| `SaturnV_Interstage` | decoupler / interstage | 10.82 | 11.3 | 4.6 / 4.6 | S-IC/S-II coupler (v2) | NASA PD |
| `SaturnV_S2` | tank + engines (mid stage) | 29.57 | 11.0 | 3.5 / 3.4 | S-II with 5 J-2 at the bottom, 16k tris | NASA PD |
| `SaturnV_S1_Upper` | tank | 18.69 | 10.9 | 5.4 / 5.0 | S-IC tank section | NASA PD |
| `SaturnV_S1_Lower` | engine stage + fins | 22.68 | 13.9 (fins) | 6.5 / 5.4 | S-IC with 5 F-1 + 4 fins, 38k tris | NASA PD |
| `SaturnV_Fin` | fin | 3.24 | 5.0 (span) | - | Single fin, 66 tris | NASA PD |
| `Orion_CrewModule` | capsule | 3.28 | 5.0 | 1.1 / 0.7 | Orion crew module, 52k tris | NASA PD |
| `ESAS_CrewModule` | capsule | 2.19 | 4.2 | 2.1 / 0.4 | Simple CEV capsule, 650 tris | NASA PD |
| `Gemini_Capsule` | capsule | 4.17 | 2.2 | 1.1 / 0.2 | Textured Gemini, 39k tris, units guessed from feet | NASA PD |
| `Shuttle_ExternalTank` | tank (large) | 47.74 | 11.6 incl. fittings (body 8.4) | 4.9 / 1.5 | Ogive nose on top | NASA PD |
| `Shuttle_SRB` | booster (side) | 45.24 | 5.9 | 2.9 / 0.2 | Solid booster, nozzle at the bottom, nose cone on top | NASA PD |
| `Shuttle_RS25` | engine | 2.60 | 2.5 | 1.3 / 0.5 | Shuttle main engine bell | NASA PD |
| `Kenney_Rocket_TopA` | nose cone (small) | 0.80 | 1.08 | 0.54 / 0.14 | Cartoon style, 50 tris | CC0 |
| `Kenney_Rocket_TopB` | nose cone with antenna | 1.10 | 1.08 | 0.54 / 0.04 | | CC0 |
| `Kenney_Rocket_FuelA` | tank (short) | 0.50 | 1.08 | 0.54 / 0.54 | | CC0 |
| `Kenney_Rocket_FuelB` | tank (tall) | 1.00 | 1.08 | 0.54 / 0.54 | | CC0 |
| `Kenney_Rocket_SidesA` | side booster | 1.00 | 1.08 | 0.54 / 0.54 | Mount on a tank's side | CC0 |
| `Kenney_Rocket_SidesB` | side booster (wide) | 1.00 | 1.32 | 0.54 / 0.54 | | CC0 |
| `Kenney_Rocket_BaseA` | engine (with legs) | 1.60 | 1.8 | 0.9 / 0.6 | | CC0 |
| `Kenney_Rocket_BaseB` | engine (short) | 1.30 | 1.8 | 0.9 / 0.5 | | CC0 |
| `Kenney_Rocket_FinsA` / `FinsB` | fins ring | 0.70 | 1.2 | 0.54 / 0.54 | Two variants | CC0 |

Consistent sets: the Kenney set is a full stack (Base -> Fuel -> Sides/Fins -> Top) at one size. The NASA parts are real-size (m) and cover a Saturn V stack (S1_Lower, Interstage, S2, S3, LemShroud/CommandModule/ServiceModule, EscapeTower), a Shuttle-style stack (ExternalTank + 2 SRB + RS25) and capsules. Use `MeshScale` in the part data asset to bring the toy-sized Kenney parts (1 m) in line with the 1 uu = 1 cm NASA parts, or scale the NASA parts down (about 0.05-0.1) for the hall. Gaps: no real decoupler ring (the Saturn interstage is the only one), no 2nd small/medium real-size tank (Kenney FuelA/FuelB are the small ones), no landing legs.
Restricted, NOT imported (non-commercial/no-derivative licences): KSP mods Kerbodyne Plus (CC BY-NC-SA 3.0), Fuel Tanks Plus and KRnD (CC BY-NC-SA 4.0), Space Shuttle Engines by dtobi (CC-BY-ND). KSP `.mu` models are also not convertible easily. Paid/unclear (not imported): Fab "Low Poly Rockets - Modular Parts Kit" (93 parts, licence not verified), itch.io MarinsPixel "7 Rockets" (paid), CGTrader modular rocket packs (royalty-free, paid). Not tried: Sketchfab/Poly Pizza (need login or no result).

## Better landers / engines / tanks (agent M1, 2026-11-14) - `Art/Meshes/Web/`
Source: github.com/alexmaietta/lunar-surface-mobility-assets (Alex Maietta), **CC0 1.0** (credit appreciated, not required). Sources, LICENSE/NOTICE and FBX in `SourceArt/WebModels/LunarLanders_Maietta/`, converter `Tools/WebModels/glb_to_fbx.py` (Blender 5.2: Z up, origin bottom-centre, 1 m = 100 uu, textures downscaled to 1024; Blender exports only base colour + normal, so no metal/roughness maps). All checked with CaptureAssetImage (orientation OK, textured). Nanite off.
| Asset | Real size | Notes |
|---|---|---|
| `Lander_ApolloLM/ApolloLM_Detailed` | 9.45 x 9.45 x 6.94 m (legs out), 27k tris, 17 materials | **Use this as the lander.** Textured Apollo LM (gold foil, legs, antennas), origin bottom-centre of footpads, upright. Replaces the NASA `ApolloLM` GLB (poor). Not yet placed in a level or BP (M2). |
| `Lander_NovaC/NovaC` | 4.29 x 4.90 x 4.23 m, 18k tris | Textured Intuitive Machines Nova-C style lander, alternative. |
| `Engine_BE7/BlueEngine_BE7` | 0.80 x 0.80 x 1.72 m | Full engine (bell, chamber, feed lines), nozzle down. **DA_Part_EngineBasic** (MeshScale 1.0). Extracted from the Blue Moon MK1 model. |
| `Engine_ApolloDPS/ApolloDPS_Engine` | 1.63 x 1.63 x 2.58 m | Apollo descent engine. **DA_Part_EngineThrottle** (MeshScale 0.7 -> 1.8 m). |
| `Engine_NovaC_VR900/NovaC_VR900_Engine` | 0.49 x 0.49 x 1.31 m | Nova-C engine, not assigned (free to use). |
**Tanks:** `DA_Part_TankSmall` / `TankLarge` now use `Rocket/SaturnV_S1_Upper` (white cylinder, 18.7 x 10.9 m) at MeshScale 0.065 (1.2 x 0.7 m) / 0.1 (1.9 x 1.1 m). The previous Shuttle ExternalTank was a 47 m long missile shape with a muddy texture scaled to ~6 cm.
**Why the old engines looked "cropped":** `NASA/ShuttleEng` and `ShuttleRCS` are the same open bell skirt only (no chamber/pumps); not a mesh/culling bug. Not fixed in the files, just replaced.
**Not imported, for the user to decide (KSP and others):** Kerbodyne Plus https://github.com/bonus-eventus/kerbodynePlus (CC BY-NC-SA 3.0); Space Shuttle Engines by dtobi https://spacedock.info/mod/3009 (CC-BY-ND-4.0); SpaceY Expanded (CC-BY-NC-SA 4.0); Mecripp/z_thing https://github.com/Mecripp/z_thing (repo says MIT, but the original parts are "dwtfywwi" and the parts' origin is unclear, not verified); zer0Kerbal/LaunchPack (licence unclear); CGTrader/RenderHub "royalty free" engines and tanks (not CC). Game files of KSP were not touched. Open: no real small propellant tank with a KSP-like look; no landing legs as separate parts.

## Part display table (agent M2, 2026-11-14) - for the C++ migration
How each mesh really sits (checked with thumbnails and editor previews of `BP_Part` on the shelves; bounds from `StaticMeshTools.get_bounds`). **Rotation** = pitch/yaw/roll applied to the mesh component (equivalent of `UMCPartDef::StackMeshRotation`; in Blueprints it is the new `PDA_PartDef.MeshRotation` read by `BP_Part.ApplyVisual`). **Offset** = not stored: `BP_Part.FitMesh` computes it from the rotated component bounds (mesh centre XY on the actor origin, bottom on the origin); the C++ port should do the same (`Mesh->Bounds`, bottom = Origin.Z - BoxExtent.Z, centre = Origin.XY in actor space). **Size** = real size in cm after rotation and scale (X x Y x Z, Z up).

| Part (DA_) | Mesh | Final scale | Rotation P,Y,R | Size cm (X x Y x Z) | Native orientation / notes |
|---|---|---|---|---|---|
| SolarSmall | NASA SatWings1 | 11.0 (was 15.6) | 90,0,0 | 2 x 108 x 99 | Native: flat sheet in XY. Pitch 90 stands it up facing local +X (the shelf front). Scale reduced so it does not overlap neighbours. |
| SolarLarge | NASA SatWings3 | 5.8 (was 8.31) | 90,0,0 | 1 x 150 x 150 | Same, four-panel cross. |
| RTG | Web NH_RTG | 1.8 | 0,0,0 | 62 x 62 x 202 | Native Z-up, origin bottom-centre. |
| BatterySmall | NASA SatBody1 | 10.83 | 0,0,0 | 21 x 21 x 65 | Native Z-up cylinder, origin at centre. |
| BatteryLarge | NASA ORU | 0.0053 | 0,0,0 | 92 x 80 x 75 | Raw GLB units are huge (17350 uu), origin far off-centre. |
| AntennaLow | NASA SatRadio1 | 4.33 | 0,0,0 | 20 x 19 x 63 | Z-up. |
| AntennaHigh | NASA VoyagerAntenna | 0.0417 | 0,0,0 | 66 x 126 x 93 | Origin far off-centre; dish tilts, mast diagonal. |
| EngineBasic | Web Engine_BE7 (M1) | 1.0 | 0,0,0 | 80 x 80 x 171 | Native Z-up, nozzle down, origin bottom. The old ShuttleRCS/ShuttleEng skirts were X-axis bells and needed pitch +90. |
| EngineThrottle | Web Engine_ApolloDPS (M1) | 0.7 | 0,0,0 | 114 x 114 x 180 | Native Z-up, nozzle down. |
| Lidar | NASA LLCD | 0.0026 | 0,90,0 | 20 x 70 x 15 | Native: long axis X, flat. Yaw 90 so it runs along the shelf. |
| TankSmall | Rocket SaturnV_S1_Upper (M1) | 0.065 | 0,0,0 | 65 x 65 x 121 | Native Z-up, origin bottom. The old Shuttle ExternalTank lay along X (nose to +X) and needed pitch +90. |
| TankLarge | Rocket SaturnV_S1_Upper (M1) | 0.10 | 0,0,0 | 100 x 100 x 186 | Same mesh, bigger. |
| NIRVSS | NASA InfraredCamera | 0.00197 | 0,0,0 | 69 x 55 x 23 | Flat on its base, native OK. |
| NSS | NASA TOMS | 0.0193 | 0,0,0 | 75 x 23 x 47 | Native OK (origin below the body). |
| MSolo | NASA COS | 0.00473 | 0,0,0 | 32 x 75 x 32 | Box, native OK. |
| Camera | NASA WFPC | 0.00832 | 0,0,0 | 60 x 59 x 24 | Curved plate, native OK (origin off in Y). |
| Drill | Engine Cylinder (placeholder) | 0.26, 0.26, 1.56 | 0,0,0 | 26 x 26 x 156 | Engine BasicShape, 100 uu, centred origin. |
| Rover | Perseverance | 0.39 | 0,0,0 | 105 x 159 x 87 | Origin at wheel bottom (Z-up). |
| DA_Tool_Wrench | NASA Wrench | 0.00477 (was 0.00318) | 0,0,0 | 14 x 74 x 4 | Flat tool, long along Y, lies on its plinth. Enlarged x1.5 so it reads. |
| DA_Tool_Cable | NASA Tether1 | 0.00311 (was 0.00207) | 0,0,0 | 10 x 52 x 10 | Enlarged x1.5. |

**Why parts were in the floor / cropped (old state):** (1) `BP_Part.FitMesh` was never executed (its entry exec pin was not connected), so every mesh kept its raw pivot (several NASA meshes have origins hundreds of mesh units from their centre); (2) long parts lay on their side because the DA had no rotation field. Both fixed (see [[Status]] "Agent M2"). `FitMesh` now resets the mesh to 0, reads `GetComponentBounds`, moves the mesh by minus the centre (in actor space) and then up by the box half-height.

**Stands (agent M2):** `Construction/Env/Stands/MI_Stand_Base` (dark metal) and `MI_Stand_Trim_{Power,Propulsion,Science,Tools}` (emissive trims: yellow / orange / green / blue). Level actors live in outliner folders `Stands/<Dept>` (`M2_PlinthBase_*`, `M2_PlinthRing_*`, `M2_Label_*`, `M2_Spot_*`, `M2_Trim*`), made of engine Cylinder/Cube + TextRender + SpotLight.

## Rocket paint (agent N2, 2026-11-14) - `Art/Materials/Rocket/`
No better textured launch vehicle with a redistributable licence could be downloaded (checked NASA 3D Resources Saturn V/Shuttle/Ares/Atlas GLBs: Saturn V is the textured one already used on the pad, the Shuttle/Atlas/Ares ones are untextured; Smithsonian, Poly Pizza, OpenGameArt, CGTrader/Meshy: no real SLS/Falcon with CC0/CC-BY and direct download). So the existing meshes were **painted** with material instances (no geometry edited).
- Masters: `M_RocketPaint` (LocalPosition roll pattern: ColorA/ColorB, RollColumns, BandHeightCm, StripeAmount, PhaseCm, Roughness, Metallic) and `M_RocketSLS` (colour by local position: ColorCore orange for |x|<BoosterXCm, ColorWhite for boosters/top above TopZCm, ColorEngine dark below EngineZCm). Foil/dark-metal MIs reuse `Construction/Env/M_HallTiled` (world-aligned textures).
- MIs: `MI_Rocket_White`, `_WhiteRoll` (white + black roll squares), `_WhiteRollSmall`, `_OrangeFoam`, `_BlackGloss`, `_EngineMetal`, `_Bronze`, `_NasaRed`, `_Silver`, `_SLS`, `_GoldFoil`, `_DarkFoil`, `_DarkMetal` (last three use ambientCG textures in `Textures/`).
- Textures: ambientCG Foil002 (gold), Foil001 (dark), Metal027 (dark anodised), CC0 1.0, 1K JPG; originals + `SOURCES.txt` in `SourceArt/AmbientCG_Rocket/`.
- Assigned (mesh slots): `Web/NASA_SLS_Block1/SLS_Block1` -> `MI_Rocket_SLS` (orange core, white boosters/top, dark engine section); `Rocket/SaturnV_S1_Lower` and `S1_Upper` -> WhiteRoll, `S2/S3/LemShroud/ServiceModule/Fin` -> White, `Interstage` -> BlackGloss, `CommandModule` -> Silver, `EscapeTower` -> WhiteRollSmall; `Shuttle_ExternalTank` body/nose -> OrangeFoam, fittings/lines -> EngineMetal; `Shuttle_SRB` -> White; `Shuttle_RS25` bell in -> DarkMetal, out -> Bronze; `Orion_CrewModule` -> White. Not repainted: Kenney (cartoon, already coloured), ESAS/Gemini (textured), the skeletal `NASA/SaturnV/SaturnV` (original NASA texture, hidden on the pad now).
- **Pad launcher:** `Maquette_Rocket_SLS` (StaticMeshActor, `SLS_Block1`, loc 900,-750,298, scale 0.28 = 27.5 m tall, 6.3 x 3.2 m, upright on the pad) replaces the Saturn V skeletal actor; the old actor `SkeletalMeshActor_0` is hidden (bHidden) in folder `Maquette/Rocket_OldSaturnV_hidden`. Maquette lander moved from the rocket top to the floor at (470,-750,-2).

## C++ part defs (Agent P1, 2026-11-14) - `/Game/MissionCrew/Construction/Data/MC/`
Class `UMCPartDef`, one per old Blueprint `DA_Part_*`; values copied 1:1 from the DAs (the old DAs are untouched). The BP `meshRotation` became `StackMeshRotation` (also applies to the `Mesh` fallback). Lander parts have no `StackMesh` (shelf `Mesh`+`MeshScale` is used). Ascent parts have `StackMesh`, `StackMeshScale` 1 and `StackHeightCm` (uniform fit, radius measured from the mesh); the stack is auto-scaled by `DisplayScale`.

| Old DA | New asset | Group | Notes |
|---|---|---|---|
| DA_Part_SolarSmall / SolarLarge | MCP_SolarSmall / MCP_SolarLarge | Lander | rot 90,0,0 |
| DA_Part_RTG | MCP_RTG | Lander | |
| DA_Part_BatterySmall / BatteryLarge | MCP_BatterySmall / MCP_BatteryLarge | Lander | |
| DA_Part_AntennaLow / AntennaHigh | MCP_AntennaLow / MCP_AntennaHigh | Lander | |
| DA_Part_Lidar | MCP_Lidar | Lander | rot 0,90,0 |
| DA_Part_NIRVSS / NSS / MSolo / Camera | MCP_NIRVSS / MCP_NSS / MCP_MSolo / MCP_Camera | Lander | science |
| DA_Part_Drill | MCP_Drill | Lander | Engine Cylinder placeholder |
| DA_Part_Rover | MCP_Rover | Lander | |
| DA_Tool_Wrench / DA_Tool_Cable | MCP_ToolWrench / MCP_ToolCable | (tool) | ToolType Wrench / Cable, SocketType Tool |
| DA_Part_EngineBasic | MCP_EngineBasic | Ascent | StackKind Engine, BE7, 172 cm |
| DA_Part_EngineThrottle | MCP_EngineThrottle | Ascent | Engine, Apollo DPS, 258 cm |
| DA_Part_TankSmall / TankLarge | MCP_TankSmall / MCP_TankLarge | Ascent | Tank, SaturnV_S1_Upper, 260 / 460 cm |

New defs (all numbers TBD placeholders, flagged in the Description), all Group Ascent:
| Asset | Socket / StackKind | Mesh | StackHeightCm | Mass / cost / other |
|---|---|---|---|---|
| MCP_TankMedium | Tank / Tank | SaturnV_S1_Upper | 360 | 48 kg, $8M, fuel 850 kg, dept 3 |
| MCP_BoosterSRB | Booster / Booster, radial x2 | Shuttle_SRB | 600 | 80 kg, $12M, thrust 9000 N, heavy, dept 3 |
| MCP_BoosterSRBShort | Booster / Booster, radial x2 | Shuttle_SRB | 400 | 45 kg, $7M, thrust 5000 N, dept 3 |
| MCP_Decoupler | Structure / Decoupler | SaturnV_Interstage | 250 | 12 kg, $3M, dept 2 |
| MCP_Capsule | Structure / Capsule | Orion_CrewModule | 160 | 30 kg, $15M, dept 2 |
| MCP_Fairing | Structure / Fairing | SaturnV_LemShroud | 220 | 18 kg, $5M, dept 2 |

Launcher presets (`UMCRocketPreset`, `Data/MC/Presets/`; preset part defs `MCP_Preset_Stage1` S1_Lower 700 cm, `_Interstage` 150 cm, `_SideSRB` Shuttle_SRB 450 cm, `_SideCore` Shuttle_SRB 700 cm, not in the Catalog; stand-in meshes, no Falcon/Vulcan model exists):
| Asset | LauncherName | CoreParts | Radial |
|---|---|---|---|
| MCP_Preset_Falcon9 | Falcon 9 | Stage1 | none |
| MCP_Preset_Vulcan | Vulcan | Stage1, Interstage | SideSRB x2 |
| MCP_Preset_FalconHeavy | Falcon Heavy | Stage1, Interstage | SideCore x2 |

Not created (no model, rule: don't invent geometry): lander gear/legs part (Apollo LM has its own legs), a separate lidar/RTG variant. Ascent set `Catalog` for the manager = the 10 MCP ascent/lander defs above (assign to `Catalog` + `LauncherPresets` in the manager).

## Player bodies (Agent Q, 2026-11-14) - `Art/Meshes/Players/Z2Suit/`
- Source: NASA 3D Resources "Z2 Spacesuit" (`3D Models/Z2 Spacesuit/Z2 Spacesuit.glb`, public domain). `SourceArt/NASA3D_Players/Z2Suit/` (GLB, FBX, `SOURCES.txt`, `Z2Suit_tex/` 1K PNGs). Also downloaded but unused: `SourceArt/NASA3D_Players/Astronaut/Astronaut.glb` (NASA "Astronaut", 6k tris, T-pose, 2 flat materials; no rig, so rejected as a player body, usable as a decoration).
- Converter: `Tools/WebModels/glb_to_fbx.py` with scale 0.9346 (-> 180 cm tall, origin at the feet centre of the bounding box); `Tools/Players/render_check.py` makes Blender front/side/top previews of a GLB.
- `Z2Suit` (StaticMesh, 31k tris, no rig, relaxed pose, ~181 cm, X +-37 / Y +-48 after import: the model faces +/-Y in UE, see the yaw note below). Material slot `Baked` -> `MI_Player_Neutral`. Texture `T_Z2_Color`.
- `M_PlayerSuit` (master: base colour = `Color` texture x lerp(white, `Accent`, `Strength` 0.65)); instances `MI_Player_Neutral` (Strength 0), `MI_Player_Power` (yellow), `MI_Player_Construction` (cyan), `MI_Player_Propulsion` (orange), `MI_Player_Science` (green) = CrewRole 0..4.
- `Construction/Blueprints/BP_MC_BayPlayer` (child of `AMCBayCharacter`): see Status "Agent Q update": component `Body` = SkeletalMesh `SciFiAstronaut/SK_SciFiAstronaut` (rigged, CC0 Quaternius, 24 anims, role accent MIs `MI_Astro_Accent_*`), var `RoleMaterials`, Tick SetMaterial by CrewRole. `BP_MC_BayGameMode.DefaultPawnClass` = `BP_MC_BayPlayer_C`.

## Space assets (agent S2d, 2026-11-15) - `Art/Meshes/Space/`, `Art/Textures/Space/`
- `Space/M_PlanetTex` (Tex, Glow, emissive+base colour), `MI_Earth` (`Textures/T_Earth_BlueMarble_2k`, NASA SVS 3615 Blue Marble no clouds, 2K), `MI_Moon` (`Textures/T_Moon_LRO_2k`, NASA SVS 4720 LROC mosaic 2K). Apply to `/Engine/BasicShapes/Sphere` (UV equirect, prime meridian seam faces -X unless rotated; Earth preview uses yaw -90). No NASA Earth/Moon mesh exists. Public domain. Source: `SourceArt/NASA_Planets/SOURCES.txt`.
- `Space/M_SkyTex` (unlit, two-sided, Tex + Intensity), `MI_Sky_Stars`, `MI_Sky_Nebula`; textures `Art/Textures/Space/T_Sky_PlainStarfield_4k`, `T_Sky_BlueNebulae_4k` (4K copies of 8K PNG). **Credit "Space Spheremaps"** (https://space-spheremaps.itch.io/space-spheremaps, https://www.spacespheremaps.com; free incl. commercial, attribution appreciated, no AI scraping). Originals incl. `hazy_nebulae_1` and `ringed_gas_giant_planet` in `SourceArt/SpaceSpheremaps/` (not imported).
- Level `Maps/L_Transit`: flight deck, layout in [[Status]] "Agent S2d".
- Animated body (current): `Art/Meshes/Players/SciFiAstronaut/` = `SK_SciFiAstronaut` + Skeleton + 24 `..._Anim_CharacterArmature_CharacterArmature_<Name>` sequences + `M_PlayerFlat`, `MI_Astro_*`. Quaternius CC0, source `SourceArt/NASA3D_Players/SciFiAstronaut/`. Details and the animation-to-property table: [[Status]] "Agent Q update".

## Agent Z additions (2026-11-15)
- `/Game/MissionCrew/Construction/Blueprints/BP_MC_DeckGameMode` (GameModeBase; PlayerState BP_MC_PlayerState, pawn BP_MC_BayPlayer; L_Transit override).
- L_AssemblyBay C++ interactables/boards (MCReadyButton x4, MCLaunchButton, MCToolStand, MCFuelPump x2, MCRocketLever x3, MCWallBoard x3, MCMassScale, MCPowerStrip) and L_Transit MCStation x4 / MCTransitManager / MCTransitDisplay, see Status 'Agent Z'.

## Agent M3b: Falcon-style launcher pieces and drill (2026-11-15)
Sources, SOURCES.txt, GLB and FBX in `SourceArt/WebModels/Poly_FalconHeavy/` and `Poly_Drill/`. Splitter: `Tools/WebModels/split_falcon_heavy.py` (Blender 5.2, whole objects only, no geometry edited). Origin bottom-centre, Z up, 1 model unit = 1 m = 100 uu. Nanite off.
- **SpaceX Falcon Heavy** (Poly Pizza https://poly.pizza/m/bdMY5H0ds2T, Carwyn Pelley, **CC-BY 3.0: credit "SpaceX Falcon Heavy by Carwyn Pelley (Poly Pizza), CC-BY 3.0" in the game credits**). Stylised, untextured, 24.7k tris, 4 flat materials (slots FH_Body / FH_Structure / FH_EngineDark / FH_Skirt, mapped to `MI_Rocket_White` / `_EngineMetal` / `_DarkMetal` / `_BlackGloss`). Engine skirts are exaggerated and there are no landing legs. Pieces in `Art/Meshes/Rocket/` (size X x Y x Z cm):
| Mesh | Size | Use |
|---|---|---|
| `Falcon_Stage1` | 62 x 62 x 841 | first stage core with engines and skirt, no fairing |
| `Falcon_Fairing` | 62 x 62 x 146 | payload fairing (ogive) |
| `Falcon_SideBooster` | 62 x 72 x 735 | side core with nose cone and engine skirt |
| `Falcon_TankCore` | 46 x 43 x 611 | plain stage tank (no engines), used for all 3 tanks and the interstage |
| `Falcon_TankBooster` | 46 x 62 x 505 | side-booster tank with clamps, not assigned |
| `Falcon_Heavy_Full` | 64 x 158 x 987 | whole Falcon Heavy (3 cores + fairing), not assigned (free for pad/maquette) |
- **Drill**: `Web/Poly_Drill/Poly_Drill` (Kay Lousberg "Space Base Bits" via Poly Pizza https://poly.pizza/m/8uBbH7Dvmb, **CC0**), 175 x 175 x 200 cm, 1.2k tris, cartoon style with a texture.
- **Part defs changed** (`Construction/Data/MC/`): `MCP_TankSmall` / `MCP_TankMedium` / `MCP_TankLarge` -> Mesh and StackMesh `Falcon_TankCore`; shelf MeshScale (1.4,1.5,0.2) / (1.9,2.0,0.245) / (2.2,2.3,0.3) = about 65x65x122 / 87x86x150 / 101x99x183 cm (non-uniform so the slender tank reads as a stub); stack height/radius 260/45, 360/60, 460/75 cm (StackRadiusCm > 0 fits XY non-uniformly). `MCP_Fairing` -> `Falcon_Fairing` (MeshScale 1.5, stack 220 cm, radius measured). `MCP_Drill` -> `Poly_Drill` (MeshScale 0.5 = 88x88x100 cm, colour alpha 0). `Presets/MCP_Preset_Stage1` -> `Falcon_Stage1` (700 cm, radius 60), `Presets/MCP_Preset_Interstage` -> `Falcon_TankCore` (120 cm, radius 42), `Presets/MCP_Preset_SideCore` -> `Falcon_SideBooster` (700 cm, radius 60). `MCP_Preset_SideSRB` (Vulcan solids) unchanged. Falcon 9 / Vulcan / Falcon Heavy presets pick these up through the shared Stage1/Interstage/SideCore defs. Old BP `DA_Part_*` untouched.
- **Update M3c:** see Agent M3c section; Falcon_* stylised meshes are mostly unused now.
- **Looked at, not imported:** BlendSwap "SpaceX Falcon 9 Reusable" (CC0 but download needs login), Kay Lousberg Lander A/B/Base and Kenney "Satellite Dish" (CC0, cartoon style, no better than the NASA/Apollo LM models), Quaternius solar panels/antennas (toy style). NASA 3D Resources has no SLS core/ICPS/Orion service module/Gateway (66 MB, not converted), drill or landing-leg model; its Mars 2020 sample tube, Agena, Atlas, Redstone and Ingenuity models are not better tanks/legs.
- **Still weak:** no separate landing legs/footpads (Apollo LM keeps its own), no real small/medium tank with KSP look (tanks are one stretched Falcon cylinder in 3 sizes), Falcon pieces are flat grey (painted with the existing rocket MIs), antennas/solar unchanged, drill is cartoon style, rover-mounted drill not found.

## Space looks, holo, screens, effects (Agent V, 2026-11-15) - `Art/Materials/Space/`
All unlit emissive masters (own sun shading), so they work under any exposure; tune with `Brightness`/`Intensity`. Parent changes keep old asset paths.
- **Planets** (`Art/Meshes/Space/`, apply to engine `Sphere`): `MI_Earth` (parent `M_PlanetEarth`): day texture `DayTex` (Blue Marble), night side `NightTex` = `Textures/Space/T_Earth_BlackMarble_2k` (NASA Black Marble 2016, city lights), procedural drifting clouds (noise, `CloudScale` 4, `CloudSpeed`, `CloudThreshold`, `CloudContrast`, `CloudAmount`; **no NASA cloud map was reachable, so clouds are noise**), ocean specular (`SpecPower`, `SpecStrength`), Fresnel atmosphere rim (`AtmoColor`, `AtmoPower`, `AtmoStrength`), `SunIntensity`, `Ambient`, `NightStrength`, `Brightness` (2.5 in the deck). `MI_Moon` (parent `M_PlanetMoon`): LRO colour `Tex`, normal built from NASA LOLA height `Textures/Space/T_Moon_LOLA_1k` with 4 height taps (`BumpStrength` -30, tune sign if relief looks inverted), phase/terminator from `SunDir`, `Brightness`. **Both need `SunDir` (vector, world-space direction FROM the planet TO the sun) set per frame from C++ via a MID** (MI default (-0.55,-0.3,0.55)). Slow rotation is NOT in the material (rotate the component/actor, e.g. 0.5 deg/s; clouds drift on their own).
- **Sky**: `MI_Sky_Stars` re-parented to `M_SkySpace` (unlit, two-sided): stars `Tex` x `Intensity` (16), per-star twinkle (`TwinkleAmount` 0.35, `TwinkleSpeed`), blended nebula layer (`NebulaTex` = BlueNebulae 4K, `NebulaIntensity`, `NebulaTint`, slow `NebulaDrift` = parallax-like), a sun disc + glow computed from `SunDir` (`SunBrightness`, `SunGlowPower`, `SunGlowStrength`, `SunColor`); sun direction uses world position minus actor position, so it works on a sky sphere anywhere. L_Transit `SunDir` = (0.814,0.296,0.5), matching `Deck_OutsideSun` (pitch -30, yaw -160). Parallax proper and dust particles were not done (no Niagara asset used).
- **Holo** `M_Holo` (additive, unlit, two-sided): Fresnel edge glow, world-Z scanlines moving with time, flicker. Params `Color`, `Intensity` 3, `ScanSpeed` 0.6, `ScanDensity` 0.12 (1/cm), `ScanAmount`, `FlickerAmount`, `FresnelPower`, `BaseFill`, `Opacity`. MIs `MI_Holo_Cyan/Orange/Green/Yellow`. Applied to the display `ShipMarker` (Yellow). **Dynamic parameter list for C++ (CreateDynamicMaterialInstance): `Intensity`, `Color`, `ScanSpeed`.**
- **Deck screens** `M_DeckScreen` (opaque emissive): scrolling grid, scanlines, pulse, vignette; params `RoleColor`, `Intensity`, `GridDensity`, `ScrollSpeed`, `ScanlineDensity`, `ScanlineAmount`, `PulseSpeed`, **`ScreenTexture` (Texture param) + `ScreenMix`** (0 procedural, 1 texture). MIs `MI_Screen_Cyan` (Systems), `_Orange` (Equipment), `_Green` (Pilot), `_Yellow` (Science), and `MI_Screen_FlightFeed` (ScreenMix 1, for the `AMCFlightView` viewscreen: set `ScreenMaterialSlot` mesh to it, `ScreenTextureParam` = `ScreenTexture`). Trim: `M_EmissivePulse` + `MI_Trim_Cyan/Orange/Green/Yellow` (`Color`, `Intensity`, `PulseSpeed`, `PulseDepth`, `FlowScale`).
- **Flight effects** (not placed, `AMCFlightView` has no material properties yet): `M_EnginePlume` / `MI_EnginePlume` (additive, UV-based: needs a cone/cylinder with V along the length; params `CoreColor`, `OuterColor`, `Intensity`, `CorePower`, `FadePower`, `FlipV`, `NoiseTile`, `FlowSpeed`, `NoiseAmount`, `Throttle` 0..1 = thrust) and `M_RcsPuff` / `MI_RcsPuff` (world-noise sphere puff; `Color`, `Intensity`, `Age` 0..1 driven by C++, `CorePower`).
- **Outside**: `M_GroundTex` (world-XY tiled colour/normal/roughness, params `ColorTex`, `NormalTex`, `RoughTex`, `Tile`, `Tint`, `RoughScale`), `MI_OutsideGround` (ambientCG Ground054), `MI_PadConcrete` (ambientCG Concrete034), `MI_DeckFloor_Gloss` (made, **unused**: its roughness map shows as cobbles, the hall floor stays `MI_Hall_Floor`). Textures `Art/Textures/Ground/T_Ground054_{C,N,R}`, `T_Concrete034_{C,N,R}`.
- **Sources / licences**: NASA Black Marble 2016 (SVS 30876, public domain), NASA SVS 4720 LOLA height (public domain), ambientCG Ground054 + Concrete034 (CC0), Space Spheremaps (see Space assets above). Files and URLs in `SourceArt/NASA_Planets/SOURCES.txt` and `SourceArt/AmbientCG_Ground/SOURCES.txt`. `eoimages.gsfc.nasa.gov` is unreachable from this machine (timeouts).

## Agent M3c: launcher re-pointing (2026-11-15)
No new downloads: realistic free Falcon/Atlas/SLS meshes with a direct download do not exist (Sketchfab needs login, BlendSwap needs login, 3DModels.org is paid). Re-used NASA 3D Resources (public domain, credit "NASA 3D Resources", no endorsement): `Shuttle_SRB`, `Shuttle_ExternalTank`, `SaturnV_S2`, `SaturnV_S3`, `SaturnV_Interstage`. Credits line to add: "Launcher models: NASA 3D Resources (public domain). Fairing/Drill: Carwyn Pelley CC-BY 3.0 (fairing from Falcon Heavy), Kay Lousberg CC0." Mapping and heights in Status "Agent M3c".
