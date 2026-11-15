import unreal
eas = unreal.EditorAssetLibrary
mat = None
for p in ("/Game/MissionCrew/Art/Materials/Space/MI_Moon", "/Game/MissionCrew/Art/Meshes/Space/MI_Moon"):
    if eas.does_asset_exist(p): mat = unreal.load_asset(p); unreal.log("ORBITMAT using " + p); break
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name() == "MCOrbitView" and mat:
        a.set_editor_property("moon_material_asset", mat); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("ORBITMAT set on %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
