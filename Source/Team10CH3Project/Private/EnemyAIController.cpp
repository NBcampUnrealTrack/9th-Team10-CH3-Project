#include "EnemyAIController.h"
#include "FPSGameMode.h"
#include "BehaviorTree/BehaviorTree.h"

AEnemyAIController::AEnemyAIController()
{
}

void AEnemyAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);

	hasStartedBehaviorTree = false;

	AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>();
	if (!gameMode)
	{
		StartBehaviorTree();
		return;
	}

	gameMode->onGameStarted.AddUniqueDynamic(this, &AEnemyAIController::StartBehaviorTree);
	if (gameMode->isGameStarted)
	{
		StartBehaviorTree();
	}
}

void AEnemyAIController::StartBehaviorTree()
{
	if (hasStartedBehaviorTree || !behaviorTreeAsset || !GetPawn())
	{
		return;
	}

	hasStartedBehaviorTree = RunBehaviorTree(behaviorTreeAsset);
	if (hasStartedBehaviorTree)
	{
		if (AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>())
		{
			gameMode->onGameStarted.RemoveDynamic(this, &AEnemyAIController::StartBehaviorTree);
		}
	}
}
