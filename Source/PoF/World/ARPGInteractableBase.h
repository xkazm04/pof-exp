#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGInteractableBase.generated.h"

class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteracted, AARPGInteractableBase*, Interactable, AActor*, Interactor);

/**
 * Base class for interactable world objects.
 * Provides a trigger sphere, interaction prompt text, and a virtual Interact() method.
 */
UCLASS(Abstract)
class POF_API AARPGInteractableBase : public AActor
{
	GENERATED_BODY()

public:
	AARPGInteractableBase();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual bool Interact(AActor* Interactor);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool CanInteract() const { return bCanInteract; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetInteractionPrompt() const { return InteractionPrompt; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsPlayerInRange() const { return bPlayerInRange; }

	UPROPERTY(BlueprintAssignable, Category = "Events|Interaction")
	FOnInteracted OnInteracted;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionPrompt = FText::FromString(TEXT("Press E to interact"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteract = true;

	bool bPlayerInRange = false;

private:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
