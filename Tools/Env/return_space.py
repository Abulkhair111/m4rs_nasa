import unreal
ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem); n = 0
for a in ss.get_all_level_actors():
    if a.get_actor_label() in ("Deck_OutsideGround", "CapsuleProp", "RecoveryHeli", "Buoy", "Lifebuoy"):
        ss.destroy_actor(a); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("RETSPACE removed %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
