#pragma once

#include "CoreMinimal.h"
#include "World/ARPGInteractableBase.h"
#include "ARPGLockedGate.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGateOpened, AARPGLockedGate*, Gate);

/**
 * A gate/door that can be locked by quest, key item, or lever.
 * When unlocked, slides or rotates open.
 */
UCLASS()
class POF_API AARPGLockedGate : public AARPGInteractableBase
{
	GENERATED_BODY()

public:
	AARPGLockedGate();

	virtual bool Interact(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "World|Gate")
	void Unlock();

	UFUNCTION(BlueprintCallable, Category = "World|Gate")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "World|Gate")
	void Close();

	UFUNCTION(BlueprintPure, Category = "World|Gate")
	bool IsLocked() const { return bIsLocked; }

	UFUNCTION(BlueprintPure, Category = "World|Gate")
	bool IsOpen() const { return bIsOpen; }

	UPROPERTY(BlueprintAssignable, Category = "Events|World")
	FOnGateOpened OnGateOpened;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	bool bIsLocked = true;

	/** Quest ID that must be completed to unlock. Empty = no quest requirement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|Requirements")
	FName RequiredQuestID;

	/** Item name that must be in inventory to unlock. Empty = no item requirement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|Requirements")
	FName RequiredItemName;

	/** If true, the key item is consumed when the gate is unlocked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|Requirements", meta = (EditCondition = "!RequiredItemName.IsNone()"))
	bool bConsumeKeyItem = false;

	/** If true, the gate can be closed after opening (interact again to toggle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	bool bCanClose = false;

	/** If true, the gate opens by sliding up. If false, by rotating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|Animation")
	bool bSlideOpen = true;

	/** Distance to slide (if bSlideOpen) or angle to rotate in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|Animation")
	float OpenDistance = 400.f;

	/** Speed of opening animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|Animation", meta = (ClampMin = "10"))
	float OpenSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|SFX")
	TObjectPtr<class USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate|SFX")
	TObjectPtr<class USoundBase> LockedSound;

private:
	bool bIsOpen = false;
	bool bIsAnimating = false;
	FVector ClosedLocation = FVector::ZeroVector;
	FRotator ClosedRotation = FRotator::ZeroRotator;
	float CurrentOpenProgress = 0.f;

	bool CheckRequirements(AActor* Interactor) const;
	void UpdateInteractionPrompt();
};
