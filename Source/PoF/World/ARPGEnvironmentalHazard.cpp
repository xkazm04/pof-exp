#include "World/ARPGEnvironmentalHazard.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Damage.h"
#include "Character/ARPGCharacterBase.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "NavModifierComponent.h"
#include "World/NavArea_Hazard.h"
#include "World/ARPGZoneManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AARPGEnvironmentalHazard::AARPGEnvironmentalHazard()
{
	PrimaryActorTick.bCanEverTick = false;

	DamageVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageVolume"));
	DamageVolume->SetBoxExtent(FVector(200.f, 200.f, 50.f));
	DamageVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	DamageVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(DamageVolume);

	HazardVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HazardVFX"));
	HazardVFX->SetupAttachment(DamageVolume);
	HazardVFX->SetAutoActivate(false);

	HazardNavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("HazardNavModifier"));
}

void AARPGEnvironmentalHazard::BeginPlay()
{
	Super::BeginPlay();

	DamageVolume->OnComponentBeginOverlap.AddDynamic(this, &AARPGEnvironmentalHazard::OnVolumeBeginOverlap);
	DamageVolume->OnComponentEndOverlap.AddDynamic(this, &AARPGEnvironmentalHazard::OnVolumeEndOverlap);

	if (HazardVFXSystem)
	{
		HazardVFX->SetAsset(HazardVFXSystem);
	}

	// Configure NavModifier for AI avoidance
	if (HazardNavModifier)
	{
		if (bAIAvoidance)
		{
			HazardNavModifier->SetAreaClass(UNavArea_Hazard::StaticClass());
		}
		else
		{
			HazardNavModifier->SetAreaClass(nullptr);
		}
	}

	// Auto-set damage type from hazard type if not configured
	if (!DamageTypeTag.IsValid())
	{
		DamageTypeTag = GetDefaultDamageTag();
	}

	// Detect actors already overlapping on level load
	{
		TArray<AActor*> AlreadyOverlapping;
		DamageVolume->GetOverlappingActors(AlreadyOverlapping);
		for (AActor* Actor : AlreadyOverlapping)
		{
			if (Actor && Actor != this && Cast<IAbilitySystemInterface>(Actor))
			{
				if (!bDamagePlayerOnly || Actor == UGameplayStatics::GetPlayerPawn(this, 0))
				{
					OverlappingActors.AddUnique(Actor);
				}
			}
		}
	}

	// One-shot and cycling are mutually exclusive — cycling overrides one-shot
	if (bCycleOnOff)
	{
		bOneShot = false;

		// Start cycle with phase offset
		FTimerDelegate CycleDel;
		CycleDel.BindUObject(this, &AARPGEnvironmentalHazard::OnCycleTimer);
		GetWorldTimerManager().SetTimer(CycleTimerHandle, CycleDel, CyclePhaseOffset > 0.f ? CyclePhaseOffset : 0.01f, false);
	}
	else if (bStartActive)
	{
		ActivateHazard();
	}
}

void AARPGEnvironmentalHazard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	GetWorldTimerManager().ClearTimer(CycleTimerHandle);
	GetWorldTimerManager().ClearTimer(RearmTimerHandle);
	GetWorldTimerManager().ClearTimer(WarningTimerHandle);

	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AARPGEnvironmentalHazard::ActivateHazard()
{
	if (bIsActive) return;

	bIsActive = true;
	bIsRearming = false;

	HazardVFX->Activate(true);

	// Play activation sound once (not per-tick). For looping ambient SFX,
	// assign a looping USoundBase — it will auto-loop via the audio component.
	if (HazardSound && !bOneShot)
	{
		if (!LoopingSoundComponent)
		{
			LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(
				HazardSound, DamageVolume, NAME_None,
				FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
				false, 0.5f, 1.f, 0.f, nullptr, nullptr, false);
		}
		else
		{
			LoopingSoundComponent->Play();
		}
	}

	if (!bOneShot)
	{
		GetWorldTimerManager().SetTimer(
			DamageTimerHandle,
			this, &AARPGEnvironmentalHazard::ApplyDamageToOverlapping,
			DamageInterval,
			true,
			FMath::Max(EntryGracePeriod, 0.01f));
	}

	UE_LOG(LogTemp, Log, TEXT("[Hazard] %s activated"), *GetName());
}

void AARPGEnvironmentalHazard::DeactivateHazard()
{
	if (!bIsActive) return;

	bIsActive = false;
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	HazardVFX->Deactivate();

	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
	}

	UE_LOG(LogTemp, Log, TEXT("[Hazard] %s deactivated"), *GetName());
}

void AARPGEnvironmentalHazard::ToggleHazard()
{
	if (bIsActive)
	{
		DeactivateHazard();
	}
	else
	{
		ActivateHazard();
	}
}

void AARPGEnvironmentalHazard::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OtherActor);
	if (!ASI) return;

	if (bDamagePlayerOnly)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (OtherActor != PlayerPawn) return;
	}

	OverlappingActors.AddUnique(OtherActor);

	// One-shot: trigger immediately on entry (skip if actor is immune from recent hit)
	if (bOneShot && bIsActive && !bIsRearming && !OneShotImmune.Contains(OtherActor))
	{
		OneShotImmune.Add(OtherActor);
		ApplyDamageToActor(OtherActor);

		bIsRearming = true;
		DeactivateHazard();
		GetWorldTimerManager().SetTimer(RearmTimerHandle, this, &AARPGEnvironmentalHazard::OnRearm, RearmDelay, false);
	}
}

void AARPGEnvironmentalHazard::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	OverlappingActors.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == OtherActor;
	});

	// Clear one-shot immunity when actor leaves — allows re-trigger on re-entry
	OneShotImmune.Remove(OtherActor);

	// Clear debuff tracking — allows debuff re-application on re-entry
	DebuffedActors.Remove(OtherActor);
}

void AARPGEnvironmentalHazard::ApplyDamageToOverlapping()
{
	if (!bIsActive) return;

	// Clean stale references
	OverlappingActors.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });

	APawn* PlayerPawn = bDamagePlayerOnly ? UGameplayStatics::GetPlayerPawn(this, 0) : nullptr;

	for (const TWeakObjectPtr<AActor>& ActorPtr : OverlappingActors)
	{
		if (ActorPtr.IsValid())
		{
			if (bDamagePlayerOnly && ActorPtr.Get() != PlayerPawn) continue;
			ApplyDamageToActor(ActorPtr.Get());
		}
	}
}

void AARPGEnvironmentalHazard::ApplyDamageToActor(AActor* Target)
{
	if (!Target) return;

	// Don't damage dead characters
	if (const AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(Target))
	{
		if (Character->IsDead()) return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	if (!ASI) return;

	UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
	if (!TargetASC) return;

	TSubclassOf<UGameplayEffect> EffectClass = DamageEffectClass;
	if (!EffectClass)
	{
		EffectClass = UGE_Damage::StaticClass();
	}

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (!SpecHandle.IsValid()) return;

	// Set damage via SetByCaller, optionally scaled by zone difficulty
	float FinalDamage = DamagePerTick;
	if (bScaleWithZoneDifficulty)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UARPGZoneManager* ZoneMgr = GI->GetSubsystem<UARPGZoneManager>())
			{
				FinalDamage *= ZoneMgr->GetCurrentZoneDifficulty();
			}
		}
	}
	SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, FinalDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Scaling, 0.f);

	// Tag with damage type
	if (DamageTypeTag.IsValid())
	{
		SpecHandle.Data->AddDynamicAssetTag(DamageTypeTag);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// One-shot traps play a hit sound per activation (not looped)
	if (bOneShot && HazardSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HazardSound, GetActorLocation(), 0.5f);
	}

	// Apply optional debuff (slow, DoT, etc.)
	ApplyDebuffToActor(Target);
}

void AARPGEnvironmentalHazard::ApplyDebuffToActor(AActor* Target)
{
	if (!DebuffEffectClass || !Target) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	if (!ASI) return;

	UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
	if (!TargetASC) return;

	// Skip if we already applied the debuff and it's still tracked (avoid spam stacking)
	TWeakObjectPtr<AActor> TargetWeak(Target);
	if (DebuffedActors.Contains(TargetWeak))
	{
		return;
	}
	DebuffedActors.Add(TargetWeak);

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DebuffEffectClass, 1.f, Context);
	if (SpecHandle.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AARPGEnvironmentalHazard::OnCycleTimer()
{
	if (bIsActive)
	{
		DeactivateHazard();
		// After off duration, either show warning or activate directly
		const float OffTime = FMath::Max(CycleOffDuration - WarningDuration, 0.1f);
		if (WarningDuration > 0.f)
		{
			GetWorldTimerManager().SetTimer(CycleTimerHandle, this, &AARPGEnvironmentalHazard::OnCycleWarningDone, OffTime, false);
		}
		else
		{
			GetWorldTimerManager().SetTimer(CycleTimerHandle, this, &AARPGEnvironmentalHazard::OnCycleTimer, CycleOffDuration, false);
		}
	}
	else
	{
		ActivateHazard();
		GetWorldTimerManager().SetTimer(CycleTimerHandle, this, &AARPGEnvironmentalHazard::OnCycleTimer, CycleOnDuration, false);
	}
}

void AARPGEnvironmentalHazard::OnCycleWarningDone()
{
	// Play warning VFX/SFX
	if (WarningVFXSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, WarningVFXSystem, GetActorLocation());
	}
	if (WarningSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WarningSound, GetActorLocation(), 0.7f);
	}

	// After warning duration, activate
	GetWorldTimerManager().SetTimer(WarningTimerHandle, this, &AARPGEnvironmentalHazard::OnCycleTimer, WarningDuration, false);
}

void AARPGEnvironmentalHazard::OnRearm()
{
	bIsRearming = false;
	ActivateHazard();
	UE_LOG(LogTemp, Log, TEXT("[Hazard] %s rearmed"), *GetName());
}

FGameplayTag AARPGEnvironmentalHazard::GetDefaultDamageTag() const
{
	switch (HazardType)
	{
	case EHazardType::FireFloor:   return ARPGGameplayTags::Damage_Fire;
	case EHazardType::PoisonCloud: return ARPGGameplayTags::Damage_Poison;
	case EHazardType::SpikeTrap:   return ARPGGameplayTags::Damage_Physical;
	case EHazardType::IceField:    return ARPGGameplayTags::Damage_Ice;
	default:                       return ARPGGameplayTags::Damage_Physical;
	}
}
