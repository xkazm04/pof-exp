#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnVFX.generated.h"

class UNiagaraSystem;

/**
 * Spawns a Niagara particle effect at a socket location when triggered.
 */
UCLASS(DisplayName = "ARPG Spawn VFX")
class POF_API UAnimNotify_SpawnVFX : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** The Niagara system to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TObjectPtr<UNiagaraSystem> NiagaraEffect;

	/** Socket or bone name to attach the effect to. If None, spawns at actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FName SocketName = NAME_None;

	/** Offset from the socket / actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FVector LocationOffset = FVector::ZeroVector;

	/** Rotation offset from the socket / actor rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** Scale of the spawned effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FVector Scale = FVector::OneVector;

	/** If true, the effect attaches to the socket and follows it. If false, spawns at world location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	bool bAttachToSocket = true;
};
