#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadeProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class ABattleSystem;

UCLASS()
class TEAM10CH3PROJECT_API AGrenadeProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGrenadeProjectile();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	USphereComponent* collisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	UStaticMeshComponent* meshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	UProjectileMovementComponent* projectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float explodeDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float damageAmount = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float damageRange = 500.0f;

	void InitGrenade(
		ABattleSystem* inBattleSystem,
		AActor* inAttackerActor,
		FVector throwVelocity
	);

private:
	FTimerHandle explodeTimerHandle;

	UPROPERTY()
	ABattleSystem* battleSystem;

	UPROPERTY()
	AActor* attackerActor;

	void Explode();
};