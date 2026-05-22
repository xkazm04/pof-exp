#include "AI/ARPGSquadManager.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UARPGSquadManager::UARPGSquadManager()
{
}

void UARPGSquadManager::CleanupStaleMembers()
{
	Members.RemoveAll([](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr) { return !Ptr.IsValid(); });
	AttackTokenHolders.RemoveAll([](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr) { return !Ptr.IsValid(); });
}

void UARPGSquadManager::RegisterMember(AARPGEnemyCharacter* Enemy)
{
	if (!Enemy) return;

	CleanupStaleMembers();

	// Avoid duplicates
	for (const auto& Existing : Members)
	{
		if (Existing.Get() == Enemy) return;
	}

	Members.Add(Enemy);
}

void UARPGSquadManager::UnregisterMember(AARPGEnemyCharacter* Enemy)
{
	if (!Enemy) return;

	Members.RemoveAll([Enemy](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr) { return Ptr.Get() == Enemy; });
	AttackTokenHolders.RemoveAll([Enemy](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr) { return Ptr.Get() == Enemy; });
}

TArray<AARPGEnemyCharacter*> UARPGSquadManager::GetAliveMembers() const
{
	TArray<AARPGEnemyCharacter*> Result;
	for (const auto& Ptr : Members)
	{
		AARPGEnemyCharacter* Enemy = Ptr.Get();
		if (Enemy && !Enemy->IsDead())
		{
			Result.Add(Enemy);
		}
	}
	return Result;
}

bool UARPGSquadManager::RequestAttackToken(AARPGEnemyCharacter* Requester)
{
	if (!Requester) return false;

	CleanupStaleMembers();

	// Already has a token
	if (HasAttackToken(Requester)) return true;

	// Remove dead holders
	AttackTokenHolders.RemoveAll([](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr)
	{
		AARPGEnemyCharacter* E = Ptr.Get();
		return !E || E->IsDead();
	});

	if (AttackTokenHolders.Num() < MaxSimultaneousAttackers)
	{
		AttackTokenHolders.Add(Requester);
		return true;
	}

	return false;
}

void UARPGSquadManager::ReleaseAttackToken(AARPGEnemyCharacter* Holder)
{
	if (!Holder) return;

	AttackTokenHolders.RemoveAll([Holder](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr) { return Ptr.Get() == Holder; });
}

bool UARPGSquadManager::HasAttackToken(AARPGEnemyCharacter* Enemy) const
{
	if (!Enemy) return false;

	for (const auto& Ptr : AttackTokenHolders)
	{
		if (Ptr.Get() == Enemy) return true;
	}
	return false;
}

FVector UARPGSquadManager::GetFlankPosition(AARPGEnemyCharacter* Enemy, AActor* Target, float DesiredDistance) const
{
	if (!Enemy || !Target)
	{
		return Enemy ? Enemy->GetActorLocation() : FVector::ZeroVector;
	}

	// Get alive members for this calculation
	TArray<AARPGEnemyCharacter*> AliveMembers = GetAliveMembers();
	if (AliveMembers.IsEmpty())
	{
		return Enemy->GetActorLocation();
	}

	// Find this enemy's index in the alive members list
	int32 MyIndex = AliveMembers.IndexOfByKey(Enemy);
	if (MyIndex == INDEX_NONE)
	{
		MyIndex = 0;
	}

	// Spread enemies evenly around the target
	const int32 NumMembers = AliveMembers.Num();
	const float AngleStep = 360.f / static_cast<float>(NumMembers);
	const float AngleDeg = AngleStep * MyIndex;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	const FVector TargetPos = Target->GetActorLocation();
	const FVector FlankOffset(
		FMath::Cos(AngleRad) * DesiredDistance,
		FMath::Sin(AngleRad) * DesiredDistance,
		0.f
	);

	return TargetPos + FlankOffset;
}

void UARPGSquadManager::SetFocusTarget(AActor* NewTarget)
{
	FocusTarget = NewTarget;
}

bool UARPGSquadManager::SignalRetreat(AARPGEnemyCharacter* Retreater)
{
	if (!Retreater) return false;

	CleanupStaleMembers();

	const TArray<AARPGEnemyCharacter*> AliveMembers = GetAliveMembers();
	if (AliveMembers.IsEmpty()) return false;

	// Count how many members are low HP
	int32 LowHPCount = 0;
	for (AARPGEnemyCharacter* Member : AliveMembers)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Member))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				const UARPGAttributeSet* Attrs = ASC->GetSet<UARPGAttributeSet>();
				if (Attrs && Attrs->GetMaxHealth() > 0.f)
				{
					const float Ratio = Attrs->GetHealth() / Attrs->GetMaxHealth();
					if (Ratio < RetreatHealthThreshold)
					{
						++LowHPCount;
					}
				}
			}
		}
	}

	const float LowHPFraction = static_cast<float>(LowHPCount) / static_cast<float>(AliveMembers.Num());
	return LowHPFraction >= CoordinatedRetreatFraction;
}
