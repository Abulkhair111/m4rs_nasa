# S1 Design & Assembly — Handoff (menu/art session, 2026-11-14)

Owner: Claude session "menu/art". Area: `/Game/MissionCrew/Construction/**`, `/Game/MissionCrew/Art/Meshes/NASA/**`, `Maps/L_AssemblyBay`, `Tools/NASA3D`, `SourceArt/NASA3D`.
Spec: `../CONSTRUCTION_PHASE_ICE_RETURN.md` §3 S1. How it works: [[Architecture]] → "Construction phase = Ice Return S1". Assets: [[Asset Catalog]]. Why: [[Decisions]].

## Done and committed
1. **The bay was remade as S1** (3D first-person bay kept). Roles by CrewRole number: 1 Power Engineer, 2 Construction Engineer, 3 Propulsion Engineer, 4 Science Specialist.
   - Fuel tanks (socket type 8). Fuel is capped by tank capacity.
   - Thrust is checked as T/W on the Moon.
   - **Wrench** (Construction Engineer) bolts modules. **Cable reel** (Power Engineer) puts them on the power grid.
   - Instruments are flagged ice-capable or not.
   - There are 10 checks with a wall board row each, a report, and a per-role accuracy log.
   - Verified by `bDebugAutoTest` on the `ConstructionManager` instance. Set it true, Play on `L_AssemblyBay`, then grep the log for `DEBUGTEST`. The run went install → bolt and wire through `ApplyTool` → fuel → READY ×4 → LAUNCH ×2, and launched, with no runtime errors.
2. **NASA 3D Resources models** (https://science.nasa.gov/3d-resources/, GitHub `nasa/NASA-3D-Resources`, "free and without copyright"):
   - The 17 GLBs are in `SourceArt/NASA3D`. The four that had animations (ApolloLM, InfraredCamera, Tether, Wrench) had their skins stripped by `Tools/NASA3D/strip_glb_skins.py`; the originals are in `SourceArt/NASA3D/original`.
   - They were imported by `Tools/NASA3D/import_nasa_parts.py` (commandlet: `UnrealEditor-Cmd.exe проект.uproject -run=pythonscript -script=Tools/NASA3D/import_nasa_parts.py`, optional env `MC_ONLY=Name1,Name2`). The meshes are in `/Game/MissionCrew/Art/Meshes/NASA/<Name>/<Name>`, with Nanite off. Some textures failed to import (non-power-of-two sizes), so a few materials have no texture.
   - **Assigned and saved in the `DA_Part_*` data assets**, with MeshScale computed so the largest side matches the old placeholder size and Color alpha 0 (keeps the NASA materials):

     | Part | NASA model |
     |---|---|
     | Solar 300 / 600 W | Satellite Kit wings 1 / 3 |
     | Battery 2 kWh / 6 kWh | Satellite Kit body 1 / ISS Orbital Replacement Unit |
     | Low-gain / high-gain antenna | Satellite Kit radio 1 / Voyager antenna |
     | R-4D / deep-throttle engine | Shuttle RCS / Shuttle main engine |
     | Lidar | LLCD laser terminal |
     | Tank 600 / 1100 kg | Shuttle External Tank (scaled) |
     | NIRVSS / NSS / MSolo / camera | Infrared Camera / TOMS / Cosmic Origins Spectrograph / WFPC |
     | Wrench / cable reel tools | Wrench / Tether |

   - No NASA model exists for: RTG, drill, Falcon 9 / Vulcan / Falcon Heavy. Those stay as placeholders. The rover keeps the user's Perseverance.
   - `ApolloLM` (643×643×501 cm) is imported but **not placed yet**.

## Unsaved in the open editor (not in git)
The "transit" agent was running PIE, which blocks saving. Save these from the editor when PIE stops (File → Save All, or `AssetTools.save_assets`):
- `BP_PartShelf.SpawnPart` and `BP_Part.Drop` were rewritten to place a part by its **real mesh bounds** (`StaticMesh|GetBounds` → `BreakBoxSphereBounds`; bottom = (BoxExtent.z − Origin.z) × MeshScale.z) instead of `MeshScale.z × 50`. The committed versions still use ×50, so the tiny-scaled NASA meshes will sit slightly into tables and the floor until this is saved. If the editor was closed without saving, redo it (the code is in the vault notes above and in [[Unreal MCP Workflow]]).
- `UI/WBP_ResourceGraphs` (new): only the widget tree exists. It has `TitleText`, `CriticalText`, and 9 rows of `Label#` + `Bar#` (ProgressBar 780×44) + `Val#` at y = 165 + 80·i, plus `FootText`. **No logic yet.**

## To do next (in order)
1. **Save the unsaved items above** and compile `BP_Part` and `BP_PartShelf`.
2. **Finish the resource graphs** (the user asked for "graphs of data: fuel, science, TWR, etc.").
   - In `WBP_ResourceGraphs`, add a function `SetBar(Bar ProgressBar, Val TextBlock, Fill float, State int, Text string)`. It uses `Class|ProgressBar|SetPercent` (clamp 0–1), `Class|ProgressBar|SetFillColorandOpacity` (green 0.3/1/0.45, yellow 1/0.8/0.2, red 1/0.25/0.2, the same formula as `WBP_WallBoard.SetRow`) and `Widget|SetText(Text)`.
   - EventTick → `Refresh`: get `BP_ConstructionManager` and fill the rows. Colour each bar by `GetCheck(i)`.

     | Row | Fill | Check |
     |---|---|---|
     | 0 Mass | TotalMassKg / max(RocketPayloadKg, 1) | 1 |
     | 1 Budget | TotalCostM / BudgetM | 2 |
     | 2 Power load | PowerUseW / max(PowerGenW, 0.001) | 3 |
     | 3 Fuel | FuelKg / max(FuelRequiredKg × 1.1, 1) | 5 |
     | 4 T/W | TWR / 2.0 | 6 |
     | 5 Science | ScienceValue / 100 | 7 |
     | 6 Data | DataMbps / max(BandwidthMbps, 0.001) | 4 |
     | 7 Wired | 1 − OffGridCount / installed | 8 |
     | 8 Bolted | 1 − UnboltedCount / installed | 9 |

     "installed" = the number of `BP_PartSocket` with a valid `Installed` (loop in the widget, or add an `InstalledCount` var to `Recalc`).
   - `CriticalText` names the highest "pressure" (with "> 100% fails the check"). Pressure is mass/payload, cost/budget, use/gen, data/link, (req × 1.1)/fuel, or 2.0/TWR, capped at 999%.
   - Show it in the bay: duplicate `BP_WallBoard` → `BP_GraphBoard`, set its WidgetComponent class to `WBP_ResourceGraphs` (keep the transparent background over the dark frame), and place it on a free wall facing the lander. Optionally add a key to toggle the same widget on the HUD.
3. **Place the Apollo LM** as the lander body in `BP_AssemblyStand`. Hide the placeholder body components, then scale and offset the LM so the 13 `Sock_*` sockets sit on its surface. Check with `CaptureViewport` (it needs an `annotations` arg); the sockets are listed in [[Asset Catalog]] → `L_AssemblyBay`.
4. Check the NASA parts in PIE: scale, orientation (solar wings are flat in XY), and pivot offsets on sockets. If a part looks wrong, tune `MeshScale` in its `DA_Part_*`. Run `bDebugAutoTest` again.
5. Manual step (can't be done via MCP): in `BP_BayCharacter`, set the `ServerInteract` and `ServerRelease` custom events to **Run on Server**, Reliable.
6. Update [[Status]], [[Asset Catalog]] and [[Architecture]] once the graphs and the LM are done.

## Warnings
- **Another agent ("transit") uses the same editor and MCP server.** Its tool results sometimes come back on your calls (errors mentioning `BP_TransitManager`, `ServerAction`, `EventTick`). If you get an error that isn't yours, re-read your target to see whether your call applied. Don't stop its PIE without asking.
- MCP lessons from this work (function rewrite = clear nodes and rewrite, no `exec` in the sandbox so generate DSL JSON with engine Python, pure-node re-evaluation) are in [[Unreal MCP Workflow]].
