#include "Test/Materials/VSArenaMaterialBindingTest.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Engine/Texture.h"

AVSArenaMaterialBindingTest::AVSArenaMaterialBindingTest()
{
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSArenaMaterialBindingTest::StartTest()
{
	Super::StartTest();

	UStaticMesh* Arena = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ArenaBuild/SM_Arena"));
	if (!Arena)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("SM_Arena missing at /Game/ArenaBuild/SM_Arena"));
		return;
	}

	const int32 NumSlots = Arena->GetStaticMaterials().Num();
	AssertTrue(NumSlots >= 1, FString::Printf(TEXT("#1 SM_Arena has >=1 material slot (%d)"), NumSlots));

	int32 NullSlots = 0;
	int32 InstancesChecked = 0;
	int32 SrgbViolations = 0;
	int32 MissingAlbedo = 0;

	for (int32 i = 0; i < NumSlots; ++i)
	{
		UMaterialInterface* Mat = Arena->GetMaterial(i);
		if (!Mat)
		{
			++NullSlots;
			continue;
		}

		// Deeper check only when the slot is an instance of the surface master
		// (so the named Albedo/Normal/Roughness parameters exist). A plain
		// one-off UMaterial just gets the non-null guarantee above.
		if (UMaterialInstance* MI = Cast<UMaterialInstance>(Mat))
		{
			++InstancesChecked;

			UTexture* Albedo = nullptr;
			MI->GetTextureParameterValue(FName(TEXT("Albedo")), Albedo);
			if (!Albedo)
			{
				++MissingAlbedo;
			}

			for (const TCHAR* ParamName : { TEXT("Normal"), TEXT("Roughness") })
			{
				UTexture* Tex = nullptr;
				if (MI->GetTextureParameterValue(FName(ParamName), Tex) && Tex && Tex->SRGB)
				{
					++SrgbViolations;
				}
			}
		}
	}

	AssertTrue(NullSlots == 0, FString::Printf(TEXT("#2 no SM_Arena slot is null (%d null of %d)"), NullSlots, NumSlots));
	AssertTrue(MissingAlbedo == 0, FString::Printf(TEXT("#3 every instanced slot resolves an Albedo texture (%d missing)"), MissingAlbedo));
	AssertTrue(SrgbViolations == 0, FString::Printf(TEXT("#4 Normal/Roughness sampled non-sRGB (%d sRGB violations)"), SrgbViolations));

	FinishTest(EFunctionalTestResult::Default,
		FString::Printf(TEXT("arena material binding OK (%d slots, %d instances inspected)"), NumSlots, InstancesChecked));
}
