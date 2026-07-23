#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ClearBlackboardValue.generated.h"

UCLASS()
class TEAM10CH3PROJECT_API UBTTask_ClearBlackboardValue : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ClearBlackboardValue();
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector keyToClear;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
};
