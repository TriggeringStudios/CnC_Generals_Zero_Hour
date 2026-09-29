using UnrealBuildTool;

public class GeneralsRTS : ModuleRules
{
	public GeneralsRTS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"MassEntity", "MassCommon", "MassSpawner" });
	}
}
