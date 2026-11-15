# S3 Orbit & Landing - Idea, 3D, Realisation, Plan (draft for the user, 2026-11-15)

Spec: `../CONSTRUCTION_PHASE_ICE_RETURN.md` §3 S3, event "Hazard surprise" (§3 table), milestone M6 (§4), art and LOLA rules (§5, §6). Template: [[S2 Launch & Transit - Plan]]. Previous stages: S1 bay ([[S1 Bay Overhaul - Plan]]), S2 deck (C++ `AMCTransitManager`, `AMCFlightView`, see [[Architecture]] "S1 to S2 flow" and "C++ flight view"). **Status: plan only. No code, no editor work, no assets touched. Waiting for the user's answers in §5.**

Numbers below are placeholders marked TBD (spec §1 #7). Real site names and facts must be checked against NASA sources before release (spec §3 "every number from the data tables").

## 1. Idea

### 1.1 How S3 follows S2
S2 ends with the ship at the Moon (`FMCTransitReport`: `Score`, `MissKm`, `bInCorridor`, `FuelLeftKg`, `Health`, `InstrumentsCalibrated/Total`, `DevicesPassed/Total`, `bAnomaly`, plus `RoleGood/RoleBad`). S3 is the same four players, same deck, same station slots, about 4 minutes. What carries over and what it does:

| From | Value | Effect in S3 |
|---|---|---|
| S2 | Fuel left (`FuelLeftKg`) | The descent budget. Powered descent burns it; run out = free fall = crash. The far site costs more. |
| S2 | Health | Start hull health. Hard touchdown subtracts it; low health lowers the limits (legs, engine). |
| S2 | Calibrated instruments (count and each one's final offset error) | Quality of the orbital ice data the Landing Analyst reads: a well calibrated ice instrument shows the ice layer sharp, an uncalibrated one adds noise to the shown ice score. Not-built instruments = no layer at all. |
| S2 | Equipment status (rover, drill, **landing legs** pass/fail, lidar) | Legs failed = tighter touchdown limits and a deploy risk; lidar absent = blind altitude/hazard scan (design doc: "blind without lidar"). Rover/drill pass flags go on to S4. |
| S2 | Course result (`MissKm`, `bInCorridor`) | Orbit insertion quality: in corridor = nominal orbit; outside = insertion costs fuel (TBD kg per km over) and the survey phase is shorter. |
| S2 | Sensor fault handled? (`bAnomaly`, and whether NAV SENSOR was swapped) | A not-swapped fault leaves a position bias on the Pilot's nav readout in S3 (small, the Systems Operator is told). |
| S1 | Loadout: engine thrust and Isp, dry mass, lander mass, lidar, rover, drill, ice instruments, tanks | Thrust-to-weight on the Moon (g 1.62) and fuel flow of the descent engine; which hazards can be sensed. |
| S1/S2 | Per-role accuracy (`S1RoleGood/Bad`, `RoleGood/RoleBad`) | Carried into the final accuracy log and summary. |

### 1.2 Roles (four players, same four slots)
S2 slots are Systems / Equipment / Pilot / Science. The spec's S3 roles are Systems Operator, Landing Analyst, Safety Officer, Pilot. Mapping (default, see decision 5):

| Slot | S1 role | S2 role | **S3 role** | Colour | Main job in S3 | Accuracy measure (spec) |
|---|---|---|---|---|---|---|
| 1 | Power | Systems Operator | **Systems Operator** | cyan | Pre-landing checklist and live indicators: radar/lidar rail, descent-engine pressure, legs unlock, thruster rails. Reports **limitations** to the Pilot (max descent rate with legs fault, blind below X m, fuel for hover). | Limitations reported in time |
| 2 | Construction | Equipment Technician | **Safety Officer** | orange | Marks rocks, steep slopes and craters on the shared map; later clears the chosen site and, in the event, marks the new zone. Knows the lander (legs/pad limits) from S2 equipment. | Hazards marked correctly out of all marked |
| 3 | Propulsion | Pilot | **Pilot** | green | Plans approaches to the two top sites, then flies the powered descent: altitude, speed, direction. | Landing inside the safe zone within limits |
| 4 | Science | Science Operator | **Landing Analyst** | yellow | Reads the orbital ice layer (quality set by S2 calibration), ranks the sites by ice likelihood, proposes the site. | Ranking matches the reference |

Why slot 2 = Safety and slot 4 = Analyst: the Science Operator calibrated the ice instruments in S2 and now uses that data; the Equipment Technician tested the legs and knows the touchdown limits. The Pilot stays in slot 3 (continuity of the flight view).

### 1.3 Who depends on whom (nobody can finish alone)
| Needs | From | What happens without it |
|---|---|---|
| Ice ranking and proposed site | Analyst -> everyone | No site lock; the stage timer picks the worst-ranked default and the score drops. |
| Hazard marks and the **safe zone** around the site | Safety -> Pilot (target ring), Analyst (sites with unsafe zones get flagged) | Pilot sees no safe ring; any touchdown counts as outside. Analyst cannot see which sites are unsafe, so a top ice site may be a death trap. |
| Ice truth quality | Science calibration in S2 -> Analyst | Noisy ranking. |
| Altitude, lidar hazard scan, legs armed | Systems -> Pilot, Safety | Rail OFF = Pilot altitude readout invalid, Safety scan blind, legs not armed = hard landing. |
| Limitations call-outs | Systems -> Pilot | The Pilot does not see the engine/legs limit numbers; he sees only his gauges. |
| Pre-planned approaches to top-2 sites | Pilot (during survey) | Without it, switching to the backup site in the event costs much more fuel (no pre-computed dogleg) and is likely impossible. |
| Site lock (three keys) | Analyst proposes, Safety clears, Pilot confirms plan | None of them can lock alone. |

Visibility rule (as in S2): each client builds only its own role's panel from replicated values. The map is shared; layers differ: Analyst sees ice + site list, Safety sees hazards/slope, Pilot sees the chosen site and the final safe-zone outline only, Systems sees the indicators.

### 1.4 No-wait rule (no role idle longer than 10 s)
| Phase | Systems | Safety | Pilot | Analyst |
|---|---|---|---|---|
| Survey (~90 s) | Pre-landing checklist with seeded faults | Marks hazards on the shared map | Plans approaches to the 2 top sites (burn timing + profile -> fuel estimate) | Reads ice layer, ranks sites |
| Site lock (~20 s) | Reports fuel/limits for both sites | Clears or rejects the proposed site | Confirms plan | Proposes, can swap with the 2nd site |
| Deorbit and powered descent (~75 s) | Live indicators, calls limits, arms legs | Live lidar scan of the final area (re-marks as the picture sharpens) | Flies | Prepares the backup ranking, picks the sample target for S4 |
| Hazard surprise (~25 s window, inside descent) | Calls the divert fuel budget | Marks the new zone | Diverts or switches site | Re-ranks the backup site given the new boulder field |
| Touchdown / report | Post-landing check (legs, tilt) | - | - | - |

### 1.5 Done criteria (spec: "lands inside a safe zone at the chosen site")
Landing succeeds when all of the following hold at touchdown (limits in `UMCLandingConfig`, TBD):
- inside the safe zone of the chosen (or diverted/backup) site, with the zone computed from the map the team marked;
- vertical speed <= 2.0 m/s (design doc), horizontal speed <= 1.0 m/s, tilt <= 12 deg, ground slope under the pad <= 10 deg;
- legs deployed (if the legs test failed in S2, vertical limit drops to 1.2 m/s);
- fuel >= 0 throughout.
Outcomes: Landed (safe), Hard landing (damage, health loss, mission goes on), Crash (speed or slope far over limits, or fuel out), Outside zone (spec M6: "a site outside the safe zone fails").

### 1.6 Random event: Hazard surprise (spec §3)
Seeded. At a seeded altitude (about 600 m, +-100 m) while descending to the chosen site, a boulder field appears over the planned touchdown point.
1. All players get the banner; Systems gets the alarm and calls the divert budget (fuel needed for a 200..400 m divert vs the backup).
2. **Safety Officer** gets a zoomed local map of the final area with the field drawn and ~25 s to mark a **new zone** (a click-circle) that is outside the field and on a slope below the limit. The server checks the mark against the terrain slope grid and the boulder field.
3. **Pilot** either diverts to the new zone (cheap) or switches to the **backup site** (the second site he pre-planned in the survey; costs more fuel, only available if the plan exists and the fuel is enough).
4. **Analyst** re-ranks the backup site given the new situation (affects the ranking score if the team switches).
Solvability guarantee: the generator always makes a slope-safe cell within 350 m of the field, and fuel for the divert is kept above the reserve in the nominal case (checked by the test in T8). Unhandled = the Pilot lands on the boulders: Crash or Hard landing.
Accuracy: Safety (new zone valid / invalid), Pilot (diverted in time), Systems (budget call in time).

### 1.7 Accuracy measures and score (placeholder, TBD)
Score out of 100: Landing 35 (zone, speeds, tilt, slope), Site choice 15 (Analyst ranking vs reference, via rank correlation of the 6 sites, plus whether the final site is in the top 2 by truth), Hazard marking 15 (Safety: correct marks / all marks, with a minimum coverage so he cannot mark nothing), Limitations reported 10 (Systems: call-outs sent before the relevant phase), Fuel left 10, Health 5, Hazard surprise handled 10. Per-role good/bad is appended to the same accuracy log as S1/S2 (arrays of size 5).

## 2. 3D presentation (same style as the S2 flight view)

### 2.1 Scenes
All three live in the deck map `L_Transit` (decision 7: no travel until S4), as read-only views driven by the replicated manager, like `AMCFlightView`. Each is a Blueprintable actor placed once far from the hall and from each other (S2 flight view is at 5..10 km; put the orbit view at ~20 km and the landing view at ~35 km, rotation 0, scale 1). They need their own sun DirectionalLight (south pole: low raking sun, 1.5..5 deg elevation).

| View | Class | Content |
|---|---|---|
| **Orbit** | `AMCOrbitView` | The Moon (engine Sphere with `MI_Moon`, the NASA LROC mosaic that already exists), south pole facing the camera, stars (existing `MI_Sky_Stars`), the ship in a polar orbit, the orbit ring, the ground track, the survey swath, the site markers placed at lat/lon on the sphere (text/widget components, no custom geometry), the two top sites highlighted. Insertion burn at the start. Camera: chase/orbit camera; the Pilot sees it with the approach planner overlay; the other three see it on the desk viewscreen. |
| **Descent** | `AMCLandingView` | Real lunar terrain mesh (section 2.3), ship = the S1 lander (reuse `AMCRocketStack::LanderMesh`, same as the flight view), engine flare light, dust puff at low altitude (a user-provided or engine Niagara emitter via a `BP_OnDust` event, no Niagara dependency in C++, like `BP_OnPulse`), landing legs deploy animation (if the lander mesh has none: a scale/offset animation of the existing leg parts, otherwise just a flag light), chase camera behind and above the ship, altitude-dependent camera pull-in. Target ring and safe-zone outline are drawn on the ground (debug-draw in dev builds; see risk R3). |
| **Surface (final 100 m)** | same `AMCLandingView` | Close-up: the lander meets the terrain with real elevation under it. Rocks/boulders (hazard instances) shown as instanced meshes, see decision 4. |

Phases in 3D: insertion burn, survey (ship over the sites, swath moves), site lock (camera moves to the chosen site), deorbit (ship drops into the descent scene), powered descent, event (boulder field appears with a short fog/flash in the HUD), touchdown (dust, dust-settle, camera orbit), results board.

### 2.2 Cameras and viewscreens
Same as S2: Pilot, when seated, gets the landing chase camera with the HUD overlay; the other three keep their station camera; **V** toggles for everyone; a `SceneCapture` viewscreen on the desk shows the current view (reuse `AMCFlightView`'s viewscreen code: factor it into a shared base `AMCViewBase` or copy it, T3 decides; the flight view must keep working).

### 2.3 Real lunar surface (what exists and its limits)
Exists (see [[Asset Catalog]] and `Tools/LOLA/README.md`):
- `Art/Meshes/Terrain/LunarTerrain_LOLA_26576_256_smooth`: real LRO LOLA LDEM_875S_5M south-pole patch, 512 x 512 samples at 5 m = **2555 m square**, relief 198 m, 522k triangles, Nanite OFF (no SM6), smoothed 3x to remove the 0.5 m quantisation. Matching `.r16` heightmap in `SourceArt/LOLA`. Approved by the user on 2026-11-14 **for the menu** (Decisions). `dem_craters.py` lists crater positions, `dem_scan.py` finds rougher windows (Status notes: moderate craters at 5 m/px, rougher windows exist near Shackleton, LDEM_875S).
Limits:
- 5 m posting: smooth, no boulders, no small craters. Rocks come from the data table, not the DEM.
- One patch: all sites would be regions of the same 2.5 km square unless more patches are generated (each is a separate `dem_to_glb.py` run, ~522k triangles each).
- The patch is not a flat landing field: relief 198 m means large areas exceed the 10 deg slope limit; the safe-zone logic has to be computed from the DEM (slope grid), which is what makes "mark the safe zones" a real task.
- A powered descent from 1500 m over a 2.5 km square is about the limit of the patch size; start higher only if the camera hides the edge (fog/horizon).
**Decision 1 for the user: use this LOLA patch (and derived data) in gameplay (default: yes).**

### 2.4 Site map table (2D, for the Landing Analyst and Safety Officer)
A UMG/C++ widget `UMCSiteMapWidget` that both roles use at once on the same replicated mark list:
- Background: a 2D image derived from the LOLA DEM: hillshade plus a slope colour layer plus a "shadow likelihood" layer (permanent-shadow proxy computed from the DEM horizon at 1.5 deg sun) used as the ice-likelihood hint. Produced offline by a new script in `Tools/LOLA` (T2), not by hand. The ice layer is game data derived from terrain and labelled so, not a claim of measured ice.
- Overlays: site markers with names and rank numbers, the survey swath, marks (circles: rock, steep slope, crater, new zone), the chosen site and the safe-zone outline. Click to place/remove a mark; mouse wheel zoom; second zoom level for the final-approach area (also used by the event).
- Layers per role as in 1.3. Marks are validated and scored on the server.
- Note: guidance in the spec §6 says 2D map images derived from LOLA are data, not geometry, but ask before use in the MVP. **Decision 2 for the user: OK to use a LOLA-derived 2D map image on the map table (default: yes).**
- Optional later: the same image on a 3D table prop in the deck (needs a table model from the user).

### 2.5 Landing mechanics
- **Frame:** local ENU in metres on the landing patch, origin at the site; vertical `z`. Gravity **g = 1.62 m/s^2** from the config.
- **Simple custom physics** (`FMCLandingPhysics`, a plain struct with a pure `Step(State, Input, dt)` so tests run without an actor): server fixed step 0.05 s. State: position `P`, velocity `V`, mass `m`, fuel `f`, tilt/heading, throttle. Acceleration = (thrust x throttle / m) along the tilted axis - g. Fuel flow `mdot = thrust x throttle / (Isp x g0)`. Tilt slews at a limited rate, max tilt 25 deg in flight. No rigid-body engine, no UE physics.
- **Pilot input** (sent at <= 10 Hz as a compact RPC): throttle up/down, tilt up/down/left/right (= horizontal acceleration command), heading, plus buttons: arm/deploy legs only if Systems armed them, switch target, divert, backup. Altitude via throttle, speed via throttle and tilt, direction via heading and tilt.
- **Indicators for the Pilot:** altitude above ground (valid only with the radar/lidar rail ON, else shown with an error that grows below 500 m, and invalid below 150 m for a ship without lidar), vertical and horizontal speed, fuel, throttle, tilt, bearing and distance to the target, the target ring and the safe ring.
- **Nominal profile:** deorbit at ~15 km, powered descent from ~1500 m altitude and ~1 km range at ~40 m/s down (TBD, tuned in T8 so a fair pilot lands with 15..30 percent of the S2 fuel left).
- **Touchdown:** when `z <= ground(x, y)` check speeds, tilt, slope under the pad (from the DEM grid), legs, zone membership -> outcome (1.5).
- **Safe zone and slope limits:** computed on the server from the slope grid (per 5 m cell) minus the marked/true hazards. The true hazards (rocks, craters, steep ground) come from the data; the **safe zone is the set of cells that are slope-OK and free of true hazards**. The team's marks only matter through the *information* they give (the Pilot's safe ring is built from Safety's marks), so wrong or missing marks send the Pilot to a bad place, which is exactly the accuracy measure.

## 3. Realisation in C++ (data-driven)
Logic in C++, Blueprints only for assets and layout (user decision 2026-11-14). All of it under `Source/MissionCrew/Landing/` (+ `Landing/UI`, `Landing/View`). No Blueprint graph work.

| Piece | Class / asset | Notes |
|---|---|---|
| State and rules | `AMCLandingManager` (replicated actor, always relevant) | Port of the `AMCTransitManager` pattern: stage machine (`EMCLandingStage`: Brief -> Insertion -> Survey -> SiteLock -> Deorbit -> Descent -> Event window -> Touchdown -> Report), seeded streams, `HandleAction(Code, A, B, Who)`, scoring, `OnLandingComplete(FMCLandingReport)`. Authoritative; clients hold no logic. |
| Config | `UMCLandingConfig` (UPrimaryDataAsset) | Gravity, durations, limits (speeds, tilt, slope), start altitude and range, event altitude and window, hazard counts, score weights, references to the site table, terrain data, map textures. Every number TBD. |
| Sites | `UMCLandingSiteTable` (UPrimaryDataAsset or DataTable of `FMCLandingSite`) | Id, name, map position and approach bearing, true ice score (reference rank derived from it), base hazards (`FMCLandingHazard`: type rock/slope/crater/boulderfield, position, radius, severity), distance (fuel cost), backup pairing, text. 6 sites (default) on one patch. A validation pass at startup fails loudly on missing fields (spec M1). |
| Terrain data | `UMCTerrainData` (UPrimaryDataAsset) | Row-major `TArray<int16>` heights (512 x 512), cell size, vertical scale, plus helper `SampleHeight(x, y)`, `SampleSlope(x, y)`. Filled once by a commandlet/Python script from the `.r16` (no runtime file IO, works packaged). Used by physics, safe-zone and the 2D map. |
| Hazards | `FMCLandingHazard` list on the manager | Built in `InitData` from the site table and the seed (jitters positions within bounds, never moves truth outside the table's constraints). The Hazard surprise adds one `boulderfield` hazard at the chosen target. |
| Marks | `FMCMapMark` (type, x, y, radius, owner role, valid flag) | Replicated `TArray` (cap ~24; plain replication is enough at this size, `FFastArraySerializer` only if profiling says so). Server validates every placement and updates "marked correct / marked wrong". |
| Mission link | reads `UMCMissionSubsystem` | See 3.2. |
| Descent view | `AMCLandingView` | Section 2. Reads the replicated ship state, interpolates (state replicated at 10 Hz; client smooths). No client logic. |
| Orbit view | `AMCOrbitView` | Section 2. Pure function of `ElapsedSec` and the stage. |
| UI | `UMCLandingHudWidget` + `UMCLandingSystemsPanel`, `UMCLandingSafetyPanel`, `UMCLandingPilotPanel`, `UMCLandingAnalystPanel` + `UMCSiteMapWidget` | Same pattern as the S2 panels (`BindWidgetOptional` + in-code fallback layout, reuse the `MCTransitUI` helpers). Each client builds only its own role's panel. |
| Player hook | `AMCBayCharacter` (already hosts station sit/leave and `ServerTransitAction`) | Add `ServerLandingAction(int32 Code, float A, float B)` (Server, Reliable for buttons; the continuous pilot input uses an Unreliable RPC at 10 Hz) and V toggle for the new views. Small generalisation: the pawn dispatches to the *active* manager (stage in the subsystem), so S2 behaviour stays unchanged. |
| Role names | helper `UMCRoleNames` | One table: stage + slot -> name, colour, so HUD and lobby show the right role per phase. |

### 3.1 Replication
- `AMCLandingManager` replicates plain values only: stage, stage end time, seed, site ids and ranks, marks, the chosen/backup site, safe-zone outline (as a short polygon or circle list, not the full grid), indicator values, ship state (`FVector_NetQuantize` position, quantised velocity, fuel, throttle, tilt) at `NetUpdateFrequency` 10, report. Same style as `AMCTransitManager` (no per-frame RepNotify spam; use `GetStageTimeLeft()` with a replicated end time).
- Terrain data and site table are identical assets on every machine; only ids and seeds travel.
- Server authority for everything: the pilot input is applied on the server, clients never integrate physics. Client-side the view interpolates between 10 Hz snapshots.
- The map click RPC sends quantised map coordinates (int16 x, int16 y), type and radius.

### 3.2 Reading the S2 result, and what must change earlier
`UMCMissionSubsystem` holds only the S1 loadout today; the S2 report lives on `AMCTransitManager::Result` and would be lost if S3 is a separate manager created later. **Changes needed in earlier stages:**
1. `UMCMissionSubsystem`: add `FMCTransitReport Transit`, a `bHasTransit`, a stage enum (`CurrentStage`) and `FMCLandingReport Landing` + `bHasLanding`; `RecordTransit(...)` called by `AMCTransitManager` when it finishes (before/with `OnTransitComplete`). Server-side only; the S3 manager reads it in `InitData`. (If S3 starts on the same map as S2 the manager could read the live `AMCTransitManager`, but going through the subsystem keeps S3 independent and testable.)
2. **Extra values the S2 result must carry** (not in `FMCTransitReport` today):
   - per-instrument calibration error/quality (array of name + error + calibrated bool), not only the counts;
   - per-device pass/fail by name (at least `LANDING LEGS`, `ROVER`, `DRILL`), not only `DevicesPassed/Total`;
   - whether the sensor fault was resolved (NAV SENSOR swapped) and the residual position bias if not;
   - `Seed` of the mission and `ElapsedSec` per stage (for the summary);
   - the final true miss vector (`ShipErrX/ShipErrY`) for the orbit insertion offset;
   - the S2 rail states are not needed (S3 has its own checklist).
3. From S1 (`FMCLoadout`/subsystem): engine thrust and Isp, dry mass, lander mass, `bHasLidar`, `bHasRover/Drill`, tank count. Verify these exist as numbers in `UMCMissionSubsystem` (it has `FuelKg`, `bHasRover/Drill/Lidar`, `Loadout`); add `DescentThrustN`, `Isp`, `DryMassKg` if missing.
4. `AMCTransitManager`: on arrival, hand over to `AMCLandingManager` instead of ending the game: either the manager is placed in `L_Transit` and auto-starts on `OnTransitComplete`/subsystem flag, or a small `AMCMissionDirector` spawns the next phase manager. Recommended: a tiny director (stage -> spawn/enable the manager, the same code S4 will use), so each phase manager stays a standalone actor.
5. `AMCBayCharacter`: the pawn dispatches input and HUD by the active stage (S2 panels vs S3 panels); `ShowTransitHud` becomes `ShowPhaseHud`. S2 behaviour must not change.
6. Lobby role names stay as they are (they belong to the other agent); `UMCRoleNames` gives the in-game names.
7. `AMCFlightView`: add an end state that hands the camera to the orbit view (no change to its logic).

### 3.3 Seeded determinism
- One mission seed (already `AMCTransitManager::Seed` / `SeedOverride`; the subsystem stores it). S3 derives named sub-streams: `FRandomStream(Seed ^ 0x53330001)` for sites/ice noise, `...0002` for hazards, `...0003` for the surprise (trigger altitude, which cell, direction), `...0004` for the checklist faults. Streams are drawn **only in `InitData`**, once, in a fixed order, so a player's actions never change the draws. Same seed = same sites, truths, hazards, event, faults.
- Physics is deterministic given the input sequence: fixed 0.05 s step, no `DeltaTime` in the integration, `float` ops in a fixed order. A scripted bot input gives the same touchdown on every run (used by the replay test).
- The seed is shown in the debrief (spec §2).

### 3.4 Scoring and the accuracy log
`ComputeReport()` on the server builds `FMCLandingReport`: `Score` and parts (landing, ranking, hazards, limitations, fuel, health, event), outcome enum, touchdown speeds/tilt/slope, distance to the target and to the safe-zone edge, site id and "was backup/diverted", fuel and health left, `RoleGood/RoleBad` (size 5, appended to the shared log), a text report (`ReportText`) with one lesson line (spec M10). Every role action calls `Score(Role, Good)` like the S1/S2 managers.
- Analyst: Spearman rank correlation of the team's ranking vs the true order of the 6 sites, mapped to 0..1.
- Safety: precision = marks that cover a true hazard / marks placed, times coverage = true hazards covered / true hazards within the survey area (so neither marking everything nor nothing pays).
- Systems: call-outs/checklist items sent before the phase that needs them.
- Pilot: landing outcome and margins.

### 3.5 Hand-over to S4 (surface)
`AMCLandingManager::OnLandingComplete` writes `FMCLandingReport Landing` into `UMCMissionSubsystem`: touchdown position and heading on the patch, site id, outcome, fuel and health left, lander tilt, device states (legs, rover, drill, lidar), calibrated ice instruments (from S2), the true ice distribution reference for the chosen site, the accuracy log, seed, and the S1..S3 score so far. S4 reads it: the lander is spawned at the touchdown point on the same terrain mesh (`L_Surface` with the same LOLA tile, so the S4 rover drives where the lander stood), the rover/drill availability follow the device flags. Crash ends the mission (decision 6). Travel: a non-seamless `ServerTravel`, same method as S1 -> S2; roles survive via the subsystem.

### 3.6 Risks
- R1: `EMCTransitStage`-style code is duplicated; keep S3 independent (own enums) to avoid editing the S2 manager more than the hand-over.
- R2: terrain data size and cost: 512 x 512 int16 = 0.5 MB, fine; the render mesh is 522k triangles, Nanite OFF; keep one patch in the level.
- R3: debug-drawn lines (orbit ring, zone outlines, target ring) do not exist in Shipping, like in the S2 flight view. For a packaged demo use a Development build, or replace them with a Niagara ribbon / decal supplied by the user, or draw the rings in the HUD canvas.
- R4: viewscreen show-only lists exclude debug lines (known S2 limit).
- R5: the world size: the landing patch is 2.5 km; keep `LANDING_VIEW` at a distance that avoids float precision issues (stay within ~40 km from the origin).
- R6: roles after non-seamless travel depend on `ApplyRoles`; S3 on the same map avoids it, S4 relies on it.

## 4. Plan (small tasks, parallel agents)
Agents never touch the editor except T6 and T9; C++ agents compile-check only (game target) and must not run while the editor is open (rebuild point R1/R2).

| # | Task | Agent | Output | Depends on |
|---|---|---|---|---|
| T0 | Confirm the decisions in §5 | user | answers | - |
| T1 | C++ core: `UMCLandingConfig`, `UMCLandingSiteTable`, `UMCTerrainData`, `FMCLandingPhysics` (pure), types (`MCLandingTypes.h`), `AMCLandingManager` (stages, seeded streams, marks validation, event, scoring, report, `OnLandingComplete`); compile-check | C++ agent A | files | T0 |
| T2 | Data pipeline: `Tools/LOLA` scripts: (a) heightmap -> `UMCTerrainData` filler (Python/commandlet), (b) slope grid and hazard candidate lists (reuse `dem_craters.py`), (c) hillshade + slope + shadow-likelihood PNGs for the map, (d) first `UMCLandingSiteTable` content (6 sites, placeholder ice truths, TBD) as a JSON the editor step turns into the asset | data agent | scripts + PNG + JSON in `SourceArt/LOLA` and `Tools/LOLA` | T0, decisions 1-3 |
| T3 | C++ views: `AMCLandingView` (terrain-attached descent view, ship, flare, chase camera, ring/zone drawing, `BP_OnDust`), `AMCOrbitView`, shared viewscreen base or copy; compile-check | C++ agent B | files | T1 types |
| T4 | C++ UI: `UMCLandingHudWidget`, 4 role panels, `UMCSiteMapWidget` (draw image, marks, zoom, click -> RPC); compile-check | C++ agent C | files | T1 types |
| T5 | C++ glue (earlier-stage changes in 3.2): subsystem fields and `RecordTransit`, extra S2 report values, `AMCMissionDirector` (or the manager auto-start), pawn `ServerLandingAction` + pilot input RPC + V toggle + active-stage dispatch, `UMCRoleNames`; compile-check; S2 must still behave identically | C++ agent D (owns `MCTransitManager`, `MCBayCharacter`, subsystem edits; nobody else touches them during this task) | files | T1 types |
| T6 | **Rebuild (R1)**: close the editor, build the editor module, open it; then data assets (`DA_LandingConfig`, `DA_LandingSites`, `DA_TerrainData`), map textures imported, widgets' layout if any | me + editor agent | working classes and assets | T1-T5 |
| T7 | Level: place `AMCOrbitView` (~20 km) and `AMCLandingView` (~35 km) in `L_Transit`, sun lights, set the landing terrain mesh under the view, `AMCLandingManager` and director, viewscreen RT/material for the desk screens, camera and sky; boulder meshes if the user provides them | editor agent | level saved | T6, user's rock model (decision 4) |
| T8 | Balance: tune descent numbers so fair play lands with 15..30 percent fuel, divert always possible; placeholder numbers stay marked TBD | me / tester | config values | T6 |
| T9 | **Rebuild (R2)** only if T8 needs code changes; final editor pass: PIE walk-through | me | - | T8 |
| T10 | Tests: `RunDebugTest` port (scripted bot for all four roles printing `DEBUGTEST`), seeded replay, 2-player listen server, no-wait check (section 4.1) | test agent | report | T6 |
| T11 | Vault + git (git only when the user asks): `Status.md`, `Architecture.md` (Landing section), `Asset Catalog.md`, `Decisions.md` | me | docs | all |

Dependencies: T1 first (types file within a few minutes so T3, T4, T5 can start), then T1, T2, T3, T4, T5 in parallel (files only; T5 is the only task allowed to edit existing S1/S2 C++ files). T6 is the single rebuild point (editor closed). T7 in the editor in parallel with T8. T10 last.

### 4.1 Tests
1. **Physics unit checks** (no actor): free fall from 1500 m with zero throttle hits the ground in about 43 s at 69.7 m/s (g 1.62); hover throttle holds altitude; fuel mass decreases as `thrust x throttle / (Isp x g0)`.
2. **Seeded replay:** run the scripted bot twice with the same seed -> identical site truths, hazards, surprise altitude/position, checklist faults, touchdown position and score; a different seed -> different data. Log with `DEBUGTEST`.
3. **Acceptance (spec M6):** a bot lands outside the safe zone -> fails; the Pilot uses the pre-planned backup site -> works; skipping the plan makes the backup unavailable; the surprise is solvable by divert and by backup; unsolved = crash.
4. **No-wait:** per role log the time since the last available action; fail if any role is idle > 10 s (summary excepted).
5. **Two players on a listen server:** each sees only their own panel; marks made by one appear for the other; pilot input applies; the view matches on both; no client-side physics.
6. **Network:** a client joining late sees current stage and marks (replicated state is enough).
7. **Hand-over:** `UMCMissionSubsystem` has the S2 report before S3 starts and the landing report after; S4 reads it (a stub is enough until S4).

## 5. Decisions needed from the user (with defaults)
1. **LOLA terrain in gameplay.** Use the existing patch `LunarTerrain_LOLA_26576_256_smooth` and LOLA-derived data (heights, slope, hillshade) for the descent and the maps. Default: **yes**. It was approved only for the menu.
2. **2D site map.** A 2D map widget over a LOLA-derived hillshade/slope/shadow-likelihood image for the Analyst and Safety Officer. Default: **yes** (data, not geometry).
3. **Site count and terrain.** 6 sites as regions of the single existing patch (default), or generate more patches for more varied scenes (more triangles, more build time).
4. **Boulders/rocks.** CLAUDE.md forbids me making models. For the boulder field and rock hazards: please provide a rock/boulder mesh (default: until then they are map markers and HUD scan returns only, plus dust/decal if available). Also a landing-leg or dust effect if you have one.
5. **Role mapping.** Slot 2 = Safety Officer, slot 4 = Landing Analyst (default), or the opposite.
6. **Crash consequence.** Crash ends the mission and goes to the debrief (default), or allow one retry of S3 with a penalty for the demo.
7. **Map for S3.** Same deck map `L_Transit` for orbit and descent, travel to `L_Surface` only at S4 (default).
8. **Numbers.** Keep the placeholder numbers (4 min, 1500 m start altitude, 2 m/s, 1 m/s, 12 deg tilt, 10 deg slope, 6 sites) until you give balanced ones. Also the real site names and ice scores: use plausible named south-pole regions with TBD numbers, to be checked against NASA sources.
9. **Orbit/zone lines.** Accept debug-drawn lines for now (Development build, like the S2 flight view), or supply a ribbon/line effect for a packaged demo.
10. **Crash realism.** Simple vertical + 2D lateral physics with a thrust vector (default) vs a more detailed attitude model (not recommended in 48 h).
11. **Course fail from S2.** Off-corridor arrival still allows S3 with a fuel/time penalty (default) or blocks it.
