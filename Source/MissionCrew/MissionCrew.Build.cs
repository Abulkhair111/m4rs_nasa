using UnrealBuildTool;

public class MissionCrew : ModuleRules
{
	public MissionCrew(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false; // several files define same-named helpers in anonymous namespaces
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore", "NetCore", "CoreOnline"
		});
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
