# Mission Crew — instructions for Claude

NASA Space Apps 2026 game, UE 5.8, Blueprint-only. Plan: `ROADMAP.md`.

## 3D models
- **Do not create 3D models yourself.** That includes meshes built from primitives (cubes, cylinders, spheres), procedural or modeled geometry, and generated assets. If a scene needs a model, **ask the user** for it.
- The team provides the models. The user is adding **rover** and **Moon** models.
- Current placeholders in `L_MainMenu`, to be replaced with the user's models:
  - `LanderModel`, an actor built from primitive components
  - `Moon`, a scaled `/Engine/BasicShapes/Sphere`
- Placing the user's models in levels, assigning materials, and setting up lighting, cameras and Blueprints is fine.
