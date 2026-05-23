#include "Animation/AnimNotifyState_TestDamage.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EngineUtils.h"
#include "GameplayTagContainer.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Damage.h"

FString UAnimNotifyState_TestDamage::GetNotifyName_Implementation() const
{
	return TEXT("ARPG Test Damage");
}

void UAnimNotifyState_TestDamage::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!Owner || !World)
	{
		return;
	}

	AARPGEnemyCharacter* Enemy = nullptr;
	for (TActorIterator<AARPGEnemyCharacter> It(World); It; ++It)
	{
		Enemy = *It;
		break;
	}
	if (!Enemy)
	{
		return;
	}

	if (bSendMeleeHitEvent)
	{
		FGameplayEventData Payload;
		Payload.Target = Enemy;
		Payload.Instigator = Owner;
		Payload.EventMagnitude = DamageAmount;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, ARPGGameplayTags::Event_MeleeHit, Payload);
	}

	if (bApplyDirectDamage)
	{
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
		if (SourceASC && TargetASC)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddSourceObject(Owner);
			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Context);
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, DamageAmount);
				Spec.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			}
		}
	}
}
