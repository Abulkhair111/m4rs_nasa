# Game Design (summary of the design PDFs)

NASA Space Apps Challenge 2026 · "Space Mission Design Game" · asymmetric co-op for 4 players · ~25 min per mission.
MVP scenario: **find ice at the lunar south pole**. References: Keep Talking and Nobody Explodes, Phasmophobia, Lethal Company.

**Core idea:** Mission Control (MCC) knows but can't act; the crew can act but doesn't know. Voice communication is mandatory.

## Roles
| Role | Side | Sees | Does | Department |
|---|---|---|---|---|
| MCC Engineer | MCC | all telemetry, error codes | diagnostics, power, comms | power & comms |
| Flight Director | MCC | plan, map, repair manual | go/no-go, sample sites, procedures | objective, rocket, budget |
| Pilot | Crew | cockpit instruments only | launch, course, landing, rover | engines & fuel |
| Scientist | Crew | panels and dashboard | hands-on repairs, drill, analysis | science instruments |

## Mission phases
0. Lobby (host creates, others join by IP, pick roles) → Briefing (goal, budget $500M, comms check)
1. **Design** 8–10 min: each player edits their own department. Shared mass, budget, power, data and time. Hard checks: mass, budget, fuel. Cheap parts break more often. Soft errors only surface in flight.
2. **Flight**: Launch ~2 min (pitch, stage separation) → Transit ~2 min (course correction, landing zone, calibration) → Landing ~3 min (touchdown < 2 m/s, blind without lidar, legs deployed) → Surface ~7 min (rover, drill, power shared by 6 consumers, data sent only during comm windows) → Ending 1 min (last comm window, send the most valuable data first)
3. **Debrief** 2–3 min: score, S–F rank, event log with causes, "Redesign" button.

## Faults (MCC code → director procedure → crew hands, 60–90 s)
PWR bus overload · FUEL leak (valve number known only to MCC) · COMM loss (crew re-aims the antenna) · DRILL overheat.
External events: solar flare, micrometeorite. Running out of time has consequences.

## Score (100 points)
Science 60 × fraction of data transmitted · Repairs 20 · Budget 10 · Landing 10. Crash → 0.
Ranks: S 90+, A 75+, B 60+, C 40+, F <40.

## NASA data / learning
Component cards with real analogues (VIPER instruments: NIRVSS, NSS, MSolo, TRIDENT drill), a south pole map from LRO data, and a tutorial first launch.
