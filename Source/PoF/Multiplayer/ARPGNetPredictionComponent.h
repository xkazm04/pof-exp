#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ARPGNetPredictionComponent.generated.h"

class UAbilitySystemComponent;
class AARPGCharacterBase;

/**
 * Saved movement state for server reconciliation.
 * Each frame the client predicts, it stores one of these keyed by sequence number.
 */
USTRUCT()
struct FARPGPredictedMoveState
{
	GENERATED_BODY()

	/** Monotonically increasing sequence number. */
	uint32 SequenceNumber = 0;

	/** Predicted world position after this move. */
	FVector Position = FVector::ZeroVector;

	/** Velocity at the time of prediction. */
	FVector Velocity = FVector::ZeroVector;

	/** Server timestamp associated with this prediction. */
	float Timestamp = 0.f;

	/** Movement input that was applied. */
	FVector2D InputVector = FVector2D::ZeroVector;
};

/**
 * Saved ability prediction for rollback.
 */
USTRUCT()
struct FARPGPredictedAbilityAction
{
	GENERATED_BODY()

	uint32 SequenceNumber = 0;
	FGameplayTag AbilityTag;
	float Timestamp = 0.f;

	/** Whether the server confirmed this prediction. */
	bool bConfirmed = false;
	bool bRejected = false;
};

/**
 * Server correction sent to owning client when predicted state diverges.
 */
USTRUCT()
struct FARPGServerCorrection
{
	GENERATED_BODY()

	uint32 AcknowledgedSequence = 0;
	FVector CorrectedPosition = FVector::ZeroVector;
	FVector CorrectedVelocity = FVector::ZeroVector;
	float ServerTimestamp = 0.f;
};

/**
 * Client-side prediction component.
 *
 * UE's CharacterMovementComponent already handles core movement prediction.
 * This component adds:
 *  - Ability prediction with server confirmation/rejection
 *  - Position reconciliation with error tolerance and smoothing
 *  - Input buffering for server RPCs
 *  - Latency estimation for prediction windows
 */
UCLASS(ClassGroup = (Multiplayer), meta = (BlueprintSpawnableComponent))
class POF_API UARPGNetPredictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGNetPredictionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =====================================================================
	// Ability Prediction
	// =====================================================================

	/**
	 * Predict an ability activation locally and send to server.
	 * Returns a prediction sequence number (0 = prediction failed to queue).
	 */
	UFUNCTION(BlueprintCallable, Category = "Prediction|Ability")
	int32 PredictAbilityActivation(FGameplayTag AbilityTag);

	/** Called when the server confirms a predicted ability. */
	void OnAbilityConfirmed(uint32 SequenceNumber);

	/** Called when the server rejects a predicted ability. */
	void OnAbilityRejected(uint32 SequenceNumber, FGameplayTag AbilityTag);

	/** Get the number of unconfirmed ability predictions in flight. */
	UFUNCTION(BlueprintPure, Category = "Prediction|Ability")
	int32 GetPendingAbilityPredictions() const { return PendingAbilities.Num(); }

	// =====================================================================
	// Position Reconciliation
	// =====================================================================

	/**
	 * Called when a server correction arrives. Compares against predicted
	 * state and snaps/smooths if error exceeds tolerance.
	 */
	void ApplyServerCorrection(const FARPGServerCorrection& Correction);

	/** Whether a position correction is currently being smoothed. */
	UFUNCTION(BlueprintPure, Category = "Prediction|Position")
	bool IsCorrecting() const { return bIsCorrecting; }

	/** Current estimated one-way latency in seconds. */
	UFUNCTION(BlueprintPure, Category = "Prediction|Network")
	float GetEstimatedLatency() const { return EstimatedLatency; }

	// =====================================================================
	// Server RPCs
	// =====================================================================

	/** Client sends predicted move to server for validation. */
	UFUNCTION(Server, Unreliable, WithValidation, Category = "Prediction|RPC")
	void Server_SendPredictedMove(uint32 SequenceNumber, FVector_NetQuantize Position, FVector_NetQuantize Velocity, FVector2D InputVector, float Timestamp);

	/** Client sends predicted ability to server. */
	UFUNCTION(Server, Reliable, WithValidation, Category = "Prediction|RPC")
	void Server_SendPredictedAbility(uint32 SequenceNumber, FGameplayTag AbilityTag, float Timestamp);

	// =====================================================================
	// Client RPCs
	// =====================================================================

	/** Server sends correction to owning client. */
	UFUNCTION(Client, Unreliable, Category = "Prediction|RPC")
	void Client_ServerCorrection(FARPGServerCorrection Correction);

	/** Server confirms an ability prediction. */
	UFUNCTION(Client, Reliable, Category = "Prediction|RPC")
	void Client_AbilityConfirmed(uint32 SequenceNumber);

	/** Server rejects an ability prediction. */
	UFUNCTION(Client, Reliable, Category = "Prediction|RPC")
	void Client_AbilityRejected(uint32 SequenceNumber, FGameplayTag AbilityTag);

protected:
	/** Max distance (cm) between predicted and server positions before correction triggers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "1.0"))
	float PositionErrorTolerance = 10.f;

	/** Max distance error that triggers a smooth correction (above this = hard snap). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "1.0"))
	float HardSnapThreshold = 200.f;

	/** Speed of smooth position correction interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "1.0"))
	float CorrectionInterpSpeed = 15.f;

	/** How many predicted move states to keep in the buffer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "16", ClampMax = "256"))
	int32 MoveBufferSize = 64;

	/** How often (seconds) to send predicted moves to server. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "0.016"))
	float MoveSendRate = 0.033f; // ~30Hz

	/** Max pending ability predictions before blocking new ones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "1"))
	int32 MaxPendingAbilities = 8;

	/** How long (seconds) before an unconfirmed ability prediction is auto-rejected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction|Tuning", meta = (ClampMin = "0.5"))
	float AbilityPredictionTimeout = 3.f;

private:
	// --- Move prediction buffer ---
	TArray<FARPGPredictedMoveState> MoveBuffer;
	uint32 NextMoveSequence = 1;
	float MoveSendTimer = 0.f;

	// --- Ability prediction ---
	TArray<FARPGPredictedAbilityAction> PendingAbilities;
	uint32 NextAbilitySequence = 1;

	// --- Correction smoothing ---
	bool bIsCorrecting = false;
	FVector CorrectionTarget = FVector::ZeroVector;
	FVector CorrectionStart = FVector::ZeroVector;
	float CorrectionAlpha = 0.f;

	// --- Latency estimation ---
	float EstimatedLatency = 0.f;
	TArray<float> LatencySamples;
	static constexpr int32 MaxLatencySamples = 16;

	void RecordMoveState();
	void SendPredictedMoveToServer();
	void UpdateCorrectionSmoothing(float DeltaTime);
	void CleanupTimedOutAbilities();
	void UpdateLatencyEstimate(float RoundTripSample);
};
