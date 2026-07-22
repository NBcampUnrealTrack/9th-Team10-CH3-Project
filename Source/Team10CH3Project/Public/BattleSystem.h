#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BattleSystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillAttackFailed);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnBasicAttackHit,
	AActor*,
	targetActor,
	float,
	finalDamage,
	bool,
	isHeadShot,
	FVector,
	hitLocation
);

class AGrenadeProjectile;

UCLASS()
class TEAM10CH3PROJECT_API ABattleSystem : public AActor
{
	GENERATED_BODY()

public:
	ABattleSystem();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void Attack(AActor* targetActor, float damageAmount, AActor* attackerActor, FVector hitLocation);

	UFUNCTION(BlueprintCallable)
	void FireLineTrace(AActor* shooterActor, float attackDamageAmount, float attackRange);

	UFUNCTION(BlueprintCallable)
	void AttackAround(AActor* attackerActor, float damageAmount, float attackRange);

	UFUNCTION(BlueprintCallable)
	void AttackAroundLocation(
		FVector attackLocation,
		AActor* attackerActor,
		float damageAmount,
		float attackRange
	);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|BasicAttack")
	float basicAttackDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|BasicAttack")
	float basicAttackRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|BasicAttack")
	float headShotMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|BasicAttack")
	FName headShotTag = TEXT("Head");

	UPROPERTY(BlueprintAssignable, Category = "Battle|BasicAttack")
	FOnBasicAttackHit onBasicAttackHit;

	UFUNCTION(BlueprintCallable)
	void RequestBasicAttack(AActor* attackerActor);

	UFUNCTION(BlueprintCallable)
	void RequestBasicAttackByView(
		AActor* attackerActor,
		FVector viewLocation,
		FVector viewDirection,
		FVector fireLocation
	);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float skillAttackDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float skillAttackRange = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float skillThrowDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float skillCooldown = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle|SkillAttack")
	bool isSkillOnCooldown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	TSubclassOf<AGrenadeProjectile> grenadeProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float grenadeThrowPower = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float grenadeUpPower = 300.0f;

	UPROPERTY(BlueprintAssignable, Category = "Battle|SkillAttack")
	FOnSkillAttackFailed onSkillAttackFailed;

	FTimerHandle skillCooldownTimerHandle;

	UFUNCTION(BlueprintCallable)
	bool CanUseSkillAttack() const;

	void StartSkillCooldown();

	void ResetSkillCooldown();

	UFUNCTION(BlueprintCallable)
	void RequestSkillAttack(AActor* attackerActor);

	UFUNCTION(BlueprintCallable)
	void RequestSkillAttackByView(
		AActor* attackerActor,
		FVector viewLocation,
		FVector viewDirection,
		FVector throwLocation
	);

private:
	bool IsHeadShot(const FHitResult& hitResult) const;
};
