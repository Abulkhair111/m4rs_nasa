# Architecture

UE **5.8** (`A:/epic games/UE_5.8`), **Blueprint-only** (Visual Studio isn't installed, so no C++). Player-facing UI text is **English** (the editor UI may still be Russian).
Content root: `/Game/MissionCrew/{Core,UI,Maps,Data,Art}`.

## Networking
- **LAN listen server** (one player hosts). No dedicated server, no internet backend.
- Host: `OpenLevel L_Lobby?listen`. Join: `open <IP>` (UDP 7777) and/or LAN session search through OnlineSubsystemNull (the other agent added `WBP_LanSessionRow`; plugins OnlineSubsystem/Null/Utils are enabled).
- Planned: authority on the server. Every player action goes **through the PlayerController with a Server RPC** (actors the client doesn't own can't receive Server RPCs).

## Flow
`L_MainMenu` → (Host/Join) → `L_Lobby` → (host START MISSION: `BP_MC_PlayerController.StartMission` → `ServerTravel L_Mission`) → `L_Mission` (intro video plays first via `BP_MissionVideoEvent`; hook `OnMissionVideoFinished` to begin gameplay) → Debrief → back to Design.

## Classes (existing)
| Class | Parent | Role |
|---|---|---|
| `BP_MC_GameInstance` | GameInstance | `Callsign`, `LastError`. HandleNetworkError/HandleTravelError set LastError (the engine returns to the default map itself). Project GameInstanceClass. *The other agent extends it with LAN sessions.* |
| `BP_MenuGameMode` | GameModeBase | No pawn, PC = BP_MenuPlayerController. Set as the GameMode override in L_MainMenu. |
| `BP_MenuPlayerController` | PlayerController | BeginPlay → if local: CreateWidget WBP_MainMenu → AddToViewport → InputMode UI Only → show cursor. Tick: slow drift of the first CameraActor around its start transform. |
| `WBP_MainMenu` | UserWidget | Callsign input, Host, LAN missions list (other agent), Quit, StatusText. Construct restores callsign + error from GI. |
| `BP_MC_GameMode` | GameModeBase | Mission GM (DefaultPawn, BP_MC_PlayerController/GameState/PlayerState). Added: `EnterConstructionPhase()` → finds `BP_ConstructionManager` → `StartPhase`; `ConstructionFinished()` stub called on LAUNCH. |

## Construction phase = Ice Return S1 "Design & Assembly" (`/Game/MissionCrew/Construction`)
Remade 2026-11-14 to the S1 spec in `../CONSTRUCTION_PHASE_ICE_RETURN.md` §3 (3D bay kept, roles and rules changed). Server-authoritative. Clients only display replicated state.
- **Roles (CrewRole number → S1 role):** 1 **Power Engineer** (Power & Comms shelf + cable reel), 2 **Construction Engineer** (Assembly Desk: rocket levers, wrench, LAUNCH), 3 **Propulsion Engineer** (engine, lidar, fuel tanks, fuel pump), 4 **Science Specialist** (instruments). The lobby still names the slots MCC Engineer / Flight Director / Pilot / Scientist; only the number matters here.
- **`BP_ConstructionManager`** (replicated, always relevant, one per bay): the single source of truth. Replicated: rocket (name, payload, cost, reliability, `SelectedLever`), `FuelKg`, `FuelMaxKg` (= total installed tank capacity; fuel is clamped to it), totals (`DryMassKg`, `TotalMassKg`, `TotalCostM`, `PowerGenW/UseW`, `StorageWh`, `DataMbps`, `BandwidthMbps`, `ScienceValue`, `Reliability`, `ThrustN`, `TWR`, `IceCount`, `UnboltedCount`, `OffGridCount`), `Checks[10]`, `DeptReady[5]`, accuracy `RoleGood[5]`/`RoleBad[5]`, `Report`, `Message`/`MessageTime`, `TimeLeft`.
  - `Recalc()`: for every socket with a part: mass, cost, tank capacity and reliability always count. A part is **live** if it is a tank (FuelCapKg > 0) or its socket has `bGrid`. Only live parts add power gen/use, storage, data, bandwidth, science, thrust, ice count, engine/lidar flags. Counts unbolted and off-grid parts. Then `RecalcChecks` → `BuildReport` + `UpdateAlerts`.
  - **Checks** (0 green, 1 yellow, 2 red): 0 launcher chosen · 1 mass (green ≤ 90% payload, red > payload) · 2 budget ($500M) · 3 power balance (red if gen < use or gen 0; green needs +10% and a battery) · 4 comms · 5 fuel (red if no tanks or fuel < 0.9 × dry; green needs +10% margin) · 6 thrust T/W = ThrustN / (total mass × 1.62) (red < 1.3 or no live engine; green ≥ 2.0 with lidar) · 7 ice science (red: no live ice-capable instrument; green: ≥ 2 and science ≥ 50) · 8 power grid (yellow if any module is off-grid) · 9 structure (red if any module unbolted).
  - Accuracy log: `Score(Role, Good)`. Install → good unless a Science part can't detect ice; wrench bolt → Construction good; cable toggle → Power good if gen ≥ use afterwards; every `RunPreflight` scores each role on its checks (Power 3, Construction 0/1/9, Propulsion 5/6, Science 7). `AccText()` appends "ACCURACY good/actions" per role to `Report`.
  - `ApplyTool(Target, ToolType)`: 1 = toggle the target's socket `bBolted`, 2 = toggle `bGrid` (tanks refuse). `TryLaunch` (all 4 READY, no red, yellow needs a second press) → `BP_MC_GameMode.ConstructionFinished`. `StartPhase()` spawns/possesses `BP_BayCharacter` at PlayerStarts tagged `Bay`. Auto-starts only on maps containing `L_AssemblyBay`. `bDebugAutoTest` runs a scripted build (rocket → install → wrench+cable via ApplyTool → fuel → READY ×4 → LAUNCH ×2) and prints `DEBUGTEST` lines.
- **Interaction:** `BP_BayCharacter` (first person, Enhanced Input `IMC_Bay` polled in Tick, `HoldPoint` 120 cm ahead). E edge → `ServerInteract` (server re-traces 350 cm with `FindTarget`) → carried item `CarrierAction`, else target `BP_BayInteractable.Use(Who)`. Release → `ServerRelease`. **Both events must be set to Run on Server by hand.**
- **`BP_BayInteractable`** base: `Prompt`, `Use/Release`, `GetPrompt(Who)`, helpers `Manager()`, `RoleOf(Who)`, `RoleAllowed(Who, Role)` (role 0 or `bFreeRoles` = allowed), `Valid(Obj)`.
  - `BP_Part`: `EventUse` → `HandleUse(Who)`: installed + bolted → refuse ("unbolt with the wrench first"); installed + loose → `RemoveFromSocket` (clears socket flags, refund) then grab; tools can only be grabbed by their department. `CarrierAction`: tool (`Def.ToolType > 0`) → `UseTool` (trace target; a BP_Part → `Manager.ApplyTool`, anything else → put the tool down); heavy part with one carrier → let go; else `TryPlace` → `Install` (attach, collision ON so the trace can hit it, socket bolted/grid reset, shelf respawn, Score, Recalc). `Drop` re-enables collision. `GetPrompt` shows "Bolts TIGHT/LOOSE · Power grid CONNECTED/OFF" for installed parts.
  - `BP_RocketLever` (role 2), `BP_FuelPump` (role 3, hold; `Rate` −100 = drain), `BP_ReadyButton` (`Department`), `BP_LaunchButton` (role 2).
- `BP_PartSocket` (`SocketType` 0 Power, 1 Battery, 2 Comms, 3 Science, 4 Drill, 5 Rover, 6 Engine, 7 Lidar, **8 Tank**; `Installed`, `bAlert`, **`bBolted`, `bGrid`** (replicated); ghost preview). `bAlert` (red pulsing marker) = installed but loose or off-grid, or a red check after preflight. `BP_PartShelf` (`Department`, `Parts[]`). Displays read the manager every tick: `BP_MassScale`, `BP_PowerStrip` (check 3), `BP_WallBoard` (+`WBP_WallBoard`, rows Check0–9, `UpdateRow9`), HUD `WBP_BayHUD`.

## Planned (see ROADMAP §1)
`BP_MC_GameMode`, `BP_MC_GameState` (Phase RepNotify, timers, resources, faults, event log), `BP_MC_PlayerState` (Role), `BP_MC_PlayerController` (all Server RPCs), `BPFL_MC_Math`, `BP_FaultManager` (seeded RandomStream), `BP_LanderSim` (custom landing physics, g = 1.62).

## Project settings
GameDefaultMap = EditorStartupMap = `L_MainMenu`. GameInstanceClass = `BP_MC_GameInstance`.
