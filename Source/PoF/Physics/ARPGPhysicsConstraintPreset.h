#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGPhysicsConstraintPreset.generated.h"

/**
 * Preset type for common constraint configurations.
 */
UENUM(BlueprintType)
enum class EARPGConstraintType : uint8
{
	/** Free rotation around one axis (doors, levers). */
	Hinge,
	/** Free rotation around two axes with limits (ragdoll joints). */
	BallAndSocket,
	/** Locked rotation, free linear movement along one axis (pistons, sliders). */
	Prismatic,
	/** Fully locked — welds two bodies together. */
	Weld,
	/** Custom — all limits set manually via properties. */
	Custom
};

/**
 * Data asset defining a reusable physics constraint configuration.
 * Create instances in the editor for ragdoll joints, door hinges, etc.
 * Apply at runtime via ApplyToConstraint().
 */
UCLASS(BlueprintType)
class POF_API UARPGPhysicsConstraintPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Preset type — determines which limits are auto-configured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint")
	EARPGConstraintType ConstraintType = EARPGConstraintType::BallAndSocket;

	// --- Angular Limits (degrees) ---

	/** Swing 1 limit in degrees (0 = locked). Used by BallAndSocket and Custom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint|Angular", meta = (ClampMin = "0", ClampMax = "180", EditCondition = "ConstraintType != EARPGConstraintType::Weld"))
	float Swing1LimitAngle = 45.f;

	/** Swing 2 limit in degrees (0 = locked). Used by BallAndSocket and Custom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint|Angular", meta = (ClampMin = "0", ClampMax = "180", EditCondition = "ConstraintType != EARPGConstraintType::Weld"))
	float Swing2LimitAngle = 45.f;

	/** Twist limit in degrees (0 = locked). Used by Hinge, BallAndSocket, and Custom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint|Angular", meta = (ClampMin = "0", ClampMax = "180", EditCondition = "ConstraintType != EARPGConstraintType::Weld"))
	float TwistLimitAngle = 30.f;

	// --- Linear Limits ---

	/** Linear limit distance for Prismatic/Custom types. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint|Linear", meta = (ClampMin = "0", EditCondition = "ConstraintType == EARPGConstraintType::Prismatic || ConstraintType == EARPGConstraintType::Custom"))
	float LinearLimitSize = 100.f;

	// --- Drive / Damping ---

	/** Angular damping on constrained bodies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint|Damping", meta = (ClampMin = "0"))
	float AngularDamping = 5.f;

	/** Linear damping on constrained bodies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constraint|Damping", meta = (ClampMin = "0"))
	float LinearDamping = 1.f;

	/**
	 * Apply this preset's settings to a UPhysicsConstraintComponent at runtime.
	 */
	UFUNCTION(BlueprintCallable, Category = "Constraint")
	void ApplyToConstraint(class UPhysicsConstraintComponent* Constraint) const;
};
