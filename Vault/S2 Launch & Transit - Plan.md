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

## 2b. 3D flight view (user decision 2026-11-15: "I want this")
The spacecraft is visible and moves as the Pilot burns. Not a console-only game.
- **Flight actor `AMCFlightView`:** deep-space scene (Space Spheremaps sky, Earth and Moon spheres with NASA textures, the planned corridor drawn as a line/ring series), the spacecraft (the S1 lander + ascent stage; reuse the `AMCRocketStack` visuals and the detailed lander mesh), a trailing flown path, the predicted path to the Moon and the target corridor at the Moon.
- **Motion:** the manager's progress drives the ship along an Earth to Moon arc (compressed distances, readable). The lateral miss (`ErrX/ErrY`) is the ship's offset from the ideal line, so a bad course visibly drifts off the corridor. The Pilot's LEFT/RIGHT/UP/DOWN pulses push the ship sideways with an engine flare (a small engine-asset emitter or light flash, no custom geometry) and RCS puffs, fuel gauge drops. A TCM window shows as a glowing ring on the path.
- **Cameras:** a chase camera behind the ship. The Pilot, when seated, sees the chase camera with the scope/burn panel overlay. The other three roles see their own station and a big **viewscreen** on the desk showing the same flight (a `SceneCapture` render target on the screen material); key V toggles station view / flight view for everyone.
- **Sensor fault:** while the sensor is faulty the Pilot's displayed scope and a ghost marker show the OLD course, but the real ship drifts; after the Systems Operator switches NAV SENSOR to B the ghost vanishes and the extra TCM-FAULT window opens (everyone sees the ship leave the corridor, which is why they must talk).
- **Phases in 3D:** countdown on the pad (the S1 rocket stack on the pad outside the window), lift-off and ascent (rocket climbs, the camera follows), coast (stage separation to lander + ascent stage), arrival (ship at the Moon, results board).

## 2c. Roles (four players, four jobs)
S1 roles (Power / Construction / Propulsion / Science) become S2 roles by CrewRole number (the design document lets roles change per submission; for now the player keeps the same slot number):

| Slot | S1 role | S2 role | Station colour | Main job in S2 | Depends on / feeds |
|---|---|---|---|---|---|
| 1 | Power Engineer | **Systems Operator** | cyan | Panel switches, alarms, the sensor swap, both power rails (instrument, mechanism) | Feeds Equipment and Science (rails) and the Pilot (true course after sensor swap) |
| 2 | Construction Engineer | **Equipment Technician** | orange | Tests rover, drill, landing legs in the right 3-step order (needs mechanism rail) | Needs Systems' rail; result goes to the score and to S3/S4 readiness |
| 3 | Propulsion Engineer | **Pilot** | green | Flies the ship: burns in timed windows, fuel budget, keeps the corridor, 3D chase view | Needs the true course (Systems) and the fuel built in S1 |
| 4 | Science Specialist | **Science Operator** | yellow | Calibrates the instruments built in S1 (offset to 50 +-3, LOCK; needs instrument rail) | Needs Systems' rail; calibrated instruments go to S4 |

Rules: nobody can finish the stage alone (the rails belong to Systems, the true course belongs to Systems, the fuel is the Pilot's). Roles are fixed per slot in the lobby; each player only gets their own panel, others' stations are locked (test mode lets one player try any).

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
