// Phase 2 F3b — Ability catalog UENUM + DataTable row.
//
// SYNC SOURCE: Mirror of PoF app `AbilityMeta` in
//   src/components/modules/core-engine/sub_character/_shared/data.ts
// Keep `Category`, `Tier`, `BaseDamage`, `GameplayTag` in lockstep.
// DataTable asset: /Game/Abilities/DT_AbilityCatalog (FARPGAbilityCatalogRow rows).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ARPGAbilityCatalogTypes.generated.h"

/**
 * Top-level grouping for catalogued character abilities.
 * UI uses these to bucket abilities (Melee / Ranged / Magical / Utility / Movement).
 */
UENUM(BlueprintType)
enum class EARPGAbilityCategory : uint8
{
	/** Up-close physical: light/heavy saber strikes, etc. */
	Melee     UMETA(DisplayName = "Melee"),
	/** Projectile/long-range damage. */
	Ranged    UMETA(DisplayName = "Ranged"),
	/** Force / spell-like abilities (Push, Pull, Lightning…). */
	Magical   UMETA(DisplayName = "Magical"),
	/** Non-damage support: stims, focus, interact. */
	Utility   UMETA(DisplayName = "Utility"),
	/** Locomotion verbs: dodge, sprint, dash. */
	Movement  UMETA(DisplayName = "Movement"),
};

/**
 * One row in DT_AbilityCatalog. Row name should be a stable id matching the PoF
 * app's AbilityMeta.id (e.g. "force_push", "light_attack").
 */
USTRUCT(BlueprintType)
struct POF_API FARPGAbilityCatalogRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Human-readable display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FName Name;

	/** Category bucket — drives UI grouping + UE-side filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	EARPGAbilityCategory Category = EARPGAbilityCategory::Melee;

	/** Relative power tier (1 = basic, 5 = most powerful). Drives default sort. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "1", ClampMax = "5"))
	int32 Tier = 1;

	/** Base damage in HP units; 0 for utility/movement abilities. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	/** Short tooltip-style description. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (MultiLine = true))
	FText Description;

	/** The GameplayTag identifying this ability (e.g. Ability.Melee.LightAttack). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FGameplayTag GameplayTag;
};
