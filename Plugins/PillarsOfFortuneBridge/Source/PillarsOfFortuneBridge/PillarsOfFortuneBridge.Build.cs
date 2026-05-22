using UnrealBuildTool;

public class PillarsOfFortuneBridge : ModuleRules
{
	public PillarsOfFortuneBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities",
			"AssetRegistry",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"RenderCore"
		});

		// Editor-only dependencies for snapshot capture and test runner
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd"
			});
		}
	}
}
