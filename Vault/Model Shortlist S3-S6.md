# Model shortlist S3-S6 (sourced 2026-11-15, NOT imported)
Paths under `SourceArt/WebModels/`. FBX: Z up, 1 m = 100 uu, origin bottom-centre. bbox in cm (X x Y x Z). Quality 1-5.
Poly Haven = CC0 (no credit needed). NASA = public domain, credit "NASA 3D Resources".

| Name | Use | Path | bbox cm | Licence | Q |
|---|---|---|---|---|---|
| moon_rock_01..07 | S3/S4 regolith rocks (small, scale up x3-10 in UE) | PolyHaven_Raw/moon_rock_0N/fbx/PH_moon_rock_0N.fbx | 21x11x8, 25x25x20, 11x9x6, 12x17x5, 7x8x7, 9x9x6, 10x7x6 | CC0 | 5 (scanned, PBR) |
| boulder_01 | S3/S4 large boulder | PolyHaven_Raw/boulder_01/fbx | 127x183x100 (66k tris) | CC0 | 4 |
| metal_toolbox | sample case / toolbox | PolyHaven_Raw/metal_toolbox/fbx | 40x29x35 | CC0 | 4 |
| plastic_crate_02, industrial_pastic_container | cargo boxes | PolyHaven_Raw/<id>/fbx | 51x41x25 / 48x63x42 | CC0 | 4 |
| propane_tank | sealed canister / cryo tank | PolyHaven_Raw/propane_tank/fbx | 34x34x55 | CC0 | 3 (earthly look) |
| portable_generator | power unit prop | PolyHaven_Raw/portable_generator/fbx | 82x56x58 | CC0 | 3 |
| projector_screen | deck screen prop | PolyHaven_Raw/projector_screen/fbx | 109x92x180 | CC0 | 2 |
| ocean_buoy, lifebuoy | Earth-return ocean props | PolyHaven_Raw/<id>/fbx | 107x95x266 / 78x16x84 | CC0 | 3 |
| Stardust capsule (closed) | re-entry capsule + panels (Orion-like substitute) | NASA3D_Stardust_Capsule/fbx | 294x501x117 | PD | 3 |
| ESAS Crew Module | Apollo-style capsule, low poly (650 tris); scale assumed cm->m, verify | NASA3D_ESAS_CrewModule/fbx | ~166x86x164 | PD | 2 |
| JSC Mission Control Room | control-room interior (11k tris, whole room, metres) | NASA3D_MissionControlRoom/fbx | 1451x2456x589 | PD | 3 |
| VAB | hangar/VAB, exterior shell at 15x21x14 m, scale up; check for interior | NASA3D_VAB/fbx | 1501x2134x1370 | PD | 2 |
| Hab Demo Unit | surface habitat; scale 0.01 assumed | NASA3D_HabDemoUnit/fbx | ~380x166x217 | PD | 3 |
| Tall Dish (dish) | antenna/dish; scale 0.01 assumed | NASA3D_TallDish/fbx | ~9x9x10 m | PD | 3 |

Downloaded raw only: CrewLockBag, RASSOR (2M tris, lunar excavator, needs decimation), Viking lander (units unverified), Crawler (280k tris), Apollo 11/14 landing-site STL (terrain heightfield, large), Poly Haven chemistry_set, industrial_microscope, circuit_board, vintage_radio_transceiver, old_military_crate, small_lpg_tank.

## Not found (free, CC0/CC-BY, no login)
- Orion capsule with parachutes: none. Existing NASA_Orion STL stays. Parachutes would need to be modelled by the team.
- Separate lander legs/footpads, rover-mounted drill, cryo cooler: none. Apollo LM (Maietta) keeps its legs.
- Recovery ship/helicopter: none free without login (Sketchfab/CGTrader gated). Ingenuity helicopter exists in NASA repo (Mars, not a recovery craft).
- Crater sets: only Apollo landing-site STL heightfields from NASA; Poly Haven has no crater geometry.
- Kenney site returned 403 to scripts; Quaternius/Poly Pizza/OpenGameArt items not re-verified this pass (earlier M3b rejected toy-style).

## Pass 2 (Poly Pizza, 2026-11-15, NOT imported)
All GLB without login, converted to FBX (Z up, origin bottom-centre, scale 1 => metres in file, x100 = cm). Dir `SourceArt/WebModels/<Name>/`. All low-poly stylised.
| Name | Use | bbox cm (X x Y x Z) | Licence | Q |
|---|---|---|---|---|
| PP_CommandPod (Poly by Google) | Apollo-style capsule | 420x420x377 (220 tris) | CC-BY 3.0 | 2 |
| PP_Mk12CommandPod (Poly by Google) | Orion/Mk-style capsule | 409x409x304 (478 tris) | CC-BY 3.0 | 2 |
| PP_LanderA / PP_LanderB (Kay Lousberg) | lander with legs (Space Base Bits) | 137x142x119 / 137x142x110 | CC0 | 3 |
| PP_Helicopter (kazuma) | recovery helicopter | 352x463x138 | CC0 | 2 |
| PP_Ship (Kenney) | ocean ship, scale down (~0.3) | 2850x8100x6550 | CC0 | 2 |
| PP_ContainerShip (Alex Safayan) | recovery ship, scale up ~x30 | 21x98x38 | CC-BY 3.0 | 2 |
| PP_CoolerBox (MilkAndBanana) | cold container, scale ~0.04 | 2123x1392x1646 | CC0 | 2 |
| PP_EnergyCanister (Nick Slough) | cryo canister | 30x29x54 | CC-BY 3.0 | 2 |
Dropped: Poly Google parachute/canopy (GLB had broken huge transforms, unusable).
Still missing: rover drill/sampling arm, real crater meshes, realistic Orion with chutes, Kenney/Quaternius/OGA/Sketchfab not retried (time).
