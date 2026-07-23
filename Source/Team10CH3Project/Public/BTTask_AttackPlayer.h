#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_AttackPlayer.generated.h"

UCLASS()
class TEAM10CH3PROJECT_API UBTTask_AttackPlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttackPlayer();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector targetActorKey;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

private:
	EBTNodeResult::Type FireShot(UBehaviorTreeComponent& ownerComp) const;
};
