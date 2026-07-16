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

void UWeaponComponent::Attack()
{
	if (!CanAttack())
	{
		return;
	}

	currentAmmo--;

	UE_LOG(LogTemp, Warning, TEXT("Attack! Ammo: %d / %d"), currentAmmo, reserveAmmo);

	if (battleSystem)
	{
		battleSystem->RequestBasicAttack(GetOwner());
	}
}

void UWeaponComponent::Reload()
{
	if (!CanReload())
	{
		return;
	}

	isReloading = true;

	UE_LOG(LogTemp, Warning, TEXT("Reload Start"));

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

	UE_LOG(LogTemp, Warning, TEXT("Reload Finish! Ammo: %d / %d"), currentAmmo, reserveAmmo);
}
