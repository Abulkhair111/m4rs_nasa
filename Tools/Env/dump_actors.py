import unreal
out = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    l = a.get_actor_location()
    out.append("%s | %s | %.0f,%.0f,%.0f" % (a.get_actor_label(), a.get_class().get_name(), l.x, l.y, l.z))
open(unreal.Paths.project_saved_dir() + "actors_dump.txt", "w").write("\n".join(out))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
