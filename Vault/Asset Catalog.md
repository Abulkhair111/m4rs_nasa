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
