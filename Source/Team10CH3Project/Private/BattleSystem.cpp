#include "BattleSystem.h"
#include "HealthComponent.h"
#include "DrawDebugHelpers.h"
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
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HealthComponent not found"));
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

	if (showDebug)
	{
		DrawDebugLine(
			GetWorld(),
			startLocation,
			endLocation,
			FColor::Red,
			false,
			2.0f
		);
	}

	if (isHit)
	{
		AActor* hitActor = hitResult.GetActor();

		Attack(hitActor, attackDamageAmount, shooterActor, hitResult.ImpactPoint);
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
		showDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
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

void ABattleSystem::RequestSkillAttack(AActor* attackerActor)
{
	AttackAround(attackerActor, skillAttackDamage, skillAttackRange);
}