# Status (update at the end of every session)

_Last updated: 2026-11-14_

## Done
- Roadmap, design summary, vault.
- Main menu: 3D backdrop (Perseverance rover on textured regolith, stars), WBP_MainMenu with callsign, Host (`L_Lobby?listen`), Quit, error display.
- Project settings (default map, GameInstance), git + LFS.
- **All player-facing text translated to English** (2026-11-14): designer texts and graph string literals in WBP_MainMenu, WBP_LanSessionRow, WBP_Lobby (incl. role cards), WBP_RoleCard, BP_MC_GameInstance error messages. Verified: no Cyrillic UI strings left in saved assets.
- **Mission-start video event** (2026-11-14): `BP_MissionVideoEvent` in `L_Mission` → `PlayMissionVideo` → fullscreen `WBP_MissionVideo` with SKIP → `OnMissionVideoFinished`. Tested in PIE: with the video file missing, the overlay closes itself and the game continues.
- **Main menu redesign** (2026-11-14, menu/art session): mission-control look. Navy glass panel (LeftShade, 800 wide) + cyan `EdgeLine`, `StatusBar` text at the top, cyan primary button (Host), dark Join, red-hover Quit, `MissionCard` (objective) bottom right, footer v0.2. Only styling and new decorative widgets; LAN logic untouched. `BP_MenuPlayerController` Tick: slow camera drift (±60 cm, yaw ±2.5°).
- **3D construction phase** (2026-11-14, menu/art session), design: `Vault/construction_phase.md` (it's a .docx with a .md name). Everything is in `/Game/MissionCrew/Construction` plus the test map `Maps/L_AssemblyBay`. Fully playable solo in PIE on `L_AssemblyBay`: rocket levers, 16 parts on 3 shelves, sockets with ghost preview, carry/snap/remove, two-person heavy carry, fuel pump/drain, mass scale, power ring, 2 wall boards, READY buttons, preflight, LAUNCH (red blocks, yellow needs a second press). Verified by a server-side self-test (`bDebugAutoTest` on the manager instance): rocket → install parts → fuel → READY ×4 → launch blocked with the right reason. **Keyboard/mouse input and the trace-based E interaction have not been tried by a human yet.** All models are **temporary engine-shape placeholders** (user approved), swap them in the `DA_Part_*` Mesh field and the BP components.

- **S1 Design & Assembly** (2026-11-14, menu/art session): the construction bay was remade to `CONSTRUCTION_PHASE_ICE_RETURN.md` S1. Changes:
  - Roles are Power, Construction, Propulsion and Science.
  - New mechanics: fuel tanks (fuel capped by tank capacity), engine thrust with a T/W check, a wrench to bolt modules, a cable reel to wire them to the power grid, and ice-capable instruments.
  - There are 10 checks, and the board, report and per-role accuracy log are updated to match.
  - Fixed old bugs: installed and dropped parts had collision off, so the E trace couldn't hit them. `Drop` read `Carrier1` after clearing it.

  Verified by `bDebugAutoTest` in PIE: the report before bolting lists unbolted, off-grid, no-thrust and no-ice problems. After wrench and cable, the yellow fuel margin is accepted on the second LAUNCH and `ConstructionFinished` fires. No runtime errors. **Still not tried by a human with keyboard and mouse.**

## In progress / open
- **S1 extras** (2026-11-14, menu/art): 17 NASA 3D Resources models were imported and assigned to the parts. Bounds-based spawn/drop and the `WBP_ResourceGraphs` dashboard are **unsaved in the editor / not finished**. Full handoff and next steps: [[S1 Design & Assembly - Handoff]].
- **S2 Launch & Transit** (2026-11-14, "transit" session): built in `/Game/MissionCrew/Transit/**` with the test map `Maps/L_Transit`. It compiles but is **not tested at runtime yet**. Full handoff: [[S2 Launch & Transit - Handoff]].
- S1 numbers (masses, costs, thrust, tank sizes, T/W limits 1.3/2.0, fuel 0.9 × dry +10%) are placeholders. The brief wants balanced numbers from the user.
- The lobby still shows the old role names (MCC Engineer, Flight Director, Pilot, Scientist) for CrewRole 1–4. In S1 they mean Power, Construction, Propulsion and Science. The lobby belongs to the other agent.
- S2–S6, briefings, `DT_*` tables and random events from `CONSTRUCTION_PHASE_ICE_RETURN.md` have not been started.
- **Craters: done** with real LOLA terrain (the user chose this option on 2026-11-14). The menu camera is on a 120 m crater rim. The craters at 5 m/px are moderate; for a dramatic scene, pick a rougher window with `Tools/LOLA/dem_scan.py` or use LDEM_875S near Shackleton.
- For `L_Mission`, plan to build a Landscape from the `.r16` (has collision and LODs), which is better for driving the rover.
- Git remote: `origin` = https://github.com/Abulkhair111/m4rs_nasa.git (main pushed 2026-11-14).
- LAN session browser in the main menu, built by **another agent**. Its state is unknown; it touched `BP_MC_GameInstance`, `WBP_MainMenu` and `WBP_LanSessionRow`.
- The other agent is building the lobby (`BP_MC_GameMode`, `BP_LobbyHUD` showed up in autosaves on 2026-11-14) and an IP-join row in the menu.

## Next
- **Construction phase, manual steps (can't be done via MCP):** in `BP_BayCharacter`, select custom events `ServerInteract` and `ServerRelease` → Details → Replicates: **Run on Server**, Reliable. Without this, clients (not the host) can't interact.
- **Hook construction into the mission:** after the briefing video, the server calls `BP_MC_GameMode.EnterConstructionPhase()`. It needs a `BP_ConstructionManager` in the loaded world, so add `L_AssemblyBay` to `L_Mission` as an Always Loaded sublevel (offset it away from the mission area), or ServerTravel to `L_AssemblyBay`. On LAUNCH the manager calls `BP_MC_GameMode.ConstructionFinished()` (stub that prints) and fires `OnConstructionComplete`. The finished loadout stays in the manager (sockets → parts) for the flight phase.
- Swap placeholder meshes for real models when the team sends them (lander body, 16 parts, 3 rockets, bay interior).
0. **User to supply the intro video** → `Content/Movies/MissionIntro.mp4` (H.264). Then test real playback and audio in PIE. Hook gameplay start to `OnMissionVideoFinished`.
1. `L_Lobby`: role selection (4 unique roles), replicated PlayerState, ready flags, host Start.
2. Core mission classes (GameState phases, PlayerController RPCs), see ROADMAP §1, §4.

## Ownership (to avoid collisions)
| Area | Owner |
|---|---|
| Main menu scene (`L_MainMenu` actors, Art/) | Claude session "menu/art" |
| LAN sessions (`BP_MC_GameInstance` session logic, `WBP_LanSessionRow`, menu LAN list) | other agent |
| Construction phase (`/Game/MissionCrew/Construction/**`, `L_AssemblyBay`), `WBP_MainMenu` styling, `BP_MenuPlayerController` | Claude session "menu/art" |
| S2 Launch & Transit (`/Game/MissionCrew/Transit/**`, `L_Transit`) | Claude session "transit" |
| `BP_MC_GameMode`: only `EnterConstructionPhase` + `ConstructionFinished` added by menu/art; the rest is the other agent's | shared |
