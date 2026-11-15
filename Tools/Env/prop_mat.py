import unreal
mat = unreal.load_asset("/Game/MissionCrew/Art/Materials/Rocket/MI_Rocket_White")
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() in ("CapsuleProp", "RecoveryHeli", "Buoy", "Lifebuoy"):
        c = a.static_mesh_component
        for i in range(max(1, c.get_num_materials())): c.set_material(i, mat)
        n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("PROPMAT set on %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
