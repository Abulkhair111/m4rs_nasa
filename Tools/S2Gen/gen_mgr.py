# Generates DSL for BP_TransitManager (Ice Return S2 Launch & Transit).
import json

NL = "\n"

def q(s):
    return '"' + s.replace('"', "'") + '"'

def cat(*parts):
    """Nested Utilities|String|Append."""
    parts = [p for p in parts if p is not None]
    if len(parts) == 1:
        return parts[0]
    expr = parts[-1]
    for p in reversed(parts[:-1]):
        expr = f"(Utilities|String|Append {p} {expr})"
    return expr

def g(v):            # member getter
    return f"(Variables|Default|Get{v})"

def s(v, val):       # member setter (value first)
    return f"(Variables|Default|Set{v} {val})"

def at(arr, i):
    return f"(Utilities|Array|Get(acopy) {g(arr)} {i})"

def put(arr, i, val):
    return f"(Utilities|Array|SetArrayElem {g(arr)} {i} {val} true)"

def add(arr, val):
    return f"(Utilities|Array|Add {g(arr)} {val})"

def clear(arr):
    return f"(Utilities|Array|Clear {g(arr)})"

def ln(arr):
    return f"(Utilities|Array|Length {g(arr)})"

def fmt(x):          # integer text
    return f"(Utilities|String|ToString(Integer) (Math|Float|Round {x}))"

def f1(x):           # one decimal
    return f"(Utilities|String|ToString(Float) (Math|Float|SafeDivide (Math|Float|Round (* {x} 10.0)) 10.0))"

def istr(x):
    return f"(Utilities|String|ToString(Integer) {x})"

def rnd(a, b):
    return f"(Math|Random|RandomFloatinRangefromStream {g('Stream')} {a} {b})"

def rndi(a, b):
    return f"(Math|Random|RandomIntegerinRangefromStream {g('Stream')} {a} {b})"

def absf(x):
    return f"(Math|Float|Absolute(Float) {x})"

def say(x):
    return f"(CallFunction|Say :Text {x})"

def score(role, good):
    return f"(CallFunction|Score :Role {role} :Good {good})"

G = {}

# ---------------------------------------------------------------- helpers
G["Say"] = f"""(fn Say (Text)
  {s('Message', 'Text')}
  {s('MessageTime', '(Utilities|Time|GetGameTimeinSeconds)')})"""

G["Score"] = f"""(fn Score (Role Good)
  (if (and (and (>= Role 1) (<= Role 4)) (>= {ln('RoleGood')} 5))
    (if Good
      {put('RoleGood', 'Role', '(+ ' + at('RoleGood', 'Role') + ' 1)')}
      (else
        {put('RoleBad', 'Role', '(+ ' + at('RoleBad', 'Role') + ' 1)')}))))"""

G["Valid"] = """(fn Valid (Obj)
  (Utilities|IsValid Obj
    (:"Is Valid"
      (return true))
    (:"Is Not Valid"
      (return false))))"""

G["RoleOf"] = """(fn RoleOf (Who)
  (bind pw (Utilities|Casting|CastToPawn Who)
    (:then
      (bind ps (Utilities|Casting|CastToBP_MC_PlayerState (Class|Pawn|GetPlayerState pw))
        (:then
          (return (Class|BPMCPlayerState|GetCrewRole ps)))
        (:CastFailed
          (return 0))))
    (:CastFailed
      (return 0))))"""

G["RoleName"] = """(fn RoleName (R)
  (switch int (- R 1)
    (:0 (return "SYSTEMS OPERATOR"))
    (:1 (return "EQUIPMENT TECHNICIAN"))
    (:2 (return "PILOT"))
    (:3 (return "SCIENCE OPERATOR"))
    (:Default (return "CREW"))))"""

G["SwName"] = """(fn SwName (I)
  (switch int I
    (:0 (return "HEATERS"))
    (:1 (return "NAV SENSOR"))
    (:2 (return "TRANSPONDER"))
    (:3 (return "WHEEL DESAT"))
    (:4 (return "INSTRUMENT RAIL"))
    (:5 (return "MECHANISM RAIL"))
    (:Default (return "?"))))"""

G["SwState"] = f"""(fn SwState (I)
  (bind v {at('Switches', 'I')})
  (if (or (== I 1) (== I 2))
    (return (Utilities|String|SelectString "B (backup)" "A (prime)" (== v 1)))
    (else
      (return (Utilities|String|SelectString "ON" "OFF" (== v 1))))))"""

G["StepName"] = """(fn StepName (DevType Step)
  (bind k (+ (* DevType 3) Step))
  (switch int k
    (:0 (return "Release wheel launch locks"))
    (:1 (return "Spin drive motors"))
    (:2 (return "Re-engage wheel locks"))
    (:3 (return "Unlatch drill stow clamp"))
    (:4 (return "Run auger spin test"))
    (:5 (return "Latch drill stow clamp"))
    (:6 (return "Read leg position sensors"))
    (:7 (return "Pyro circuit continuity test"))
    (:8 (return "Safe the pyro circuits"))
    (:9 (return "Open lidar lens cover"))
    (:10 (return "Laser rangefinder self-test"))
    (:11 (return "Close lidar lens cover"))
    (:Default (return "?"))))"""

G["DevName"] = """(fn DevName (DevType)
  (switch int DevType
    (:0 (return "ROVER"))
    (:1 (return "DRILL"))
    (:2 (return "LANDING LEGS"))
    (:3 (return "LANDING LIDAR"))
    (:Default (return "?"))))"""

# ---------------------------------------------------------------- loadout
SOCK = "/Game/MissionCrew/Construction/Blueprints/BP_PartSocket.BP_PartSocket_C"
G["LoadLoadout"] = f"""(fn LoadLoadout ()
  {clear('InstrNames')}
  {clear('DevTypes')}
  {s('LoadoutFromBay', 'false')}
  {s('FuelKg', '600.0')}
  {s('FuelReserveKg', '400.0')}
  (bind cm {g('ConsMgr')})
  (bind okcm (CallFunction|Valid :Obj cm))
  (if okcm
    (if (Class|BPConstructionManager|GetLaunched cm)
      {s('LoadoutFromBay', 'true')}
      {s('FuelKg', '(Class|BPConstructionManager|GetFuelKg cm)')}
      {s('FuelReserveKg', '(* (Class|BPConstructionManager|GetFuelRequiredKg cm) 0.8)')}
      {s('TotalMassKg', '(Class|BPConstructionManager|GetTotalMassKg cm)')}
      (bind rr (Class|BPConstructionManager|GetRocketReliability cm))
      {s('LaunchReliability', '(Math|Float|Clamp(Float) (Math|Float|SelectFloat (/ rr 100.0) rr (> rr 1.0)) 0.5 1.0)')}))
  (if (CallFunction|GetLoadoutFromBay)
    (bind socks (Actor|GetAllActorsOfClass "{SOCK}"))
    (for sk socks
      (bind sock (Utilities|Casting|CastToBP_PartSocket sk)
        (:then
          (bind part (Class|BPPartSocket|GetInstalled sock))
          (bind okp (CallFunction|Valid :Obj part))
          (if okp
            (bind st (Class|BPPartSocket|GetSocketType sock))
            (if (and (== st 3) (< {ln('InstrNames')} 3))
              {add('InstrNames', '(Class|PDAPartDef|GetDisplayName (Class|BPPart|GetDef part))')})
            (if (== st 5)
              {add('DevTypes', '0')})
            (if (== st 4)
              {add('DevTypes', '1')})
            (if (== st 7)
              {add('DevTypes', '3')})))
        (:CastFailed))))
  (if (== {ln('InstrNames')} 0)
    {add('InstrNames', q('NSS neutron spectrometer'))}
    {add('InstrNames', q('NIRVSS near-infrared spectrometer'))}
    {add('InstrNames', q('MSolo mass spectrometer'))})
  (if (not (CallFunction|GetLoadoutFromBay))
    {add('DevTypes', '0')}
    {add('DevTypes', '1')})
  {add('DevTypes', '2')})"""
# NOTE: CallFunction|GetLoadoutFromBay is replaced below with the variable getter.
G["LoadLoadout"] = G["LoadLoadout"].replace("(CallFunction|GetLoadoutFromBay)", g("LoadoutFromBay"))

# ---------------------------------------------------------------- init (all random draws, fixed order)
G["InitData"] = f"""(fn InitData ()
  (bind so {g('SeedOverride')})
  (bind sd (Math|Integer|SelectInt so (Math|Random|RandomIntegerinRange 1000 9999) (> so 0)))
  {s('Seed', 'sd')}
  {s('Stream', '(Math|Random|MakeRandomStream sd)')}
  {clear('GoFlags')}
  {clear('RoleGood')}
  {clear('RoleBad')}
  (for i (range 5)
    {add('GoFlags', 'false')}
    {add('RoleGood', '0')}
    {add('RoleBad', '0')})
  {clear('Switches')}
  {add('Switches', '1')}
  (for i (range 5)
    {add('Switches', '0')})
  {clear('WarnText')}
  {add('WarnText', q('BATTERY TEMP LOW -14 C: heater breaker tripped'))}
  {add('WarnText', q('TRANSPONDER A: no carrier lock with the Deep Space Network'))}
  {add('WarnText', q('REACTION WHEEL MOMENTUM 92%: desaturate the wheels'))}
  {add('WarnText', q('STAR TRACKER A disagrees with the IMU (sensor fault)'))}
  {clear('WarnSwitch')}
  {add('WarnSwitch', '0')}
  {add('WarnSwitch', '2')}
  {add('WarnSwitch', '3')}
  {add('WarnSwitch', '1')}
  {clear('WarnValue')}
  {clear('WarnActive')}
  {clear('WarnTriggered')}
  {clear('WarnAge')}
  {clear('WarnTimes')}
  (for i (range 4)
    {add('WarnValue', '1')}
    {add('WarnActive', 'false')}
    {add('WarnTriggered', 'false')}
    {add('WarnAge', '0.0')})
  {s('FaultTime', rnd('45.0', '95.0'))}
  {add('WarnTimes', rnd('15.0', '45.0'))}
  {add('WarnTimes', rnd('60.0', '100.0'))}
  {add('WarnTimes', rnd('105.0', '145.0'))}
  {add('WarnTimes', g('FaultTime'))}
  {s('KickX', '(* ' + rnd('30.0', '50.0') + ' (Math|Float|SelectFloat 1.0 -1.0 (Math|Random|RandomBoolfromStream ' + g('Stream') + ')))')}
  {s('KickY', rnd('-20.0', '20.0'))}
  {clear('WindowTimes')}
  {add('WindowTimes', '20.0')}
  {add('WindowTimes', '65.0')}
  {add('WindowTimes', '110.0')}
  {add('WindowTimes', '150.0')}
  {clear('DriftXs')}
  {clear('DriftYs')}
  (for i (range 5)
    {add('DriftXs', rnd('-0.5', '0.5'))}
    {add('DriftYs', rnd('-0.5', '0.5'))})
  {s('ErrX', rnd('-30.0', '30.0'))}
  {s('ErrY', rnd('-30.0', '30.0'))}
  {s('Anomaly', '(> ' + rnd('0.0', '1.0') + ' ' + g('LaunchReliability') + ')')}
  {clear('CalTarget')}
  {clear('CalKnob')}
  {clear('CalDone')}
  (for i (range 4)
    {add('CalTarget', rnd('15.0', '85.0'))})
  (for i (range {ln('InstrNames')})
    {add('CalKnob', '50.0')}
    {add('CalDone', 'false')})
  {clear('Perm')}
  (for i (range 4)
    {add('Perm', rndi('0', '5'))})
  {clear('DevNames')}
  {clear('DevSteps')}
  {clear('ButtonStep')}
  {clear('DevProgress')}
  {clear('DevDone')}
  (for d (range {ln('DevTypes')})
    (bind dt {at('DevTypes', 'd')})
    (bind dn (CallFunction|DevName :DevType dt))
    {add('DevNames', 'dn')}
    (bind sa (CallFunction|StepName :DevType dt :Step 0))
    (bind sb (CallFunction|StepName :DevType dt :Step 1))
    (bind sc (CallFunction|StepName :DevType dt :Step 2))
    {add('DevSteps', 'sa')}
    {add('DevSteps', 'sb')}
    {add('DevSteps', 'sc')}
    {add('DevProgress', '0')}
    {add('DevDone', 'false')}
    (bind p {at('Perm', '(Math|Integer|%(Integer) d 4)')})
    (switch int p
      (:0 {add('ButtonStep', '2')} {add('ButtonStep', '0')} {add('ButtonStep', '1')})
      (:1 {add('ButtonStep', '1')} {add('ButtonStep', '2')} {add('ButtonStep', '0')})
      (:2 {add('ButtonStep', '2')} {add('ButtonStep', '1')} {add('ButtonStep', '0')})
      (:3 {add('ButtonStep', '0')} {add('ButtonStep', '2')} {add('ButtonStep', '1')})
      (:4 {add('ButtonStep', '1')} {add('ButtonStep', '0')} {add('ButtonStep', '2')})
      (:Default {add('ButtonStep', '0')} {add('ButtonStep', '1')} {add('ButtonStep', '2')})))
  {clear('PanelText')}
  (for i (range 5)
    {add('PanelText', '""')})
  {clear('SwitchLabels')}
  (for i (range 6)
    {add('SwitchLabels', '""')})
  {clear('EqLabels')}
  (for i (range 3)
    {add('EqLabels', '""')})
  {s('CurInstr', '0')}
  {s('CurDev', '0')}
  {s('DevTestTime', '0.0')}
  {s('NextWindow', '0')}
  {s('WindowOpen', 'false')}
  {s('SensorFault', 'false')}
  {s('FaultTriggered', 'false')}
  {s('WarnResolved', '0')}
  {s('NudgeCount', '0')}
  {s('FuelUsedKg', '0.0')}
  {s('Health', '100.0')}
  {s('Done', 'false')}
  {s('Report', '""')}
  {s('DriftX', at('DriftXs', '0'))}
  {s('DriftY', at('DriftYs', '0'))})"""

PAWN = "/Game/MissionCrew/Transit/Blueprints/BP_TransitPawn.BP_TransitPawn_C"
G["StartPhase"] = f"""(fn StartPhase ()
  (if (not {g('Started')})
    {s('Started', 'true')}
    (CallFunction|LoadLoadout)
    (CallFunction|InitData)
    {s('FuelStartKg', g('FuelKg'))}
    {s('Stage', '0')}
    {s('StageTime', '0.0')}
    {s('TransitTime', '0.0')}
    {s('Active', 'true')}
    (bind xf (Transformation|GetActorTransform))
    (bind pcs (Actor|GetAllActorsOfClass "/Script/Engine.PlayerController"))
    (for pc pcs
      (bind pc2 (Utilities|Casting|CastToPlayerController pc)
        (:then
          (bind np (Game|SpawnActorfromClass :Class "{PAWN}" :SpawnTransform xf :CollisionHandlingOverride "AlwaysSpawn"))
          (bind np2 (Utilities|Casting|CastToPawn np)
            (:then
              (Pawn|Possess pc2 np2))
            (:CastFailed)))
        (:CastFailed)))
    {say(q('S2 LAUNCH & TRANSIT. Every station: check your panel, then press GO FOR LAUNCH.'))}
    (CallFunction|BuildUi)))"""

G["CheckLaunchGo"] = f"""(fn CheckLaunchGo ()
  (if (and (and {at('GoFlags', '1')} {at('GoFlags', '2')}) (and {at('GoFlags', '3')} {at('GoFlags', '4')}))
    {s('Stage', '1')}
    {s('StageTime', '0.0')}
    {say(q('All stations GO. Launch countdown started.'))}))"""

G["DoGo"] = f"""(fn DoGo (Role)
  (if (and (== {g('Stage')} 0) (and (>= Role 1) (<= Role 4)))
    (if (not {at('GoFlags', 'Role')})
      {put('GoFlags', 'Role', 'true')}
      {score('Role', 'true')}
      (bind rn (CallFunction|RoleName :R Role))
      {say(cat('rn', q(' is GO for launch.')))}
      (CallFunction|CheckLaunchGo))))"""

G["HandleAction"] = f"""(fn HandleAction (Code Who)
  (bind role (/ Code 100))
  (bind act (Math|Integer|%(Integer) Code 100))
  (if {g('Active')}
    (bind mine (CallFunction|RoleOf :Who Who))
    (if (or {g('FreeRoles')} (== mine role))
      (if (== act 90)
        (CallFunction|DoGo :Role role)
        (else
          (if (== role 1)
            (CallFunction|DoSystems :A act))
          (if (== role 2)
            (CallFunction|DoEquip :A act))
          (if (== role 3)
            (CallFunction|DoPilot :A act))
          (if (== role 4)
            (CallFunction|DoScience :A act))))
      (CallFunction|BuildUi)
      (else
        {say(q('That control belongs to another station.'))}))))"""

# ---------------------------------------------------------------- systems
G["DoSystems"] = f"""(fn DoSystems (A)
  (if (and (>= A 0) (<= A 5))
    {put('Switches', 'A', '(- 1 ' + at('Switches', 'A') + ')')}
    (bind nv {at('Switches', 'A')})
    {s('Tmp', 'false')}
    (for i (range 4)
      (if (and (and {at('WarnActive', 'i')} (== {at('WarnSwitch', 'i')} A)) (== {at('WarnValue', 'i')} nv))
        {put('WarnActive', 'i', 'false')}
        {s('Tmp', 'true')}
        {s('WarnResolved', '(+ ' + g('WarnResolved') + ' 1)')}
        {say(cat(q('RESOLVED: '), at('WarnText', 'i')))}
        (if (== i 3)
          {s('SensorFault', 'false')}
          (CallFunction|OpenWindow :Name "TCM-FAULT")
          {say(q('Star tracker B online: navigation shows the TRUE course now. Pilot: extra correction window TCM-FAULT is open.'))})))
    (bind sn (CallFunction|SwName :I A))
    (bind ss (CallFunction|SwState :I A))
    (if {g('Tmp')}
      {score('1', 'true')}
      (elif (and (>= A 4) (== nv 1))
        {score('1', 'true')}
        {say(cat('sn', q(' powered ON.')))}
        (else
          {score('1', 'false')}
          {say(cat('sn', q(' -> '), 'ss', q('. No alarm needed that switch.')))})))))"""

# ---------------------------------------------------------------- equipment
G["DoEquip"] = f"""(fn DoEquip (A)
  (bind c {g('CurDev')})
  (if (!= {g('Stage')} 3)
    {say(q('Equipment checks start after the TLI burn (coast phase).'))}
    (elif (>= c {ln('DevNames')})
      {say(q('All equipment already tested.'))}
      (elif (> {g('DevTestTime')} 0.0)
        {say(q('Test running, stand by...'))}
        (elif (== {at('Switches', '5')} 0)
          {say(q('MECHANISM RAIL is OFF. Ask the Systems Operator to power it.'))}
          (else
            (bind st {at('ButtonStep', '(+ (* c 3) A)')})
            (bind pr {at('DevProgress', 'c')})
            (bind nm {at('DevNames', 'c')})
            (if (== st pr)
              {put('DevProgress', 'c', '(+ pr 1)')}
              {score('2', 'true')}
              (if (== pr 2)
                {s('DevTestTime', '4.0')}
                {say(cat('nm', q(': procedure complete, functional test running...')))}
                (else
                  {say(cat(q('Step OK: '), at('DevSteps', '(+ (* c 3) st)')))}))
              (else
                {put('DevProgress', 'c', '0')}
                {score('2', 'false')}
                {say(cat(q('PROCEDURE VIOLATION on '), 'nm', q(': step out of order. Test reset, start again at step 1.')))}))))))))"""

G["FinishDevTest"] = f"""(fn FinishDevTest ()
  (bind c {g('CurDev')})
  (if (< c {ln('DevNames')})
    {put('DevDone', 'c', 'true')}
    {say(cat(at('DevNames', 'c'), q(' TEST PASSED.')))}
    {s('CurDev', '(+ c 1)')}))"""

# ---------------------------------------------------------------- pilot
G["BurnCost"] = f"""(fn BurnCost ()
  (return (* {g('NudgeFuelKg')} (+ 1.0 (* 2.0 (Math|Float|SafeDivide {g('TransitTime')} {g('TransitSec')}))))))"""

G["DoPilot"] = f"""(fn DoPilot (A)
  (if (!= {g('Stage')} 3)
    {say(q('Thrusters are safed until the coast phase.'))}
    (elif (not {g('WindowOpen')})
      {say(q('No burn window open. Wait for the next TCM window.'))}
      (else
        (bind cost (CallFunction|BurnCost))
        (if (< {g('FuelKg')} cost)
          {say(q('Not enough fuel for a correction pulse!'))}
          (else
            (bind nk {g('NudgeKm')})
            {s('ErrX', '(+ ' + g('ErrX') + ' (Math|Float|SelectFloat (- 0.0 nk) (Math|Float|SelectFloat nk 0.0 (== A 1)) (== A 0)))')}
            {s('ErrY', '(+ ' + g('ErrY') + ' (Math|Float|SelectFloat nk (Math|Float|SelectFloat (- 0.0 nk) 0.0 (== A 3)) (== A 2)))')}
            {s('FuelKg', '(- ' + g('FuelKg') + ' cost)')}
            {s('FuelUsedKg', '(+ ' + g('FuelUsedKg') + ' cost)')}
            {s('NudgeCount', '(+ ' + g('NudgeCount') + ' 1)')}
            (CallFunction|UpdateDisp)))))))"""

G["UpdateDisp"] = f"""(fn UpdateDisp ()
  (bind sf {g('SensorFault')})
  {s('DispX', '(+ ' + g('ErrX') + ' (Math|Float|SelectFloat ' + g('BiasX') + ' 0.0 sf))')}
  {s('DispY', '(+ ' + g('ErrY') + ' (Math|Float|SelectFloat ' + g('BiasY') + ' 0.0 sf))')})"""

G["OpenWindow"] = f"""(fn OpenWindow (Name)
  {s('WindowOpen', 'true')}
  {s('WindowName', 'Name')}
  {s('WindowTimeLeft', g('WindowSec'))}
  {say(cat('Name', q(' burn window OPEN. Pilot: pulse the thrusters to centre the predicted miss.')))})"""

G["CloseWindow"] = f"""(fn CloseWindow ()
  {s('WindowOpen', 'false')}
  (bind c {g('CorridorKm')})
  (bind ok (and (<= {absf(g('ErrX'))} c) (<= {absf(g('ErrY'))} c)))
  (bind okd (and (<= {absf(g('DispX'))} c) (<= {absf(g('DispY'))} c)))
  {score('3', 'ok')}
  {say(cat(g('WindowName'), '(Utilities|String|SelectString " closed. Navigation shows the course INSIDE the corridor." " closed. Navigation shows the course OUTSIDE the corridor!" okd)'))}
  (bind seg (Math|Integer|Min(Integer) {g('NextWindow')} 4))
  {s('DriftX', at('DriftXs', 'seg'))}
  {s('DriftY', at('DriftYs', 'seg'))})"""

# ---------------------------------------------------------------- science
G["DoScience"] = f"""(fn DoScience (A)
  (bind c {g('CurInstr')})
  (if (!= {g('Stage')} 3)
    {say(q('Instruments are stowed until the coast phase.'))}
    (elif (>= c {ln('InstrNames')})
      {say(q('All instruments already calibrated.'))}
      (elif (== {at('Switches', '4')} 0)
        {say(q('INSTRUMENT RAIL is OFF. Ask the Systems Operator to power it.'))}
        (else
          (bind k {at('CalKnob', 'c')})
          (bind nm {at('InstrNames', 'c')})
          (switch int A
            (:0 {put('CalKnob', 'c', '(Math|Float|Max(Float) 0.0 (- k 5.0))')})
            (:1 {put('CalKnob', 'c', '(Math|Float|Max(Float) 0.0 (- k 1.0))')})
            (:2 {put('CalKnob', 'c', '(Math|Float|Min(Float) 100.0 (+ k 1.0))')})
            (:3 {put('CalKnob', 'c', '(Math|Float|Min(Float) 100.0 (+ k 5.0))')})
            (:4
              (bind off {absf('(- k ' + at('CalTarget', 'c') + ')')})
              (if (<= off 3.0)
                {put('CalDone', 'c', 'true')}
                {score('4', 'true')}
                {say(cat('nm', q(' CALIBRATED (offset '), f1('off'), q(').')))}
                {s('CurInstr', '(+ c 1)')}
                (else
                  {score('4', 'false')}
                  {say(cat('nm', q(': OUT OF TOLERANCE. The reading is off target, adjust and lock again.')))})))
            (:Default)))))))"""

# ---------------------------------------------------------------- warnings / flight loop
G["TriggerWarning"] = f"""(fn TriggerWarning (I)
  {put('WarnTriggered', 'I', 'true')}
  {put('WarnActive', 'I', 'true')}
  {put('WarnAge', 'I', '0.0')}
  (bind sw {at('WarnSwitch', 'I')})
  (bind v {at('WarnValue', 'I')})
  (if (== {at('Switches', 'sw')} v)
    {put('Switches', 'sw', '(- 1 v)')})
  {say(cat(q('MASTER ALARM: '), at('WarnText', 'I')))})"""

G["TransitTick"] = f"""(fn TransitTick (Dt)
  {s('TransitTime', '(+ ' + g('TransitTime') + ' Dt)')}
  (bind tt {g('TransitTime')})
  {s('ErrX', '(+ ' + g('ErrX') + ' (* ' + g('DriftX') + ' Dt))')}
  {s('ErrY', '(+ ' + g('ErrY') + ' (* ' + g('DriftY') + ' Dt))')}
  (if (and (not {g('FaultTriggered')}) (>= tt {g('FaultTime')}))
    {s('FaultTriggered', 'true')}
    {s('SensorFault', 'true')}
    {s('ErrX', '(+ ' + g('ErrX') + ' ' + g('KickX') + ')')}
    {s('ErrY', '(+ ' + g('ErrY') + ' ' + g('KickY') + ')')}
    {s('BiasX', '(- 0.0 ' + g('KickX') + ')')}
    {s('BiasY', '(- 0.0 ' + g('KickY') + ')')}
    (CallFunction|TriggerWarning :I 3))
  (CallFunction|UpdateDisp)
  (for i (range 3)
    (if (and (not {at('WarnTriggered', 'i')}) (>= tt {at('WarnTimes', 'i')}))
      (CallFunction|TriggerWarning :I i)))
  (for i (range 4)
    (if {at('WarnActive', 'i')}
      {put('WarnAge', 'i', '(+ ' + at('WarnAge', 'i') + ' Dt)')}
      {s('Health', '(Math|Float|Max(Float) 0.0 (- ' + g('Health') + ' (* 0.4 Dt)))')}))
  (if {g('WindowOpen')}
    {s('WindowTimeLeft', '(- ' + g('WindowTimeLeft') + ' Dt)')}
    (if (<= {g('WindowTimeLeft')} 0.0)
      (CallFunction|CloseWindow))
    (else
      (bind nw {g('NextWindow')})
      (if (< nw 4)
        (if (>= tt {at('WindowTimes', 'nw')})
          {s('NextWindow', '(+ nw 1)')}
          (CallFunction|OpenWindow :Name {cat(q('TCM-'), istr('(+ nw 1)'))})))))
  (if (> {g('DevTestTime')} 0.0)
    {s('DevTestTime', '(- ' + g('DevTestTime') + ' Dt)')}
    (if (<= {g('DevTestTime')} 0.0)
      (CallFunction|FinishDevTest)))
  (bind ci {g('CurInstr')})
  (if (< ci {ln('InstrNames')})
    {s('CalReading', '(Math|Float|Clamp(Float) (+ (+ 50.0 (- ' + at('CalKnob', 'ci') + ' ' + at('CalTarget', 'ci') + ')) (* 2.5 (Math|Trig|Sin(Radians) (* tt 1.7)))) 0.0 100.0)')})
  (if (>= tt {g('TransitSec')})
    (CallFunction|Arrive)))"""

G["ServerTick"] = f"""(fn ServerTick (Dt)
  {s('StageTime', '(+ ' + g('StageTime') + ' Dt)')}
  (bind st {g('Stage')})
  (if (== st 1)
      (if (>= {g('StageTime')} {g('CountdownSec')})
        {s('Stage', '2')}
        {s('StageTime', '0.0')}
        {say(q('LIFTOFF! Ascent in progress.'))}))
  (if (== st 2)
      (if (>= {g('StageTime')} {g('AscentSec')})
        {s('Stage', '3')}
        {s('StageTime', '0.0')}
        {s('TransitTime', '0.0')}
        (if {g('Anomaly')}
          {s('ErrX', '(+ ' + g('ErrX') + ' 40.0)')}
          {s('FuelKg', '(* ' + g('FuelKg') + ' 0.95)')}
          {say(q('ASCENT ANOMALY: the upper stage underperformed. Bigger course error, 5% fuel lost.'))}
          (else
            {say(q('TLI burn complete, coasting to the Moon. Systems: power the instrument and mechanism rails.'))}))))
  (if (== st 3)
      (CallFunction|TransitTick :Dt Dt))
  {s('UiAccum', '(+ ' + g('UiAccum') + ' Dt)')}
  (if (>= {g('UiAccum')} 0.2)
    {s('UiAccum', '0.0')}
    (CallFunction|BuildUi)))"""

G["Arrive"] = f"""(fn Arrive ()
  {s('Stage', '4')}
  {s('WindowOpen', 'false')}
  (bind c {g('CorridorKm')})
  (bind miss (Math|Float|Sqrt (+ (* {g('ErrX')} {g('ErrX')}) (* {g('ErrY')} {g('ErrY')}))))
  (bind onc (and (<= {absf(g('ErrX'))} c) (<= {absf(g('ErrY'))} c)))
  (if onc
    {s('CourseResult', cat(q('ON COURSE: arrived inside the corridor (miss '), fmt('miss'), q(' km).')))}
    (else
      (bind extra (* (Math|Float|Max(Float) 0.0 (- miss c)) 1.5))
      {s('FuelKg', '(- ' + g('FuelKg') + ' extra)')}
      {s('FuelUsedKg', '(+ ' + g('FuelUsedKg') + ' extra)')}
      {s('CourseResult', cat(q('OFF COURSE: missed the corridor by '), fmt('(- miss c)'), q(' km. Emergency orbit-insertion burn used '), fmt('extra'), q(' kg of fuel.')))}))
  {s('TmpI', '0')}
  (for i (range {ln('CalDone')})
    (if {at('CalDone', 'i')}
      {s('TmpI', '(+ ' + g('TmpI') + ' 1)')}))
  {s('TmpJ', '0')}
  (for i (range {ln('DevDone')})
    (if {at('DevDone', 'i')}
      {s('TmpJ', '(+ ' + g('TmpJ') + ' 1)')}))
  {s('TmpK', '0')}
  (for i (range 4)
    (if {at('WarnActive', 'i')}
      {s('TmpK', '(+ ' + g('TmpK') + ' 1)')}))
  (bind ni (Math|Integer|Max(Integer) 1 {ln('CalDone')}))
  (bind nd (Math|Integer|Max(Integer) 1 {ln('DevDone')}))
  (bind fuelok (>= {g('FuelKg')} {g('FuelReserveKg')}))
  (bind pc (Math|Float|SelectFloat 30.0 (Math|Float|Max(Float) 0.0 (- 30.0 (- miss c))) onc))
  (bind pi (* 20.0 (Math|Float|SafeDivide {g('TmpI')} ni)))
  (bind pd (* 20.0 (Math|Float|SafeDivide {g('TmpJ')} nd)))
  (bind ph (* 15.0 (/ {g('Health')} 100.0)))
  (bind pf (Math|Float|SelectFloat 15.0 0.0 fuelok))
  (bind total (Math|Float|Round (+ (+ (+ pc pi) (+ pd ph)) pf)))
  {s('TransitScore', 'total')}
  (bind acc (CallFunction|AccText))
  {s('Report', cat(q('S2 LAUNCH & TRANSIT REPORT   score ' ), istr('total'), q('/100' + NL),
      g('CourseResult'), q(NL + 'Fuel: used '), fmt(g('FuelUsedKg')), q(' kg, left '), fmt(g('FuelKg')), q(' kg, reserve needed for landing '), fmt(g('FuelReserveKg')),
      '(Utilities|String|SelectString " kg (OK)." " kg: NOT ENOUGH, landing at risk!" fuelok)',
      q(NL + 'Instruments calibrated: '), istr(g('TmpI')), q('/'), istr(ln('CalDone')),
      q('    Equipment tested: '), istr(g('TmpJ')), q('/'), istr(ln('DevDone')),
      q(NL + 'Alarms resolved: '), istr(g('WarnResolved')), q(', still open: '), istr(g('TmpK')), q(', spacecraft health '), fmt(g('Health')), q('%'),
      q(NL + 'Course pulses: '), istr(g('NudgeCount')), q('   Seed: '), istr(g('Seed')),
      q(NL + 'Carried into S3/S4: uncalibrated instruments give weaker ice readings; untested equipment may fail on the surface.'),
      'acc'))}
  {s('Done', 'true')}
  {say(q('LUNAR ARRIVAL. Transit report on the board.'))}
  (CallFunction|BuildUi)
  (Development|PrintString {g('Report')})
  (Default|CallOnTransitComplete))"""

def accpart(r, name):
    return [q(f'  {name} '), istr(at('RoleGood', str(r))), q('/'), istr('(+ ' + at('RoleGood', str(r)) + ' ' + at('RoleBad', str(r)) + ')')]

G["AccText"] = f"""(fn AccText ()
  (return {cat(q(NL + 'ACCURACY (good/actions):'), *accpart(1,'Systems'), *accpart(2,'Equipment'), *accpart(3,'Pilot'), *accpart(4,'Science'))}))"""

# ---------------------------------------------------------------- UI strings
G["StageLine"] = f"""(fn StageLine ()
  (bind st {g('Stage')})
  (bind t {g('StageTime')})
  (switch int st
    (:0 (return "PRE-LAUNCH: GO / NO-GO POLL"))
    (:1 (return {cat(q('COUNTDOWN  T-'), fmt('(- ' + g('CountdownSec') + ' t)'))}))
    (:2 (return {cat(q('ASCENT  T+'), fmt('t'), q('  '), '(Utilities|String|SelectString "MAX-Q" (Utilities|String|SelectString "STAGE SEPARATION" "TRANS-LUNAR INJECTION BURN" (< t 12.0)) (< t 6.0))')}))
    (:3 (return {cat(q('COAST TO THE MOON  '), fmt('(* 384400.0 (- 1.0 (Math|Float|SafeDivide ' + g('TransitTime') + ' ' + g('TransitSec') + ')))'), q(' km to go'))}))
    (:4 (return "LUNAR ARRIVAL"))
    (:Default (return ""))))"""

def chk(cond):
    return f'(Utilities|String|SelectString "[x] " "[ ] " {cond})'

G["BuildUi"] = f"""(fn BuildUi ()
  (bind sl (CallFunction|StageLine))
  {s('TopText', cat(q('S2 LAUNCH & TRANSIT   |   '), 'sl', q('   |   Fuel '), fmt(g('FuelKg')), q(' kg (landing reserve '), fmt(g('FuelReserveKg')), q(')   |   Health '), fmt(g('Health')), q('%   |   Seed '), istr(g('Seed'))))}
  {s('TmpI', '0')}
  (for i (range {ln('CalDone')})
    (if {at('CalDone', 'i')}
      {s('TmpI', '(+ ' + g('TmpI') + ' 1)')}))
  {s('TmpJ', '0')}
  (for i (range {ln('DevDone')})
    (if {at('DevDone', 'i')}
      {s('TmpJ', '(+ ' + g('TmpJ') + ' 1)')}))
  {s('TmpK', '0')}
  {s('TmpS', '""')}
  (for i (range 4)
    (if {at('WarnActive', 'i')}
      {s('TmpK', '(+ ' + g('TmpK') + ' 1)')}
      {s('TmpS', cat(g('TmpS'), q('(!) '), at('WarnText', 'i'), q('   ['), fmt(at('WarnAge', 'i')), q(' s]' + NL)))}))
  (bind c {g('CorridorKm')})
  (bind onc (and (<= {absf(g('DispX'))} c) (<= {absf(g('DispY'))} c)))
  (bind miss (Math|Float|Sqrt (+ (* {g('DispX')} {g('DispX')}) (* {g('DispY')} {g('DispY')}))))
  (bind nw {g('NextWindow')})
  (bind wl (Utilities|String|SelectString {cat(g('WindowName'), q(' OPEN  '), fmt(g('WindowTimeLeft')), q(' s left'))} (Utilities|String|SelectString {cat(q('next TCM in '), fmt('(- ' + at('WindowTimes', '(Math|Integer|Min(Integer) nw 3)') + ' ' + g('TransitTime') + ')'), q(' s'))} "no more windows" (< nw 4)) {g('WindowOpen')}))
  (bind ni {ln('CalDone')})
  (bind nd {ln('DevDone')})
  (if {g('Done')}
    {s('BoardText', g('Report'))}
    (else
      {s('BoardText', cat(q('MISSION BOARD  (S2 done criteria)' + NL),
          chk('onc'), q('On course (navigation): miss '), fmt('miss'), q(' km, corridor +/-'), fmt('c'), q(' km' + NL),
          chk('(== ' + g('TmpI') + ' ni)'), q('Instruments calibrated '), istr(g('TmpI')), q('/'), istr('ni'), q(NL),
          chk('(== ' + g('TmpJ') + ' nd)'), q('Equipment tested '), istr(g('TmpJ')), q('/'), istr('nd'), q(NL),
          chk('(== ' + g('TmpK') + ' 0)'), q('No open alarms ('), istr(g('TmpK')), q(' open)' + NL + NL),
          q('Burn window: '), 'wl', q(NL + 'GO poll:  SYS '), chk(at('GoFlags', '1')), q(' EQP '), chk(at('GoFlags', '2')), q(' PIL '), chk(at('GoFlags', '3')), q(' SCI '), chk(at('GoFlags', '4')),
          q(NL + 'Loadout: '), '(Utilities|String|SelectString "from the assembly bay" "default test lander" ' + g('LoadoutFromBay') + ')'))}))
  (for i (range 6)
    (bind sn (CallFunction|SwName :I i))
    (bind ss (CallFunction|SwState :I i))
    {put('SwitchLabels', 'i', cat('sn', q(': '), 'ss'))})
  {put('PanelText', '1', cat(q('ALARMS' + NL), '(Utilities|String|SelectString ' + g('TmpS') + ' "No active alarms." (> ' + g('TmpK') + ' 0))',
      q(NL + NL + 'Each alarm has ONE correct switch. Flipping a switch that fixes nothing counts against you.' + NL + 'After launch the INSTRUMENT and MECHANISM rails are OFF: power them when Science and Equipment ask.')))}
  (bind cd {g('CurDev')})
  (if (< cd nd)
    (bind pr {at('DevProgress', 'cd')})
    {put('PanelText', '2', cat(q('TESTING: '), at('DevNames', 'cd'), q('  ('), istr('(+ cd 1)'), q('/'), istr('nd'), q(')' + NL + 'Procedure (do the steps in this order):' + NL + '  1. '), at('DevSteps', '(* cd 3)'),
        q(NL + '  2. '), at('DevSteps', '(+ (* cd 3) 1)'), q(NL + '  3. '), at('DevSteps', '(+ (* cd 3) 2)'),
        q(NL + 'Steps done: '), istr('pr'), q('/3'),
        '(Utilities|String|SelectString "   TEST RUNNING..." "" (> ' + g('DevTestTime') + ' 0.0))',
        '(Utilities|String|SelectString "" "' + NL + 'MECHANISM RAIL is OFF: ask the Systems Operator." (== ' + at('Switches', '5') + ' 1))'))}
    (for b (range 3)
      {put('EqLabels', 'b', at('DevSteps', '(+ (* cd 3) ' + at('ButtonStep', '(+ (* cd 3) b)') + ')'))})
    (else
      {put('PanelText', '2', q('All equipment tested. Rover, drill and landing systems are ready for arrival.'))}
      (for b (range 3)
        {put('EqLabels', 'b', '"-"')})))
  (bind cost (CallFunction|BurnCost))
  {put('PanelText', '3', cat(q('PREDICTED MISS AT THE MOON (navigation sensor)' + NL + 'X '), f1(g('DispX')), q(' km    Y '), f1(g('DispY')), q(' km    corridor +/- '), fmt('c'), q(' km' + NL + 'Burn window: '), 'wl',
      q(NL + 'Each pulse moves the miss '), fmt(g('NudgeKm')), q(' km and costs '), f1('cost'), q(' kg of fuel.' + NL + 'Early corrections are cheaper. The course drifts between windows.')))}
  (bind ci {g('CurInstr')})
  (if (< ci ni)
    {put('PanelText', '4', cat(q('CALIBRATING: '), at('InstrNames', 'ci'), q('  ('), istr('(+ ci 1)'), q('/'), istr('ni'), q(')' + NL + 'Reading '), f1(g('CalReading')), q('    target 50 +/- 3' + NL + 'Adjust the offset until the reading sits on 50, then LOCK.'),
        '(Utilities|String|SelectString "" "' + NL + 'INSTRUMENT RAIL is OFF: ask the Systems Operator." (== ' + at('Switches', '4') + ' 1))'))}
    (else
      {put('PanelText', '4', q('All instruments calibrated and ready to look for ice.'))})))"""

G["FindCons"] = f"""(fn FindCons ()
  (bind cm (Actor|GetActorOfClass "/Game/MissionCrew/Construction/Blueprints/BP_ConstructionManager.BP_ConstructionManager_C"))
  (bind cm2 (Utilities|Casting|CastToBP_ConstructionManager cm)
    (:then
      {s('ConsMgr', 'cm2')})
    (:CastFailed)))"""

# ---------------------------------------------------------------- event graph
G["EventGraph"] = f"""(event EventBeginPlay
  (if (Networking|HasAuthority)
    (CallFunction|FindCons)
    (bind ln (Game|GetCurrentLevelName true))
    (if (and {g('AutoStartinTestMap')} (Utilities|String|Contains ln "L_Transit"))
      (Utilities|FlowControl|Delay 0.5)
      (CallFunction|StartPhase)
      (if {g('DebugAutoTest')}
        (Utilities|FlowControl|Delay 1.0)
        (CallFunction|RunDebugTest)))))

(event EventTick (DeltaSeconds)
  (if (Networking|HasAuthority)
    (if {g('Active')}
      (CallFunction|ServerTick :Dt DeltaSeconds)
      (else
        (if (not {g('Started')})
          (bind cm {g('ConsMgr')})
          (bind okc (CallFunction|Valid :Obj cm))
          (if okc
            (if (Class|BPConstructionManager|GetLaunched cm)
              (CallFunction|StartPhase))))))))

(event Custom|BeginTransit
  (CallFunction|StartPhase))

(event Custom|RunDebugTest
  (Development|PrintString "DEBUGTEST S2 start")
  (CallFunction|HandleAction :Code 190 :Who self)
  (CallFunction|HandleAction :Code 290 :Who self)
  (CallFunction|HandleAction :Code 390 :Who self)
  (CallFunction|HandleAction :Code 490 :Who self)
  (Development|PrintString {cat(q('DEBUGTEST stage after GO x4 = '), istr(g('Stage')))})
  (Utilities|FlowControl|Delay 32.0)
  (Development|PrintString {cat(q('DEBUGTEST stage after ascent = '), istr(g('Stage')), q(' msg: '), g('Message'))})
  (CallFunction|HandleAction :Code 401 :Who self)
  (Development|PrintString {cat(q('DEBUGTEST science with rail off: '), g('Message'))})
  (CallFunction|HandleAction :Code 104 :Who self)
  (CallFunction|HandleAction :Code 105 :Who self)
  (CallFunction|HandleAction :Code 103 :Who self)
  (Development|PrintString {cat(q('DEBUGTEST wrong switch: '), g('Message'))})
  (CallFunction|DebugSolveEquip)
  (CallFunction|DebugSolveScience)
  (Development|PrintString {cat(q('DEBUGTEST eq/sci: '), g('BoardText'))})
  (Utilities|FlowControl|Delay 12.0)
  (CallFunction|DebugSolveEquip)
  (CallFunction|DebugSolveScience)
  (CallFunction|DebugPilot)
  (Utilities|FlowControl|Delay 12.0)
  (CallFunction|DebugSolveEquip)
  (CallFunction|DebugSolveScience)
  (CallFunction|DebugFixAlarms)
  (Development|PrintString {cat(q('DEBUGTEST t=55 board: '), g('BoardText'), q(' | panel1: '), at('PanelText', '1'))})
  (Utilities|FlowControl|Delay 15.0)
  (CallFunction|DebugFixAlarms)
  (CallFunction|DebugPilot)
  (CallFunction|DebugSolveEquip)
  (CallFunction|DebugSolveScience)
  (Development|PrintString {cat(q('DEBUGTEST t=70 msg: '), g('Message'), q(' | pilot: '), at('PanelText', '3'))})
  (Utilities|FlowControl|Delay 20.0)
  (CallFunction|DebugFixAlarms)
  (CallFunction|DebugPilot)
  (Utilities|FlowControl|Delay 25.0)
  (CallFunction|DebugFixAlarms)
  (CallFunction|DebugPilot)
  (Utilities|FlowControl|Delay 20.0)
  (CallFunction|DebugFixAlarms)
  (CallFunction|DebugPilot)
  (Utilities|FlowControl|Delay 20.0)
  (CallFunction|DebugFixAlarms)
  (CallFunction|DebugPilot)
  (Utilities|FlowControl|Delay 20.0)
  (CallFunction|DebugFixAlarms)
  (CallFunction|DebugPilot)
  (Utilities|FlowControl|Delay 15.0)
  (Development|PrintString {cat(q('DEBUGTEST END stage='), istr(g('Stage')), q(' done='), '(Utilities|String|ToString(Boolean) ' + g('Done') + ')', q(' REPORT: '), g('Report'))}))"""

# debug helpers (server-side cheats used only by the self-test)
G["DebugSolveEquip"] = f"""(fn DebugSolveEquip ()
  (bind c {g('CurDev')})
  (if (< c {ln('DevNames')})
    (if (<= {g('DevTestTime')} 0.0)
      (for st (range 3)
        (for b (range 3)
          (if (== {at('ButtonStep', '(+ (* c 3) b)')} st)
            (if (== {at('DevProgress', 'c')} st)
              (CallFunction|HandleAction :Code (+ 200 b) :Who self))))))))"""

G["DebugSolveScience"] = f"""(fn DebugSolveScience ()
  (bind c {g('CurInstr')})
  (if (< c {ln('InstrNames')})
    (bind d (- {at('CalTarget', 'c')} {at('CalKnob', 'c')}))
    (for i (range (Math|Float|Round {absf('d')}))
      (CallFunction|HandleAction :Code (Math|Integer|SelectInt 402 401 (> d 0.0)) :Who self))
    (CallFunction|HandleAction :Code 404 :Who self)))"""

G["DebugPilot"] = f"""(fn DebugPilot ()
  (if {g('WindowOpen')}
    (for i (range 12)
      (bind nk {g('NudgeKm')})
      (bind h (* nk 0.5))
      (if (> {g('DispX')} h)
        (CallFunction|HandleAction :Code 300 :Who self)
        (elif (< {g('DispX')} (- 0.0 h))
          (CallFunction|HandleAction :Code 301 :Who self)))
      (if (> {g('DispY')} h)
        (CallFunction|HandleAction :Code 303 :Who self)
        (elif (< {g('DispY')} (- 0.0 h))
          (CallFunction|HandleAction :Code 302 :Who self))))))"""

G["DebugFixAlarms"] = f"""(fn DebugFixAlarms ()
  (for i (range 4)
    (if {at('WarnActive', 'i')}
      (CallFunction|HandleAction :Code (+ 100 {at('WarnSwitch', 'i')}) :Who self))))"""

json.dump(G, open(r"C:/Users/Denis/Documents/Unreal Projects/m4rs_nasa/Saved/S2Gen/mgr_dsl.json", "w"), indent=0)
for k, v in G.items():
    if v.count("(") != v.count(")"):
        print("PAREN MISMATCH", k, v.count("("), v.count(")"))
print("ok", len(G))
