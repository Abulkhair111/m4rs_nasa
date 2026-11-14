# S1 Bay Overhaul — Plan (2026-11-14)

User request: (1) more NASA 3D models in the bay, (2) the bay should look like a real **vertical** construction department (a VAB-style tall hall), beautiful, (3) graphs plus **TWR visible in the bay** (they were building blind), (4) **choose-to-place** instead of carrying parts by hand; light parts may fly in on their own, (5) it must fit the NASA game (Ice Return).
Base: [[S1 Design & Assembly - Handoff]], [[Architecture]] → Construction phase. Rules: `../CLAUDE.md` (no self-made 3D models, ask the user).

## How it fits the game (Ice Return)
S1 is the first of six stages. Its output is the **loadout** (sockets → parts, fuel, bolts, grid) held in `BP_ConstructionManager`. S2 (`BP_TransitManager`) already reads that loadout (devices from Rover/Drill/Lidar sockets, instruments from Science sockets, fuel, reliability) and starts on `bLaunched`. So the bay UI must teach the same numbers the later stages punish: **T/W for ascent, fuel for burns, power for the instrument rails, ice-capable instruments for the science stage**. The graphs should show these. The bay's story: the Mission Control crew prepares the lander in a vertical integration hall, and the lander then rolls out and launches.

## Design decisions
- **Vertical hall:** tall (≥ 40 m) integration cell. The lander stack stands upright in the middle on a pad. Around it are 3–4 gantry platform levels (one per department) with a lift or stairs. Parts are installed at the socket heights. Environment geometry comes from **NASA/user models only** (see B); no primitive-built architecture. If no suitable model exists, the agent asks the user and meanwhile uses lighting, fog and the existing engine-shape placeholders.
- **Choose-to-place:** a player looks at a socket (or the stack) and presses E. A picker panel opens listing only the parts that fit that socket and the player's department. Each row shows mass, cost and thrust/power effect, plus the **predicted T/W, fuel margin and power balance** if chosen. Confirm → the server installs the part with an animation. **Light parts** (instruments, antennas, small batteries) fly from the shelf along an arc. **Heavy parts** (engines, tanks) are lowered by an overhead crane with a visible hook. No hand carrying for parts. The wrench and the cable reel stay as tools.
- **Live instruments:** one big wall display (T/W gauge, fuel, mass vs payload, power, cost, science, comms bars, red/yellow/green) plus a small T/W readout on the player's HUD at all times.
- **Server-authoritative:** all state stays in `BP_ConstructionManager`. Clients display replicated values. Picker confirm goes through a Run-on-Server event (manual flag, see Status → Next).

## Ownership (parallel agents; one editor, so no overlapping assets)
| Agent | Owns | Must NOT touch |
|---|---|---|
| **A: Hall & lighting** | `Maps/L_AssemblyBay` level actors (not the BP internals), `Construction/Env/**` (new), lighting, fog, post-process | any Blueprint graph |
| **B: NASA models** | `Tools/NASA3D`, `SourceArt/NASA3D`, `Art/Meshes/NASA/**`, `DA_Part_*` Mesh/MeshScale fields | graphs, level actors |
| **C: Choose-to-place** | `BP_Part`, `BP_PartSocket`, `BP_PartShelf`, `BP_BayCharacter`, new `WBP_PartPicker`, `BP_PartMover` (new), manager functions it adds (`Preview*`, `PlacePart`) | `WBP_ResourceGraphs`, `BP_GraphBoard`, the level |
| **D: Graphs & TWR** | `WBP_ResourceGraphs`, new `BP_GraphBoard`, `WBP_BayHUD` additions, `WBP_WallBoard` | `BP_ConstructionManager` graphs (read-only; ask C via the Status note if a value is missing) |

Shared rules: edit only your own assets, one `write_graph_dsl` batch per BP at a time, never leave PIE running without the lock (`Saved/pie.lock`: create it before `StartPIE`, delete it after `StopPIE`; if it exists, wait). Save assets (`save_assets`, verify on disk) before finishing. Each agent appends a short section to [[Status]] and updates [[Asset Catalog]] for its assets. Commit nothing (the user decides).

## Task list (small tasks, in order within each agent)
**A: Hall & lighting**
- A1 Survey `L_AssemblyBay`: current actors, stand position, shelf/board positions (capture screenshots).
- A2 Check `nasa/NASA-3D-Resources` for hall/tower/gantry/crawler/VAB/launch-pad models (with B). List candidates to the user if the choice is unclear.
- A3 Place the hall shell (imported model or user model): tall walls, floor, ceiling trusses.
- A4 Gantry platform levels at the departments' heights, with railings from imported models; keep `PlayerStart` tags `Bay` valid.
- A5 Lighting: dark navy ambient, warm work lights, cyan accents, volumetric fog, a spot on the stack, exposure fixed.
- A6 Screenshot pass from each department; tune; save.

**B: NASA models**
- B1 Verify the 17 imported parts in PIE-free viewport captures (scale, pivot, orientation).
- B2 Fix pivots/rotations in `DA_Part_*` (`MeshScale`, offsets); list models that look wrong.
- B3 Find more NASA models for RTG/drill/rockets/lander (and hall props for A); import them with `Tools/NASA3D/import_nasa_parts.py`.
- B4 Assign them to the remaining `DA_Part_*` (RTG, drill, Falcon 9, Vulcan, Falcon Heavy).
- B5 Place the Apollo LM as the lander body in `BP_AssemblyStand` **only if C and A are not editing it**; otherwise hand the exact transform to Status.
- B6 Update the catalog table.

**C: Choose-to-place**
- C0 First: save the unsaved bounds-based `SpawnPart`/`Drop` (see handoff) or redo them.
- C1 Manager: `GetCandidates(SocketType, Dept)` and `Preview(Part, Socket)` → predicted `TWR`, `FuelMargin`, `PowerBalance`, mass, cost (reuse `Recalc` maths without writing state).
- C2 `WBP_PartPicker` (list, effect deltas, predicted T/W colour, Confirm/Cancel), opened by the bay character on E at a socket.
- C3 Server event `ServerPlacePart(Socket, PartDef)`; validates role, budget/availability; calls `Install`.
- C4 `BP_PartMover`: light part arc flight (Tick lerp with arc), heavy part crane lowering (hook actor + cable), replicated start/end so clients animate locally.
- C5 Remove hand carrying of parts; keep removal (look at a part → Remove, only if unbolted) and tools.
- C6 Self-test via `bDebugAutoTest` (use `PlacePart`); then PIE check.

**D: Graphs & TWR**
- D1 `WBP_ResourceGraphs` logic per the handoff table (`SetBar`, `Refresh` on Tick, `CriticalText`).
- D2 Big T/W gauge with the 1.3 and 2.0 limits marked (needle or bar), updated live.
- D3 `BP_GraphBoard` (3D widget on a wall facing the stack) in the bay, visible from every platform level.
- D4 Small always-on T/W + fuel readout on `WBP_BayHUD`.
- D5 Colour rules from `GetCheck(i)`; test with the auto-test values; capture screenshots.

## Wave order
Wave 1 (now, parallel): A1–A2, B1–B3, C0–C2, D1–D2. Wave 2: A3–A6, B4–B6, C3–C6, D3–D5. Then the user plays it (keyboard and mouse have still never been tried by a human).

## C++ port notes (Agent C, choose-to-place) - 2026-11-14
Blueprint graph work on the bay is paused: the project is moving to C++. Existing BP assets are left as-is (BP_PartMover, `BP_PartSocket.bReserved`, empty `BP_ConstructionManager.PlacePart/Preview` shells, `TmpStart`). They are unsaved in the editor, so don't rely on them.

### Manager (AConstructionManager, replicated, one per bay)
Replicated vars: bActive, TimeLeft, RocketName, RocketPayloadKg, RocketCostM, RocketReliability, SelectedLever, FuelKg, FuelMaxKg, DryMassKg, FuelRequiredKg, TotalMassKg, TotalCostM, PowerGenW, PowerUseW, StorageWh, DataMbps, BandwidthMbps, ScienceValue, Reliability, ThrustN, TWR, IceCount, UnboltedCount, OffGridCount, bHasEngine, bHasLidar, Checks[10], DeptReady[5], RoleGood[5], RoleBad[5], Report, Message, MessageTime. Config: LanderBaseMassKg, LanderBaseCostM, BudgetM ($500M), bFreeRoles, bDebugAutoTest.
Functions: StartPhase, SelectRocket, AddFuel, ToggleReady, TryLaunch, ApplyTool(Target, ToolType 1=bolt 2=grid), Recalc, RecalcChecks, BuildReport, UpdateAlerts, RunPreflight, Score(Role, Good), Say, new: PlacePart, Preview, GetCandidates.

### Recalc maths
For each socket with an installed part: DryMass = LanderBaseMass + sum(massKg); TotalCost = LanderBaseCost + RocketCost + sum(costM); FuelMax = sum(fuelCapKg); Reliability = RocketReliability (min 1 if <=0) x product(part reliability). A part is **live** if fuelCapKg > 0 (tank) or its socket bGrid. Only live parts add: PowerGen += max(powerW,0); PowerUse += max(-powerW,0); StorageWh, DataMbps, BandwidthMbps, ScienceValue, ThrustN; IceCount if bIceCapable; bHasEngine (socketType 6), bHasLidar (7). Counts: UnboltedCount = installed and not bBolted; OffGridCount = installed, not live-by-grid and not tank. FuelKg = clamp(FuelKg, 0, FuelMax).
TotalMassKg = DryMassKg + FuelKg. TWR = ThrustN / (TotalMassKg x 1.62). Checks (0 green, 1 yellow, 2 red): 0 lever chosen; 1 mass (green <= 0.9 payload, red > payload); 2 budget; 3 power (red gen < use or gen 0; green gen >= 1.1 use and a battery); 4 comms; 5 fuel (red no tanks or fuel < 0.9 x dry; green >= +10%); 6 TWR (red < 1.3 or no live engine; green >= 2.0 with lidar); 7 ice science; 8 grid (yellow any off-grid); 9 structure (red any unbolted).

### Preview(Def) (client-side, no state writes)
Assume the part will be bolted and wired. dMass = massKg, dCost = costM, gen/use from powerW sign, dThrust = thrustN, dFuelCap = fuelCapKg. Outputs: TWR' = (ThrustN + dThrust) / ((DryMass + dMass + min(FuelKg, FuelMax + dFuelCap)) x 1.62); FuelMargin = (fuel' - 0.9 x (Dry + dMass)) / (0.9 x (Dry + dMass)); PowerBalance = (Gen' - Use'); plus new mass and cost. Show colours with the same 1.3 / 2.0 thresholds.

### PlacePart(Socket, Def, Role, Who) - server only
Reject (with Say message) unless all hold: bActive; Socket valid, Installed invalid and not bReserved; Def.socketType == Socket.SocketType; Role == 0 or Role == Def.department or bFreeRoles; TotalCostM + Def.costM <= BudgetM. Role = Pawn.PlayerState -> BP_MC_PlayerState.CrewRole (0 if the cast fails). Then: Socket.bReserved = true; spawn the part (collision off, Def set, no shelf); spawn PartMover; on arrival call Part.Install(Socket) (attach snap, bBolted/bGrid reset, Score, Recalc, Say), then clear bReserved. Candidates for a socket = catalog defs with socketType match and (department == role or role == 0 or bFreeRoles), ToolType == 0. Department number == role number (1 Power, 3 Propulsion, 4 Science).

### PartMover parameters (replicated actor, Tick on all machines, positions from server world time)
Part, Socket, StartLoc, EndLoc, StartTime, Duration, ArcHeight, bHeavy. Alpha = clamp((now - StartTime)/Duration, 0, 1); s = a*a*(3-2a); pos = lerp(Start, End, s) + (0,0,ArcHeight x sin(pi a)). Light: Start = matching-department shelf (+120 up), ArcHeight 150-160, Duration about 1.4 s. Heavy (bHeavy): Start = socket + (0,0,500), ArcHeight 0, Duration about 2.6 s, draw a cable and hook (needs a user-supplied crane model; the BP used debug lines). Server finishes when alpha >= 1 and not done.

### Input and cursor mode
Standard FPS: WASD move, mouse look, Space jump, E interact/open picker, Tab toggles cursor mode. Add IMC_Bay on possess and on restart (including clients and after PIE) via the EnhancedInput local-player subsystem, in the pawn's PossessedBy/OnRep_Controller (client) and BeginPlay; bIgnoreAllPressedKeysUntilRelease off. Cursor mode: bShowMouseCursor = true, SetInputMode GameAndUI (or UIOnly) with the picker as focus widget, and a flag that skips look/move input. The picker opens in cursor mode and restores game-only input on close or confirm. The picker is sent to the server via a Run-on-Server RPC ServerPlacePart(Socket, Def); the existing ServerInteract/ServerRelease are also RPCs. Part text: name, one-line description (agent B's DA field `description`; fallback built from massKg, costM, powerW, thrustN, fuelCapKg, storageWh, dataMbps/bandwidthMbps, scienceValue) and key numbers, in picker rows and the look-at prompt.

### Known facts
- Old placement used MeshScale.z x 50 for part height; the planned fix is bottom = (BoxExtent.z - Origin.z) x MeshScale.z from the mesh bounds.
- E is edge-triggered in LocalTick; installed parts are looked up by a 350 cm line trace; sockets are plain Actors without collision, so aim selection should use a view-cone check over the sockets array rather than a trace.

## Constructible rocket (Agent K) - editor setup steps
Code: `Source/MissionCrew/Bay/MCRocketStack.*`, see [[Architecture]] "Constructible rocket". Needs a Live Coding/Editor build first (the game target compiled; the editor target was not built).
1. **Place one `MCRocketStack`** actor in `L_AssemblyBay` at the pad: XY (900,-750), Z = pad top (298), yaw 0. Hide or delete the maquette `Maquette_Rocket_Launcher` and `Maquette_Rocket_Lander` actors (the stack replaces them; keep `Maquette_Rocket_Pad`).
2. On that actor set **LanderMesh** (Apollo LM static mesh; the maquette used scale 0.2 -> `LanderScale` (0.2,0.2,0.2) or set `LanderHeightCm` to fit), `MaxStackHeightCm` 3500, leave the rest default. If the lander faces sideways set `LanderRotation`.
3. **Ascent sockets**: either tick `bAutoSpawnSockets` (spawns Engine x1, Tank x3, Structure x2, Booster x4 at the stack, only if no ascent socket exists; existing ascent sockets in the ASCENT zone count, so spawn is skipped), or place `MCPartSocket` actors by hand with `SocketGroup = Ascent` and types from `GetAscentSlots` (names `Ascent_Engine_0`, `Ascent_Tank_0..2`, `Ascent_Structure_0..1`, `Ascent_Booster_0..3`). To get layout numbers in an editor script call `UMCRocketStackLibrary::GetAscentSlots` / `SpawnAscentSockets(world, transform, params, class)`. Tune `SocketLayout.CoreColumnOffset` so the column stands where players can aim at it.
4. **Part defs** (Agent J models in `/Game/MissionCrew/Art/Meshes/Rocket/*`): for every ascent `UMCPartDef` set `Group = Ascent`, `SocketType` (Engine / Tank / Booster, and the new `Structure` for decoupler/capsule/fairing), `StackKind`, `StackMesh` (+ `StackMeshRotation` so local Z is up, `StackMeshScale`), optional `StackHeightCm` (fits the mesh) and `StackRadiusCm`; boosters: `bRadial` (implied by kind Booster) and `RadialCount` 1/2/4. Without StackMesh the old `Mesh`/`MeshScale` is used (rotation field still applies). Assign the defs to the manager `Catalog`.
5. Verify in PIE: pick a tank in the picker -> a translucent ghost appears and the stack grows; confirm -> it drops in; add a booster with RadialCount 2 -> two side boosters; height stays under 35 m.
