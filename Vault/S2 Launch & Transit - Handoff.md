# S2 Launch & Transit: handoff (2026-11-14)

The assignment was to implement **S2 Launch & Transit** from `../CONSTRUCTION_PHASE_ICE_RETURN.md` §3. Another session owns S1 Design & Assembly.
**State:** the assets are built, compile with no errors, and are saved. **Nothing has been tested at runtime yet.** A PIE self-test was started on `L_Transit` and then interrupted before any results were read. PIE may still be running: call `IsPIERunning`, then `StopPIE`.

## What exists (all new, all in my own folder)
| Asset | Purpose |
|---|---|
| `/Game/MissionCrew/Transit/Blueprints/BP_TransitManager` | Replicated, always-relevant actor. Server-authoritative state for all of S2. |
| `/Game/MissionCrew/Transit/Blueprints/BP_TransitPawn` | Bare Pawn, one per player. Carries the only Server RPC and creates the HUD. |
| `/Game/MissionCrew/Transit/UI/WBP_TransitHUD` | UMG screen: top bar, message line, role panel (WidgetSwitcher P1–P4), mission board, test-mode role tabs. |
| `/Game/MissionCrew/Maps/L_Transit` | Test map (copy of Template_Default). GameMode override = `BP_MC_GameMode`. It holds one `BP_TransitManager` instance with `bDebugAutoTest=true`, `bFreeRoles=true`, `SeedOverride=4242`. |

No shared Blueprints were edited: not GameMode, GameState, PlayerController, PlayerState, the lobby, or anything in Construction. The manager only reads `BP_ConstructionManager` and `BP_PartSocket`, and the pawn only reads `BP_MC_PlayerState.CrewRole`.

## Design (UMG only, about 3.5 minutes)
Stages (`Stage`): 0 GO/NO-GO poll (each role presses GO FOR LAUNCH) → 1 countdown (`CountdownSec` 10) → 2 ascent (`AscentSec` 20; a seeded anomaly roll against `LaunchReliability` adds course error and costs 5% of fuel) → 3 coast to the Moon (`TransitSec` 180) → 4 arrival with a report.
Roles, using the same CrewRole numbers as S1: **1 Systems Operator** (was Power), **2 Equipment Technician** (was Construction), **3 Pilot** (was Propulsion), **4 Science Operator** (was Science).

- **Systems (1):** 6 switches (`Switches[6]`): HEATERS, NAV SENSOR A/B, TRANSPONDER A/B, WHEEL DESAT, INSTRUMENT RAIL, MECHANISM RAIL. Both rails start OFF. 4 seeded alarms (`WarnText/WarnSwitch/WarnValue/WarnTimes`). A flip that fixes an alarm, or a flip that turns a rail ON, scores good; any other flip scores bad. Each open alarm drains `Health`.
- **Pilot (3):** predicted miss at the Moon (`ErrX/ErrY` true, `DispX/DispY` as displayed). The course drifts between windows (`DriftXs/DriftYs`, seeded). Burn windows TCM-1..4 open at 20/65/110/150 s for `WindowSec` 15 s. Each pulse moves the miss by `NudgeKm` (6) and costs `NudgeFuelKg × (1 + 2 × progress)`. The scope dot in the HUD is 1.6 px/km, and the corridor is ±`CorridorKm` (15).
- **Sensor fault event (brief S2):** at a seeded `FaultTime` (45–95 s) the true error jumps by `KickX/KickY`, but the display keeps showing the old value (bias). It raises alarm 3. Systems flips NAV SENSOR → B, the display then shows the true course, and an extra window **TCM-FAULT** opens.
- **Equipment (2):** devices come from the S1 loadout: LANDING LEGS always, plus ROVER, DRILL, or LIDAR if those sockets are filled. In test mode the devices are ROVER, DRILL, LANDING LEGS. Each device has a 3-step procedure shown in order, with the buttons scrambled per seed. A step out of order resets the device (bad). Every device needs MECHANISM RAIL ON. A completed procedure runs a 4 s test, then shows PASS.
- **Science (4):** instruments come from the S1 science sockets (up to 3). The default set is NSS, NIRVSS, and MSolo. Players set the offset to −5/−1/+1/+5 so the reading (which wobbles) sits on 50 ±3, then press LOCK. Needs INSTRUMENT RAIL ON.
- **Arrival:** an off-corridor miss costs 1.5 kg of fuel per km. Score out of 100: course 30, instruments 20, equipment 20, health 15, fuel at or above the landing reserve 15. The `Report` includes per-role accuracy (`RoleGood/RoleBad[5]`). At arrival, `bDone` is set, `TransitScore` is recorded, and the dispatcher `OnTransitComplete` fires.
- **Determinism:** every random draw happens in `InitData` from `Stream` (seeded by `SeedOverride`, or a random seed shown on the HUD), so the same seed always produces the same events.

## Code layout
- Manager functions: `StartPhase` (LoadLoadout → InitData → spawns `BP_TransitPawn` for every PlayerController and possesses it), `HandleAction(Code, Who)`, `DoGo/DoSystems/DoEquip/DoPilot/DoScience`, `ServerTick → TransitTick`, `TriggerWarning`, `Open/CloseWindow`, `FinishDevTest`, `Arrive`, `BuildUi`. `BuildUi` runs every 0.2 s on the server and fills the replicated strings `TopText`, `BoardText`, `PanelText[5]`, `SwitchLabels[6]`, and `EqLabels[3]`. The HUD only displays these strings, so the client has no game logic.
- **Action code** = `role × 100 + action`. 90 is GO. Systems actions 0–5 are switches, Equipment 0–2 are buttons, Pilot 0–3 are LEFT/RIGHT/UP/DOWN, and Science 0–4 are −5/−1/+1/+5/LOCK. The server accepts a code only if `bFreeRoles` is set or the role matches the player's `CrewRole`.
- **Start triggers:** the manager auto-starts on maps whose name contains `L_Transit`. On any other map, its Tick polls `ConsMgr.bLaunched` (from `BP_ConstructionManager`) and starts itself after the S1 LAUNCH. The custom event `BeginTransit` also starts it.
- The debug self-test is the custom event `RunDebugTest`, plus the cheat helpers `DebugSolveEquip/Science`, `DebugPilot`, and `DebugFixAlarms`. It prints `DEBUGTEST ...` lines to the log.
- Generator sources (DSL generated with Python): `Saved/S2Gen/gen_mgr.py`, `gen_ui.py`, `style.py`, `tree.json`, and `vars.py`/`fns.py`. To change a graph, edit the generator, regenerate, delete the graph's non-entry nodes, and run `write_graph_dsl` again.

## Not done / next steps
1. **Run the self-test:** open `L_Transit`, start PIE, wait about 220 s, then grep `Saved/Logs/проект.log` for `DEBUGTEST` and for the report (`S2 LAUNCH & TRANSIT REPORT`). Fix any runtime bugs. Nothing has been verified yet: the integer `/` in `HandleAction`, the multi-exec casts, and string formatting are all untested.
2. Look at the HUD in PIE (CaptureViewport) and fix the layout.
3. **Manual step (can't be done via MCP):** in `BP_TransitPawn`, select the custom event **`ActionSig_Event`** → Details → Replicates: **Run on Server**, Reliable. Without this, client buttons do nothing (the host still works). The odd name exists because MCP can't add parameters to a custom event, so the event was generated from the dispatcher `ActionSig(Code:int)`. Keep that dispatcher.
4. Integration: place one `BP_TransitManager` in the mission or bay world. It starts after the S1 LAUNCH by itself. Leave `bFreeRoles` false in real play.
5. All numbers are placeholders (TBD, per brief §1 #7). The lobby still uses the old role names.

## Gotchas found this session (also add them to `Unreal MCP Workflow.md`)
- A variable and a function with the same name: the function graph gets a `_0` suffix (`Score` → `Score_0`). I renamed the variable to `TransitScore`.
- DSL `switch int`: case pins are 0..N−1 (one per case). Cases starting at 1 fail, so switch on `(- x 1)`.
- DSL: `switch` and casts with `(:then …)` end the exec flow. Any statement after them in the same block is "unreachable" or silently skipped. Use if chains, or move the cast into a small helper function.
- DSL can't create parameters on a custom event (`Undefined variable`), and `add_node_pin` refuses. Workaround: add an event dispatcher with the params, `create_node Default|Assign<Dispatcher>` (this spawns a matching custom event), delete the Assign node, and call the event as `Class|<BP>|<Dispatcher>Event`.
- Node IDs: widget tick is `UserInterface|EventTick`; `Widget|Transform|SetRenderTranslation`; `Progress|SetPercent`; `Switcher|SetActiveWidgetIndex`; `Widget|GetOwningPlayer`; `Player|GetOwningPlayerPawn`; `UserInterface|Viewport|AddtoViewport`. There is no SelectVisibility node, so use if/else.
- Each `write_graph_dsl` recompiles the whole Blueprint (about 1–1.5 min per graph once the BP is big). Batch 4–8 graphs per script and expect timeouts. The call keeps running in the background.
