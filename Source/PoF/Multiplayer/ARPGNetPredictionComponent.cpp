#include "ARPGNetPredictionComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/ARPGCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UARPGNetPredictionComponent::UARPGNetPredictionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UARPGNetPredictionComponent::BeginPlay()
{
	Super::BeginPlay();

	MoveBuffer.Reserve(MoveBufferSize);

	// Only the owning client needs to tick for prediction
	AActor* Owner = GetOwner();
	if (!Owner || Owner->HasAuthority())
	{
		// Server doesn't predict — it's authoritative
		// But server still needs to tick to process incoming predictions
		if (!Owner || !Owner->HasAuthority())
		{
			PrimaryComponentTick.SetTickFunctionEnable(false);
		}
	}
}

void UARPGNetPredictionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (Owner->HasAuthority())
	{
		// Server: no prediction needed
		return;
	}

	// Client: record predicted state and periodically send to server
	RecordMoveState();

	MoveSendTimer += DeltaTime;
	if (MoveSendTimer >= MoveSendRate)
	{
		MoveSendTimer = 0.f;
		SendPredictedMoveToServer();
	}

	UpdateCorrectionSmoothing(DeltaTime);
	CleanupTimedOutAbilities();
}

// =====================================================================
// Move Prediction
// =====================================================================

void UARPGNetPredictionComponent::RecordMoveState()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	ACharacter* Character = Cast<ACharacter>(Owner);

	FARPGPredictedMoveState State;
	State.SequenceNumber = NextMoveSequence++;
	State.Position = Owner->GetActorLocation();
	State.Velocity = Character ? Character->GetVelocity() : FVector::ZeroVector;
	State.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Get current input from the movement component
	if (Character && Character->GetCharacterMovement())
	{
		FVector LastInput = Character->GetCharacterMovement()->GetLastInputVector();
		State.InputVector = FVector2D(LastInput.X, LastInput.Y);
	}

	MoveBuffer.Add(State);

	// Trim buffer
	while (MoveBuffer.Num() > MoveBufferSize)
	{
		MoveBuffer.RemoveAt(0);
	}
}

void UARPGNetPredictionComponent::SendPredictedMoveToServer()
{
	if (MoveBuffer.Num() == 0) return;

	const FARPGPredictedMoveState& Latest = MoveBuffer.Last();
	Server_SendPredictedMove(
		Latest.SequenceNumber,
		FVector_NetQuantize(Latest.Position),
		FVector_NetQuantize(Latest.Velocity),
		Latest.InputVector,
		Latest.Timestamp
	);
}

// =====================================================================
// Ability Prediction
// =====================================================================

int32 UARPGNetPredictionComponent::PredictAbilityActivation(FGameplayTag AbilityTag)
{
	if (!AbilityTag.IsValid()) return 0;
	if (PendingAbilities.Num() >= MaxPendingAbilities) return 0;

	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return 0;

	UAbilitySystemComponent* ASC = CharBase->GetAbilitySystemComponent();
	if (!ASC) return 0;

	uint32 Seq = NextAbilitySequence++;

	// Record the prediction
	FARPGPredictedAbilityAction Action;
	Action.SequenceNumber = Seq;
	Action.AbilityTag = AbilityTag;
	Action.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	PendingAbilities.Add(Action);

	// Optimistically activate locally for responsiveness
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);
	ASC->TryActivateAbilitiesByTag(TagContainer);

	// Send to server for authoritative validation
	Server_SendPredictedAbility(Seq, AbilityTag, Action.Timestamp);

	return Seq;
}

void UARPGNetPredictionComponent::OnAbilityConfirmed(uint32 SequenceNumber)
{
	PendingAbilities.RemoveAll([SequenceNumber](const FARPGPredictedAbilityAction& A)
	{
		return A.SequenceNumber == SequenceNumber;
	});
}

void UARPGNetPredictionComponent::OnAbilityRejected(uint32 SequenceNumber, FGameplayTag AbilityTag)
{
	PendingAbilities.RemoveAll([SequenceNumber](const FARPGPredictedAbilityAction& A)
	{
		return A.SequenceNumber == SequenceNumber;
	});

	// Cancel the locally predicted ability
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	UAbilitySystemComponent* ASC = CharBase->GetAbilitySystemComponent();
	if (!ASC) return;

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);
	ASC->CancelAbilities(&TagContainer);

	UE_LOG(LogNet, Verbose, TEXT("NetPrediction: Ability %s rejected by server (seq %u)"), *AbilityTag.ToString(), SequenceNumber);
}

void UARPGNetPredictionComponent::CleanupTimedOutAbilities()
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	for (int32 i = PendingAbilities.Num() - 1; i >= 0; --i)
	{
		if (CurrentTime - PendingAbilities[i].Timestamp > AbilityPredictionTimeout)
		{
			UE_LOG(LogNet, Warning, TEXT("NetPrediction: Ability prediction seq %u timed out"), PendingAbilities[i].SequenceNumber);
			OnAbilityRejected(PendingAbilities[i].SequenceNumber, PendingAbilities[i].AbilityTag);
		}
	}
}

// =====================================================================
// Position Reconciliation
// =====================================================================

void UARPGNetPredictionComponent::ApplyServerCorrection(const FARPGServerCorrection& Correction)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Find the predicted state that matches the acknowledged sequence
	int32 AckIndex = -1;
	for (int32 i = 0; i < MoveBuffer.Num(); ++i)
	{
		if (MoveBuffer[i].SequenceNumber == Correction.AcknowledgedSequence)
		{
			AckIndex = i;
			break;
		}
	}

	// Discard all moves up to and including the acknowledged one
	if (AckIndex >= 0)
	{
		MoveBuffer.RemoveAt(0, AckIndex + 1);
	}

	// Check error between server position and our predicted position at that time
	float Error = FVector::Dist(Owner->GetActorLocation(), FVector(Correction.CorrectedPosition));

	if (Error <= PositionErrorTolerance)
	{
		// Within tolerance — prediction was accurate
		return;
	}

	if (Error > HardSnapThreshold)
	{
		// Too far off — hard snap
		Owner->SetActorLocation(FVector(Correction.CorrectedPosition));
		ACharacter* Character = Cast<ACharacter>(Owner);
		if (Character && Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->Velocity = FVector(Correction.CorrectedVelocity);
		}
		bIsCorrecting = false;
		UE_LOG(LogNet, Log, TEXT("NetPrediction: Hard snap correction (error=%.1f)"), Error);
	}
	else
	{
		// Smooth correction
		CorrectionStart = Owner->GetActorLocation();
		CorrectionTarget = FVector(Correction.CorrectedPosition);
		CorrectionAlpha = 0.f;
		bIsCorrecting = true;
	}

	// Update latency estimate from round-trip
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float RTT = CurrentTime - Correction.ServerTimestamp;
	if (RTT > 0.f && RTT < 2.f) // Sanity check
	{
		UpdateLatencyEstimate(RTT);
	}
}

void UARPGNetPredictionComponent::UpdateCorrectionSmoothing(float DeltaTime)
{
	if (!bIsCorrecting) return;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		bIsCorrecting = false;
		return;
	}

	CorrectionAlpha += DeltaTime * CorrectionInterpSpeed;
	if (CorrectionAlpha >= 1.f)
	{
		Owner->SetActorLocation(CorrectionTarget);
		bIsCorrecting = false;
	}
	else
	{
		FVector Smoothed = FMath::Lerp(CorrectionStart, CorrectionTarget, CorrectionAlpha);
		// Blend the correction with the current predicted position
		FVector Current = Owner->GetActorLocation();
		FVector Blended = FMath::Lerp(Current, Smoothed, FMath::Min(CorrectionAlpha, 1.f));
		Owner->SetActorLocation(Blended);
	}
}

void UARPGNetPredictionComponent::UpdateLatencyEstimate(float RoundTripSample)
{
	LatencySamples.Add(RoundTripSample * 0.5f); // One-way estimate
	while (LatencySamples.Num() > MaxLatencySamples)
	{
		LatencySamples.RemoveAt(0);
	}

	float Sum = 0.f;
	for (float S : LatencySamples)
	{
		Sum += S;
	}
	EstimatedLatency = Sum / LatencySamples.Num();
}

// =====================================================================
// Server RPCs
// =====================================================================

bool UARPGNetPredictionComponent::Server_SendPredictedMove_Validate(uint32 SequenceNumber, FVector_NetQuantize Position, FVector_NetQuantize Velocity, FVector2D InputVector, float Timestamp)
{
	// Basic validation: input magnitude should be reasonable
	return InputVector.SizeSquared() <= 2.f;
}

void UARPGNetPredictionComponent::Server_SendPredictedMove_Implementation(uint32 SequenceNumber, FVector_NetQuantize Position, FVector_NetQuantize Velocity, FVector2D InputVector, float Timestamp)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Compare client's predicted position against the server's authoritative position
	FVector ServerPos = Owner->GetActorLocation();
	float Error = FVector::Dist(ServerPos, FVector(Position));

	if (Error > PositionErrorTolerance)
	{
		// Send correction back to client
		FARPGServerCorrection Correction;
		Correction.AcknowledgedSequence = SequenceNumber;
		Correction.CorrectedPosition = ServerPos;

		ACharacter* Character = Cast<ACharacter>(Owner);
		Correction.CorrectedVelocity = Character ? Character->GetVelocity() : FVector::ZeroVector;
		Correction.ServerTimestamp = Timestamp;

		Client_ServerCorrection(Correction);
	}
}

bool UARPGNetPredictionComponent::Server_SendPredictedAbility_Validate(uint32 SequenceNumber, FGameplayTag AbilityTag, float Timestamp)
{
	return AbilityTag.IsValid();
}

void UARPGNetPredictionComponent::Server_SendPredictedAbility_Implementation(uint32 SequenceNumber, FGameplayTag AbilityTag, float Timestamp)
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	UAbilitySystemComponent* ASC = CharBase->GetAbilitySystemComponent();
	if (!ASC) return;

	// Server tries to activate the ability authoritatively
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);

	if (ASC->TryActivateAbilitiesByTag(TagContainer))
	{
		Client_AbilityConfirmed(SequenceNumber);
	}
	else
	{
		Client_AbilityRejected(SequenceNumber, AbilityTag);
	}
}

// =====================================================================
// Client RPCs
// =====================================================================

void UARPGNetPredictionComponent::Client_ServerCorrection_Implementation(FARPGServerCorrection Correction)
{
	ApplyServerCorrection(Correction);
}

void UARPGNetPredictionComponent::Client_AbilityConfirmed_Implementation(uint32 SequenceNumber)
{
	OnAbilityConfirmed(SequenceNumber);
}

void UARPGNetPredictionComponent::Client_AbilityRejected_Implementation(uint32 SequenceNumber, FGameplayTag AbilityTag)
{
	OnAbilityRejected(SequenceNumber, AbilityTag);
}
