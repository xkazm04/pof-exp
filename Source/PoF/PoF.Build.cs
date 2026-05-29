using UnrealBuildTool;

public class PoF : ModuleRules
{
	public PoF(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { "PoF" });

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Niagara", "MotionWarping", "GameplayAbilities", "GameplayTags", "GameplayTasks", "AIModule", "NavigationSystem", "PhysicsCore", "GeometryCollectionEngine", "FieldSystemEngine", "ChaosSolverEngine", "ProceduralMeshComponent", "StateTreeModule", "GameplayStateTreeModule", "GameplayDebugger", "OnlineSubsystem", "OnlineSubsystemUtils", "FunctionalTesting", "RenderCore", "Json" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
