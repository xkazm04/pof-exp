#include "Performance/ARPGObjectPool.h"
#include "Debug/ARPGLogCategories.h"

DEFINE_LOG_CATEGORY_STATIC(LogARPGPool, Log, All);

void UARPGObjectPool::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogARPGPool, Display, TEXT("ObjectPool subsystem initialized"));
}

void UARPGObjectPool::Deinitialize()
{
	// Destroy all pooled actors
	for (auto& Pair : Pools)
	{
		for (TWeakObjectPtr<AActor>& WeakActor : Pair.Value.InactiveActors)
		{
			if (AActor* Actor = WeakActor.Get())
			{
				Actor->Destroy();
			}
		}
	}
	Pools.Empty();

	Super::Deinitialize();
}

AActor* UARPGObjectPool::Acquire(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	if (!ActorClass)
	{
		return nullptr;
	}

	FPoolEntry& Entry = Pools.FindOrAdd(ActorClass.Get());

	// Try to reuse a pooled actor
	while (Entry.InactiveActors.Num() > 0)
	{
		TWeakObjectPtr<AActor> WeakActor = Entry.InactiveActors.Pop();
		if (AActor* Actor = WeakActor.Get())
		{
			ActivateActor(Actor, SpawnTransform);
			Entry.ActiveCount++;
			return Actor;
		}
		// Stale reference, skip
	}

	// No pooled actors available — spawn a new one
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* NewActor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
	if (NewActor)
	{
		Entry.ActiveCount++;
	}

	return NewActor;
}

void UARPGObjectPool::Release(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	UClass* ActorClass = Actor->GetClass();
	FPoolEntry& Entry = Pools.FindOrAdd(ActorClass);

	Entry.ActiveCount = FMath::Max(0, Entry.ActiveCount - 1);

	// Prune stale weak pointers
	Entry.InactiveActors.RemoveAll([](const TWeakObjectPtr<AActor>& W) { return !W.IsValid(); });

	// If pool is full, destroy instead of returning
	if (Entry.InactiveActors.Num() >= Entry.MaxSize)
	{
		Actor->Destroy();
		return;
	}

	DeactivateActor(Actor);
	Entry.InactiveActors.Add(Actor);
}

void UARPGObjectPool::PreWarm(TSubclassOf<AActor> ActorClass, int32 Count, UWorld* World)
{
	if (!ActorClass || !World || Count <= 0)
	{
		return;
	}

	FPoolEntry& Entry = Pools.FindOrAdd(ActorClass.Get());

	const FTransform HiddenTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -10000.f));

	for (int32 i = 0; i < Count && Entry.InactiveActors.Num() < Entry.MaxSize; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* Actor = World->SpawnActor<AActor>(ActorClass, HiddenTransform, SpawnParams);
		if (Actor)
		{
			DeactivateActor(Actor);
			Entry.InactiveActors.Add(Actor);
		}
	}

	UE_LOG(LogARPGPool, Display, TEXT("PreWarmed %d actors of class %s (pool size: %d)"),
		Count, *ActorClass->GetName(), Entry.InactiveActors.Num());
}

void UARPGObjectPool::SetMaxPoolSize(TSubclassOf<AActor> ActorClass, int32 MaxSize)
{
	if (!ActorClass)
	{
		return;
	}

	FPoolEntry& Entry = Pools.FindOrAdd(ActorClass.Get());
	Entry.MaxSize = FMath::Max(1, MaxSize);

	// Trim excess
	while (Entry.InactiveActors.Num() > Entry.MaxSize)
	{
		TWeakObjectPtr<AActor> WeakActor = Entry.InactiveActors.Pop();
		if (AActor* Actor = WeakActor.Get())
		{
			Actor->Destroy();
		}
	}
}

int32 UARPGObjectPool::GetPooledCount(TSubclassOf<AActor> ActorClass) const
{
	if (!ActorClass)
	{
		return 0;
	}

	const FPoolEntry* Entry = Pools.Find(ActorClass.Get());
	return Entry ? Entry->InactiveActors.Num() : 0;
}

int32 UARPGObjectPool::GetActiveCount(TSubclassOf<AActor> ActorClass) const
{
	if (!ActorClass)
	{
		return 0;
	}

	const FPoolEntry* Entry = Pools.Find(ActorClass.Get());
	return Entry ? Entry->ActiveCount : 0;
}

void UARPGObjectPool::DeactivateActor(AActor* Actor)
{
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// Move far away to prevent any residual interactions
	Actor->SetActorLocation(FVector(0.f, 0.f, -10000.f));

	// Reset lifespan so it doesn't auto-destroy while pooled
	Actor->SetLifeSpan(0.f);
}

void UARPGObjectPool::ActivateActor(AActor* Actor, const FTransform& Transform)
{
	Actor->SetActorTransform(Transform);
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);
}
