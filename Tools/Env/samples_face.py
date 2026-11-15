import unreal, math
ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in ss.get_all_level_actors():
    if a.get_class().get_name() == "PlayerStart":
        l = a.get_actor_location()
        yaw = math.degrees(math.atan2(0 - l.y, 450 - l.x))
        a.set_actor_rotation(unreal.Rotator(0, yaw, 0), False); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("SAMPLESFACE %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
