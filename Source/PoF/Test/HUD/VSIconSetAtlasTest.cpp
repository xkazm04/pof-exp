#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/ARPGIconAtlasRules.h"

/**
 * Icon-sets L3 gate (icon-sets, runtimeDeferred('VSIconSetAtlasTest')). Asserts the
 * production atlas packing math: 16x16 grid = 256 cells, 224 allocated + 32 reserved,
 * mip chain floors at 32px. Pure math — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSIconSetAtlasTest,
	"Project.Functional Tests.PoF.IconSets.VSIconSetAtlasTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSIconSetAtlasTest::RunTest(const FString& /*Parameters*/)
{
	TestEqual(TEXT("4096/256 = 16 cells per axis"), UARPGIconAtlasRules::CellsPerAxis(), 16);
	TestEqual(TEXT("16x16 = 256 total cells"), UARPGIconAtlasRules::TotalCells(), 256);

	// 224 allocated + 32 reserved must exactly fill the 256-cell budget.
	TestEqual(TEXT("allocated = 224"), UARPGIconAtlasRules::AllocatedSlots, 224);
	TestEqual(TEXT("allocated + reserved == total cells"),
		UARPGIconAtlasRules::AllocatedSlots + UARPGIconAtlasRules::ReservedSlots,
		UARPGIconAtlasRules::TotalCells());

	// Mip chain: 256 → 128 → 64 → 32 (floor) = 4 levels.
	TestEqual(TEXT("mip floor = 32px"), UARPGIconAtlasRules::MipFloorPx, 32);
	TestEqual(TEXT("mip count = 4 (256→32)"), UARPGIconAtlasRules::MipCount(), 4);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
