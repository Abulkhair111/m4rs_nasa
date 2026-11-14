import json
ROB = {"refPath": "/Engine/EngineFonts/Roboto.Roboto"}
def font(sz, face="Bold"):
    return {"fontObject": ROB, "typefaceFontName": face, "size": sz}
def col(r, g, b, a=1.0):
    return {"specifiedColor": {"r": r, "g": g, "b": b, "a": a}, "colorUseRule": "UseColor_Specified"}
def lc(r, g, b, a=1.0):
    return {"r": r, "g": g, "b": b, "a": a}
def canvas(x, y, ax=0, ay=0, axmax=None, aymax=None, al=(0, 0), auto=True, right=0, bottom=0):
    return {"layoutData": {"offsets": {"left": x, "top": y, "right": right, "bottom": bottom},
                           "anchors": {"minimum": {"x": ax, "y": ay}, "maximum": {"x": axmax if axmax is not None else ax, "y": aymax if aymax is not None else ay}},
                           "alignment": {"x": al[0], "y": al[1]}}, "bAutoSize": auto}
def brush(r, g, b, a=1.0, w=None, h=None):
    d = {"tintColor": col(r, g, b, a)}
    if w is not None:
        d["imageSize"] = {"x": w, "y": h}
    return {"brush": d}
pad = lambda l, t, r, b: {"padding": {"left": l, "top": t, "right": r, "bottom": b}}

W = {}   # widget props
S = {}   # slot props
CYAN = (0.55, 0.9, 1.0)
W["Bg"] = brush(0.008, 0.015, 0.035)
S["Bg"] = canvas(0, 0, 0, 0, 1, 1, auto=False)
W["TopText"] = {"font": font(20), "colorAndOpacity": col(*CYAN), "text": "S2 LAUNCH & TRANSIT"}
S["TopText"] = canvas(24, 14)
W["MsgText"] = {"font": font(18), "colorAndOpacity": col(1, 0.85, 0.3), "text": ""}
S["MsgText"] = canvas(24, 50)
S["Tabs"] = canvas(24, 88)
S["Left"] = canvas(24, 140)
W["BoardBox"] = {"background": {"tintColor": col(0.02, 0.05, 0.1, 0.9)}, **pad(16, 14, 16, 14)}
S["BoardBox"] = canvas(-24, 88, 1, 0, al=(1, 0))
W["BoardText"] = {"font": font(16, "Regular"), "colorAndOpacity": col(0.85, 0.95, 1.0), "text": "MISSION BOARD"}
W["PanelTitle"] = {"font": font(26), "colorAndOpacity": col(*CYAN), "text": "STATION"}
S["PanelTitle"] = pad(0, 0, 0, 6)
W["PanelText"] = {"font": font(17, "Regular"), "colorAndOpacity": col(0.92, 0.95, 1.0), "text": ""}
S["PanelText"] = pad(0, 0, 0, 12)
W["GoBtn"] = {"backgroundColor": lc(0.1, 0.65, 0.2)}
S["GoBtn"] = {**pad(0, 0, 0, 12), "horizontalAlignment": "HAlign_Left"}
W["GoT"] = {"font": font(22), "colorAndOpacity": col(1, 1, 1), "text": "  GO FOR LAUNCH  "}
labels = {"Tab1T": "SYSTEMS", "Tab2T": "EQUIPMENT", "Tab3T": "PILOT", "Tab4T": "SCIENCE",
          "P0T": "<  LEFT", "P1T": "RIGHT  >", "P2T": "UP  ^", "P3T": "DOWN  v",
          "C0T": "-5", "C1T": "-1", "C2T": "+1", "C3T": "+5", "C4T": "LOCK CAL",
          "S0T": "switch", "S1T": "switch", "S2T": "switch", "S3T": "switch", "S4T": "switch", "S5T": "switch",
          "E0T": "step", "E1T": "step", "E2T": "step"}
for k, v in labels.items():
    W[k] = {"font": font(17), "colorAndOpacity": col(1, 1, 1), "text": "  " + v + "  "}
    btn = k[:-1]
    W[btn] = {"backgroundColor": lc(0.06, 0.32, 0.5)}
    S[btn] = {**pad(0, 4, 10, 4)}
for t in ["Tab1", "Tab2", "Tab3", "Tab4"]:
    W[t] = {"backgroundColor": lc(0.25, 0.25, 0.3)}
W["C4"] = {"backgroundColor": lc(0.1, 0.55, 0.25)}
W["ScopeSize"] = {"widthOverride": 320, "heightOverride": 320, "bOverride_WidthOverride": True, "bOverride_HeightOverride": True}
S["ScopeSize"] = {**pad(0, 4, 0, 10), "horizontalAlignment": "HAlign_Left"}
W["ScopeBg"] = brush(0.02, 0.06, 0.12)
S["ScopeBg"] = {"horizontalAlignment": "HAlign_Fill", "verticalAlignment": "VAlign_Fill"}
for n, (w, h, c) in {"CrossH": (320, 1, (0.2, 0.4, 0.5, 1)), "CrossV": (1, 320, (0.2, 0.4, 0.5, 1)),
                     "Corr": (48, 48, (0.1, 0.8, 0.3, 0.35)), "Dot": (14, 14, (1.0, 0.5, 0.1, 1))}.items():
    W[n] = brush(*c, w=w, h=h)
    S[n] = {"horizontalAlignment": "HAlign_Center", "verticalAlignment": "VAlign_Center"}
W["CalSize"] = {"widthOverride": 520, "heightOverride": 30, "bOverride_WidthOverride": True, "bOverride_HeightOverride": True}
S["CalSize"] = {**pad(0, 4, 0, 10), "horizontalAlignment": "HAlign_Left"}
W["CalBar"] = {"percent": 0.5, "fillColorAndOpacity": lc(0.2, 0.9, 0.4)}
json.dump({"W": W, "S": S}, open(r"C:/Users/Denis/Documents/Unreal Projects/m4rs_nasa/Saved/S2Gen/style.json", "w"))
print(len(W), len(S))
