import unreal
mi = unreal.load_asset("/Game/MissionCrew/Art/Materials/Space/MI_OutsideGround")
unreal.log("MOONMAT parent=%s" % mi.get_editor_property("parent"))
unreal.log("MOONMAT scalars=%s" % [ (str(p.get_editor_property("parameter_info").get_editor_property("name")), p.get_editor_property("parameter_value")) for p in mi.get_editor_property("scalar_parameter_values")])
open(unreal.Paths.project_saved_dir()+"moonmat.txt","w").write(str([ (str(p.get_editor_property("parameter_info").get_editor_property("name")), p.get_editor_property("parameter_value")) for p in mi.get_editor_property("scalar_parameter_values")]) + "\n" + str(mi.get_editor_property("parent")))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
