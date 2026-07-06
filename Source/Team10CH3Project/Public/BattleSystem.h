#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleSystem.generated.h"

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
	void Attack(AActor* targetActor, float damageAmount);

	UFUNCTION(BlueprintCallable)
	void FireLineTrace(AActor* shooterActor, float attackDamageAmount, float attackRange);

	UFUNCTION(BlueprintCallable)
	void AttackAround(AActor* attackerActor, float damageAmount, float attackRange);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|BasicAttack")
	float basicAttackDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|BasicAttack")
	float basicAttackRange = 3000.0f;

	UFUNCTION(BlueprintCallable)
	void RequestBasicAttack(AActor* attackerActor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float skillAttackDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|SkillAttack")
	float skillAttackRange = 500.0f;

	UFUNCTION(BlueprintCallable)
	void RequestSkillAttack(AActor* attackerActor);




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Debug")
	bool showDebug = true;
};