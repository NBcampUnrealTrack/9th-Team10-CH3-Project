#include "WeaponComponent.h"
#include "BattleSystem.h"
#include "HealthComponent.h"
#include "TimerManager.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	currentAmmo = maxAmmo;
}

void UWeaponComponent::Init(ABattleSystem* inBattleSystem, UHealthComponent* inHealthComponent)
{
	battleSystem = inBattleSystem;
	healthComponent = inHealthComponent;
}

bool UWeaponComponent::CanAttack() const
{
	const bool isAlive = !healthComponent || !healthComponent->isDead;
	return isAlive && !isReloading && currentAmmo > 0;
}

bool UWeaponComponent::CanReload() const
{
	const bool isAlive = !healthComponent || !healthComponent->isDead;
	return isAlive && !isReloading && currentAmmo < maxAmmo && reserveAmmo > 0;
}

void UWeaponComponent::Attack(AActor* attackerActor, const FVector& viewLocation, const FVector& viewDirection, const FVector& fireLocation)
{
	if (!CanAttack())
	{
		return;
	}

	currentAmmo--;

	if (battleSystem)
	{
		battleSystem->RequestBasicAttackByView(attackerActor, viewLocation, viewDirection, fireLocation);
	}
}

void UWeaponComponent::Reload()
{
	if (!CanReload())
	{
		return;
	}

	isReloading = true;

	GetWorld()->GetTimerManager().SetTimer(
		reloadTimerHandle,
		this,
		&UWeaponComponent::FinishReload,
		reloadTime,
		false
	);
}

void UWeaponComponent::FinishReload()
{
	const int neededAmmo = maxAmmo - currentAmmo;
	const int reloadAmmo = FMath::Min(neededAmmo, reserveAmmo);

	currentAmmo += reloadAmmo;
	reserveAmmo -= reloadAmmo;

	isReloading = false;

}
