using UnrealBuildTool;

public class PoFEditor : ModuleRules
{
	public PoFEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"PoF",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"ContentBrowser",
			"Niagara",
			"IKRig",
			"IKRigDeveloper",
			"PythonScriptPlugin",
			// AnimBP procedural authoring (UPoFAnimBPAuthoringLibrary)
			"BlueprintGraph",
			"AnimGraph",
			"AnimGraphRuntime",
			"Kismet",
			"KismetCompiler",
			"AnimationBlueprintEditor",
			"AssetRegistry",
			// EditorAssetLibrary used by the AnimBP authoring automation test for cleanup
			"EditorScriptingUtilities"
		});
	}
}
