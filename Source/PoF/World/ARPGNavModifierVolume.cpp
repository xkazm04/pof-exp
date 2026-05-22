#include "World/ARPGNavModifierVolume.h"
#include "Components/BoxComponent.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Obstacle.h"
#include "World/NavArea_LowCost.h"
#include "World/NavArea_Chokepoint.h"

AARPGNavModifierVolume::AARPGNavModifierVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	VolumeBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBounds"));
	VolumeBounds->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	VolumeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(VolumeBounds);

	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
}

void AARPGNavModifierVolume::BeginPlay()
{
	Super::BeginPlay();
	ApplyModifierSettings();
}

#if WITH_EDITOR
void AARPGNavModifierVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AARPGNavModifierVolume, ModifierType))
	{
		ApplyModifierSettings();
	}
}
#endif

void AARPGNavModifierVolume::ApplyModifierSettings()
{
	if (!NavModifier) return;

	switch (ModifierType)
	{
	case ENavModifierType::Avoidance:
		NavModifier->SetAreaClass(UNavArea_Obstacle::StaticClass());
		break;

	case ENavModifierType::Preferred:
		NavModifier->SetAreaClass(UNavArea_LowCost::StaticClass());
		break;

	case ENavModifierType::Blocked:
		NavModifier->SetAreaClass(UNavArea_Null::StaticClass());
		break;

	case ENavModifierType::Chokepoint:
		NavModifier->SetAreaClass(UNavArea_Chokepoint::StaticClass());
		break;
	}
}
