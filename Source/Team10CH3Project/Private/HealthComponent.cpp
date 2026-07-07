#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	currentHealth = maxHealth;

	onDeath.AddDynamic(this, &UHealthComponent::HandleDeath);
}

void UHealthComponent::TakeDamage(float damageAmount, AActor* attackerActor, FVector hitLocation)
{

	if (isDead)
	{
		return;
	}

	onDamaged.Broadcast(damageAmount, attackerActor, hitLocation);

	currentHealth -= damageAmount;

	if (currentHealth < 0.0f)
	{
		currentHealth = 0.0f;
	}
	
	if (currentHealth <= 0.0f)
	{
		isDead = true;
		onDeath.Broadcast();
	}
}

FVector UHealthComponent::GetDirectionOwnerToActor(AActor* targetActor) const
{
	AActor* owner = GetOwner();

	if (owner == nullptr || targetActor == nullptr)
	{
		return FVector::ZeroVector;
	}

	FVector direction = targetActor->GetActorLocation() - owner->GetActorLocation();
	direction.Normalize();

	return direction;
}

void UHealthComponent::HandleDeath()
{
	AActor* owner = GetOwner();
}