#include "Test/Materials/VSMasterMaterialInstanceTest.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"

AVSMasterMaterialInstanceTest::AVSMasterMaterialInstanceTest()
{
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSMasterMaterialInstanceTest::StartTest()
{
	Super::StartTest();

	UMaterialInterface* Master = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_ARPG_Surface_Master"));
	AssertTrue(Master != nullptr, TEXT("#0 surface master exists at /Game/Materials/M_ARPG_Surface_Master"));
	if (!Master)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("master material missing"));
		return;
	}

	static const TCHAR* InstancePaths[] = {
		TEXT("/Game/Materials/MI_Arena_Floor"),
		TEXT("/Game/Materials/MI_Arena_Wall"),
		TEXT("/Game/Materials/MI_Arena_Pillar"),
		TEXT("/Game/Materials/MI_EnemyRed"),
	};

	int32 Missing = 0;
	int32 WrongParent = 0;
	int32 Checked = 0;

	for (const TCHAR* Path : InstancePaths)
	{
		UMaterialInstance* MI = LoadObject<UMaterialInstance>(nullptr, Path);
		if (!MI)
		{
			++Missing;
			AssertTrue(false, FString::Printf(TEXT("instance missing: %s"), Path));
			continue;
		}
		++Checked;

		// Pointer identity: both resolve from the same asset path, so a correctly
		// reparented instance shares the loaded master object.
		const bool bParentOK = (MI->Parent == Master);
		if (!bParentOK)
		{
			++WrongParent;
		}
		AssertTrue(bParentOK, FString::Printf(TEXT("%s parents to M_ARPG_Surface_Master (parent=%s)"),
			Path, MI->Parent ? *MI->Parent->GetName() : TEXT("null")));
	}

	AssertTrue(Missing == 0, FString::Printf(TEXT("#1 all consolidation instances exist (%d missing)"), Missing));
	AssertTrue(WrongParent == 0, FString::Printf(TEXT("#2 all instances parent to the master (%d wrong)"), WrongParent));

	FinishTest(EFunctionalTestResult::Default,
		FString::Printf(TEXT("master/instance consolidation OK (%d instances checked)"), Checked));
}
