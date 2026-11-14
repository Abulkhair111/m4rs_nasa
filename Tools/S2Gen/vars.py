import json
R='Replicated'
V=[
# name, type, container, rep, editable
('bActive','bool',None,R,0),('Stage','int',None,R,0),('StageTime','float',None,R,0),('TransitTime','float',None,R,0),
('Seed','int',None,R,0),('GoFlags','bool','ARRAY',R,0),('FuelKg','float',None,R,0),('FuelStartKg','float',None,R,0),
('FuelReserveKg','float',None,R,0),('Health','float',None,R,0),('ErrX','float',None,R,0),('ErrY','float',None,R,0),
('DispX','float',None,R,0),('DispY','float',None,R,0),('WindowOpen','bool',None,R,0),('WindowName','string',None,R,0),
('WindowTimeLeft','float',None,R,0),('bSensorFault','bool',None,R,0),('Switches','int','ARRAY',R,0),('WarnActive','bool','ARRAY',R,0),
('WarnText','string','ARRAY',R,0),('InstrNames','string','ARRAY',R,0),('CalKnob','float','ARRAY',R,0),('CalTarget','float','ARRAY',R,0),
('CalDone','bool','ARRAY',R,0),('CurInstr','int',None,R,0),('CalReading','float',None,R,0),('DevNames','string','ARRAY',R,0),
('DevSteps','string','ARRAY',R,0),('ButtonStep','int','ARRAY',R,0),('DevProgress','int','ARRAY',R,0),('DevDone','bool','ARRAY',R,0),
('CurDev','int',None,R,0),('DevTestTime','float',None,R,0),('RoleGood','int','ARRAY',R,0),('RoleBad','int','ARRAY',R,0),
('Message','string',None,R,0),('MessageTime','float',None,R,0),('Report','string',None,R,0),('bDone','bool',None,R,0),
('Score','int',None,R,0),('bFreeRoles','bool',None,R,1),('TopText','string',None,R,0),('BoardText','string',None,R,0),
('PanelText','string','ARRAY',R,0),('SwitchLabels','string','ARRAY',R,0),('EqLabels','string','ARRAY',R,0),
# server only / config
('SeedOverride','int',None,None,1),('DriftX','float',None,None,0),('DriftY','float',None,None,0),('NextWindow','int',None,None,0),
('WindowTimes','float','ARRAY',None,0),('WarnTimes','float','ARRAY',None,0),('WarnTriggered','bool','ARRAY',None,0),
('WarnSwitch','int','ARRAY',None,0),('WarnValue','int','ARRAY',None,0),('WarnAge','float','ARRAY',None,0),
('FaultTime','float',None,None,0),('bFaultTriggered','bool',None,None,0),('BiasX','float',None,None,0),('BiasY','float',None,None,0),
('bAutoStartInTestMap','bool',None,None,1),('bDebugAutoTest','bool',None,None,1),('CountdownSec','float',None,None,1),
('AscentSec','float',None,None,1),('TransitSec','float',None,None,1),('WindowSec','float',None,None,1),('CorridorKm','float',None,None,1),
('NudgeKm','float',None,None,1),('NudgeFuelKg','float',None,None,1),('LaunchReliability','float',None,R,1),('TotalMassKg','float',None,R,1),
('bLoadoutFromBay','bool',None,R,0),('UiAccum','float',None,None,0),('bAnomaly','bool',None,R,0),('WarnResolved','int',None,R,0),
('NudgeCount','int',None,R,0),('FuelUsedKg','float',None,R,0),('CourseResult','string',None,R,0),
]
json.dump(V,open(r'C:/Users/Denis/Documents/Unreal Projects/m4rs_nasa/Saved/S2Gen/vars.json','w'))
print(len(V))
