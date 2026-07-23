#include "EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
AEnemyAIController::AEnemyAIController()
{
}
void AEnemyAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);
	if (behaviorTreeAsset)
	{
		RunBehaviorTree(behaviorTreeAsset);
	}
}
