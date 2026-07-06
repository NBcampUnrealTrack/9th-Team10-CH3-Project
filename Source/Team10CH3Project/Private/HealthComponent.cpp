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

void UHealthComponent::TakeDamage(float damageAmount)
{

	if (isDead)
	{
		return;
	}

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

void UHealthComponent::HandleDeath()
{
	AActor* owner = GetOwner();

	if (owner)
	{
		owner->Destroy();
	}
}