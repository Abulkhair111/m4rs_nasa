# Status (update at the end of every session)

_Last updated: 2026-11-14_

## Done
- Roadmap, design summary, vault.
- Main menu: 3D backdrop (Perseverance rover on textured regolith, stars), WBP_MainMenu with callsign, Host (`L_Lobby?listen`), Quit, error display.
- Project settings (default map, GameInstance), git + LFS.

## In progress / open
- **Craters**: not done. Waiting for the user to choose: their own terrain model, a Landscape from a real LRO south-pole heightmap, or material-only craters (no geometry). Don't build terrain geometry without approval.
- The user will also supply a Moon model (may replace `MoonGround`).
- LAN session browser in the main menu, built by **another agent**. Its state is unknown; it touched `BP_MC_GameInstance`, `WBP_MainMenu` and `WBP_LanSessionRow`.
- Git remote not configured yet (no `gh` CLI). Ask the user for the remote URL.

## Next
1. `L_Lobby`: role selection (4 unique roles), replicated PlayerState, ready flags, host Start.
2. Core mission classes (GameState phases, PlayerController RPCs), see ROADMAP §1, §4.

## Ownership (to avoid collisions)
| Area | Owner |
|---|---|
| Main menu scene (`L_MainMenu` actors, Art/) | Claude session "menu/art" |
| LAN sessions (`BP_MC_GameInstance` session logic, `WBP_LanSessionRow`, menu LAN list) | other agent |
