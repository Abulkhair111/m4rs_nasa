"""Strip the L_Transit leftovers from a stage map (MC_MAP=L_Orbit|L_Surface|L_Samples|L_Return). Only removes/moves existing actors."""
import unreal, os
m = os.environ.get("MC_MAP", "")
ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
COMMON = ("St_", "Deck_HoloPedestal", "Deck_HoloRing", "HoloSpot", "Deck_LaunchPad", "Deck_Rocket_SLS", "Deck_Gantry", "Deck_PadLight")
HALL = ("Deck_Floor", "Deck_Wall", "Deck_Ceiling", "Deck_WinTrim", "Deck_HallLight")
rm = list(COMMON)
if m in ("L_Orbit", "L_Surface", "L_Return"): rm += list(HALL)
if m == "L_Orbit": rm.append("Deck_OutsideGround")
n = 0
for a in ss.get_all_level_actors():
    lab = a.get_actor_label()
    if any(lab.startswith(p) for p in rm):
        ss.destroy_actor(a); n += 1
    elif lab == "Deck_OutsideGround" and m in ("L_Surface", "L_Return"):
        origin, ext = a.get_actor_bounds(False)
        s = a.get_actor_scale3d()
        f = 32000.0 / max(1.0, 2 * max(ext.x, ext.y))
        a.set_actor_location(unreal.Vector(0, 0, -12), False, False)
        a.set_actor_scale3d(unreal.Vector(s.x * f, s.y * f, s.z))
        unreal.log("CLEANUP ground ext=%.0f,%.0f f=%.2f" % (ext.x, ext.y, f))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("CLEANUP %s removed %d" % (m, n))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
