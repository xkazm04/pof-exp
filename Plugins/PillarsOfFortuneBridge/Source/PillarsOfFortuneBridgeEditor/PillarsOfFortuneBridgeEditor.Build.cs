using UnrealBuildTool;

public class PillarsOfFortuneBridgeEditor : ModuleRules
{
	public PillarsOfFortuneBridgeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"PillarsOfFortuneBridge"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"Slate",
			"SlateCore",
			"HTTP",
			"HTTPServer",
			"AssetRegistry",
			"BlueprintGraph",
			"KismetCompiler",
			"LevelEditor",
			"Json",
			"JsonUtilities",
			"RenderCore",
			"InputCore",
			"LiveCoding",
			"WebSocketNetworking",
			"Networking",
			"Sockets"
		});
	}
}
