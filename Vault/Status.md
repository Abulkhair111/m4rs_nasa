# Status (update at the end of every session)

_Last updated: 2026-11-14_

## Done
- Roadmap, design summary, vault.
- Main menu: 3D backdrop (Perseverance rover on textured regolith, stars), WBP_MainMenu with callsign, Host (`L_Lobby?listen`), Quit, error display.
- Project settings (default map, GameInstance), git + LFS.

## In progress / open
- **Craters: done** with real LOLA terrain (the user chose this option on 2026-11-14). The menu camera is on a 120 m crater rim. The craters at 5 m/px are moderate; for a dramatic scene, pick a rougher window with `Tools/LOLA/dem_scan.py` or use LDEM_875S near Shackleton.
- For `L_Mission`, plan to build a Landscape from the `.r16` (has collision and LODs), which is better for driving the rover.
- Git remote: `origin` = https://github.com/Abulkhair111/m4rs_nasa.git (main pushed 2026-11-14).
- LAN session browser in the main menu, built by **another agent**. Its state is unknown; it touched `BP_MC_GameInstance`, `WBP_MainMenu` and `WBP_LanSessionRow`.
- The other agent is building the lobby (`BP_MC_GameMode`, `BP_LobbyHUD` showed up in autosaves on 2026-11-14) and an IP-join row in the menu.

## Next
1. `L_Lobby`: role selection (4 unique roles), replicated PlayerState, ready flags, host Start.
2. Core mission classes (GameState phases, PlayerController RPCs), see ROADMAP §1, §4.

## Ownership (to avoid collisions)
| Area | Owner |
|---|---|
| Main menu scene (`L_MainMenu` actors, Art/) | Claude session "menu/art" |
| LAN sessions (`BP_MC_GameInstance` session logic, `WBP_LanSessionRow`, menu LAN list) | other agent |
