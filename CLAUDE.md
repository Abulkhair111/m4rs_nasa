# Mission Crew — instructions for Claude

NASA Space Apps 2026 game, UE 5.8, editor driven via unreal-mcp. **Logic is moving to C++ (user decision 2026-11-14)**: new gameplay code goes in `Source/`, Blueprints stay for assets, widgets' layout and the existing lobby/menu. Visual Studio 2022 (MSVC 14.44) is installed. Build only with the editor closed.

## Start here
- **Read `Vault/00 Index.md` first**, then only the vault notes your task needs. Don't scan `Content/`, the logs, or the whole repo to orient yourself, and don't re-read the design PDFs. `Vault/Game Design.md` summarises them.
- Editing the editor via MCP? Read `Vault/Unreal MCP Workflow.md` first. The editor is in Russian, which breaks the Blueprint DSL.
- **At the end of a session, update the vault**: `Status.md` always; `Asset Catalog.md`, `Architecture.md` and `Decisions.md` when they change.

## 3D models
- **Do not create 3D models yourself.** That includes meshes built from primitives, procedural or modeled geometry, generated terrain, and craters made of geometry. If a scene needs a model, **ask the user**.
- The team provides the models. The rover (Perseverance) is in. The user may supply a Moon model.
- Placing the user's models in levels, assigning materials and textures, and setting up lighting, cameras and Blueprints is fine.

## Collaboration
- Another agent may be working in the same editor. Check `Vault/Status.md` → Ownership before editing shared Blueprints.
- Commit or push only when the user asks.
