#include "BattleSystem.h"
#include "HealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"

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
	if (targetActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("targetActor is null"));
		return;
	}

	UHealthComponent* healthComponent = targetActor->FindComponentByClass<UHealthComponent>();

	if (healthComponent)
	{
		healthComponent->TakeDamage(damageAmount, attackerActor, hitLocation);
	}
}

void ABattleSystem::FireLineTrace(AActor* shooterActor, float attackDamageAmount, float attackRange)
{
	if (shooterActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("shooterActor is null"));
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
		AActor* hitActor = hitResult.GetActor();

		if (hitActor == nullptr || hitActor->FindComponentByClass<UHealthComponent>() == nullptr)
		{
			return;
		}

		bool isHeadShot = IsHeadShot(hitResult);
		float finalDamage = attackDamageAmount;

		if (isHeadShot)
		{
			finalDamage *= headShotMultiplier;
		}

		Attack(hitActor, finalDamage, shooterActor, hitResult.ImpactPoint);	
		onBasicAttackHit.Broadcast(hitActor, finalDamage, isHeadShot, hitResult.ImpactPoint);
	}
}

void ABattleSystem::AttackAround(AActor* attackerActor, float damageAmount, float attackRange)
{
	if (attackerActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("attackerActor is null"));
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
			AActor* hitActor = hitResult.GetActor();

			if (hitActor && !damagedActors.Contains(hitActor) && hitActor->FindComponentByClass<UHealthComponent>())
			{
				damagedActors.Add(hitActor);
				Attack(hitActor, damageAmount, attackerActor, hitResult.ImpactPoint);
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
		UE_LOG(LogTemp, Warning, TEXT("attackerActor is null"));
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
			AActor* hitActor = hitResult.GetActor();

			if (hitActor && !damagedActors.Contains(hitActor) && hitActor->FindComponentByClass<UHealthComponent>())
			{
				damagedActors.Add(hitActor);
				Attack(hitActor, damageAmount, attackerActor, hitResult.ImpactPoint);
			}
		}
	}
}

void ABattleSystem::RequestBasicAttack(AActor* attackerActor)
{
	FireLineTrace(attackerActor, basicAttackDamage, basicAttackRange);
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
		UE_LOG(LogTemp, Warning, TEXT("Skill attack failed: cooldown"));
		onSkillAttackFailed.Broadcast();
		return;
	}

	if (attackerActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("attackerActor is null"));
		return;
	}

	FVector skillLocation = attackerActor->GetActorLocation() + attackerActor->GetActorForwardVector() * skillThrowDistance;

	AttackAroundLocation(skillLocation, attackerActor, skillAttackDamage, skillAttackRange);

	StartSkillCooldown();
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