#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	maxHealth = FMath::Max(1.0f, FMath::RoundToFloat(maxHealth));
	currentHealth = maxHealth;
}

void UHealthComponent::TakeDamage(float damageAmount, AActor* attackerActor, FVector hitLocation)
{

	if (isDead || damageAmount <= 0.0f)
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

	currentHealth = FMath::Clamp(
		FMath::RoundToFloat(currentHealth - damageAmount),
		0.0f,
		maxHealth
	);

	// Broadcast after updating health so UI and hit reactions read the new value.
	onDamaged.Broadcast(damageAmount, attackerActor, hitLocation, attackDirection);
	
	if (currentHealth <= 0.0f)
	{
		isDead = true;
		onDeath.Broadcast(GetOwner());
	}
}
