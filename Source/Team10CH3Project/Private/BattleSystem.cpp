#include "BattleSystem.h"
#include "HealthComponent.h"
#include "GrenadeProjectile.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	AActor* ResolveDamageableActor(AActor* hitActor)
	{
		TSet<AActor*> visitedActors;
		AActor* currentActor = hitActor;

		while (IsValid(currentActor) && !visitedActors.Contains(currentActor))
		{
			visitedActors.Add(currentActor);

			if (currentActor->FindComponentByClass<UHealthComponent>())
			{
				return currentActor;
			}

			AActor* nextActor = currentActor->GetOwner();
			if (!IsValid(nextActor))
			{
				nextActor = currentActor->GetAttachParentActor();
			}

			currentActor = nextActor;
		}

		return nullptr;
	}
}

ABattleSystem::ABattleSystem()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABattleSystem::BeginPlay()
{
	Super::BeginPlay();
}

void ABattleSystem::Attack(AActor* targetActor, float damageAmount, AActor* attackerActor, FVector hitLocation)
{
	AActor* damageableActor = ResolveDamageableActor(targetActor);
	if (!damageableActor)
	{
		return;
	}

	UHealthComponent* healthComponent = damageableActor->FindComponentByClass<UHealthComponent>();

	if (healthComponent)
	{
		healthComponent->TakeDamage(damageAmount, attackerActor, hitLocation);
	}
}

void ABattleSystem::FireLineTrace(AActor* shooterActor, float attackDamageAmount, float attackRange)
{
	if (shooterActor == nullptr)
	{
		return;
	}

	FVector startLocation = shooterActor->GetActorLocation();
	FVector forwardVector = shooterActor->GetActorForwardVector();
	FVector endLocation = startLocation + forwardVector * attackRange;

	FHitResult hitResult;

	FCollisionQueryParams traceParams;
	traceParams.AddIgnoredActor(shooterActor);

	bool isHit = GetWorld()->LineTraceSingleByChannel(
		hitResult,
		startLocation,
		endLocation,
		ECC_Visibility,
		traceParams
	);

	if (isHit)
	{
		AActor* damageableActor = ResolveDamageableActor(hitResult.GetActor());
		if (!damageableActor)
		{
			return;
		}

		bool isHeadShot = IsHeadShot(hitResult);
		float finalDamage = attackDamageAmount;

		if (isHeadShot)
		{
			finalDamage *= headShotMultiplier;
		}

		Attack(damageableActor, finalDamage, shooterActor, hitResult.ImpactPoint);
		onBasicAttackHit.Broadcast(damageableActor, finalDamage, isHeadShot, hitResult.ImpactPoint);
	}
}

void ABattleSystem::AttackAround(AActor* attackerActor, float damageAmount, float attackRange)
{
	if (attackerActor == nullptr)
	{
		return;
	}

	FVector attackLocation = attackerActor->GetActorLocation();

	TArray<AActor*> ignoreActors;
	ignoreActors.Add(attackerActor);

	TArray<FHitResult> hitResults;
	TArray<AActor*> damagedActors;

	bool isHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		attackLocation,
		attackLocation,
		attackRange,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ignoreActors,
		EDrawDebugTrace::None,
		hitResults,
		true
	);

	if (isHit)
	{
		for (FHitResult hitResult : hitResults)
		{
			AActor* damageableActor = ResolveDamageableActor(hitResult.GetActor());

			if (damageableActor && !damagedActors.Contains(damageableActor))
			{
				damagedActors.Add(damageableActor);
				Attack(damageableActor, damageAmount, attackerActor, hitResult.ImpactPoint);
			}
		}
	}
}

void ABattleSystem::AttackAroundLocation(
	FVector attackLocation,
	AActor* attackerActor,
	float damageAmount,
	float attackRange
)
{
	if (attackerActor == nullptr)
	{
		return;
	}

	TArray<AActor*> ignoreActors;
	ignoreActors.Add(attackerActor);

	TArray<FHitResult> hitResults;
	TArray<AActor*> damagedActors;

	bool isHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		attackLocation,
		attackLocation,
		attackRange,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ignoreActors,
		EDrawDebugTrace::None,
		hitResults,
		true
	);

	if (isHit)
	{
		for (FHitResult hitResult : hitResults)
		{
			AActor* damageableActor = ResolveDamageableActor(hitResult.GetActor());

			if (damageableActor && !damagedActors.Contains(damageableActor))
			{
				damagedActors.Add(damageableActor);
				Attack(damageableActor, damageAmount, attackerActor, hitResult.ImpactPoint);
			}
		}
	}
}

void ABattleSystem::RequestBasicAttack(AActor* attackerActor)
{
	FireLineTrace(attackerActor, basicAttackDamage, basicAttackRange);
}

void ABattleSystem::RequestBasicAttackByView(
	AActor* attackerActor,
	FVector viewLocation,
	FVector viewDirection,
	FVector fireLocation
)
{
	if (attackerActor == nullptr)
	{
		return;
	}

	FVector aimStartLocation = viewLocation;
	FVector aimEndLocation = viewLocation + viewDirection * basicAttackRange;

	FHitResult aimHitResult;

	FCollisionQueryParams aimTraceParams;
	aimTraceParams.AddIgnoredActor(attackerActor);

	bool isAimHit = GetWorld()->LineTraceSingleByChannel(
		aimHitResult,
		aimStartLocation,
		aimEndLocation,
		ECC_Visibility,
		aimTraceParams
	);

	FVector aimPoint = aimEndLocation;

	if (isAimHit)
	{
		aimPoint = aimHitResult.ImpactPoint;
	}

	FVector fireDirection = (aimPoint - fireLocation).GetSafeNormal();
	FVector fireEndLocation = fireLocation + fireDirection * basicAttackRange;

	FHitResult hitResult;

	FCollisionQueryParams fireTraceParams;
	fireTraceParams.AddIgnoredActor(attackerActor);

	bool isHit = GetWorld()->LineTraceSingleByChannel(
		hitResult,
		fireLocation,
		fireEndLocation,
		ECC_Visibility,
		fireTraceParams
	);

	if (isHit)
	{
		AActor* damageableActor = ResolveDamageableActor(hitResult.GetActor());
		if (!damageableActor)
		{
			return;
		}

		bool isHeadShot = IsHeadShot(hitResult);
		float finalDamage = basicAttackDamage;

		if (isHeadShot)
		{
			finalDamage *= headShotMultiplier;
		}

		Attack(damageableActor, finalDamage, attackerActor, hitResult.ImpactPoint);
		onBasicAttackHit.Broadcast(damageableActor, finalDamage, isHeadShot, hitResult.ImpactPoint);
	}
}

bool ABattleSystem::CanUseSkillAttack() const
{
	return !isSkillOnCooldown;
}

void ABattleSystem::StartSkillCooldown()
{
	isSkillOnCooldown = true;

	GetWorldTimerManager().SetTimer(
		skillCooldownTimerHandle,
		this,
		&ABattleSystem::ResetSkillCooldown,
		skillCooldown,
		false
	);
}

void ABattleSystem::ResetSkillCooldown()
{
	isSkillOnCooldown = false;
}

void ABattleSystem::RequestSkillAttack(AActor* attackerActor)
{
	if (!CanUseSkillAttack())
	{
		onSkillAttackFailed.Broadcast();
		return;
	}

	if (attackerActor == nullptr)
	{
		return;
	}

	FVector skillLocation = attackerActor->GetActorLocation() + attackerActor->GetActorForwardVector() * skillThrowDistance;

	AttackAroundLocation(skillLocation, attackerActor, skillAttackDamage, skillAttackRange);

	StartSkillCooldown();
}

void ABattleSystem::RequestSkillAttackByView(
	AActor* attackerActor,
	FVector viewLocation,
	FVector viewDirection,
	FVector throwLocation
)
{
	if (!CanUseSkillAttack())
	{
		onSkillAttackFailed.Broadcast();
		return;
	}

	if (attackerActor == nullptr)
	{
		return;
	}

	if (grenadeProjectileClass == nullptr)
	{
		return;
	}

	FVector throwDirection = viewDirection.GetSafeNormal();

	FVector throwVelocity = throwDirection * grenadeThrowPower
		+ FVector::UpVector * grenadeUpPower;

	FActorSpawnParameters spawnParams;
	spawnParams.Owner = attackerActor;

	AGrenadeProjectile* grenadeProjectile = GetWorld()->SpawnActor<AGrenadeProjectile>(
		grenadeProjectileClass,
		throwLocation,
		throwDirection.Rotation(),
		spawnParams
	);

	if (grenadeProjectile)
	{
		grenadeProjectile->damageAmount = skillAttackDamage;
		grenadeProjectile->damageRange = skillAttackRange;

		grenadeProjectile->InitGrenade(
			this,
			attackerActor,
			throwVelocity
		);

		StartSkillCooldown();
	}
}

bool ABattleSystem::IsHeadShot(const FHitResult& hitResult) const
{
	UPrimitiveComponent* hitComponent = hitResult.GetComponent();

	if (hitComponent == nullptr)
	{
		return false;
	}

	return hitComponent->ComponentHasTag(headShotTag);
}
