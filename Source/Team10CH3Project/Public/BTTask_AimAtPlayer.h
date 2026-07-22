#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_AimAtPlayer.generated.h"

/** Stops the enemy and gives the player a short reaction window before firing. */
UCLASS()
class TEAM10CH3PROJECT_API UBTTask_AimAtPlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AimAtPlayer();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector targetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector combatReadyKey;

	UPROPERTY(EditAnywhere, Category = "Aim", meta = (ClampMin = "0.0"))
	float aimDuration = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Aim", meta = (ClampMin = "0.0"))
	float randomDeviation = 0.2f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
};
