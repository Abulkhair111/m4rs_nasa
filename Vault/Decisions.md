# Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-11-14 | UE5 Windows build, not a browser game | UE5 can't target the browser. |
| 2026-11-14 | **3D is in** (greybox MCC + cockpit, Widget Component screens) | User decision. |
| 2026-11-14 | **LAN listen server only**, join by IP / LAN search | User decision. No dedicated servers. |
| 2026-11-14 | Blueprint-only | No Visual Studio on the dev machine. |
| 2026-11-14 | UI text in Russian | Matches the design docs and the team. |
| 2026-11-14 | Crash = −100 → score 0 (assumed) | Design said "Крушение: 100"; not yet confirmed. |
| 2026-11-14 | **Claude must not create 3D models; ask instead** | User rule (CLAUDE.md). The team supplies models. |
| 2026-11-14 | Moon ground = flat engine plane + CC0 regolith material | Low-poly sphere looked faceted and made the rover float. |
| 2026-11-14 | Rover Nanite disabled | Project lacks SM6. Alternative: enable SM6 in Project Settings → Platforms → Windows. |
| 2026-11-14 | Git + LFS, source art in `SourceArt/` | Binary assets; avoid accidental auto-import. |
