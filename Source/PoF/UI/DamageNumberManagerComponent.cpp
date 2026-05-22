#include "UI/DamageNumberManagerComponent.h"
#include "UI/DamageNumberWidget.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Blueprint/UserWidget.h"

UDamageNumberManagerComponent::UDamageNumberManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default to the C++ widget, which builds its own UI tree. A Blueprint
	// subclass can be assigned in the editor to override the visual style.
	DamageNumberWidgetClass = UDamageNumberWidget::StaticClass();
}

void UDamageNumberManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Subscribe to the global static delegate from any UARPGAttributeSet
	GlobalDelegateHandle = UARPGAttributeSet::OnDamageNumberRequestedGlobal.AddUObject(
		this, &UDamageNumberManagerComponent::HandleDamageNumber);
}

void UDamageNumberManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UARPGAttributeSet::OnDamageNumberRequestedGlobal.Remove(GlobalDelegateHandle);
	Super::EndPlay(EndPlayReason);
}

void UDamageNumberManagerComponent::SpawnDamageNumber(
	AActor* TargetActor, float DamageAmount, bool bIsCrit,
	bool bIsHeal, FGameplayTag DamageType, FVector HitLocation)
{
	if (!DamageNumberWidgetClass) return;

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	// Prune expired widgets
	ActiveNumbers.RemoveAll([](const TWeakObjectPtr<UDamageNumberWidget>& W)
	{
		return !W.IsValid();
	});

	// Enforce max active limit — remove oldest
	while (ActiveNumbers.Num() >= MaxActiveNumbers)
	{
		if (UDamageNumberWidget* Old = ActiveNumbers[0].Get())
		{
			Old->RemoveFromParent();
		}
		ActiveNumbers.RemoveAt(0);
	}

	// Create the widget
	UDamageNumberWidget* Widget = CreateWidget<UDamageNumberWidget>(PC, DamageNumberWidgetClass);
	if (!Widget) return;

	Widget->AddToViewport(100); // High Z-order so it's on top
	Widget->InitDamageNumber(DamageAmount, bIsCrit, bIsHeal, DamageType, HitLocation);

	ActiveNumbers.Add(Widget);
}

void UDamageNumberManagerComponent::HandleDamageNumber(
	AActor* TargetActor, float DamageAmount, bool bIsCrit,
	bool bIsHeal, FGameplayTag DamageType, FVector HitLocation)
{
	SpawnDamageNumber(TargetActor, DamageAmount, bIsCrit, bIsHeal, DamageType, HitLocation);
}
