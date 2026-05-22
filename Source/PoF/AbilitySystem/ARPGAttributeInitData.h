#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ARPGAttributeInitData.generated.h"

/**
 * Data Table row for per-level attribute initialization.
 *
 * Create a Data Table asset (DT_AttributeDefaults) with this row struct.
 * Each row represents one character archetype (e.g., "Player", "Skeleton", "Boss").
 *
 * Base values are the Level 1 values. Per-level scaling is done via a Curve Table:
 *   FinalValue = BaseValue * CurveTable->Eval(Level)
 *
 * If no Curve Table is assigned, BaseValue is used directly.
 */
USTRUCT(BlueprintType)
struct POF_API FARPGAttributeInitRow : public FTableRowBase
{
	GENERATED_BODY()

	// --- Vitals ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	float Mana = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	float MaxMana = 50.f;

	// --- Primary Stats ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary")
	float Strength = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary")
	float Dexterity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary")
	float Intelligence = 10.f;

	// --- Combat ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Armor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CriticalChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CriticalDamage = 1.5f;

	// --- Elemental Resistances (0.0 = none, 0.9 max) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance")
	float FireResistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance")
	float IceResistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance")
	float LightningResistance = 0.f;

	// --- Progression ---

	/** Starting XP (typically 0 for a fresh character). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float CurrentXP = 0.f;

	/** Starting character level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float CharacterLevel = 1.f;
};

/**
 * Per-level scaling curve names used in the Curve Table.
 *
 * Create a Curve Table asset (CT_LevelScaling) with these row names:
 *   - Health, MaxHealth, Mana, MaxMana
 *   - Strength, Dexterity, Intelligence
 *   - Armor, AttackPower, CriticalChance, CriticalDamage
 *
 * A separate Curve Table (CT_XPRequirements) defines XP per level:
 *   Row name: "XPToNextLevel", X = level, Y = absolute XP required.
 *
 * X axis = character level, Y axis = multiplier applied to the base value.
 * Example curve for MaxHealth: (1, 1.0), (5, 1.5), (10, 2.5), (20, 5.0)
 */
namespace ARPGCurveRowNames
{
	const FName Health          = TEXT("Health");
	const FName MaxHealth       = TEXT("MaxHealth");
	const FName Mana            = TEXT("Mana");
	const FName MaxMana         = TEXT("MaxMana");
	const FName Strength        = TEXT("Strength");
	const FName Dexterity       = TEXT("Dexterity");
	const FName Intelligence    = TEXT("Intelligence");
	const FName Armor           = TEXT("Armor");
	const FName AttackPower     = TEXT("AttackPower");
	const FName CriticalChance  = TEXT("CriticalChance");
	const FName CriticalDamage  = TEXT("CriticalDamage");

	const FName FireResistance  = TEXT("FireResistance");
	const FName IceResistance   = TEXT("IceResistance");
	const FName LightningResistance = TEXT("LightningResistance");

	/** Row name in a dedicated XP Curve Table. X = level, Y = XP required for that level. */
	const FName XPToNextLevel   = TEXT("XPToNextLevel");
}
