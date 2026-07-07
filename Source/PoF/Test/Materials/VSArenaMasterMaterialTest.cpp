#include "Test/Materials/VSArenaMasterMaterialTest.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

/**
 * Materials Test Gate — the ArenaBuild master materials.
 *
 * Asset-content gate (EditorContext, so LoadObject on /Game paths is safe
 * headless — no PIE/world/map needed, safe under shared-tree concurrency).
 * Loads the three arena master materials the colosseum build depends on and
 * asserts each one is real, parameterized (tunable, not a baked constant
 * graph), and carries a valid blend mode + shading model. A missing or
 * hollow asset fails the gate: this is the packaging truth for the arena.
 *
 * Runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.Materials;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSArenaMasterMaterialTest,
	"Project.Functional Tests.PoF.Materials.ArenaMasters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSArenaMasterMaterialTest::RunTest(const FString& /*Parameters*/)
{
	// The three arena master materials (verified on disk under Content/ArenaBuild).
	static const TCHAR* ArenaMasterPaths[] =
	{
		TEXT("/Game/ArenaBuild/M_Arena_Floor.M_Arena_Floor"),
		TEXT("/Game/ArenaBuild/M_Arena_Pillar.M_Arena_Pillar"),
		TEXT("/Game/ArenaBuild/M_Arena_Wall.M_Arena_Wall")
	};

	for (const TCHAR* Path : ArenaMasterPaths)
	{
		// 1. Packaging truth: the asset must exist and load. Hard fail if not.
		UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, Path);
		if (!TestNotNull(*FString::Printf(TEXT("Arena master material loads: %s"), Path), Mat))
		{
			continue; // Report every missing asset, not just the first.
		}

		// 2. Masters are base UMaterials, not instances of something else.
		TestNotNull(
			*FString::Printf(TEXT("%s is a master (base UMaterial, not an instance)"), Path),
			Cast<UMaterial>(Mat));

		// 3. Genuinely parameterized: at least one scalar/vector/texture
		//    parameter overall — a constant-only graph is a regression (the
		//    arena tuning pipeline drives these parameters).
		TArray<FMaterialParameterInfo> ScalarInfos, VectorInfos, TextureInfos;
		TArray<FGuid> ScalarIds, VectorIds, TextureIds;
		Mat->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
		Mat->GetAllVectorParameterInfo(VectorInfos, VectorIds);
		Mat->GetAllTextureParameterInfo(TextureInfos, TextureIds);

		const int32 ParamTotal = ScalarInfos.Num() + VectorInfos.Num() + TextureInfos.Num();
		TestTrue(
			*FString::Printf(TEXT("%s is parameterized (scalar %d + vector %d + texture %d >= 1)"),
				Path, ScalarInfos.Num(), VectorInfos.Num(), TextureInfos.Num()),
			ParamTotal >= 1);

		// 4. Valid render configuration: blend mode in range, shading model set
		//    non-empty. Arena surfaces are solid architecture — opaque masters.
		const EBlendMode Blend = Mat->GetBlendMode();
		TestTrue(
			*FString::Printf(TEXT("%s has a valid blend mode (%d)"), Path, static_cast<int32>(Blend)),
			Blend >= BLEND_Opaque && Blend < BLEND_MAX);
		TestEqual(
			*FString::Printf(TEXT("%s blend mode is Opaque"), Path),
			static_cast<int32>(Blend), static_cast<int32>(BLEND_Opaque));
		TestTrue(
			*FString::Printf(TEXT("%s has a valid shading model set"), Path),
			Mat->GetShadingModels().IsValid());
	}

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
