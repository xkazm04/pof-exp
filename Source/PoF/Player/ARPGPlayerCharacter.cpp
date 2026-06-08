#include "ARPGPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/WidgetComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/ARPGAbilityUnlockComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/PlayerStart.h"
#include "Loot/ARPGWorldItem.h"
#include "Loot/ARPGLootChest.h"
#include "Dialogue/ARPGDialogueComponent.h"
#include "Dialogue/ARPGNPCActor.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Quest/ARPGQuestTypes.h"
#include "UI/ARPGHUD.h"
#include "GameplayCueManager.h"
#include "HAL/IConsoleManager.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
	// --- Debug Arrow ---
	DebugArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DebugArrow"));
	DebugArrow->SetupAttachment(RootComponent);
	DebugArrow->SetHiddenInGame(false);
	DebugArrow->ArrowSize = 2.0f;
	DebugArrow->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	// --- Overhead widget component (for health bar / name plate) ---
	OverheadWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComp->SetupAttachment(RootComponent);
	OverheadWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	OverheadWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComp->SetDrawAtDesiredSize(true);
	// Widget class is assigned in Blueprint via OverheadWidgetComp->SetWidgetClass(...)

	// --- Perception stimuli source (AI enemies can detect this character) ---
	PerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSource"));
	PerceptionStimuliSource->bAutoRegister = true;
	PerceptionStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuliSource->RegisterForSense(UAISense_Damage::StaticClass());

	// --- Ability Unlock/Upgrade Component ---
	AbilityUnlockComp = CreateDefaultSubobject<UARPGAbilityUnlockComponent>(TEXT("AbilityUnlockComp"));

	// --- Top-down mouse-aim control scheme (ARPG baseline) ---
	// Player faces the cursor, not its movement direction. PLAYER-ONLY: the shared
	// AARPGCharacterBase keeps bOrientRotationToMovement=true for AI/NPCs.
	bCursorAimActive = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AARPGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// GAS owns Health/MaxHealth. Mirror them into the deprecated floats whenever
	// the GAS attributes change, so the legacy float readers + OnHealthChanged
	// listeners stay in lockstep with the source of truth.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
			.AddUObject(this, &AARPGPlayerCharacter::OnGASHealthChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
			.AddUObject(this, &AARPGPlayerCharacter::OnGASHealthChanged);

		// Seed the floats from GAS so the initial broadcast reflects the source of truth.
		Health = ReadGASHealth(/*bMax=*/false, Health);
		MaxHealth = ReadGASHealth(/*bMax=*/true, MaxHealth);
	}

	// Broadcast initial values so any listening UI gets correct state
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnManaChanged.Broadcast(Mana, MaxMana);

	// Enforce the mouse-aim scheme at runtime so a stale Blueprint default can't
	// silently restore face-movement. Feel values stay BP-tunable (read each tick).
	bCursorAimActive = true;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
	}

	// Enable mouse cursor for interaction targeting
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
	}
}

void AARPGPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAlive) return;

	UpdateManaRegen(DeltaTime);
	UpdateCursorAim(DeltaTime);
	UpdateInteractionScan(DeltaTime);
	UpdateDebugDisplay(DeltaTime);
}

// =========================================================================
// Health
// =========================================================================

float AARPGPlayerCharacter::TakeDamageAmount(float DamageAmount)
{
	if (!bIsAlive || DamageAmount <= 0.f) return 0.f;

	// Dodge invulnerability blocks all damage
	if (IsInvulnerable()) return 0.f;

	const float OldHealth = Health;
	Health = FMath::Max(Health - DamageAmount, 0.f);
	const float ActualDamage = OldHealth - Health;

	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.f)
	{
		Die();
	}

	return ActualDamage;
}

float AARPGPlayerCharacter::Heal(float HealAmount)
{
	if (!bIsAlive || HealAmount <= 0.f) return 0.f;

	const float OldHealth = Health;
	Health = FMath::Min(Health + HealAmount, MaxHealth);
	const float ActualHeal = Health - OldHealth;

	if (ActualHeal > 0.f)
	{
		OnHealthChanged.Broadcast(Health, MaxHealth);
	}

	return ActualHeal;
}

// --- GAS-backed health (source of truth = UARPGAttributeSet) ---

float AARPGPlayerCharacter::ReadGASHealth(bool bMax, float Fallback) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayAttribute Attr = bMax
			? UARPGAttributeSet::GetMaxHealthAttribute()
			: UARPGAttributeSet::GetHealthAttribute();
		bool bFound = false;
		const float Val = ASC->GetGameplayAttributeValue(Attr, bFound);
		if (bFound) { return Val; }
	}
	return Fallback; // pre-possession / no ASC yet
}

float AARPGPlayerCharacter::GetHealth() const     { return ReadGASHealth(/*bMax=*/false, Health); }
float AARPGPlayerCharacter::GetMaxHealth() const  { return ReadGASHealth(/*bMax=*/true, MaxHealth); }

float AARPGPlayerCharacter::GetHealthPercent() const
{
	const float Max = GetMaxHealth();
	return Max > 0.f ? GetHealth() / Max : 0.f;
}

void AARPGPlayerCharacter::OnGASHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	// Re-read both from GAS (the handler is shared by Health + MaxHealth changes),
	// mirror into the deprecated floats, and re-broadcast the legacy delegate.
	Health = ReadGASHealth(/*bMax=*/false, Health);
	MaxHealth = ReadGASHealth(/*bMax=*/true, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

// =========================================================================
// Mana
// =========================================================================

bool AARPGPlayerCharacter::SpendMana(float Amount)
{
	if (Amount <= 0.f || Mana < Amount) return false;

	Mana -= Amount;
	OnManaChanged.Broadcast(Mana, MaxMana);
	return true;
}

float AARPGPlayerCharacter::RestoreMana(float Amount)
{
	if (Amount <= 0.f) return 0.f;

	const float OldMana = Mana;
	Mana = FMath::Min(Mana + Amount, MaxMana);
	const float ActualRestore = Mana - OldMana;

	if (ActualRestore > 0.f)
	{
		OnManaChanged.Broadcast(Mana, MaxMana);
	}

	return ActualRestore;
}

// =========================================================================
// Ability Loadout
// =========================================================================

bool AARPGPlayerCharacter::TryActivateAbilitySlot(int32 SlotIndex)
{
	if (!bIsAlive) return false;
	if (SlotIndex < 0 || SlotIndex >= AbilitySlotCount) return false;

	const TSubclassOf<UGameplayAbility>* Found = AbilityLoadout.Find(SlotIndex);
	if (!Found || !*Found) return false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;

	bool bSuccess = ASC->TryActivateAbilityByClass(*Found);

	if (bSuccess)
	{
		OnAbilityActivated.Broadcast(SlotIndex);
		UE_LOG(LogTemp, Log, TEXT("[Loadout] Slot %d activated: %s"), SlotIndex, *(*Found)->GetName());
	}

	return bSuccess;
}

void AARPGPlayerCharacter::AssignAbilityToSlot(int32 SlotIndex, TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (SlotIndex < 0 || SlotIndex >= AbilitySlotCount) return;

	if (AbilityClass)
	{
		AbilityLoadout.Add(SlotIndex, AbilityClass);
	}
	else
	{
		AbilityLoadout.Remove(SlotIndex);
	}

	OnLoadoutChanged.Broadcast(SlotIndex);

	UE_LOG(LogTemp, Log, TEXT("[Loadout] Slot %d = %s"), SlotIndex,
		AbilityClass ? *AbilityClass->GetName() : TEXT("(empty)"));
}

void AARPGPlayerCharacter::ClearAbilitySlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= AbilitySlotCount) return;

	AbilityLoadout.Remove(SlotIndex);
	OnLoadoutChanged.Broadcast(SlotIndex);
}

TSubclassOf<UGameplayAbility> AARPGPlayerCharacter::GetSlotAbility(int32 SlotIndex) const
{
	const TSubclassOf<UGameplayAbility>* Found = AbilityLoadout.Find(SlotIndex);
	return Found ? *Found : nullptr;
}

// =========================================================================
// Interaction
// =========================================================================

void AARPGPlayerCharacter::PerformInteraction()
{
	if (!bIsAlive) return;

	AActor* Target = InteractionTarget.Get();
	if (!Target) return;

	UE_LOG(LogTemp, Log, TEXT("[Interaction] Interacting with: %s"), *Target->GetName());

	// World item pickup
	if (Target->ActorHasTag(FName("WorldItem")))
	{
		if (AARPGWorldItem* WorldItem = Cast<AARPGWorldItem>(Target))
		{
			WorldItem->TryPickup(this);
			return;
		}
	}

	// Chest opening
	if (Target->ActorHasTag(FName("Chest")))
	{
		if (AARPGLootChest* Chest = Cast<AARPGLootChest>(Target))
		{
			Chest->TryOpen(this);
			return;
		}
	}

	// NPC dialogue
	if (UARPGDialogueComponent* DialogueComp = Target->FindComponentByClass<UARPGDialogueComponent>())
	{
		// Report TalkTo event for quest objective tracking
		// Use NPC's NPCID if it's an ARPGNPCActor, otherwise use actor name
		FName TalkTargetID;
		if (AARPGNPCActor* NPC = Cast<AARPGNPCActor>(Target))
		{
			TalkTargetID = NPC->NPCID;
		}
		else
		{
			TalkTargetID = Target->GetFName();
		}

		if (!TalkTargetID.IsNone())
		{
			if (UARPGQuestSubsystem* QS = GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
			{
				QS->ReportEvent(EARPGObjectiveType::TalkTo, TalkTargetID);
			}
		}

		if (DialogueComp->StartDialogue(this))
		{
			// Show dialogue UI via HUD
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				if (AARPGHUD* HUD = Cast<AARPGHUD>(PC->GetHUD()))
				{
					HUD->ShowDialogue(DialogueComp);
				}
			}
		}
		return;
	}
}

// =========================================================================
// Death / Respawn
// =========================================================================

void AARPGPlayerCharacter::Die()
{
	if (!bIsAlive) return;

	bIsAlive = false;
	Health = 0.f;
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnPlayerDeath.Broadcast();

	// Disable movement and collision
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}
	SetActorEnableCollision(false);

	// Hide the mesh (simulate ragdoll/death anim)
	if (GetMesh())
	{
		GetMesh()->SetVisibility(false, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Death] Player has died. Respawning in %.1fs"), RespawnDelay);

	// Schedule respawn
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this,
		&AARPGPlayerCharacter::ExecuteRespawn, RespawnDelay, false);
}

void AARPGPlayerCharacter::OnDeathFromAbility()
{
	if (!bIsAlive) return;

	bIsAlive = false;
	Health = 0.f;
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnPlayerDeath.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("[Death] Player died via GAS. Respawning in %.1fs"), RespawnDelay);

	// Schedule respawn — GA_Death handles movement/collision/ragdoll
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this,
		&AARPGPlayerCharacter::ExecuteRespawn, RespawnDelay, false);
}

void AARPGPlayerCharacter::Respawn(FVector RespawnLocation)
{
	PendingRespawnLocation = RespawnLocation;
	ExecuteRespawn();
}

void AARPGPlayerCharacter::ExecuteRespawn()
{
	// Find respawn location
	FVector SpawnLoc = PendingRespawnLocation;
	if (SpawnLoc.IsZero())
	{
		// Use the default player start
		AActor* PlayerStart = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass());
		if (PlayerStart)
		{
			SpawnLoc = PlayerStart->GetActorLocation();
		}
	}

	// Cancel GA_Death to clear State.Dead tag and release ActivationOwnedTags
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		FGameplayTagContainer DeadTag(ARPGGameplayTags::State_Dead);
		ASC->CancelAbilities(&DeadTag, nullptr, nullptr);
	}

	// Re-enable input (GA_Death disabled it after montage)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->EnableInput(PC);
	}

	// Teleport to spawn
	SetActorLocation(SpawnLoc);

	// Restore state
	bIsAlive = true;
	Health = MaxHealth;
	Mana = MaxMana;

	// Re-enable movement and collision
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}
	SetActorEnableCollision(true);

	// Restore mesh from ragdoll
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetAllBodiesSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComp->AttachToComponent(GetRootComponent(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		MeshComp->SetRelativeLocationAndRotation(
			FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
		MeshComp->SetVisibility(true, true);
	}

	// Reinitialize attributes to full values
	InitializeAttributes();

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnManaChanged.Broadcast(Mana, MaxMana);
	OnPlayerRespawn.Broadcast();

	PendingRespawnLocation = FVector::ZeroVector;

	UE_LOG(LogTemp, Log, TEXT("[Respawn] Player respawned at %s"), *SpawnLoc.ToString());
}

// =========================================================================
// Level / XP
// =========================================================================

void AARPGPlayerCharacter::AddExperience(float Amount)
{
	if (Amount <= 0.f || !bIsAlive) return;

	// Max level cap
	static constexpr int32 MaxLevel = 50;
	if (PlayerLevel >= MaxLevel) return;

	Experience += Amount;

	// Check for level ups (loop handles multi-level jumps from large XP gains)
	while (Experience >= ExperienceToNextLevel && PlayerLevel < MaxLevel)
	{
		Experience -= ExperienceToNextLevel;
		LevelUp();
	}

	// Cap excess XP at max level
	if (PlayerLevel >= MaxLevel)
	{
		Experience = 0.f;
	}

	// Sync GAS attributes to match local authoritative state
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetCurrentXPAttribute(), EGameplayModOp::Override, Experience);
	}
}

void AARPGPlayerCharacter::LevelUp()
{
	PlayerLevel++;

	// Boost stats
	MaxHealth += HealthPerLevel;
	Health = MaxHealth; // Full heal on level up
	MaxMana += ManaPerLevel;
	Mana = MaxMana; // Full mana restore on level up

	// Scale XP requirement
	ExperienceToNextLevel = CalculateXPForLevel(PlayerLevel);

	// Award skill points
	if (AbilityUnlockComp)
	{
		AbilityUnlockComp->AddSkillPoints(AbilityUnlockComp->GetSkillPointsPerLevel());
	}

	// Award attribute points
	UnspentAttributePoints += AttributePointsPerLevel;
	OnAttributePointsChanged.Broadcast(UnspentAttributePoints);

	// Sync GAS attributes
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetCharacterLevelAttribute(), EGameplayModOp::Override, static_cast<float>(PlayerLevel));
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetXPToNextLevelAttribute(), EGameplayModOp::Override, ExperienceToNextLevel);
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, MaxHealth);
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, Health);
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetMaxManaAttribute(), EGameplayModOp::Override, MaxMana);
		ASC->ApplyModToAttribute(UARPGAttributeSet::GetManaAttribute(), EGameplayModOp::Override, Mana);

		// Fire level-up GameplayCue for VFX/SFX
		FGameplayCueParameters CueParams;
		CueParams.RawMagnitude = static_cast<float>(PlayerLevel);
		ASC->ExecuteGameplayCue(ARPGGameplayTags::GameplayCue_LevelUp, CueParams);
	}

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnManaChanged.Broadcast(Mana, MaxMana);
	OnPlayerLevelUp.Broadcast(PlayerLevel);

	UE_LOG(LogTemp, Log, TEXT("[LevelUp] Player reached level %d (HP: %.0f, MP: %.0f, NextXP: %.0f, AttrPts: %d)"),
		PlayerLevel, MaxHealth, MaxMana, ExperienceToNextLevel, UnspentAttributePoints);
}

// =========================================================================
// Attribute Point Allocation
// =========================================================================

bool AARPGPlayerCharacter::AllocateStrength()
{
	if (UnspentAttributePoints <= 0) return false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;

	ASC->ApplyModToAttribute(UARPGAttributeSet::GetStrengthAttribute(), EGameplayModOp::Additive, 1.f);
	ASC->ApplyModToAttribute(UARPGAttributeSet::GetAttackPowerAttribute(), EGameplayModOp::Additive, AttackPowerPerStrength);

	AllocatedStrength++;
	UnspentAttributePoints--;
	OnAttributePointsChanged.Broadcast(UnspentAttributePoints);

	UE_LOG(LogTemp, Log, TEXT("[Attributes] Allocated Strength (%d total). +%.0f AttackPower. %d points remaining."),
		AllocatedStrength, AttackPowerPerStrength, UnspentAttributePoints);
	return true;
}

bool AARPGPlayerCharacter::AllocateDexterity()
{
	if (UnspentAttributePoints <= 0) return false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;

	ASC->ApplyModToAttribute(UARPGAttributeSet::GetDexterityAttribute(), EGameplayModOp::Additive, 1.f);
	ASC->ApplyModToAttribute(UARPGAttributeSet::GetCriticalChanceAttribute(), EGameplayModOp::Additive, CritChancePerDexterity);

	AllocatedDexterity++;
	UnspentAttributePoints--;
	OnAttributePointsChanged.Broadcast(UnspentAttributePoints);

	UE_LOG(LogTemp, Log, TEXT("[Attributes] Allocated Dexterity (%d total). +%.1f%% CritChance. %d points remaining."),
		AllocatedDexterity, CritChancePerDexterity * 100.f, UnspentAttributePoints);
	return true;
}

bool AARPGPlayerCharacter::AllocateIntelligence()
{
	if (UnspentAttributePoints <= 0) return false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;

	ASC->ApplyModToAttribute(UARPGAttributeSet::GetIntelligenceAttribute(), EGameplayModOp::Additive, 1.f);
	ASC->ApplyModToAttribute(UARPGAttributeSet::GetMaxManaAttribute(), EGameplayModOp::Additive, MaxManaPerIntelligence);

	AllocatedIntelligence++;
	UnspentAttributePoints--;
	OnAttributePointsChanged.Broadcast(UnspentAttributePoints);

	UE_LOG(LogTemp, Log, TEXT("[Attributes] Allocated Intelligence (%d total). +%.0f MaxMana. %d points remaining."),
		AllocatedIntelligence, MaxManaPerIntelligence, UnspentAttributePoints);
	return true;
}

float AARPGPlayerCharacter::CalculateXPForLevel(int32 Level) const
{
	// Base 100 XP * level^exponent curve
	return 100.f * FMath::Pow(static_cast<float>(Level), ExperienceCurveExponent);
}

// =========================================================================
// Internal update functions
// =========================================================================

void AARPGPlayerCharacter::UpdateManaRegen(float DeltaTime)
{
	if (Mana < MaxMana && ManaRegenPerSec > 0.f)
	{
		const float OldMana = Mana;
		Mana = FMath::Min(Mana + ManaRegenPerSec * DeltaTime, MaxMana);
		if (FMath::Abs(Mana - OldMana) > KINDA_SMALL_NUMBER)
		{
			OnManaChanged.Broadcast(Mana, MaxMana);
		}
	}
}

void AARPGPlayerCharacter::UpdateCursorAim(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// Cursor world position: a fixed override (test/AI seam) wins; otherwise trace under the mouse.
	if (bUseCursorWorldOverride)
	{
		CursorWorldLocation = CursorWorldOverride;
	}
	else
	{
		// Robust top-down aim: deproject the mouse onto a horizontal plane at the character's
		// height (GetHitResultUnderCursor was fragile — in a top-down view it hit the pawn's
		// own capsule). A screen-space override (test seam) feeds the SAME deproject path with
		// a deterministic input, so the contract suite exercises this real code, not a proxy.
		float MouseX = 0.f, MouseY = 0.f;
		bool bHaveScreen;
		if (bUseCursorScreenOverride)
		{
			MouseX = CursorScreenOverride.X;
			MouseY = CursorScreenOverride.Y;
			bHaveScreen = true;
		}
		else
		{
			bHaveScreen = PC->GetMousePosition(MouseX, MouseY);
		}
		FVector WorldOrigin, WorldDir;
		if (bHaveScreen &&
			PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDir) &&
			!FMath::IsNearlyZero(WorldDir.Z))
		{
			const float PlaneZ = GetActorLocation().Z;
			const float T = (PlaneZ - WorldOrigin.Z) / WorldDir.Z;
			if (T > 0.f)
			{
				CursorWorldLocation = WorldOrigin + WorldDir * T;
			}
		}
	}

	// Rotate character toward cursor only when cursor aim is active
	if (!bCursorAimActive) return;

	// Don't fight the dodge lunge — hold the roll's facing while dodging.
	if (IsDodging()) return;

	FVector Direction = CursorWorldLocation - GetActorLocation();
	Direction.Z = 0.f;

	if (Direction.SizeSquared() > 100.f) // Avoid jitter when cursor is on character
	{
		const FRotator TargetRot = Direction.Rotation();
		const FRotator CurrentRot = GetActorRotation();
		const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, CursorAimRotationSpeed);
		SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
	}
}

void AARPGPlayerCharacter::UpdateInteractionScan(float DeltaTime)
{
	InteractionScanTimer -= DeltaTime;
	if (InteractionScanTimer > 0.f) return;
	InteractionScanTimer = InteractionScanRate;

	// Sphere overlap to find nearest interactable actor
	const FVector Origin = GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionRange);

	AActor* BestTarget = nullptr;
	float BestDistSq = InteractionRange * InteractionRange;

	if (GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Visibility, Sphere))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Other = Overlap.GetActor();
			if (!Other || Other == this) continue;

			// Check if the actor has the "Interactable" tag
			if (!Other->ActorHasTag(FName("Interactable"))) continue;

			const float DistSq = FVector::DistSquared(Origin, Other->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = Other;
			}
		}
	}

	InteractionTarget = BestTarget;
}

void AARPGPlayerCharacter::UpdateDebugDisplay(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	if (!GEngine) return;

	// HUD/UI folder-04 §6: gated off by default — see ARPG.ShowDebugStats
	// (registered in ARPGCharacterBase.cpp). Found once and cached.
	static IConsoleVariable* CVarShowDebugStats =
		IConsoleManager::Get().FindConsoleVariable(TEXT("ARPG.ShowDebugStats"));
	if (CVarShowDebugStats && CVarShowDebugStats->GetInt() == 0) return;

	// Health/Mana bar
	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red,
		FString::Printf(TEXT("HP: %.0f / %.0f  |  MP: %.0f / %.0f"),
			Health, MaxHealth, Mana, MaxMana));

	// Level/XP
	GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Yellow,
		FString::Printf(TEXT("Lv %d  XP: %.0f / %.0f"),
			PlayerLevel, Experience, ExperienceToNextLevel));

	// Interaction target
	if (AActor* Target = InteractionTarget.Get())
	{
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Green,
			FString::Printf(TEXT("[F] Interact: %s"), *Target->GetName()));
	}

	// Alive state
	if (!bIsAlive)
	{
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Red, TEXT("** DEAD **"));
	}

	// Cursor aim
	if (bCursorAimActive)
	{
		GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Cyan, TEXT("[CURSOR AIM]"));
	}
#endif
}
