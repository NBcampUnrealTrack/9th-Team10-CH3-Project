// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTTask_FindPatrolPoint.h"

// 블랙보드에 값을 저장하는 UBlackboardComponent 정의
#include "BehaviorTree/BlackboardComponent.h"

// 이 태스크를 실행 중인 AI 컨트롤러(AAIController)를 가져오기 위해 필요
#include "AIController.h"

// UNavigationSystemV1(NavMesh 위에서 랜덤 지점을 찾아주는 시스템)을 쓰기 위해 필요
#include "NavigationSystem.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	// BT 에디터 그래프에 표시될 노드 이름
	NodeName = "Find Patrol Point";

	// patrolLocationKey로 고를 수 있는 블랙보드 키를 Vector 타입으로 제한한다.
	// (순찰 목적지는 위치이므로 Vector가 맞다. Object 키를 실수로 고르는 걸 막아준다.)
	patrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, patrolLocationKey));
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	// 이 BT가 쓰는 블랙보드를 가져온다.
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	if (!blackboard)
	{
		return EBTNodeResult::Failed;
	}

	// 이 BT를 실행 중인 AI 컨트롤러와, 그 컨트롤러가 조종 중인 폰(적)을 가져온다.
	AAIController* aiController = ownerComp.GetAIOwner();
	APawn* enemyPawn = aiController ? aiController->GetPawn() : nullptr;
	if (!enemyPawn)
	{
		return EBTNodeResult::Failed;
	}

	// UNavigationSystemV1::GetCurrent(월드)는 현재 월드의 내비게이션 시스템(NavMesh 관리자)을 가져온다.
	UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(enemyPawn->GetWorld());
	if (!navSystem)
	{
		return EBTNodeResult::Failed;
	}

	// FNavLocation은 "NavMesh 위의 한 지점"을 담는 구조체다. 여기에 결과가 채워진다.
	FNavLocation randomPoint;

	// GetRandomReachablePointInRadius(중심, 반경, 결과)는 중심 위치에서 반경 안에 있으면서
	// "실제로 걸어서 갈 수 있는(NavMesh로 연결된)" 랜덤 지점 하나를 찾아준다.
	// 성공하면 true를 반환하고 randomPoint에 위치를 채운다.
	const bool found = navSystem->GetRandomReachablePointInRadius(
		enemyPawn->GetActorLocation(),
		patrolRadius,
		randomPoint
	);

	if (!found)
	{
		// 반경 안에 갈 수 있는 지점을 못 찾았으면(예: NavMesh 밖이거나 막힌 경우) 실패 처리.
		return EBTNodeResult::Failed;
	}

	// 찾은 위치를 블랙보드의 patrolLocation 키에 저장한다. 이후 Move To가 이 값을 목적지로 쓴다.
	blackboard->SetValueAsVector(patrolLocationKey.SelectedKeyName, randomPoint.Location);

	return EBTNodeResult::Succeeded;
}
