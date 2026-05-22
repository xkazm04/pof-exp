#include "LevelDesign/ARPGEnvironmentalProp.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AARPGEnvironmentalProp::AARPGEnvironmentalProp()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AARPGEnvironmentalProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildProps();
	UpdateDecal();
	UpdateAccentLight();
}

void AARPGEnvironmentalProp::RebuildProps()
{
	// Clean up existing prop components
	for (UStaticMeshComponent* Comp : PropMeshComponents)
	{
		if (Comp) Comp->DestroyComponent();
	}
	PropMeshComponents.Empty();

	// Create new prop components
	for (int32 i = 0; i < Props.Num(); ++i)
	{
		const FEnvironmentalPropEntry& Entry = Props[i];
		UStaticMesh* Mesh = Entry.Mesh.LoadSynchronous();
		if (!Mesh) continue;

		const FName CompName = *FString::Printf(TEXT("Prop_%d"), i);
		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this, CompName);
		MeshComp->SetStaticMesh(Mesh);
		MeshComp->SetupAttachment(GetRootComponent());
		MeshComp->SetRelativeLocation(Entry.LocalOffset);
		MeshComp->SetRelativeRotation(Entry.LocalRotation);
		MeshComp->SetRelativeScale3D(Entry.Scale);
		MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
		MeshComp->RegisterComponent();

		PropMeshComponents.Add(MeshComp);
	}
}

void AARPGEnvironmentalProp::UpdateDecal()
{
	if (DecalMaterial)
	{
		if (!GroundDecal)
		{
			GroundDecal = NewObject<UDecalComponent>(this, TEXT("GroundDecal"));
			GroundDecal->SetupAttachment(GetRootComponent());
			GroundDecal->RegisterComponent();
		}

		GroundDecal->SetDecalMaterial(DecalMaterial);
		GroundDecal->DecalSize = DecalSize;
		// Decals project downward by default
		GroundDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	}
	else if (GroundDecal)
	{
		GroundDecal->DestroyComponent();
		GroundDecal = nullptr;
	}
}

void AARPGEnvironmentalProp::UpdateAccentLight()
{
	if (bUseAccentLight)
	{
		if (!AccentLight)
		{
			AccentLight = NewObject<UPointLightComponent>(this, TEXT("AccentLight"));
			AccentLight->SetupAttachment(GetRootComponent());
			AccentLight->RegisterComponent();
		}

		AccentLight->SetLightColor(AccentLightColor);
		AccentLight->SetIntensity(AccentLightIntensity);
		AccentLight->SetAttenuationRadius(AccentLightRadius);
		AccentLight->SetRelativeLocation(AccentLightOffset);
		AccentLight->SetCastShadows(false); // Accent lights are subtle, no shadows
	}
	else if (AccentLight)
	{
		AccentLight->DestroyComponent();
		AccentLight = nullptr;
	}
}
