using UnrealBuildTool;
using System.Collections.Generic;

public class MissionCrewEditorTarget : TargetRules
{
	public MissionCrewEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MissionCrew");
	}
}
