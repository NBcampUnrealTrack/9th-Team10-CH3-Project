#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_DetectPlayer.generated.h"
UCLASS()
class TEAM10CH3PROJECT_API UBTService_DetectPlayer : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_DetectPlayer();
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector targetActorKey;
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector lastKnownLocationKey;
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector combatReadyKey;

protected:
	virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
};
