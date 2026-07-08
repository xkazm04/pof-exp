#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ARPGVFXLodRules.h"

/**
 * VFX L3 gate (vfx, runtimeDeferred('VSVFXPerfTest')). Asserts the deterministic Niagara
 * LOD-band selection: LOD0 (0–15 m), LOD1 (15–35 m), LOD2 culled (>35 m). The GPU-budget /
 * Niagara-render check is the RHI visual concern (separate); this encodes the LOD decision.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSVFXPerfTest,
	"Project.Functional Tests.PoF.VFX.VSVFXPerfTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSVFXPerfTest::RunTest(const FString& /*Parameters*/)
{
	using D = EARPGVFXLod;

	TestEqual(TEXT("10m → LOD0 full"), UARPGVFXLodRules::LodForDistanceMeters(10.f), D::Lod0Full);
	TestEqual(TEXT("15m → LOD1 (band start)"), UARPGVFXLodRules::LodForDistanceMeters(15.f), D::Lod1Medium);
	TestEqual(TEXT("25m → LOD1 medium"), UARPGVFXLodRules::LodForDistanceMeters(25.f), D::Lod1Medium);
	TestEqual(TEXT("35m → LOD1 (band end)"), UARPGVFXLodRules::LodForDistanceMeters(35.f), D::Lod1Medium);
	TestEqual(TEXT("36m → LOD2 culled"), UARPGVFXLodRules::LodForDistanceMeters(36.f), D::Lod2Culled);
	TestEqual(TEXT("50m → LOD2 culled"), UARPGVFXLodRules::LodForDistanceMeters(50.f), D::Lod2Culled);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
