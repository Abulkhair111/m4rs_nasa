# Balance Proposal (Agent BAL, 2026-11-15)

**Status: PROPOSAL for the user to review. Every number here is a TBD placeholder until the user approves it. Nothing in the project was changed** (no C++, no assets, no editor). The data is in [[Balance Proposal data.csv]] (`Vault/Balance Proposal data.csv`), ready for an import script that sets the `UMCPartDef` fields and the manager's `Rockets` table.

Rule sources read: `Source/MissionCrew/Construction/MCConstructionManager.cpp` (`Rockets`, `Recalc`, `RecalcChecks`, `PreviewSwap`), `MCConstructionTypes.h`, `MCTransitTypes.h` (`UMCTransitConfig`), `MCTransitManager.cpp`, [[Architecture]], [[Decisions]], [[Asset Catalog]], `CONSTRUCTION_PHASE_ICE_RETURN.md` S1/S2, [[Game Design]].

## 0. Summary of the proposal

- Keep the manager defaults that are already in code: lander base **1200 kg / $100M**, **budget $500M**, T/W limits **1.3 / 2.0** at **1.62 m/s^2**, fuel need **0.9 x dry mass**, green margin **+10%**.
- Launchers (`Rockets[]`): **Falcon 9 3300 kg / $70M / 0.95**, **Vulcan 4800 kg / $110M / 0.975**, **Falcon Heavy 8000 kg / $150M / 0.99**. The three differ in payload, cost and reliability (cheap = small = riskier).
- A sensible build is about 1.6 t dry, 3.2 t at take-off, $300-370M. It passes with 1 or 2 yellows on Falcon 9 and all green on Vulcan.
- Greedy builds fail a specific check: all-science on Falcon 9 or Vulcan fails **Mass**; the same build on Falcon Heavy fails **Budget**.
- S2: shorten the run to about **3.5 minutes** (coast 150 s, windows of 12 s), nudge cost 12 kg, scoring weights 30/20/20/20/10.
- Two small, optional code changes are suggested (section 4); everything else is data.

## 1. Method and real-world anchors

Rule of thumb used: take a real public figure, then scale it to the game's ~3 t take-off vehicle so that the 10 checks have the intended bite. The game is not a simulation: values are rounded, and cheap parts are made less reliable ("cheap parts break more often", [[Game Design]]).

Fetched figures (Wikipedia pages, which cite the primary sources; fetched 2026-11-15):

| Anchor | Figure | URL |
|---|---|---|
| Nova-C (Intuitive Machines, IM-1, launched on Falcon 9) | dry 675 kg, launch 1,908 kg, main engine 3.1 kN (VR900), payload ~100-130 kg | https://en.wikipedia.org/wiki/Nova-C |
| Blue Ghost M1 (Firefly, Falcon 9) | dry 469 kg, launch 1,517 kg, payload ~94 kg, solar up to 400 W | https://en.wikipedia.org/wiki/Blue_Ghost_Mission_1 |
| Peregrine (Astrobotic, Vulcan VC2S) | launch mass 1,283 kg, payload 90 kg, failed (propellant leak) | https://en.wikipedia.org/wiki/Peregrine_Mission_One |
| Apollo Lunar Module | ascent stage dry 2,150 kg, gross 4,700 kg, APS 16 kN, Isp 311 s, ascent dv 2,220 m/s; descent engine 45 kN (throttle 10-60%), dv 2,500 m/s | https://en.wikipedia.org/wiki/Apollo_Lunar_Module |
| MMRTG (RTG) | 45 kg, 125 W electric at start (Perseverance: ~110 W at launch), ~2 kW thermal | https://en.wikipedia.org/wiki/Multi-mission_radioisotope_thermoelectric_generator , https://en.wikipedia.org/wiki/Perseverance_(rover) |
| Perseverance | 1,025 kg; UHF relay up to 2 Mbit/s; ~$2.75B mission | https://en.wikipedia.org/wiki/Perseverance_(rover) |
| VIPER (NSS, NIRVSS, MSolo, TRIDENT drill) | rover 430 kg; drill ~20 kg (public figure via a Blue Origin post); no verified instrument mass/power figures found | https://en.wikipedia.org/wiki/VIPER_(rover) |
| Li-ion batteries | 160-300 Wh/kg at cell level (pack/system level lower, ~65-100 Wh/kg used here) | https://en.wikipedia.org/wiki/Lithium-ion_battery |
| Falcon 9 Block 5 | LEO 22.8 t expended / 17.4 t reusable; GTO 8.3 t / 5.8 t; liftoff ~7.6 MN; price $74M (2026) | https://en.wikipedia.org/wiki/Falcon_9 |
| Vulcan Centaur | TLI 2.1 t (VC0), 6.2 t (VC2), 9.1 t (VC4), 11.3 t (VC6); GTO 8.3 t (VC2); from $110M | https://en.wikipedia.org/wiki/Vulcan_Centaur |
| Falcon Heavy | LEO 63.8 t expended; GTO 26.7 t expended / ~16 t side boosters recovered; ~22.8 MN; $97M reusable (2022), $150M expended; 14/14 flights | https://en.wikipedia.org/wiki/Falcon_Heavy |

Not verified (flagged TBD in the CSV notes): landing-lidar mass and power, NSS/NIRVSS/MSolo mass, power and data rates, capsule mass, antenna rates. These are game-scale guesses in the right order of magnitude. If the team wants real values, NASA's VIPER instrument pages and the instrument papers are the next place to look.

### How the anchors became game numbers

- **Launcher payload** = "mass the launcher can throw to the Moon", scaled to the game vehicle. Proxy: GTO capability x ~0.6 for Falcon 9 (5.8 t reusable -> 3.3 t) and Vulcan (8.3 t -> 4.8 t), ~0.5 for Falcon Heavy (16 t side-recovered -> 8 t). Vulcan VC2 TLI (6.2 t) is the same order. Ratio 1 : 1.45 : 2.4. Cost $70M / $110M / $150M = the figures already on the lever labels and close to the public prices.
- **Engines**: the Basic engine (7.5 kN) is a pressure-fed hypergolic class (LM APS 16 kN scaled to a 3 t vehicle); the Throttle engine (12.5 kN) the DPS class (45 kN) scaled. For a ~3.2 t take-off at 1.62 m/s^2 the weight is 5.2 kN: Basic alone gives T/W ~1.4 (yellow), Throttle ~2.4 (green).
- **Tanks**: tank mass is 5.5-6% of capacity (old values kept: 35 / 48 / 60 kg for 600 / 850 / 1100 kg, $6M / $8M / $10M).
- **Fuel need 0.9 x dry**: with Isp 300 s (ve 2.94 km/s) this is dv = ve ln(1.9) = **1.89 km/s**, the same order as the LM ascent stage (2.22 km/s). So the S1 fuel check means "enough propellant to get off the Moon and into orbit", with the return abstracted.
- **Solar / battery**: arrays ~13-17 W per kg (structure and mechanism included); batteries ~65 Wh/kg at system level. RTG 120 W / 45 kg (MMRTG class), so a solar wing beats it on W/kg and $, but the RTG never fails and keeps working in shadow (S4 will need this; see section 4.2).
- **Instruments**: ice-capable set NSS, NIRVSS, MSolo, Drill, Rover (as in the current DAs). Camera is not ice-capable. Science values 22 / 18 / 25 / 30 / 20 / 6 so that the green rule (>= 2 ice parts and science >= 50) needs three instruments, or two plus the drill or rover.

## 2. Formulas, verified in `MCConstructionManager.cpp`

Definitions (`Recalc`): `L` = lander group, `A` = ascent group. Dry mass `D = LanderBaseMassKg + sum(MassKg of every installed part)`. Take-off mass `M = D + FuelKg`. `FuelKg` is clamped to the installed tank capacity (`FuelMaxKg`). `FuelRequiredKg = 0.9 x D`. `TotalCostM = LanderBaseCostM + sum(CostM) + RocketCostM`. Launcher reliability multiplied by each part's `Reliability` (a field of 0 counts as 1.0).

A part is "live" if it is self-powered or its socket has `bGrid` (cable). Self-powered (`MCPartSocket.h`): fuel tank (`FuelCapKg > 0`), booster, structure part. Only live parts add power, storage, data, bandwidth, science, thrust, ice count. **Engines, lidar, antennas, instruments and power parts all need the cable**, so an engine without its cable adds no thrust (Thrust red) and counts as off-grid (Grid yellow). The examples assume every part is bolted and wired.

| # | Check | Red | Yellow | Green |
|---|---|---|---|---|
| 0 | Launcher | none chosen | - | chosen |
| 1 | Mass | payload <= 0 or `M > payload` | otherwise | `M <= 0.9 x payload` |
| 2 | Budget | `TotalCostM > 500` | otherwise | `<= 0.9 x 500 = 450` |
| 3 | Power | `Gen <= 0` or `Gen < Use` | otherwise | `Gen >= 1.1 x Use` and `Storage > 0` |
| 4 | Comms | `Bandwidth <= 0` or `Data > Bandwidth` | otherwise | `Bandwidth >= 1.1 x Data` |
| 5 | Fuel | no tank, or `FuelKg < 0.9 x D` | otherwise | `FuelKg >= 1.1 x 0.9 x D` |
| 6 | Thrust | no live engine and no live booster, or `T/W < 1.3` | otherwise | `T/W >= 2.0` and a live lidar |
| 7 | Ice | `IceCount = 0` | otherwise | `IceCount >= 2` and `Science >= 50` |
| 8 | Grid | - | any off-grid module | none |
| 9 | Structure | any unbolted part | - | none |

`T/W = ThrustN(ascent group: engines + live boosters) / (M x 1.62)`. A booster part counts once in the sums (its stats are for the **radial pair**, `RadialCount` 2 is only visual). Data, bandwidth, science, ice come from the lander group; power and storage are summed over both groups. `Gen` and `Use` are the sums of positive and of negative `PowerW`.

Existing inconsistency to be aware of: `Rockets` in the constructor says 1800 / 3000 / 5500 kg and $60M for Falcon 9, while the lever labels say 1200 / 2500 / 4000 kg and $70M. This proposal replaces both.

## 3. Part table (26 defs)

Mass in kg, cost in $M, power in W (+ generates, - consumes), storage in Wh, data and bandwidth in Mbps, thrust in N, fuel capacity in kg. Reliability is the multiplicative factor. "Heavy" and department are unchanged from the current assets. Full precision lives in the CSV.

### Lander group

| Part | Socket | Mass | Cost | Power | Storage | Data | BW | Sci | Rel | Ice | Role in the trade-offs |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SolarSmall | Power | 22 | 4 | +180 | | | | | 0.985 | | cheap, weak |
| SolarLarge | Power | 42 | 9 | +400 | | | | | 0.995 | | best W per kg and $ |
| RTG (heavy) | Power | 45 | 35 | +120 | | | | | 1.000 | | dear, never fails, shadow-proof |
| BatterySmall | Battery | 12 | 3 | | 800 | | | | 0.990 | | |
| BatteryLarge (heavy) | Battery | 32 | 8 | | 2400 | | | | 0.998 | | |
| AntennaLow | Comms | 5 | 3 | -10 | | | 3.0 | | 0.980 | | enough for 2 instruments |
| AntennaHigh | Comms | 22 | 14 | -45 | | | 10.0 | | 0.995 | | needed for rover, drill, camera |
| Lidar | Lidar | 12 | 18 | -50 | | | | | 0.990 | | gates green Thrust |
| NSS | Science | 9 | 16 | -20 | | 0.3 | | 22 | 0.990 | yes | cheap ice detector |
| NIRVSS | Science | 5 | 12 | -30 | | 2.0 | | 18 | 0.990 | yes | light, data heavy |
| MSolo | Science | 14 | 22 | -35 | | 0.6 | | 25 | 0.980 | yes | best science, dear |
| Camera | Science | 3 | 4 | -8 | | 5.0 | | 6 | 0.995 | no | data hog, no ice (trap) |
| Drill | Drill | 22 | 28 | -140 | | 0.2 | | 30 | 0.970 | yes | needs 2 solar wings |
| Rover | Rover | 80 | 45 | -110 | | 1.5 | | 20 | 0.980 | yes | heaviest lander part |
| MCP_ToolWrench / ToolCable | Tool | 0 | 0 | | | | | | 1 | | no stats |

### Ascent group

| Part | Socket | Mass | Cost | Thrust | Fuel cap | Rel | Notes |
|---|---|---|---|---|---|---|---|
| EngineBasic | Engine | 45 | 9 | 7500 | | 0.980 | T/W alone ~1.4 -> yellow |
| EngineThrottle | Engine | 90 | 30 | 12500 | | 0.997 | T/W alone ~2.4 -> green |
| TankSmall | Tank | 35 | 6 | | 600 | 0.998 | |
| TankMedium | Tank | 48 | 8 | | 850 | 0.998 | |
| TankLarge (heavy) | Tank | 60 | 10 | | 1100 | 0.998 | |
| BoosterSRBShort | Booster | 85 | 5 | 5000 | | 0.975 | per installed pair, includes solid propellant |
| BoosterSRB (heavy) | Booster | 150 | 9 | 11000 | | 0.960 | per installed pair |
| Decoupler | Structure | 12 | 3 | | | 1.000 | mass + cost only |
| Capsule | Structure | 46 | 20 | | | 1.000 | return capsule; S6 later |
| Fairing | Structure | 18 | 5 | | | 1.000 | mass + cost only |

### Launchers (`Rockets[]`)

| Launcher | Payload (kg, vs take-off mass) | Cost ($M) | Reliability | Comment |
|---|---|---|---|---|
| Falcon 9 | 3300 | 70 | 0.95 | fits a lean build, yellow mass |
| Vulcan | 4800 | 110 | 0.975 | comfortable for one drill or rover |
| Falcon Heavy | 8000 | 150 | 0.990 | carries everything but leaves little budget |

Reliability feeds S2's ascent anomaly roll (`AnomalyRoll > LaunchReliability`: +18 km course error and 5% fuel). A normal build has total reliability 0.84-0.93, i.e. an anomaly in 7-16% of runs.

### Why these numbers produce the intended play

- **Mass is the main wall.** Take-off mass includes fuel at about 1x the dry mass, so with the 1200 kg lander base a Falcon 9 build has only ~350 kg of free hardware. The rover (80 kg) or the drill alone moves a Falcon 9 build into yellow or red; Vulcan costs $40M more.
- **Power vs science.** Drill (-140 W) plus rover (-110 W) need both solar wings (800 W); one wing (400 W) carries three instruments, lidar and the high-gain antenna (-180 W). Each wing is 42 kg, so power competes with payload mass.
- **Thrust vs mass vs cost.** Throttle engine: 90 kg, $30M, T/W 2.4 alone. Basic + short booster: 130 kg, $14M, T/W 2.4, but reliability 0.98 x 0.975. The cheap route costs 40 kg and ~2% reliability.
- **Tier clarity.** Falcon 9 (small, cheapest, 0.95), Vulcan (middle), Falcon Heavy (large, expensive, 0.99).
- **Traps.** Camera has data 5.0 and zero ice value. Low antenna is cheap but fails the comms check once rover or camera are added. No battery means yellow power. No lidar means yellow thrust.

## 4. Worked examples (all by the manager's formulas; checked with a throwaway script)

Common: lander base 1200 kg / $100M. Every part is bolted and wired. Check order = 0 Launcher, 1 Mass, 2 Budget, 3 Power, 4 Comms, 5 Fuel, 6 Thrust, 7 Ice, 8 Grid, 9 Structure. G green, Y yellow, R red.

### E1: Good build on Falcon 9 (passes with a yellow, LAUNCH twice)

Lander: SolarLarge 42, BatterySmall 12, AntennaHigh 22, NSS 9, NIRVSS 5, MSolo 14, Lidar 12 = 116 kg -> 1316 kg.
Ascent: EngineBasic 45, BoosterSRBShort 85, TankLarge 60, TankMedium 48, Capsule 46 = 284 kg.
- Dry D = 1316 + 284 = **1600 kg**. Fuel loaded 1600 (cap 1950). Required 0.9 x 1600 = 1440; green at 1584 -> **G**.
- Take-off M = 3200 kg. Payload 3300; 0.9 x 3300 = 2970 < 3200 <= 3300 -> Mass **Y** (97%).
- Cost = 100 + lander parts (9+3+14+16+12+22+18 = 94) + ascent (9+5+10+8+20 = 52) + launcher 70 = **$316M** <= 450 -> **G**.
- Power: Gen 400; Use 45+20+30+35+50 = 180; 400 >= 198, storage 800 -> **G**.
- Comms: data 0.3+2.0+0.6 = 2.9; bandwidth 10 >= 3.19 -> **G**.
- Thrust: 7500 + 5000 = 12500 N; T/W = 12500 / (3200 x 1.62) = **2.41**, lidar live -> **G**.
- Ice: 3 ice parts, science 22+18+25 = 65 -> **G**. Grid G, Structure G, Launcher G.
- Reliability 0.95 x (0.995 x 0.99 x 0.995 x 0.99 x 0.99 x 0.98 x 0.99 x 0.98 x 0.975 x 0.998 x 0.998) = **0.843**.
- Result: `G Y G G G G G G G G`. First LAUNCH press shows "Warnings remain"; second launches. dv at Isp 300 s = 2.94 x ln(3200/1600) = 2.04 km/s.

### E2: Same payload, all green, on Vulcan

Same lander, ascent = EngineThrottle 90, TankLarge 60, TankMedium 48, Capsule 46 = 244 kg. D = 1560 kg, fuel 1560 (need 1404, green at 1544).
- M = 3120 vs Vulcan 4800 (65%) G. Cost = 100 + 94 + (30+10+8+20 = 68) + 110 = **$372M** G. T/W = 12500 / (3120 x 1.62) = **2.47** G. Reliability 0.975 x ... = 0.902.
- Result: `G G G G G G G G G G`. Cost of being comfortable: +$56M and +0.06 reliability compared with E1.

### E3: Cheap build that fails (Fuel red)

Parts: SolarSmall 22, AntennaLow 5, NSS 9, NIRVSS 5, EngineBasic 45, TankSmall 35. D = 1200 + 41 + 80 = 1321 kg. Fuel 600 (the only tank holds 600). Required 0.9 x 1321 = 1189 -> **600 < 1189: Fuel R**.
- Cost 100 + (4+3+16+12) + (9+6) + 70 = $220M G. Mass: M = 1921 (58%) G.
- Power: Gen 180, Use 10+20+30 = 60, storage 0 -> **Y** (no battery). Comms: data 2.3, bandwidth 3.0 >= 2.53 G.
- Thrust: 7500 / (1921 x 1.62) = 2.41 but no lidar -> **Y**. Ice: 2 parts, science 40 < 50 -> **Y**.
- Result: `G G G Y G R Y Y G G`; LAUNCH is aborted ("red checks remain"). The sneaky part: a tank cannot hold enough, because the lander base alone needs 1080 kg of fuel.

### E4a: Over-mass greedy build on Falcon 9 (Mass red); E4b the same on Falcon Heavy (Budget red)

Lander: SolarLarge x2 84, BatteryLarge 32, AntennaHigh 22, NSS 9, NIRVSS 5, MSolo 14, Camera 3, Drill 22, Rover 80, Lidar 12 = 283 -> 1483. Ascent: EngineThrottle 90, BoosterSRB 150, TankLarge x3 180, Capsule 46 = 466. D = **1949 kg**, fuel 3300 (full), M = **5249 kg**.
- E4a (Falcon 9, 3300): M = 5249 = 159% -> **Mass R**. Cost = 100 + 185 + 89 + 70 = $444M G (just under 450). Power: Gen 800, Use 438 G. Comms: data 0.3+2.0+0.6+5.0+0.2+1.5 = 9.6, bandwidth 10 < 10.56 -> **Y**. Fuel: need 1754, green 1929, have 3300 G. T/W = (12500+11000) / (5249 x 1.62) = 2.76 G. Ice: 5 parts, science 121 G.
  Result: `G R G G Y G G G G G`. The fix is to drop the rover and a tank, or to pay for Vulcan (4800 still < 5249: the full stack needs Falcon Heavy).
- E4b (Falcon Heavy, 8000): M = 66% G, but cost = 100 + 185 + 89 + 150 = **$524M -> Budget R**. Result: `G G R G Y G G G G G`. Reliability 0.99 x ... = 0.833.

### E5: No ice instrument (Ice red)

Parts: SolarLarge 42, BatterySmall 12, AntennaHigh 22, Camera 3, Lidar 12 | EngineThrottle 90, TankLarge 60, TankMedium 48, Capsule 46. D = 1200 + 91 + 244 = 1535; fuel 1530 (green at 1.1 x 1382 = 1520). Vulcan: M = 3065 (64%) G, cost $326M G, power Gen 400 / Use 103 G, comms data 5.0 vs 10 G, T/W = 12500 / (3065 x 1.62) = 2.52 G.
- Ice: `IceCount = 0` -> **R**, science 6. Result: `G G G G G G G R G G`. Everything else is fine, so this is a pure "Science picked the wrong part" failure (the accuracy log also scores the Camera install as bad for Science).

### Two more, for tuning

- **E6, weak engine / too much fuel** (E1 without the booster, F9): D = 1515 kg, M = 3115 kg, T/W = 7500 / (3115 x 1.62) = **1.49** -> Thrust Y. With EngineBasic and three full tanks (M ~ 4.9 t) T/W falls to 0.95 (R): thrust and mass fight each other.
- **E7, full science on Vulcan** (2 wings, large battery, NSS, NIRVSS, MSolo, drill, rover, lidar, high antenna, Throttle, two Large tanks, Capsule, fuel 1720): D = 1736, M = 3456 (72%), cost $461M -> Budget **Y**, T/W 2.23, science 115. Passes with one yellow.

## 5. Proposed answers to the five open guesses (proposals, not decisions)

1. **Fuel need = 0.9 x total dry mass.** Keep. It equals dv 1.89 km/s at Isp 300 s (LM ascent 2.22 km/s) and counts the lander base as dry mass, so a heavier lander needs more fuel. Optional: show "dv available = 2.94 x ln(M/D)" on the HUD, so players see a physical number (E1: 2.04 km/s). Expose 0.9 as a config float `FuelPerDry`.
2. **Power check scope.** Keep "all consumers vs all generators, ascent included" (worst case, matches the six-consumer surface design). Proposed addition (code change, optional): green also needs `StorageWh >= 2 x UseW` (two hours of load). Then a battery is a real choice (small 800 Wh covers 400 W; E7's 430 W needs the large one: 860 <= 2400) and a token 1 Wh battery no longer gives green. All examples above satisfy it. RTG stays as a premium reliability option; a future "shadow case" (RTG-only generation vs essential loads) is better added in S4 than here.
3. **Lidar gating green Thrust.** Keep as is. It models "blind landing without lidar" ([[Game Design]]), costs a real 12 kg / $18M / 50 W, and only affects green: a build without it can still launch with a yellow. Suggest renaming the check "Thrust and landing sensors" in the UI.
4. **Boosters-only.** Keep: boosters satisfy the Thrust check on their own (no engine needed), but the Fuel check still demands a tank holding >= 0.9 x dry, so boosters cannot replace the engine-and-tank pair. Booster mass includes the solid propellant (5 kN x ~30 s / 2.45 km/s = ~60 kg propellant + case = 85 kg), so they are cheap and fuel-free but heavy and 0.96-0.975 reliable.
5. **New part values** (TankMedium, two boosters, Decoupler, Capsule, Fairing): TankMedium 48 kg / $8M / 850 kg (the P1 value kept); boosters revised from 9000 N / 80 kg / $12M and 5000 N / 45 kg / $7M to 11000 N / 150 kg / $9M and 5000 N / 85 kg / $5M, so they are cheap thrust but not free mass; structure parts are mass and cost only (Capsule 46 kg / $20M is the return capsule that S6 will need).

## 6. S2 Launch & Transit numbers

Current values are the `UMCTransitConfig` defaults. Run length now = GO poll + 10 + 20 + 180 + 15 = about 225 s plus the poll, i.e. ~4 min. Target about 3.5 min.

| Item | Current | Proposed | Reason |
|---|---|---|---|
| GO poll | (players) | ~20 s expected | four taps |
| `CountdownSec` | 10 | 8 | |
| `AscentSec` | 20 | 15 | |
| `TransitSec` (coast) | 180 | 150 | |
| `ArrivalHoldSec` | 15 | 15 | report time |
| **Total** | ~4.0 min | **~3.5 min** (20+8+15+150+15 = 208 s) | |
| `WindowOpenSec` TCM-1..4 | 20, 65, 110, 150 | 15, 50, 85, 118 | 35 s gaps; the last closes at 130 s, before arrival |
| `WindowSec` | 15 | 12 | enough for 4-6 pulses |
| `CorridorKm` | 15 | 15 | initial error ±25 km per axis (mean ~19 km) needs 1-2 pulses to enter the corridor |
| `NudgeKm` | 6 | 6 | 6 km is 0.4 of the corridor; the sensor-fault kick (20-30 km) needs 4-5 pulses in the 12 s extra window |
| `InitialErrMaxKm`, `DriftMaxKm`, `AnomalyErrKm` | 25, 10, 18 | same | |
| `FaultTimeMin/Max` | 45-95 | 35-80 | the fault must be fixable before the coast ends |
| `AlarmMin/Max` | 12-140 | 10-120 | |
| `NudgeFuelKg` | 8 (x1..3 by progress) | **12** (12 kg early, 36 kg at the end) | ~15 m/s on a 3 t craft at Isp 300 s is ~15 kg; early burns are cheaper (a lesson) |
| `OffCorridorFuelKgPerKm` | 1.5 | 4 | an arrival 10 km outside costs 40 kg |
| `LandingReserveFrac` | 0.5 | **1.0** | reserve = `FuelRequiredKg` (the S1 requirement) |
| Weights course / instruments / equipment / health / fuel | 30/20/20/15/15 | **30/20/20/20/10** | role split: Pilot 30 + 10 fuel, Science 20, Equipment 20, Systems 20 |
| `HealthDrainPerAlarmSec` | 0.4 | 0.4 | |

**Relation to the S1 fuel margin.** S1 requires `FuelKg >= 0.9 x D`; green needs +10% (~140 kg for E1). S2 spends fuel on pulses. Estimated spend: a clean run uses ~3 early pulses (~40 kg) + 5 for the fault (~100 kg) + 2-3 for drift (~60 kg) = about 200 kg; a sloppy run 400+ kg. The fuel score is `WeightFuel x min(1, FuelLeft / Reserve)` with Reserve = 0.9 x D:
- E1-type loading (1600 kg, reserve 1440) after a clean run: left 1400 -> 9.7 of 10.
- A build at the red/yellow edge (1450 kg): left 1250 -> 8.7; after a sloppy run (400 kg) left 1050 -> 7.3.
- A fuller build (1950 kg, E2-type with big tanks): always 10.
So a bigger S1 margin lets S2 be careless, and a thin margin makes the Pilot's pulses count, although the score range is only ~3 points. If the team wants fuel to matter more, the formula could become `Weight x clamp(1 - (Reserve - Left) / (0.25 x Reserve))` (code change, optional).

**Expected S2 score of a competent team** ~ course 30 + instruments 20 + equipment 20 + health 15-20 + fuel 8-10 = **93-100**; a team that ignores the sensor-fault alarm loses course points (the kick leaves the true miss at 20-30 km, outside the corridor by 5-15 km -> 30 x (1 - (miss-15)/30), i.e. 15-25 of 30) and some health. Anomaly (7-16% of runs) adds a pulse or two.

Optional (not in code): a minimum interval of ~1 s between pulses so that 6 pulses in a window take real hand work.

## 7. Open points for the user

- Approve or change: the three launcher rows, the base 1200 kg / $100M, the fuel factor 0.9, the S2 timeline and weights.
- Decide whether the two small code changes (storage >= 2 h of use; fuel score formula) are wanted. All examples hold without them.
- Part counts per socket are taken from the level notes: Lander has 2 power sockets, 1 battery, 1 comms, 1 lidar, 3 science, 1 drill, 1 rover; Ascent has 1 engine, 3 tanks, 4 booster sockets, 2 structure (`bAutoSpawnSockets`). The examples respect that.
- Lidar, instrument and capsule masses are unverified guesses.
- Not committed. Import script: read `Vault/Balance Proposal data.csv`, set each `MCP_*` def's fields and the manager's `Rockets` array.
