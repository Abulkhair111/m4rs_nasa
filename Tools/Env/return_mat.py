import unreal
eas = unreal.EditorAssetLibrary
mat = None
for p in eas.list_assets("/Game/MissionCrew", recursive=True, include_folder=False):
    n = p.split("/")[-1].split(".")[0]
    if n in ("MI_Rocket_White", "MI_Rocket_Core", "MI_Rocket_Body"):
        mat = eas.load_asset(p); unreal.log("RETMAT using " + p); break
if mat is None:
    for p in eas.list_assets("/Game/MissionCrew", recursive=True, include_folder=False):
        if p.split("/")[-1].startswith("MI_Rocket_"):
            mat = eas.load_asset(p); unreal.log("RETMAT fallback " + p); break
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name() == "MCReturnView" and mat:
        c = a.get_editor_property("capsule_mesh")
        for i in range(max(1, c.get_num_materials())): c.set_material(i, mat)
        n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("RETMAT set on %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
