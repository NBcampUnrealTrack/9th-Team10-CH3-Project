#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	currentHealth = maxHealth;
}

void UHealthComponent::TakeDamage(float damageAmount, AActor* attackerActor, FVector hitLocation)
{

	if (isDead)
	{
		return;
	}

	FVector attackDirection = FVector::ZeroVector;

	AActor* owner = GetOwner();

	if (owner != nullptr && attackerActor != nullptr)
	{
		attackDirection = owner->GetActorLocation() - attackerActor->GetActorLocation();
		attackDirection.Normalize();
	}

	onDamaged.Broadcast(damageAmount, attackerActor, hitLocation, attackDirection);

	currentHealth -= damageAmount;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Damage: %f / CurrentHealth: %f / DamagedBy: %s / HitLocation: %s"),
		damageAmount,
		currentHealth,
		attackerActor ? *attackerActor->GetName() : TEXT("None"),
		*hitLocation.ToString()
	);

	if (currentHealth < 0.0f)
	{
		currentHealth = 0.0f;
	}
	
	if (currentHealth <= 0.0f)
	{
		isDead = true;
		onDeath.Broadcast(GetOwner());
	}
}