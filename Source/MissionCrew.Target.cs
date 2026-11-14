using UnrealBuildTool;
using System.Collections.Generic;

// Target name is ASCII on purpose: the .uproject is named in Cyrillic, but UBT accepts any
// <TargetName>.Target.cs inside Source/ when -Project=<path to .uproject> is given.
public class MissionCrewTarget : TargetRules
{
	public MissionCrewTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MissionCrew");
	}
}
