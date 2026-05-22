#pragma once

#include "Engine/EngineTypes.h"

/**
 * Custom collision channels matching DefaultEngine.ini definitions.
 * ECC_GameTraceChannel1..4 map to the named channels configured in project settings.
 */
namespace ARPGCollision
{
	/** Projectile channel — overlaps pawns, blocks world, ignores other projectiles. */
	constexpr ECollisionChannel Projectile = ECC_GameTraceChannel1;

	/** Interactable channel — physics objects the player can grab/interact with. */
	constexpr ECollisionChannel Interactable = ECC_GameTraceChannel2;

	/** Weapon trace channel — used for melee sweep and line traces. */
	constexpr ECollisionChannel WeaponTrace = ECC_GameTraceChannel3;

	/** Destructible channel — breakable objects that respond to damage and impacts. */
	constexpr ECollisionChannel Destructible = ECC_GameTraceChannel4;

	/** Profile name constants matching the collision profile names in DefaultEngine.ini. */
	namespace Profiles
	{
		inline const FName Projectile   = TEXT("Projectile");
		inline const FName Interactable = TEXT("Interactable");
		inline const FName WeaponTrace  = TEXT("WeaponTrace");
		inline const FName Destructible = TEXT("Destructible");
	}
}
