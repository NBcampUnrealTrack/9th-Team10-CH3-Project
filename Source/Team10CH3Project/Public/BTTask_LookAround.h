#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LookAround.generated.h"

UCLASS()
class TEAM10CH3PROJECT_API UBTTask_LookAround : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LookAround();
	UPROPERTY(EditAnywhere, Category = "LookAround")
	float lookDuration = 3.0f;
	UPROPERTY(EditAnywhere, Category = "LookAround")
	float lookAngleDegrees = 60.0f;
	UPROPERTY(EditAnywhere, Category = "LookAround")
	float lookSpeed = 1.5f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override;
};
