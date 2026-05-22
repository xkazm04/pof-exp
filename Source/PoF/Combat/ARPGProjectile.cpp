#include "Combat/ARPGProjectile.h"
#include "Physics/ARPGCollisionChannels.h"
#include "Physics/ARPGPhysicalMaterial.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

AARPGProjectile::AARPGProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(SphereRadius);
	CollisionSphere->SetCollisionProfileName(ARPGCollision::Profiles::Projectile);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ARPGCollision::Projectile);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ARPGCollision::Destructible, ECR_Block);
	SetRootComponent(CollisionSphere);

	// Projectile movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// Optional VFX component (Niagara system assigned in Blueprint)
	VFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFXComponent"));
	VFXComponent->SetupAttachment(CollisionSphere);
	VFXComponent->bAutoActivate = true;

	// Auto-destroy
	InitialLifeSpan = MaxLifeTime;
}

void AARPGProjectile::InitDamage(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffect, float InBaseDamage, float InLevel)
{
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageEffect;
	DamageBaseMagnitude = InBaseDamage;
	EffectLevel = InLevel;
}

void AARPGProjectile::EnableHoming(AActor* HomingTarget)
{
	if (!HomingTarget || !ProjectileMovement) return;

	USceneComponent* TargetRoot = HomingTarget->GetRootComponent();
	if (!TargetRoot) return;

	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingTargetComponent = TWeakObjectPtr<USceneComponent>(TargetRoot);
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
}

void AARPGProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Sync runtime properties with UPROPERTY values (BP may override CDO defaults)
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;

	// Bind collision events
	CollisionSphere->OnComponentHit.AddDynamic(this, &AARPGProjectile::OnHit);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AARPGProjectile::OnOverlap);

	if (bShouldBounce)
	{
		ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AARPGProjectile::OnBounce);
	}
}

void AARPGProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// If bouncing is enabled, don't destroy on world hits — OnBounce handles that
	if (bShouldBounce && OtherActor && !Cast<APawn>(OtherActor))
	{
		SpawnImpactEffects(Hit.ImpactPoint, Hit.ImpactNormal);
		return;
	}

	OnImpact(OtherActor, Hit);
}

void AARPGProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetInstigator()) return;

	OnImpact(OtherActor, SweepResult);
}

void AARPGProjectile::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	BounceCount++;

	SpawnImpactEffects(ImpactResult.ImpactPoint, ImpactResult.ImpactNormal);

	if (MaxBounces > 0 && BounceCount >= MaxBounces)
	{
		if (bAreaDamage)
		{
			ApplyAreaDamage(ImpactResult.ImpactPoint);
		}
		Destroy();
	}
}

void AARPGProjectile::OnImpact(AActor* OtherActor, const FHitResult& HitResult)
{
	if (OtherActor && OtherActor != GetInstigator())
	{
		ApplyDamageToTarget(OtherActor, HitResult);
	}

	const FVector ImpactLocation = HitResult.ImpactPoint.IsZero() ? FVector(GetActorLocation()) : FVector(HitResult.ImpactPoint);
	const FVector ImpactNormal = HitResult.ImpactNormal.IsZero() ? FVector::UpVector : FVector(HitResult.ImpactNormal);

	if (bAreaDamage)
	{
		ApplyAreaDamage(ImpactLocation);
	}

	SpawnImpactEffects(ImpactLocation, ImpactNormal);

	Destroy();
}

void AARPGProjectile::ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult)
{
	if (!SourceASC || !DamageEffectClass || !TargetActor) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	if (HitResult.bBlockingHit)
	{
		Context.AddHitResult(HitResult);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, EffectLevel, Context);
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, DamageBaseMagnitude);

	// Apply damage type tags so the execution can look up elemental resistance
	for (const FGameplayTag& Tag : DamageTags)
	{
		SpecHandle.Data->AddDynamicAssetTag(Tag);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void AARPGProjectile::ApplyAreaDamage(const FVector& Origin)
{
	if (!SourceASC || !DamageEffectClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileAoE), false, this);
	Params.AddIgnoredActor(GetInstigator());

	World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(AreaDamageRadius), Params);

	TSet<AActor*> DamagedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapActor || DamagedActors.Contains(OverlapActor)) continue;

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OverlapActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC) continue;

		DamagedActors.Add(OverlapActor);

		// Calculate falloff based on distance
		const float Distance = FVector::Dist(Origin, OverlapActor->GetActorLocation());
		const float DistanceRatio = FMath::Clamp(Distance / AreaDamageRadius, 0.f, 1.f);
		const float DamageMultiplier = FMath::Lerp(1.f, AreaDamageFalloff, DistanceRatio);

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, EffectLevel, Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, DamageBaseMagnitude * DamageMultiplier);

		// Apply damage type tags for elemental resistance lookup
		for (const FGameplayTag& Tag : DamageTags)
		{
			SpecHandle.Data->AddDynamicAssetTag(Tag);
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void AARPGProjectile::SpawnImpactEffects(const FVector& Location, const FVector& Normal)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, ImpactVFX, Location, Normal.Rotation(),
			FVector(1.f), true, true, ENCPoolMethod::None);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}
}
