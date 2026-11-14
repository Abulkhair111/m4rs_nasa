# DSL for BP_TransitPawn and WBP_TransitHUD graphs.
import json

MGR = "/Game/MissionCrew/Transit/Blueprints/BP_TransitManager.BP_TransitManager_C"
HUD = "/Game/MissionCrew/Transit/UI/WBP_TransitHUD.WBP_TransitHUD_C"

P = {}
P["EventGraph"] = f"""(event Custom|ActionSig_Event (Code)
  (bind m (Actor|GetActorOfClass "{MGR}"))
  (bind m2 (Utilities|Casting|CastToBP_TransitManager m)
    (:then
      (Class|BPTransitManager|HandleAction m2 :Code Code :Who self))
    (:CastFailed)))

(event EventTick (DeltaSeconds)
  (if (Pawn|IsLocallyControlled)
    (if (not (Variables|Default|GetHudMade))
      (bind pc (Utilities|Casting|CastToPlayerController (Pawn|GetController))
        (:then
          (Variables|Default|SetHudMade true)
          (bind w (UserInterface|CreateWidget :Class "{HUD}" :OwningPlayer pc))
          (UserInterface|Viewport|AddtoViewport w)
          (Input|SetInputModeGameAndUI pc w)
          (Variables|Default|SetHud w)
          (Class|PlayerController|SetShowMouseCursor true pc))
        (:CastFailed)))))"""

def tx(s):
    return f"(Utilities|Text|ToText(String) {s})"

def mg(v):
    return f"(Class|BPTransitManager|Get{v} m)"

W = {}
W["Send"] = """(fn Send (A)
  (bind pw (Utilities|Casting|CastToBP_TransitPawn (Player|GetOwningPlayerPawn))
    (:then
      (Class|BPTransitPawn|ActionSigEvent pw :Code (+ (* (Variables|Default|GetViewRole) 100) A)))
    (:CastFailed)))"""

W["Refresh"] = f"""(fn Refresh ()
  (bind m (Variables|Default|GetMgr))
  (bind okm (CallFunction|Valid :Obj m))
  (if (not okm)
    (bind a (Actor|GetActorOfClass "{MGR}"))
    (bind a2 (Utilities|Casting|CastToBP_TransitManager a)
      (:then
        (Variables|Default|SetMgr a2))
      (:CastFailed))
    (else
      (if (not {mg('FreeRoles')})
        (bind cr (CallFunction|MyRole))
        (Variables|Default|SetViewRole (Math|Integer|SelectInt cr 1 (and (>= cr 1) (<= cr 4)))))
      (bind vr (Math|Integer|Clamp(Integer) (Variables|Default|GetViewRole) 1 4))
      (CallFunction|Apply :M m :VR vr))))"""

def settext(widget, s):
    return f"(Widget|SetText(Text) (Variables|WBP_TransitHUD|Get{widget}) {tx(s)})"

def arr(v, i):
    return f"(Utilities|Array|Get(acopy) {mg(v)} {i})"

W["Apply"] = f"""(fn Apply (M VR)
  (bind m M)
  {settext('TopText', mg('TopText'))}
  {settext('BoardText', mg('BoardText'))}
  (bind age (- (Utilities|Time|GetGameTimeinSeconds) {mg('MessageTime')}))
  {settext('MsgText', '(Utilities|String|SelectString ' + mg('Message') + ' "" (< age 12.0))')}
  (bind rn (Class|BPTransitManager|RoleName m :R VR))
  {settext('PanelTitle', '(Utilities|String|Append rn (Utilities|String|SelectString "   (test mode: any station)" "" ' + mg('FreeRoles') + '))')}
  {settext('PanelText', arr('PanelText', 'VR'))}
  (Switcher|SetActiveWidgetIndex (Variables|WBP_TransitHUD|GetPanels) (- VR 1))
  (if {mg('FreeRoles')}
    (Widget|SetVisibility (Variables|WBP_TransitHUD|GetTabs) "Visible")
    (else
      (Widget|SetVisibility (Variables|WBP_TransitHUD|GetTabs) "Collapsed")))
  (bind showgo (and (== {mg('Stage')} 0) (not {arr('GoFlags', 'VR')})))
  (if showgo
    (Widget|SetVisibility (Variables|WBP_TransitHUD|GetGoBtn) "Visible")
    (else
      (Widget|SetVisibility (Variables|WBP_TransitHUD|GetGoBtn) "Collapsed")))
  {settext('S0T', arr('SwitchLabels', '0'))}
  {settext('S1T', arr('SwitchLabels', '1'))}
  {settext('S2T', arr('SwitchLabels', '2'))}
  {settext('S3T', arr('SwitchLabels', '3'))}
  {settext('S4T', arr('SwitchLabels', '4'))}
  {settext('S5T', arr('SwitchLabels', '5'))}
  {settext('E0T', arr('EqLabels', '0'))}
  {settext('E1T', arr('EqLabels', '1'))}
  {settext('E2T', arr('EqLabels', '2'))}
  (bind dx (* (Math|Float|Clamp(Float) {mg('DispX')} -95.0 95.0) 1.6))
  (bind dy (* (Math|Float|Clamp(Float) {mg('DispY')} -95.0 95.0) -1.6))
  (Widget|Transform|SetRenderTranslation (Variables|WBP_TransitHUD|GetDot) (Math|Vector2D|MakeVector2D dx dy))
  (Progress|SetPercent (Variables|WBP_TransitHUD|GetCalBar) (/ {mg('CalReading')} 100.0)))"""

W["MyRole"] = """(fn MyRole ()
  (bind ps (Utilities|Casting|CastToBP_MC_PlayerState (Class|Controller|GetPlayerState (Widget|GetOwningPlayer)))
    (:then
      (return (Class|BPMCPlayerState|GetCrewRole ps)))
    (:CastFailed
      (return 0))))"""

W["Valid"] = """(fn Valid (Obj)
  (Utilities|IsValid Obj
    (:"Is Valid"
      (return true))
    (:"Is Not Valid"
      (return false))))"""

ev = ["(event UserInterface|EventTick (MyGeometry InDeltaTime)\n  (CallFunction|Refresh))"]
for i in range(6):
    ev.append(f"(event OnClicked(S{i})\n  (CallFunction|Send :A {i}))")
for i in range(3):
    ev.append(f"(event OnClicked(E{i})\n  (CallFunction|Send :A {i}))")
for i in range(4):
    ev.append(f"(event OnClicked(P{i})\n  (CallFunction|Send :A {i}))")
for i in range(5):
    ev.append(f"(event OnClicked(C{i})\n  (CallFunction|Send :A {i}))")
ev.append("(event OnClicked(GoBtn)\n  (CallFunction|Send :A 90))")
for i in range(1, 5):
    ev.append(f"(event OnClicked(Tab{i})\n  (Variables|Default|SetViewRole {i}))")
W["EventGraph"] = "\n\n".join(ev)

json.dump({"P": P, "W": W}, open(r"C:/Users/Denis/Documents/Unreal Projects/m4rs_nasa/Saved/S2Gen/ui_dsl.json", "w"))
print("ok")
