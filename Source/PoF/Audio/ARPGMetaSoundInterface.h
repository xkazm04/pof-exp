#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGMetaSoundInterface.generated.h"

class UAudioComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EMetaSoundParameterType : uint8
{
	Float,
	Int,
	Bool,
	String
};

USTRUCT(BlueprintType)
struct FMetaSoundParameterBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMetaSoundParameterType ParameterType = EMetaSoundParameterType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultFloat = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DefaultInt = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool DefaultBool = false;
};

/**
 * Component that wraps MetaSound source patches, providing a typed parameter
 * interface for driving procedural audio from gameplay code.
 * Exposes trigger/set methods for real-time parameter manipulation.
 */
UCLASS(ClassGroup = "Audio", meta = (BlueprintSpawnableComponent))
class POF_API UARPGMetaSoundInterface : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGMetaSoundInterface();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Audio|MetaSound")
	void PlayMetaSound();

	UFUNCTION(BlueprintCallable, Category = "Audio|MetaSound")
	void StopMetaSound(float FadeOutTime = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "Audio|MetaSound")
	void SetFloatParameter(FName ParamName, float Value);

	UFUNCTION(BlueprintCallable, Category = "Audio|MetaSound")
	void SetIntParameter(FName ParamName, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Audio|MetaSound")
	void SetBoolParameter(FName ParamName, bool Value);

	UFUNCTION(BlueprintCallable, Category = "Audio|MetaSound")
	void TriggerParameter(FName ParamName);

	UFUNCTION(BlueprintPure, Category = "Audio|MetaSound")
	bool IsPlaying() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaSound")
	TObjectPtr<USoundBase> MetaSoundSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaSound")
	TArray<FMetaSoundParameterBinding> ParameterBindings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaSound")
	bool bAutoPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaSound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BaseVolume = 1.f;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComp;

	void ApplyDefaultParameters();
};
