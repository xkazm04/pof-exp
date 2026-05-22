#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ARPGPlaySound.generated.h"

class USoundBase;

/**
 * Plays a sound at a socket location when triggered.
 * Wrapper around UGameplayStatics::SpawnSoundAttached / AtLocation.
 */
UCLASS(DisplayName = "ARPG Play Sound")
class POF_API UAnimNotify_ARPGPlaySound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** The sound to play (SoundCue, SoundWave, MetaSoundSource, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

	/** Socket or bone to play the sound at. If None, uses actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	FName SocketName = NAME_None;

	/** Volume multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float VolumeMultiplier = 1.f;

	/** Pitch multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float PitchMultiplier = 1.f;

	/** If true, attaches to the socket so the sound follows the character. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bAttachToSocket = false;
};
