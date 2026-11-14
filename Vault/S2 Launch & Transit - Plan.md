# S2 Launch & Transit — Idea, Realisation, Plan (draft for the user, 2026-11-14)

Spec: `../CONSTRUCTION_PHASE_ICE_RETURN.md` §3 S2 (called a "submission" there). Earlier work: [[S2 Launch & Transit - Handoff]] (Blueprint + UMG version, compiles, **never run**). Previous stage: S1 bay, which now runs on C++ ([[S1 Bay Overhaul - Plan]]). **Status: plan delivered to the user, implementation started with the default answers in §5 (user: "give it to me, and then start to implement").**

## 1. Idea
After the crew builds the spacecraft in S1 and presses LAUNCH, the same four players fly it to the Moon in about 3.5 minutes. Everyone sits at their own station. Four jobs run **in parallel**, and each one needs information that only another player has, so the team has to talk:

| Role (S1 CrewRole) | Station | What the player does | Accuracy measure |
|---|---|---|---|
| 1 Systems Operator | Systems console | Switches (heaters, nav sensor A/B, transponder A/B, wheel desaturation, instrument rail, mechanism rail). Reads the alarms and flips the right switch. | Alarm fixed with the correct switch |
| 2 Equipment Technician | Equipment bay | Tests the rover, drill and landing legs: a 3-step procedure per device, buttons scrambled per seed. Needs the mechanism rail ON (the Systems Operator holds that switch). | Tests passed in the right order |
| 3 Pilot | Flight console | Predicted miss at the Moon on a scope. Burns in timed windows (TCM-1..4). Each pulse costs fuel from the S1 tanks. | Course inside the ±15 km corridor |
| 4 Science Operator | Science rack | Calibrates the instruments chosen in S1 (offset to 50 ±3, then LOCK). Needs the instrument rail ON (Systems Operator). | Calibrations in tolerance |

**Why it is a team game (no solo play):** the rails belong to Systems; the Pilot's burns use the fuel chosen in S1; the Pilot only sees the true error after Systems switches the sensor; the devices and instruments tested are exactly the ones built in S1 (a missing drill = nothing to test, a non-ice instrument = nothing to calibrate).

**Random event (from the spec): Sensor fault.** At a seeded time the true course jumps but the display keeps the old value. Systems gets an alarm, flips NAV SENSOR to B, the display shows the truth, an extra window TCM-FAULT opens for the Pilot.

**Result:** score out of 100 (course 30, instruments 20, equipment 20, health 15, fuel above landing reserve 15), per-role accuracy log, and the fuel / health / calibrated instruments are handed to S3 (landing).

**Done when (spec):** on course, instruments calibrated, equipment tested.

## 2. What the player sees (3D, not only 2D panels)
- A **flight deck** inside the existing hall style: four stations in a ring around a central **holo trajectory view**: Earth and Moon (NASA 3D Resources / LOLA textures from `Tools/LOLA`, ask the user before using them), the spacecraft marker and the predicted path, a live clock and mission phase. It is a shared view, so everyone sees the same stage (countdown, ascent, coast).
- Each station has a **big screen** (the C++ widget panels), so the player at that station sees only their own information; the shared screen shows only common data.
- Every player has the **astronaut body** (Agent Q) at their station; you can see teammates working.
- **Launch moment:** the S1 rocket (the stack the players built) lifts off on the pad outside the window: countdown 10 s, ascent 20 s, then the camera/holo switches to the coast to the Moon. Rocket model = the same `AMCRocketStack`, so what you built is what flies.

## 3. Realisation (C++ port, data-driven)
Decision (user, 2026-11-14): logic in C++, Blueprints only for assets and layout.

| Piece | Class | Notes |
|---|---|---|
| State and rules | `AMCTransitManager` (replicated actor) | Port of `BP_TransitManager`: stage machine (GO/NO-GO → countdown → ascent → coast → arrival), seeded `FRandomStream`, `InitData` draws every random value once, `HandleAction(Code, Who)` server RPC target, scoring, report, `OnTransitComplete` delegate. Replicates plain values and short strings; the clients hold no logic. |
| Data | `UMCTransitConfig` (data asset) | All numbers (durations, corridor, nudge km, windows, scoring weights) as placeholders marked TBD, per spec §1 #7. |
| Input to S2 | reads `AMCConstructionManager` | Loadout from S1: tanks/fuel, engines (ascent reliability), rover/drill/lidar sockets → Equipment devices, science sockets → Science instruments, `bLaunched`. |
| Player | `AMCBayCharacter` child or `AMCStationCharacter` | Walks to a station and presses E to "sit" (locks to the station camera), Tab cursor mode for the console UI; server RPC `ServerTransitAction(Code)` built in. |
| Console UI | `UMCTransitHudWidget` + one C++ panel widget per role (`UMCSystemsPanel`, `UMCEquipPanel`, `UMCPilotPanel`, `UMCSciencePanel`) with `BindWidgetOptional` + in-code fallback layout (same pattern as the bay widgets) | Each client builds only its own role's panel from replicated values. |
| Shared view | `AMCTransitDisplay` | Holo Earth–Moon view, trajectory line and marker, countdown. |
| Level | `Maps/L_Transit` (exists) → deck layout in the same hall style | Reuse stands/lighting. 3D models only from the user or the NASA/web sets; no self-made geometry. |
| Flow | `BP_MC_BayGameMode` → after LAUNCH → `OnConstructionComplete` → `AMCTransitManager::StartPhase` | Same map (rocket lifts off, deck stays) to avoid a travel; the lobby flow (`L_Lobby` → mission) is hooked in a later step. |

The Blueprint version is kept as the reference (`Vault/S2 Launch & Transit - Handoff.md` documents its rules); no Blueprint graph work is planned.

## 4. Plan (small tasks, parallel agents)
| # | Task | Agent | Output |
|---|---|---|---|
| T0 | Confirm the decisions in §5 | user | answers |
| T1 | C++ core: `UMCTransitConfig`, `AMCTransitManager` (stages, seeded events, actions, scoring, report); compile-check | C++ agent A | files |
| T2 | C++ UI: `UMCTransitHudWidget` + 4 role panels + shared display `AMCTransitDisplay`; compile-check | C++ agent B | files |
| T3 | C++ player: station sit/leave, camera lock, `ServerTransitAction` RPC, role-restricted input | C++ agent C | files |
| T4 | Level: deck layout in `L_Transit` / hall, stations, big screens, launch pad outside the window, holo display placement; assets: Earth/Moon models (NASA 3D/LOLA, ask user) | editor agent | level |
| T5 | Rebuild the editor module (editor closed), then data asset values, place actors, hook the flow after S1 LAUNCH | me + editor agent | working scene |
| T6 | Tests: `RunDebugTest` port (scripted run printing `DEBUGTEST`), seeded replay (same seed = same events), 2-player listen-server run, no-wait check (no role idle >10 s) | test agent | report |
| T7 | Vault + git (only when the user asks) | me | docs |

Dependencies: T1, T2, T3 in parallel (files only); T4 in parallel in the editor; T5 needs the editor closed once for the rebuild; T6 last.

## 5. Decisions needed from the user
1. **Same map or separate map** for S2 (my default: same hall/pad, deck inside, no travel).
2. **Earth/Moon visuals:** OK to use the NASA Earth and Moon models/LOLA textures for the holo view (default: yes, NASA public domain).
3. **Seating:** walk to a station and sit (default) or each player is simply placed at their station.
4. **Numbers:** keep the placeholder numbers (3.5 min, ±15 km corridor, 4 burn windows) until you give balanced ones.
5. **Do you want the launch cinematic** (rocket lift-off seen from the deck) in this first pass, or only the panels first (default: panels first, lift-off as T4 extra).
