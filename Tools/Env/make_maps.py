"""Headless map builder: copies L_Transit into L_Orbit/L_Surface/L_Samples/L_Return, strips the S2-only actors,
places each stage's manager/view and sets the travel chain. Run with the editor closed:
UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<this file> -unattended -nosplash -nullrhi
Writes Saved/make_maps.log.txt. No geometry is created; only existing actors are kept/removed."""
import unreal, os, traceback

LOG = os.path.join(unreal.Paths.project_saved_dir(), "make_maps.log.txt")
lines = []
def log(m):
    lines.append(str(m)); unreal.log("MAKEMAPS " + str(m))

MAPS = "/Game/MissionCrew/Maps/"
SRC = MAPS + "L_Transit"
STRIP = ("MCStation", "MCTransitDisplay", "MCFlightView", "MCTransitManager")

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.EditorAssetLibrary
eal = unreal.EditorActorSubsystem

def setp(obj, name, val):
    try:
        obj.set_editor_property(name, val); log("  set %s.%s=%s" % (obj.get_name(), name, val)); return True
    except Exception as e:
        log("  FAILED %s.%s: %s" % (obj.get_name(), name, str(e).splitlines()[0][:90])); return False

def cls(name):
    c = getattr(unreal, name, None)
    if c is None: log("  MISSING class " + name)
    return c

def build(name, placements, props):
    dst = MAPS + name
    actors = unreal.get_editor_subsystem(eal).get_all_level_actors()
    removed = 0
    for a in actors:
        if any(a.get_class().get_name().startswith(s) for s in STRIP):
            unreal.get_editor_subsystem(eal).destroy_actor(a); removed += 1
    log("%s: removed %d S2 actors" % (name, removed))
    starts = [a for a in unreal.get_editor_subsystem(eal).get_all_level_actors() if a.get_class().get_name() == "PlayerStart"]
    log("%s: PlayerStarts=%d" % (name, len(starts)))
    ss = unreal.get_editor_subsystem(eal)
    for i, (cname, loc) in enumerate(placements):
        c = cls(cname)
        if c is None: continue
        a = ss.spawn_actor_from_class(c, unreal.Vector(*loc))
        if a is None: log("  spawn failed " + cname); continue
        log("  placed " + cname)
        for k, v in props.get(cname, {}).items():
            setp(a, k, v)
    les.save_current_level()
    log("saved " + dst)

try:
    which = os.environ.get("MC_MAP", "")
    if which == "L_Orbit":
        build("L_Orbit", [("MCOrbitManager", (0, 0, 0)), ("MCOrbitView", (0, 0, 0))],
              {"MCOrbitManager": {"free_roles": True, "debug_auto_test": False, "next_map_name": MAPS + "L_Surface"}})
    elif which == "L_Surface":
        build("L_Surface", [("MCSurfaceManager", (0, 0, 0))],
              {"MCSurfaceManager": {"free_roles": True, "next_map": MAPS + "L_Samples"}})
    elif which == "L_Samples":
        build("L_Samples", [("MCSamplesManager", (0, 0, 0))],
              {"MCSamplesManager": {"free_roles": True, "return_map_path": MAPS + "L_Return"}})
    elif which == "L_Return":
        build("L_Return", [("MCReturnManager", (0, 0, 0)), ("MCReturnView", (0, 0, 0))],
              {"MCReturnManager": {"free_roles": True}})
except Exception:
    log("EXCEPTION " + traceback.format_exc())

with open(LOG, "w") as f:
    f.write("\n".join(lines))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
