#include "Test/Environment/VSArenaCollisionTest.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AVSArenaCollisionTest::AVSArenaCollisionTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 15.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSArenaCollisionTest::StartTest()
{
	Super::StartTest();

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Sphere)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Sphere mesh missing"));
		return;
	}

	const TArray<FVector> Spots = {
		FVector(0.f, 0.f, 300.f),
		FVector(600.f, 600.f, 300.f),
		FVector(-600.f, 600.f, 300.f),
		FVector(600.f, -600.f, 300.f),
		FVector(-600.f, -600.f, 300.f),
	};

	for (const FVector& Spot : Spots)
	{
		AStaticMeshActor* Probe = GetWorld()->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), Spot, FRotator::ZeroRotator);
		if (!Probe) continue;
		UStaticMeshComponent* SMC = Probe->GetStaticMeshComponent();
		SMC->SetMobility(EComponentMobility::Movable);
		SMC->SetStaticMesh(Sphere);
		SMC->SetWorldScale3D(FVector(0.5f));
		SMC->SetCollisionProfileName(TEXT("BlockAll"));
		SMC->SetSimulatePhysics(true);
		Probes.Add(Probe);
	}

	if (Probes.Num() == 0)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("No physics probes spawned"));
	}
}

void AVSArenaCollisionTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsRunning() || bAsserted) return;

	Elapsed += DeltaSeconds;
	if (Elapsed < 2.5f) return; // let physics settle
	bAsserted = true;

	int32 Rested = 0;
	for (const TWeakObjectPtr<AActor>& P : Probes)
	{
		if (!P.IsValid()) continue;
		const float Z = P->GetActorLocation().Z;
		if (Z > 0.f) ++Rested;
	}
	AssertTrue(Rested == Probes.Num(),
		FString::Printf(TEXT("#1 collision: all %d probes rested on the arena floor (Z>0); %d rested"),
			Probes.Num(), Rested));
	FinishTest(EFunctionalTestResult::Default, TEXT("arena floor collision holds"));
}
