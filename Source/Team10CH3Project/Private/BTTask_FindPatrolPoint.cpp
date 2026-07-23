#include "BTTask_FindPatrolPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	NodeName = "Find Patrol Point";
	patrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, patrolLocationKey));
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	if (!blackboard)
	{
		return EBTNodeResult::Failed;
	}
	AAIController* aiController = ownerComp.GetAIOwner();
	APawn* enemyPawn = aiController ? aiController->GetPawn() : nullptr;
	if (!enemyPawn)
	{
		return EBTNodeResult::Failed;
	}
	UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(enemyPawn->GetWorld());
	if (!navSystem)
	{
		return EBTNodeResult::Failed;
	}
	FNavLocation randomPoint;
	const bool found = navSystem->GetRandomReachablePointInRadius(
		enemyPawn->GetActorLocation(),
		patrolRadius,
		randomPoint
	);

	if (!found)
	{
		return EBTNodeResult::Failed;
	}
	blackboard->SetValueAsVector(patrolLocationKey.SelectedKeyName, randomPoint.Location);

	return EBTNodeResult::Succeeded;
}
