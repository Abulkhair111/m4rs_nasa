# Ice Return MVP: Construction Phase (brief for the developer agent)

**Status: DRAFT, 2026-11-14.** This file is not yet reconciled with `ROADMAP.md` or `Vault/Game Design.md`. Do not start building until the user confirms the decisions in §1.

"Construction phase" in this file means the build stage of the MVP. It does not mean the in-game 3D assembly bay (`Vault/construction_phase.md`), which already exists.

## 0. Read first

1. `CLAUDE.md`: Blueprint-only. Do not create 3D models; ask the user. Commit or push only when the user asks. Update the vault at the end of each session.
2. `Vault/00 Index.md`, then `Vault/Unreal MCP Workflow.md` before any editor work. The editor is in Russian and breaks the Blueprint DSL.
3. `Vault/Status.md`, section "Ownership", before editing any shared Blueprint.
4. This file, §3, for the MVP concept.

## 1. Decisions needed before building (blocking)

| # | Question | Default until the user answers |
|---|---|---|
| 1 | Is Ice Return a new mission in this project, or does it replace Mission Crew? The two designs differ in roles, phases, and length. | New mission and map. Keep the lobby, LAN networking, and main menu. |
| 2 | Roles. The concept assigns 4 roles per submission after each briefing (24 slots in total). Mission Crew fixes 4 roles per player for the whole mission. | Concept model: roles are assigned per submission. |
| 3 | Length. The concept has 6 submissions. ROADMAP targets about 25 minutes. | Target 25 minutes total, about 4 minutes per submission. Cut detail if needed. |
| 4 | Sample container: active cooler or passive cold container. | Passive. Build both behind a data flag. |
| 5 | Is anyone on board the spacecraft? The concept has a Pilot role but does not say whether it is crewed. | No life support. The Pilot is a flight role. |
| 6 | Multiplayer. | Reuse the LAN listen-server lobby for 4 players. No split-screen. |
| 7 | Numbers for budget, mass, power, and launcher capacity. The concept has none. The Shadow Ice numbers from an earlier example are not part of this MVP. | Use placeholder values in the project's data tables. Mark every MVP value "TBD". The user supplies balanced numbers. |
| 8 | Art. The rover (Perseverance) is in the project. The lander, parts, bay, and Moon model are not. | UMG-only screens. No primitive greybox (see §6). Ask the user for any 3D asset. |

## 2. Architecture

- **Phases:** `E_Phase` runs Lobby → Briefing → S1 Design & Assembly → S2 Launch & Transit → S3 Orbit & Landing → S4 Surface → S5 Sample Preservation → S6 Return & Debrief → Summary. Mission Crew's Design, Launch, Landing, and Surface phases map onto S1–S4.
- **Roles:** `BP_MC_GameState` holds a role table per submission (4 slots each). Holders are replicated through `BP_MC_PlayerState`.
- **Shared board:** `BP_MC_GameState` holds mass, budget, power, fuel, data, time, and the handoff queue. Everything is replicated with RepNotify.
- **No-wait rule:** every role has its own task stream. A role posts its result to the shared board, and the next role reads the latest state. Timers run on the server.
- **Random events:** a seeded RandomStream, as in `BP_FaultManager`. The seed is shown in the lobby and in the summary. Events live in `DT_Events` with a trigger submission and an effect.
- **Accuracy log:** every role action is recorded as correct or incorrect, per player, in `BP_MC_GameState`. The summary reads from this log.
- **Server authority:** all player actions go through `BP_MC_PlayerController` Server RPCs. Each new RPC event must be set to Replicates → Run on Server by hand in the editor (see ROADMAP §1).
- **Data:** reuse `DT_Components` and `DT_Rockets`. Add `DT_Submissions` (goal, roles, done criteria), `DT_Briefings` (text per submission, facts-checked), and `DT_Events`.

## 3. MVP concept

**Mission:** Ice Return. Players fly a lunar ice sample return mission in six submissions. Each submission has a briefing and four roles.

**Primary goal:** bring at least one verified ice sample back to Earth.
**Secondary goals:** find ice at more than one site, stay within budget, finish on time.

**Random events** (five designed, at least two required):

| Event | Submission | What the team must solve |
|---|---|---|
| Sensor fault | S2 | A sensor misreads. The Pilot makes an extra correction, and the Systems Operator isolates the sensor. |
| Hazard surprise | S3 | A boulder field appears at the chosen site. The Safety Officer marks a new zone, and the Pilot diverts or switches to the backup site. |
| Power drop | S4 | The battery loses capacity. The Power Operator cuts non-essential loads, and the drill pauses. |
| Cooler fault (or hold-time warning if the passive container is chosen) | S5 | Storage temperature rises. The Systems Operator switches to backup cooling, and the Sample Scientist updates storage conditions. |
| Reentry blackout | S6 | Radio contact drops during reentry. The Data Specialist's report must be queued before the blackout. |

**Briefings:** each submission opens with a heist-style briefing: goal, equipment, spacecraft status, roles, and clock. Build it in UMG with a Level Sequence, not 3D. Every number comes from `DT_Briefings`, checked against NASA sources before release.

### S1: Design & Assembly

**Goal:** build a spacecraft that fits the launcher, has enough power, thrust, and fuel for the whole flight, and stays within budget.
**Parallel work:** all four roles work on the same diagram. Totals update live.
**Done when:** mass is within the launcher limit, power balance is positive, thrust covers the planned burns, fuel covers the flight with margin, and budget is not exceeded.

| Role | Responsibility | Accuracy measure |
|---|---|---|
| Power Engineer | Connects modules to the power grid and distributes limited energy. | Power balance stays positive |
| Science Specialist | Selects instruments from the list that can find ice, within budget. | Chosen instruments suit the ice search |
| Construction Engineer | Places modules on the diagram, connects them, and checks that everything fits. | Placement and connection errors |
| Propulsion Engineer | Chooses the engine (thrust) and fuel tanks, and checks that fuel covers the flight. | Fuel plan covers the flight with margin |

### S2: Launch & Transit

**Goal:** reach the Moon on course, with systems healthy and equipment ready.
**Parallel work:** the Science Operator and Equipment Technician check separate equipment. The Pilot's corrections happen in timed windows. The Systems Operator takes warnings as they arrive.
**Done when:** the spacecraft is on course, instruments are calibrated, and equipment is tested.

| Role | Responsibility | Accuracy measure |
|---|---|---|
| Systems Operator | Monitors the spacecraft panel and resolves warnings with the correct switches. | Warnings resolved with the correct switch |
| Science Operator | Calibrates the instruments with a simple mini-game and checks readings. | Calibrations within tolerance |
| Equipment Technician | Tests the rover, drill, and landing mechanisms before arrival. | Tests run and passed correctly |
| Pilot | Keeps the spacecraft on course and performs trajectory corrections. | Corrections keep the course within the window |

### S3: Orbit & Landing

**Goal:** land safely at a site with a good chance of ice.
**Parallel work:** the Landing Analyst and Safety Officer mark the same map at once. The Pilot plans approaches to the two top candidate sites before the choice is final.
**Done when:** the spacecraft lands inside a safe zone at the chosen site.

| Role | Responsibility | Accuracy measure |
|---|---|---|
| Systems Operator | Monitors indicators and reports limitations to the Pilot. | Limitations reported in time |
| Landing Analyst | Studies the Moon map and ranks sites by likelihood of ice. | Ranking matches the reference |
| Safety Officer | Finds rocks, slopes, and craters, and marks safe zones. | Hazards marked correctly out of all marked |
| Pilot | Performs the landing, controlling altitude, speed, and direction. | Landing inside the safe zone, within limits |

### S4: Surface Exploration & Ice Detection

**Goal:** find the best ice site and collect at least one sample.
**Parallel work:** the Drill Operator sets depth while the Rover Driver is still driving. The Power Operator and Science Analyst keep working throughout.
**Done when:** at least one sample is collected from a site the Science Analyst rated as likely to hold ice.

| Role | Responsibility | Accuracy measure |
|---|---|---|
| Power Operator | Monitors battery charge and distributes energy among the equipment. | Energy allocated with no shortage |
| Science Analyst | Reviews readings and works out where ice is most likely. | Site choice supported by the readings |
| Rover Driver | Operates the rover, chooses the route, and avoids obstacles. | Obstacles avoided, route completed |
| Drill Operator | Operates the drill, selects depth, and extracts samples. | Sample taken at the chosen depth |

### S5: Sample Preservation & Return Preparation

**Goal:** store, seal, and load all samples correctly for the return trip.
**Parallel work:** a pipeline. The Sample Technician seals one sample while the Sample Scientist labels the next. The Cargo Operator secures each sealed container as it arrives.
**Done when:** all samples are sealed, labeled, stored, and secured on board.

| Role | Responsibility | Accuracy measure |
|---|---|---|
| Systems Operator | Monitors storage temperature and keeps the required cooling mode. | Time storage stayed in range |
| Sample Scientist | Identifies sample types, labels them, and selects storage conditions. | Samples labeled and matched to conditions |
| Sample Technician | Places samples in containers, seals them, and checks packaging. | Seals and packaging checks passed |
| Cargo Operator | Loads the containers and secures them before return. | Containers secured, nothing loose |

### S6: Return to Earth & Scientific Debriefing

**Goal:** return the samples safely and send the final report to Earth.
**Parallel work:** the Data Specialist sends the report during transit while the Recovery Specialist prepares the capsule and the Pilot flies.
**Done when:** the capsule lands, the beacon is active, samples are handed over, and the report is sent.

| Role | Responsibility | Accuracy measure |
|---|---|---|
| Systems Operator | Monitors the spacecraft's condition and resolves warnings. | Warnings resolved correctly |
| Data Specialist | Checks the science data and sends the final report. | Report complete and sent before the blackout |
| Recovery Specialist | Activates the beacon, checks capsule equipment, and prepares samples for handover. | Beacon on time, checks passed |
| Pilot | Flies the return and performs the landing on Earth. | Landing inside the target zone |

## 4. Milestones

Each milestone is done only when its acceptance check passes.

| # | Milestone | Deliverables | Acceptance |
|---|---|---|---|
| M0 | Reconcile | Answers to §1 recorded in `Vault/Decisions.md`. | The user has confirmed §1. |
| M1 | Data | `DT_Submissions`, `DT_Briefings`, `DT_Events`; updated `DT_Components` and `DT_Rockets`. | Tables load at startup. Validation fails loudly on missing fields. Hard checks: mass ≤ launcher capacity, budget ≤ limit, fuel ≥ required. |
| M2 | Phase machine and roles | `E_Phase` with S1–S6; per-submission role table; replicated holders. | 4 clients in the lobby. Each gets a role. Phase changes reach all clients. |
| M3 | Briefing | UMG and Level Sequence briefing, skippable, driven by `DT_Briefings`. | Plays before each submission. Every number matches `DT_Briefings`. |
| M4 | S1 Design & Assembly | Four role panels, live totals, hard checks, done criteria. | An over-mass build is blocked. A valid build passes. |
| M5 | S2 Launch & Transit | Panel puzzle, calibration mini-game, equipment tests, timed corrections, Sensor fault event. | The event triggers and is solvable. |
| M6 | S3 Orbit & Landing | 2D site map, hazard marking, pre-planned approaches, Hazard surprise event. | A site outside the safe zone fails. The backup site works. |
| M7 | S4 Surface | 2D map, rover and drill, energy shared across consumers, Power drop event. | Drill does not run without enough power. The event is handled. |
| M8 | S5 Sample Preservation | Pipeline, container choice, Cooler fault or hold-time warning. | The chosen container changes mass, power, and hold time. |
| M9 | S6 Return & Debrief | Reentry corridor, blackout, beacon, report queue, Reentry blackout event. | A report not queued before blackout is lost. |
| M10 | Summary | Score, time per submission, per-player accuracy, one lesson line per submission. | Summary numbers match the event and accuracy logs. |
| M11 | Network and no-wait test | 4-player LAN run; seeded replay. | Same seed gives the same events. No role has zero available actions for more than 10 seconds (summary excepted). |
| M12 | Packaging | Windows build, demo script. | The full mission runs from lobby to summary on 4 machines. |

## 5. Tests

- Data validation runs at startup.
- Seeded replay: the same seed produces the same events.
- No-wait check: with 4 clients, a busy role never blocks another. Log any role idle for more than 10 seconds.
- Accuracy check: the summary matches the accuracy log.
- Network check: each client sees only its own role's panel.

## 6. Art and assets

- The agent does not create 3D models (CLAUDE.md). Ask the user for the lander, parts, bay, and Moon model.
- The Perseverance rover is available.
- ROADMAP §2 proposes greybox rooms built from primitives. CLAUDE.md forbids meshes built from primitives. Use UMG-only screens instead, and get the user's approval before any greybox.
- 2D map images derived from the LOLA data in `Tools/LOLA` are data, not geometry. Ask the user before using them in the MVP.

## 7. Ownership and coordination

- Check `Vault/Status.md` → Ownership before editing. Current owners:
  - Main menu scene and `WBP_MainMenu` styling, construction phase, `BP_MenuPlayerController`: "menu/art" session.
  - LAN session logic, `BP_MC_GameInstance` session code, `WBP_LanSessionRow`: another agent.
  - `BP_MC_GameMode`: shared.
- Ice Return changes `E_Phase`, `BP_MC_GameState`, and `BP_MC_GameMode`. These are shared, so coordinate before editing and note the change in `Vault/Status.md`.
- Do not change the in-game assembly bay unless the user decides that S1 replaces it.

## 8. Vault and git

- At the end of each session, update `Status.md`. Update `Asset Catalog.md`, `Architecture.md`, and `Decisions.md` when they change.
- Commit or push only when the user asks. The remote is `origin`, branch `main`.

## 9. Definition of done (MVP)

1. Four players complete all six submissions over the LAN, from lobby to summary.
2. Each role sees only its own information, and the mission can't be completed without talking to the team.
3. Hard checks block bad designs. Soft errors appear only in flight.
4. At least two random events are implemented and seeded.
5. Each submission has a briefing, roles, done criteria, and a parallel-work path.
6. The summary shows the score, time per submission, and per-player accuracy.
7. A packaged Windows build runs on 4 machines.
