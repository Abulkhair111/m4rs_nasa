# Status (update at the end of every session)

_Last updated: 2026-11-14_

## Done
- Roadmap, design summary, vault.
- Main menu: 3D backdrop (Perseverance rover on textured regolith, stars), WBP_MainMenu with callsign, Host (`L_Lobby?listen`), Quit, error display.
- Project settings (default map, GameInstance), git + LFS.
- **All player-facing text translated to English** (2026-11-14): designer texts and graph string literals in WBP_MainMenu, WBP_LanSessionRow, WBP_Lobby (incl. role cards), WBP_RoleCard, BP_MC_GameInstance error messages. Verified: no Cyrillic UI strings left in saved assets.
- **Mission-start video event** (2026-11-14): `BP_MissionVideoEvent` in `L_Mission` → `PlayMissionVideo` → fullscreen `WBP_MissionVideo` with SKIP → `OnMissionVideoFinished`. Tested in PIE: with the video file missing, the overlay closes itself and the game continues.
- **Main menu redesign** (2026-11-14, menu/art session): mission-control look. Navy glass panel (LeftShade, 800 wide) + cyan `EdgeLine`, `StatusBar` text at the top, cyan primary button (Host), dark Join, red-hover Quit, `MissionCard` (objective) bottom right, footer v0.2. Only styling and new decorative widgets; LAN logic untouched. `BP_MenuPlayerController` Tick: slow camera drift (±60 cm, yaw ±2.5°).
- **3D construction phase** (2026-11-14, menu/art session), design: `Vault/construction_phase.md` (it's a .docx with a .md name). Everything is in `/Game/MissionCrew/Construction` plus the test map `Maps/L_AssemblyBay`. Fully playable solo in PIE on `L_AssemblyBay`: rocket levers, 16 parts on 3 shelves, sockets with ghost preview, carry/snap/remove, two-person heavy carry, fuel pump/drain, mass scale, power ring, 2 wall boards, READY buttons, preflight, LAUNCH (red blocks, yellow needs a second press). Verified by a server-side self-test (`bDebugAutoTest` on the manager instance): rocket → install parts → fuel → READY ×4 → launch blocked with the right reason. **Keyboard/mouse input and the trace-based E interaction have not been tried by a human yet.** All models are **temporary engine-shape placeholders** (user approved), swap them in the `DA_Part_*` Mesh field and the BP components.

- **S1 Design & Assembly** (2026-11-14, menu/art session): the construction bay was remade to `CONSTRUCTION_PHASE_ICE_RETURN.md` S1. Changes:
  - Roles are Power, Construction, Propulsion and Science.
  - New mechanics: fuel tanks (fuel capped by tank capacity), engine thrust with a T/W check, a wrench to bolt modules, a cable reel to wire them to the power grid, and ice-capable instruments.
  - There are 10 checks, and the board, report and per-role accuracy log are updated to match.
  - Fixed old bugs: installed and dropped parts had collision off, so the E trace couldn't hit them. `Drop` read `Carrier1` after clearing it.

  Verified by `bDebugAutoTest` in PIE: the report before bolting lists unbolted, off-grid, no-thrust and no-ice problems. After wrench and cable, the yellow fuel margin is accepted on the second LAUNCH and `ConstructionFinished` fires. No runtime errors. **Still not tried by a human with keyboard and mouse.**

## In progress / open
- **S1 extras** (2026-11-14, menu/art): 17 NASA 3D Resources models were imported and assigned to the parts. Bounds-based spawn/drop and the `WBP_ResourceGraphs` dashboard are **unsaved in the editor / not finished**. Full handoff and next steps: [[S1 Design & Assembly - Handoff]].
- **S2 Launch & Transit** (2026-11-14, "transit" session): built in `/Game/MissionCrew/Transit/**` with the test map `Maps/L_Transit`. It compiles but is **not tested at runtime yet**. Full handoff: [[S2 Launch & Transit - Handoff]].
- S1 numbers (masses, costs, thrust, tank sizes, T/W limits 1.3/2.0, fuel 0.9 × dry +10%) are placeholders. The brief wants balanced numbers from the user.
- The lobby still shows the old role names (MCC Engineer, Flight Director, Pilot, Scientist) for CrewRole 1–4. In S1 they mean Power, Construction, Propulsion and Science. The lobby belongs to the other agent.
- S2–S6, briefings, `DT_*` tables and random events from `CONSTRUCTION_PHASE_ICE_RETURN.md` have not been started.
- **Craters: done** with real LOLA terrain (the user chose this option on 2026-11-14). The menu camera is on a 120 m crater rim. The craters at 5 m/px are moderate; for a dramatic scene, pick a rougher window with `Tools/LOLA/dem_scan.py` or use LDEM_875S near Shackleton.
- For `L_Mission`, plan to build a Landscape from the `.r16` (has collision and LODs), which is better for driving the rover.
- Git remote: `origin` = https://github.com/Abulkhair111/m4rs_nasa.git (main pushed 2026-11-14).
- LAN session browser in the main menu, built by **another agent**. Its state is unknown; it touched `BP_MC_GameInstance`, `WBP_MainMenu` and `WBP_LanSessionRow`.
- The other agent is building the lobby (`BP_MC_GameMode`, `BP_LobbyHUD` showed up in autosaves on 2026-11-14) and an IP-join row in the menu.

## Next
- **Construction phase, manual steps (can't be done via MCP):** in `BP_BayCharacter`, select custom events `ServerInteract` and `ServerRelease` → Details → Replicates: **Run on Server**, Reliable. Without this, clients (not the host) can't interact.
- **Hook construction into the mission:** after the briefing video, the server calls `BP_MC_GameMode.EnterConstructionPhase()`. It needs a `BP_ConstructionManager` in the loaded world, so add `L_AssemblyBay` to `L_Mission` as an Always Loaded sublevel (offset it away from the mission area), or ServerTravel to `L_AssemblyBay`. On LAUNCH the manager calls `BP_MC_GameMode.ConstructionFinished()` (stub that prints) and fires `OnConstructionComplete`. The finished loadout stays in the manager (sockets → parts) for the flight phase.
- Swap placeholder meshes for real models when the team sends them (lander body, 16 parts, 3 rockets, bay interior).
0. **User to supply the intro video** → `Content/Movies/MissionIntro.mp4` (H.264). Then test real playback and audio in PIE. Hook gameplay start to `OnMissionVideoFinished`.
1. `L_Lobby`: role selection (4 unique roles), replicated PlayerState, ready flags, host Start.
2. Core mission classes (GameState phases, PlayerController RPCs), see ROADMAP §1, §4.

## Ownership (to avoid collisions)
| Area | Owner |
|---|---|
| Main menu scene (`L_MainMenu` actors, Art/) | Claude session "menu/art" |
| LAN sessions (`BP_MC_GameInstance` session logic, `WBP_LanSessionRow`, menu LAN list) | other agent |
| Construction phase (`/Game/MissionCrew/Construction/**`, `L_AssemblyBay`), `WBP_MainMenu` styling, `BP_MenuPlayerController` | Claude session "menu/art" |
| S2 Launch & Transit (`/Game/MissionCrew/Transit/**`, `L_Transit`) | Claude session "transit" |
| `BP_MC_GameMode`: only `EnterConstructionPhase` + `ConstructionFinished` added by menu/art; the rest is the other agent's | shared |

## Agent A (Hall & lighting), 2026-11-14
- **A1 survey:** hall was 32x32x10 m of primitive cubes (walls StaticMeshActor_3..6, floor _1, ceiling _2, 9 emissive panels _7.._15), 5 point lights, no fog/PP/sky. Stack at origin.
- **A2 NASA candidates** (nasa/NASA-3D-Resources): `Gantry` (launch tower + service structure, ~20x14x18 m, static, used), `Mobile Launcher` (pad only, skinned, imported but unused, 1.0 uu = 1 m so scale ~8-30), `Mobile Launcher (assembled)`, `Crawler`, `Vehicle Assembly Building (VAB)` (solid exterior box with flag/logo, no interior, not usable as interior), `Headquarters Building`. **No real interior hall, platforms or railings model exists**; A4 (gantry platform levels at socket heights) is blocked, see "Needs user".
- **A3 done (partial):** the existing wall/ceiling placeholders were made 40 m tall and the hall extended by 29 m to the +X side (annex x 1625..4600) so the NASA `Gantry` (scale 1.4 at 3100,0,195) stands inside as the tall backdrop. No new primitive architecture was created, only existing actors were rescaled. `BP_WallBoard_C_1` moved to x=4565 (now hidden behind the gantry; agent D should place `BP_GraphBoard` on the west wall or near the stack instead).
- **A5 done:** dark navy `MI_Hall_Wall/Floor/Ceiling` (Env), `ExponentialHeightFog` volumetric, unbound `PostProcessVolume` (manual exposure bias 9.5, bloom, vignette), hero `SpotLight` over the stack (0,0,3800), warm work `PointLight`s (existing 0..4 retuned), 24 cyan `HallAccent_*` point lights at z 1400/2600/3500, 6 `HallGantrySpot_*` over the annex. SkyLight removed (needs SkyAtmosphere). All lights Movable.
- **A6:** screenshots in `Saved/capA/` (a1, a2, b1, b2, b3 .png). Level saved and verified on disk.
- Imports for env models: `Tools/Env/import_env_models.py` (SourceArt/NASA3D_Env -> `/Game/MissionCrew/Construction/Env/Meshes/<Name>`), separate from B's folder. Skinned GLBs need `Tools/NASA3D/strip_glb_skins.py` first.
- **Needs user:** a real hall interior / platform-level / railing / crane model (VAB interior, high-bay with catwalks) if desired; NASA has none. Otherwise A4 stays unbuilt.
- Note: concurrent MCP calls from other agents can leak their errors into your result (saw a `TestFn` Branch error twice); re-check state after such an error.

## Agent B (NASA models), 2026-11-14
- **Imported** (commandlet, `Art/Meshes/NASA/<Name>`): SaturnV, Ares1B, Atlas9, JupiterC, SRB, SuperLightTank, VikingLander, Pioneer10, HabitatHDU, BaseStation, LaunchPad, Crawler, VAB (+ duplicates Gantry, MobileLauncher; agent A uses its own copies in `Construction/Env`). RASSOR skipped (hung the import, >3 GB build). See [[Asset Catalog]].
- **Parts** (`DA_Part_*`): all 18 now have larger `MeshScale`, honest display names (name states the real NASA model) and a one-line `description` with the key numbers. **Agent C: the picker can show `Description` and `DisplayName` straight from the DA; the field already existed (no new field).** DA has no pivot or rotation fields, so orientation fixes would need an offset in `BP_Part`/socket (agent C's area).
- **Still without a real model:** RTG (cylinder), Drill (cylinder). NASA repo has no isolated RTG or drill; the Viking Lander and Pioneer 10 contain RTG housings but are whole probes. Mini rover stays the user's Perseverance. Falcon 9 / Vulcan / Falcon Heavy are not `DA_Part_*`; candidates for their visuals: Atlas9, SaturnV, Ares1B, JupiterC. Needs user decision if a real RTG/drill model is wanted.
- **B5 (Apollo LM in `BP_AssemblyStand`) not done**: the stand was not confirmed free. `ApolloLM` is 643x643x501 cm at import scale 1.
- A visual check (B1) of every part in a level was not done; sizes were verified from mesh bounds x MeshScale instead (all parts 25-250 cm longest side).

### Agent A follow-up (compact hall, gantry materials)
- **Hall shrunk** to 27x27 m footprint (walls at x,y = +-1350), still 40 m tall. The 29 m annex is gone. `BP_WallBoard_C_0/1` now at x=-+1315 (z 620) on the west/east walls. `PlayerStart_1` moved (650,650) -> (500,500). Shelves, sockets, levers untouched.
- **Gantry** is now at scale 0.75, yaw 90, location (1050,900,150): the tower sits in the NE corner (x 700..1350, y 560..1250), the rest of the model pokes through the east wall. Materials `MI_Gantry_Steel/BlueGrey/Hazard` (dark metal + orange accents; the GLB has no textures, only 3 flat materials).
- **Free spots for the graph board (agent D):** the whole west wall (x=-1350) except `WallBoard_0` at y=0; free bands y -1250..-500 and 500..1250 at z 300..1000 (launch button is low at (-950,700)). Also the north wall near x=-1000..-500. The east wall has `WallBoard_1` at y=0. The gantry never occupies the west half or any wall board.
- Stray lights: ~17 lights from the old annex were zeroed (Intensity 0) and parked at (0,0,3900); delete them (`PointLight_13..46` that are at 0,0,3900, and `SpotLight_1..6`) once no PIE runs, they are harmless. `Tools/Env/make_gantry_mis.py` creates the gantry MIs via commandlet.
- Screenshots: `Saved/capA/c1.png`, `c2.png`, `c3.png`.
- PIE was running (not started by me, no lock) during this pass, so asset creation/save and actor removal via MCP failed; MIs were created with a commandlet, actor edits applied in the editor world. **The level `L_AssemblyBay` still needs a save** once PIE ends (see report).

## Agent D (Graphs & TWR), 2026-11-14
- **Done and saved on disk:** `Construction/UI/WBP_ResourceGraphs` (1920x1080 canvas, transparent, no background). Big T/W gauge (`TWRBar` 0..3.0 with white markers at 1.3 and 2.0, `BigTWR` text), 9 rows (Mass, Budget, Power, Fuel, T/W, Science, Data link, Wired, Bolted) with bar + value text, `CriticalText` (names the highest pressure, "FAIL:" if over 100%), `FootText` (manager `Message`). Bars coloured green/yellow/red from `Checks[i]` (0/1/2). Logic: `Tick` -> GetActorOfClass(BP_ConstructionManager) -> cast -> `Mgr`; GetAllActorsOfClass(BP_PartSocket) -> `Sockets`; then `Refresh` (reads manager vars read-only, counts installed sockets into `InstalledF`) -> `SetBar` per row. Compiles with no errors from this widget.
- **Not done (blocked):** `BP_GraphBoard`, its level placement (suggested: west wall x=-1350 at y=-900 or 900, z 620, face +X), and the `WBP_BayHUD` T/W + fuel readout. A PIE session without `Saved/pie.lock` (not mine) kept running for 15+ minutes, so every asset tool (exists/duplicate/create/save) returned "Asset does not exist". Not tested in PIE or in a screenshot.
- **DSL lessons (Russian editor):** DSL operators `+ == select` fail (use `Утилиты|Операторы|Прибавить/Равно(==)/Больше(>)...`). `bind` of an impure node takes the `then` exec pin, so impure functions with outputs can't be used in DSL (keyword args only; I used pure vars `Mgr`/`Sockets` set by hand-wired Tick nodes). Positional args don't work; use `:Pin value`. `return` in a `fn` doesn't create the result node; add the output param and connect by hand.
- **Request to agent C:** exposing an `InstalledCount` on the manager would replace the socket loop in the widget.

### Agent A pass 3 (textures, gantry fit) - BLOCKED by a long-running PIE
- PIE has been running in the editor for over an hour (no `Saved/pie.lock`), so asset tools, actor removal and saving fail. While PIE runs, MCP actor tools may address the PIE world, so **my earlier compact-hall/gantry edits may not be in the saved level**: re-check `L_AssemblyBay` after PIE ends (walls at +-1350, gantry at 1050,900, WallBoards at x=+-1315).
- Commandlets don't run now: `проект.uproject` lists a C++ module `MissionCrew` (new `Source/MissionCrew`, not built), so `UnrealEditor-Cmd` aborts with "could not find game module".
- Staged and ready: CC0 textures from ambientCG in `SourceArt/AmbientCG/` (`SOURCES.txt`: Concrete034 walls, MetalPlates006 floor, Metal038 ceiling, 1K JPG, licence CC0 1.0, https://ambientcg.com). `Tools/Env/make_hall_materials.py` imports them and builds the world-aligned master `M_HallTiled` plus `MI_Hall_*` (same logic can be done with MaterialTools/TextureTools over MCP once PIE is off).
- Planned gantry fix: scale 0.4, yaw 90, location (1040,900,58): footprint x 756..1317, y 489..1302, fully inside the walls, clear of PlayerStart_1, shelves, boards.

## Ready to play (Agent G, 2026-11-14) - Blueprint bay `L_AssemblyBay`
- **Controls:** WASD move, mouse look, Space jump, E pick up / install / use / press, **Tab toggles cursor mode** (shows cursor, GameAndUI, pauses move/look/interaction polling; Tab again returns to game-only).
- **Done:** `BP_MC_GameMode` got `ConstructionFinished` (prints) and `EnterConstructionPhase` (finds `BP_ConstructionManager` -> `StartPhase`) again, so `BP_ConstructionManager` compiles. `BP_BayCharacter`: new `bCursorMode` + `TabCheck`, look/move gated by it; IMC added with `bIgnoreAllPressedKeysUntilRelease=false` (IMC_Bay maps were already correct; pitch is already negated in LocalTick). `BP_GraphBoard` (copy of WallBoard, `WBP_ResourceGraphs` 1920x1080 at scale 0.3125) placed as `BP_GraphBoard_C_0` at (-1310,-900,620) on the west wall (wall inner face is about -1320, x=-1335 was inside the wall). Verified in PIE: board renders the graphs. `WBP_BayHUD` got `TWRText` ("T/W x.xx (liftoff needs 1.3, good 2.0)", updated in the widget's Tick; fuel was already in `SummaryText`).
- **Part pivots:** new `BP_Part.FitMesh(Scale)` (called from `ApplyVisual`) recentres each mesh on XY and puts its bottom at the actor origin from the mesh bounds; shelf `SpawnPart` and `Drop` no longer add `MeshScale.z*50` (origin is now the bottom). Parts now sit on the shelf/floor in PIE.
- **Known issues:** long NASA parts (tanks/engines) lie on their side on the floor (orientation not fixed: DA has no rotation field; needs per-part rotation, planned in the C++ port); socket install now seats the bottom of a part at the socket origin (was centre); `bDebugAutoTest` set at runtime does not start the test (must be on at BeginPlay), so board colours were not auto-verified; mouse/keyboard still not human-tested; DSL `.x`/`if` use English ids (broken in the Russian editor), use `Математика|Вектор|BreakVector` and hand-wired Branch.
- **Manual (can't set via MCP):** `BP_BayCharacter` custom events `ServerInteract` / `ServerRelease` -> Replicates: Run on Server, Reliable (clients only; solo/host play works without).

### Agent A pass 3 result (PIE stopped, done)
- Verified the saved level had the compact hall (walls +-1350, boards x=-+1315, PlayerStart_1 at 500,500; G's `BP_GraphBoard_C_0` at -1310,-900,620 untouched).
- **Gantry**: scale (0.45,0.45,0.8), yaw 90, location (1000,830,113); bounds x 681..1312, y 368..1283, z 2..1455 (14.5 m tall, tower in NE corner, fully inside walls, clear of stack, shelves, boards).
- **Textures**: ambientCG CC0 (Concrete034 walls, MetalPlates006 floor, Metal038 ceiling) imported to `Construction/Env/Textures/T_Hall_*`, master `M_HallTiled` (world-aligned triplanar via Custom node, params TexColor/TexRough/TileSizeCm/Tint/RoughScale/Metal), `MI_Hall_Wall/Floor/Ceiling` repointed (tile 400/300/500 cm). Roughness/colour only, no normal maps. Built over MCP (no commandlet).
- **Placement check (captures `Saved/capA/d1..d5.png`)**: shelves, sockets, stand, levers, buttons, boards, gantry are fine at floor level. `BP_Part_*` are NOT level actors (runtime-spawned in PIE), so `BP_Part_C_12/C_19` below the floor was a runtime-spawn issue (spawn at bounds centre, half under floor), for the C++ port. `BP_MassScale_C_0`: the dial faces the stack but its "MASS vs LAUNCHER" text is mirrored when seen from the stack side (text faces the other way); fix in the BP/C++ port. Part orientation inside sockets: not checked (runtime only).
