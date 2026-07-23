#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FindPatrolPoint.generated.h"

UCLASS()
class TEAM10CH3PROJECT_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector patrolLocationKey;
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float patrolRadius = 1500.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
};
