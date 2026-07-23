#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"
class UBehaviorTree;
UCLASS()
class TEAM10CH3PROJECT_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AEnemyAIController();
	UPROPERTY(EditAnywhere, Category = "AI")
	UBehaviorTree* behaviorTreeAsset;
protected:
	virtual void OnPossess(APawn* inPawn) override;
};
