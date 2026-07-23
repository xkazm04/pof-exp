#include "Test/HUD/AVSHUDFunctionalTest.h"
#include "Character/ARPGEnemyCharacter.h"
#include "UI/VSHUDWidget.h"
#include "UI/ARPGCodeWidgetBase.h"
#include "UI/DamageNumberWidget.h"
#include "Components/ProgressBar.h"
#include "AbilitySystemComponent.h"
#include "UObject/UObjectIterator.h"

AVSHUDFunctionalTest::AVSHUDFunctionalTest()
{
	TimeLimit = 30.f;
	Phases = { TEXT("HUDStructure"), TEXT("HUDBinding"), TEXT("DamageNumbers") };
}

void AVSHUDFunctionalTest::OnTestStarted()
{
	// Own the fixture: spawn a fresh enemy instead of borrowing whatever the shared
	// PIE world has left over. Sequential functional tests share the world, so at
	// some placements (Test_Inventory) the map's enemies are already dead or gone —
	// binding one of those left the health bar at 1.0 forever (the pre-existing red).
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector SpawnLoc = GetActorLocation() + FVector(300.f, 0.f, 100.f);
	if (AARPGEnemyCharacter* E = GetWorld()->SpawnActor<AARPGEnemyCharacter>(
			AARPGEnemyCharacter::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params))
	{
		Enemy = E; // GetFirstEnemy()/GetEnemyASC() now resolve THIS fixture
		SpawnedFixture = E;
	}

	// Build the slice HUD widget exactly as AVSHUD does.
	Widget = CreateWidget<UVSHUDWidget>(GetWorld(), UVSHUDWidget::StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(UARPGCodeWidgetBase::DefaultHUDZOrder);
	}
}

EARPGPhaseResult AVSHUDFunctionalTest::RunPhase(int32 /*PhaseIndex*/, FName PhaseName, float DeltaSeconds)
{
	if (PhaseName == TEXT("HUDStructure"))
	{
		if (!AssertTrue(Widget != nullptr, TEXT("HUD widget created")))
		{
			return EARPGPhaseResult::Fail;
		}
		AssertTrue(Widget->GetWidgetFromName(TEXT("PlayerHealthBar"))  != nullptr, TEXT("PlayerHealthBar exists"));
		AssertTrue(Widget->GetWidgetFromName(TEXT("EnemyHealthBar"))   != nullptr, TEXT("EnemyHealthBar exists"));
		AssertTrue(Widget->GetWidgetFromName(TEXT("PlayerHealthText")) != nullptr, TEXT("PlayerHealthText exists"));
		AssertTrue(Widget->GetWidgetFromName(TEXT("EnemyHealthText"))  != nullptr, TEXT("EnemyHealthText exists"));
		AssertTrue(Widget->GetWidgetFromName(TEXT("EnemyNameText"))    != nullptr, TEXT("EnemyNameText exists"));
		return EARPGPhaseResult::Advance;
	}

	if (PhaseName == TEXT("HUDBinding"))
	{
		if (IsFirstTickOfPhase(DeltaSeconds))
		{
			Widget->BindEnemy(GetEnemyASC(), TEXT("Enemy"));
			ApplyDamage(GetFirstEnemy(), 25.f); // non-lethal
		}
		UProgressBar* Bar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("EnemyHealthBar")));
		const EARPGWait W = WaitForCondition(
			[Bar]() { return Bar != nullptr && Bar->GetPercent() < 0.999f; }, 3.f);
		if (W == EARPGWait::Satisfied)
		{
			AssertTrue(true, TEXT("enemy health bar updated on damage"));
			return EARPGPhaseResult::Advance;
		}
		if (W == EARPGWait::TimedOut)
		{
			AssertTrue(false, TEXT("enemy health bar did not update within 3s"));
			return EARPGPhaseResult::Fail;
		}
		return EARPGPhaseResult::Running;
	}

	if (PhaseName == TEXT("DamageNumbers"))
	{
		if (IsFirstTickOfPhase(DeltaSeconds))
		{
			ApplyDamage(GetFirstEnemy(), 20.f);
		}
		const EARPGWait W = WaitForCondition([]()
		{
			for (TObjectIterator<UDamageNumberWidget> It; It; ++It)
			{
				if (IsValid(*It) && It->IsInViewport())
				{
					return true;
				}
			}
			return false;
		}, 3.f);
		if (W == EARPGWait::Satisfied)
		{
			AssertTrue(true, TEXT("floating damage number spawned + in viewport"));
			return EARPGPhaseResult::Advance;
		}
		if (W == EARPGWait::TimedOut)
		{
			AssertTrue(false, TEXT("no floating damage number appeared within 3s"));
			return EARPGPhaseResult::Fail;
		}
		return EARPGPhaseResult::Running;
	}

	return EARPGPhaseResult::Advance;
}

void AVSHUDFunctionalTest::CleanUp()
{
	// Shared-PIE-world hygiene: remove the fixture so later tests inherit a clean map.
	if (SpawnedFixture.IsValid())
	{
		SpawnedFixture->Destroy();
		SpawnedFixture = nullptr;
	}
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	Super::CleanUp();
}
