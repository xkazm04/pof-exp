#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGCoverPoint.generated.h"

class UArrowComponent;
class UBoxComponent;

/** Type of cover this point provides. */
UENUM(BlueprintType)
enum class ECoverType : uint8
{
	Half    UMETA(DisplayName = "Half Cover (Crouch)"),
	Full    UMETA(DisplayName = "Full Cover (Standing)"),
	Pillar  UMETA(DisplayName = "Pillar (Directional)")
};

/**
 * A cover marker for AI tactical positioning.
 * Place in encounter arenas at positions where AI or players can take cover.
 * AI behavior trees query these to find tactical positions during combat.
 */
UCLASS()
class POF_API AARPGCoverPoint : public AActor
{
	GENERATED_BODY()

public:
	AARPGCoverPoint();

	/** Whether this cover point is currently available (not occupied). */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Cover")
	bool IsAvailable() const { return !OccupantActor.IsValid(); }

	/** Claim this cover point for an actor. Returns false if already occupied. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Cover")
	bool Claim(AActor* Claimant);

	/** Release this cover point. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Cover")
	void Release();

	/** Get the cover facing direction (world space). */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Cover")
	FVector GetCoverDirection() const;

	/** Get the lean-out peek positions (left and right of cover). */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Cover")
	void GetPeekPositions(FVector& OutLeftPeek, FVector& OutRightPeek) const;

	/** Check if a target position is behind this cover from the attacker. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Cover")
	bool ProvidesCoverFrom(const FVector& AttackerPosition) const;

	UFUNCTION(BlueprintPure, Category = "LevelDesign|Cover")
	ECoverType GetCoverType() const { return CoverType; }

protected:
	/** Type of cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	ECoverType CoverType = ECoverType::Half;

	/** Width of the cover surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "50"))
	float CoverWidth = 150.f;

	/** Horizontal distance to peek out from cover edges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "10"))
	float PeekDistance = 80.f;

	/** Max angle (degrees) from cover normal where cover is effective. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "10", ClampMax = "90"))
	float CoverAngle = 60.f;

#if WITH_EDITORONLY_DATA
	/** Arrow showing cover facing direction. */
	UPROPERTY(VisibleAnywhere, Category = "Cover")
	TObjectPtr<UArrowComponent> CoverDirectionArrow;

	/** Box showing cover area in editor. */
	UPROPERTY(VisibleAnywhere, Category = "Cover")
	TObjectPtr<UBoxComponent> CoverVisualizer;
#endif

private:
	TWeakObjectPtr<AActor> OccupantActor;
};
