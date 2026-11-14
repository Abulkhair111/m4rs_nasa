import json
F={ # name: (inputs [(n,type)], outputs [(n,type)])
'Say':([('Text','string')],[]),'Score':([('Role','int'),('Good','bool')],[]),'Valid':([('Obj','OBJ')],[('Result','bool')]),
'RoleOf':([('Who','ACTOR')],[('Result','int')]),'RoleName':([('R','int')],[('Result','string')]),'SwName':([('I','int')],[('Result','string')]),
'SwState':([('I','int')],[('Result','string')]),'StepName':([('DevType','int'),('Step','int')],[('Result','string')]),'DevName':([('DevType','int')],[('Result','string')]),
'LoadLoadout':([],[]),'InitData':([],[]),'StartPhase':([],[]),'CheckLaunchGo':([],[]),'DoGo':([('Role','int')],[]),
'HandleAction':([('Code','int'),('Who','ACTOR')],[]),'DoSystems':([('A','int')],[]),'DoEquip':([('A','int')],[]),'FinishDevTest':([],[]),
'BurnCost':([],[('Result','float')]),'DoPilot':([('A','int')],[]),'UpdateDisp':([],[]),'OpenWindow':([('Name','string')],[]),'CloseWindow':([],[]),
'DoScience':([('A','int')],[]),'TriggerWarning':([('I','int')],[]),'TransitTick':([('Dt','float')],[]),'ServerTick':([('Dt','float')],[]),
'Arrive':([],[]),'AccText':([],[('Result','string')]),'StageLine':([],[('Result','string')]),'BuildUi':([],[]),
'DebugSolveEquip':([],[]),'DebugSolveScience':([],[]),'DebugPilot':([],[]),'DebugFixAlarms':([],[]),
}
EXTRA=[('DevTypes','int','ARRAY'),('Perm','int','ARRAY'),('DriftXs','float','ARRAY'),('DriftYs','float','ARRAY'),('KickX','float',None),('KickY','float',None),
('bStarted','bool',None),('bTmp','bool',None),('TmpI','int',None),('TmpJ','int',None),('TmpK','int',None),('TmpS','string',None)]
json.dump({'F':F,'EXTRA':EXTRA},open('fns.json','w'))
