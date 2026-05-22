#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGEnvironmentalProp.generated.h"

class UPointLightComponent;
class UDecalComponent;
class UStaticMeshComponent;

/** Type of environmental storytelling this prop cluster represents. */
UENUM(BlueprintType)
enum class EEnvironmentalStoryType : uint8
{
	/** Abandoned campsite, discarded items. */
	Abandoned      UMETA(DisplayName = "Abandoned Site"),
	/** Battle aftermath — weapons, scorch marks. */
	BattleScar     UMETA(DisplayName = "Battle Scar"),
	/** Shrine, altar, or ritual site. */
	Ritual         UMETA(DisplayName = "Ritual/Shrine"),
	/** Trail markers, breadcrumbs guiding players. */
	PathMarker     UMETA(DisplayName = "Path Marker"),
	/** Warning signs, danger indicators. */
	Warning        UMETA(DisplayName = "Warning"),
	/** Treasure/reward area hint. */
	TreasureHint   UMETA(DisplayName = "Treasure Hint"),
	/** Generic atmospheric prop cluster. */
	Atmosphere     UMETA(DisplayName = "Atmosphere")
};

/** A single prop within the environmental cluster. */
USTRUCT(BlueprintType)
struct FEnvironmentalPropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Offset from cluster center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FRotator LocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FVector Scale = FVector::OneVector;
};

/**
 * Environmental storytelling prop cluster.
 * Groups props, decals, and accent lighting into a single placeable
 * unit for rapid atmospheric level dressing.
 *
 * Supports:
 * - Multiple static mesh props arranged around a center
 * - Ground decal (blood, scorch marks, moss, etc.)
 * - Accent point light for mood
 * - Story type classification for design documentation
 */
UCLASS()
class POF_API AARPGEnvironmentalProp : public AActor
{
	GENERATED_BODY()

public:
	AARPGEnvironmentalProp();

	UFUNCTION(BlueprintPure, Category = "LevelDesign|Story")
	EEnvironmentalStoryType GetStoryType() const { return StoryType; }

	/** Rebuild prop meshes from the Props array. Call after modifying Props at runtime. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Story")
	void RebuildProps();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	/** What story this prop cluster tells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EEnvironmentalStoryType StoryType = EEnvironmentalStoryType::Atmosphere;

	/** Designer notes — what narrative does this cluster convey? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (MultiLine = true))
	FString DesignNotes;

	/** Props in this cluster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Props")
	TArray<FEnvironmentalPropEntry> Props;

	/** Optional ground decal material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Decal")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	/** Decal size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Decal", meta = (ClampMin = "10"))
	FVector DecalSize = FVector(100.f, 100.f, 100.f);

	/** Whether to add an accent light to the cluster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Light")
	bool bUseAccentLight = false;

	/** Accent light color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Light", meta = (EditCondition = "bUseAccentLight"))
	FLinearColor AccentLightColor = FLinearColor(1.f, 0.8f, 0.5f, 1.f);

	/** Accent light intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Light", meta = (EditCondition = "bUseAccentLight", ClampMin = "0"))
	float AccentLightIntensity = 500.f;

	/** Accent light attenuation radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Light", meta = (EditCondition = "bUseAccentLight", ClampMin = "50"))
	float AccentLightRadius = 300.f;

	/** Accent light offset from cluster center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Light", meta = (EditCondition = "bUseAccentLight"))
	FVector AccentLightOffset = FVector(0.f, 0.f, 150.f);

private:
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> PropMeshComponents;

	UPROPERTY()
	TObjectPtr<UDecalComponent> GroundDecal;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> AccentLight;

	void UpdateDecal();
	void UpdateAccentLight();
};
