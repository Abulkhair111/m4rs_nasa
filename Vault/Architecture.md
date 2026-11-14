# Architecture

UE **5.8** (`A:/epic games/UE_5.8`), **Blueprint-only** (Visual Studio isn't installed, so no C++). The UI language is Russian.
Content root: `/Game/MissionCrew/{Core,UI,Maps,Data,Art}`.

## Networking
- **LAN listen server** (one player hosts). No dedicated server, no internet backend.
- Host: `OpenLevel L_Lobby?listen`. Join: `open <IP>` (UDP 7777) and/or LAN session search through OnlineSubsystemNull (the other agent added `WBP_LanSessionRow`; plugins OnlineSubsystem/Null/Utils are enabled).
- Planned: authority on the server. Every player action goes **through the PlayerController with a Server RPC** (actors the client doesn't own can't receive Server RPCs).

## Flow
`L_MainMenu` → (Host/Join) → `L_Lobby` → `L_Mission` → Debrief → back to Design.

## Classes (existing)
| Class | Parent | Role |
|---|---|---|
| `BP_MC_GameInstance` | GameInstance | `Callsign`, `LastError`. HandleNetworkError/HandleTravelError set LastError (the engine returns to the default map itself). Project GameInstanceClass. *The other agent extends it with LAN sessions.* |
| `BP_MenuGameMode` | GameModeBase | No pawn, PC = BP_MenuPlayerController. Set as the GameMode override in L_MainMenu. |
| `BP_MenuPlayerController` | PlayerController | BeginPlay → if local: CreateWidget WBP_MainMenu → AddToViewport → InputMode UI Only → show cursor. |
| `WBP_MainMenu` | UserWidget | Callsign input, Host, LAN missions list (other agent), Quit, StatusText. Construct restores callsign + error from GI. |

## Planned (see ROADMAP §1)
`BP_MC_GameMode`, `BP_MC_GameState` (Phase RepNotify, timers, resources, faults, event log), `BP_MC_PlayerState` (Role), `BP_MC_PlayerController` (all Server RPCs), `BPFL_MC_Math`, `BP_FaultManager` (seeded RandomStream), `BP_LanderSim` (custom landing physics, g = 1.62).

## Project settings
GameDefaultMap = EditorStartupMap = `L_MainMenu`. GameInstanceClass = `BP_MC_GameInstance`.
