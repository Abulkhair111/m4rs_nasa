# Mission Crew — Vault Index

This is the project's knowledge base. **Read this first.** Then open only the notes your task needs. Don't scan `Content/` or the whole repo to orient yourself.

| Need | Read |
|---|---|
| What the game is: roles, phases, scoring | [[Game Design]] |
| Classes, maps, networking, game flow | [[Architecture]] |
| Where an asset is and what it does | [[Asset Catalog]] |
| How to edit the editor via unreal-mcp (gotchas!) | [[Unreal MCP Workflow]] |
| Why something is the way it is | [[Decisions]] |
| What's done and what's next | [[Status]] |
| S1 Design & Assembly (state, NASA models, graphs to-do) | [[S1 Design & Assembly - Handoff]] |
| S2 Launch & Transit (state, design, next steps) | [[S2 Launch & Transit - Handoff]] |
| 3D construction (assembly bay) design | Current rules: Ice Return S1 in `../CONSTRUCTION_PHASE_ICE_RETURN.md` §3. Original bay design: `construction_phase.md` (actually a .docx, so open it in Word). Implementation: [[Architecture]] → Construction phase |

Full plan with the 48h timeline: `../ROADMAP.md` (Russian). The original design PDFs are summarised in [[Game Design]].

## Rules for agents
- **Keep the vault current.** After changing assets or architecture, update [[Asset Catalog]] / [[Architecture]] / [[Status]] in the same session, and record new decisions in [[Decisions]].
- **Don't create 3D models.** Ask the user (see `../CLAUDE.md`).
- **Another agent may be editing the same editor.** Check [[Status]] → "Ownership" before touching shared Blueprints.
