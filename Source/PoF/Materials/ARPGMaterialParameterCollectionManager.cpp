#include "Materials/ARPGMaterialParameterCollectionManager.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/KismetMaterialLibrary.h"

void UARPGMaterialParameterCollectionManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UARPGMaterialParameterCollectionManager::Tick(float DeltaTime)
{
	AccumulatedTime += DeltaTime;
	UpdateMPCParameters();
}

TStatId UARPGMaterialParameterCollectionManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UARPGMaterialParameterCollectionManager, STATGROUP_Tickables);
}

void UARPGMaterialParameterCollectionManager::SetMPC(UMaterialParameterCollection* InMPC)
{
	MPC = InMPC;
	MPCInstance = nullptr;

	if (MPC)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			MPCInstance = World->GetParameterCollectionInstance(MPC);
		}
	}
}

void UARPGMaterialParameterCollectionManager::SetWindDirection(FVector Direction)
{
	WindDirection = Direction.GetSafeNormal();
}

void UARPGMaterialParameterCollectionManager::SetWindStrength(float Strength)
{
	WindStrength = FMath::Max(Strength, 0.f);
}

void UARPGMaterialParameterCollectionManager::SetGlobalTint(FLinearColor Tint)
{
	GlobalTint = Tint;
}

void UARPGMaterialParameterCollectionManager::SetWetness(float InWetness)
{
	Wetness = FMath::Clamp(InWetness, 0.f, 1.f);
}

void UARPGMaterialParameterCollectionManager::UpdateMPCParameters()
{
	if (!MPC)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, TimeParamName, AccumulatedTime);
	UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, WindDirXParamName, WindDirection.X);
	UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, WindDirYParamName, WindDirection.Y);
	UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, WindStrengthParamName, WindStrength);
	UKismetMaterialLibrary::SetVectorParameterValue(World, MPC, GlobalTintParamName, GlobalTint);
	UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, WetnessParamName, Wetness);
}
