// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTTask_LookAround.h"

// 이 태스크를 실행 중인 AI 컨트롤러(AAIController)를 가져오기 위해, 그리고 시선(컨트롤 회전)을
// 직접 조작하기 위해 필요하다.
#include "AIController.h"

// 개체별로 "시작 방향"과 "지금까지 지난 시간"을 따로 기억해야 해서 NodeMemory에 저장할 구조체.
// 이 태스크 노드 자체는 모든 적이 공유하므로, 평범한 멤버 변수로는 개체별 상태를 못 담는다
// (BTTask_AttackPlayer의 쿨타임과 같은 이유).
struct FBTLookAroundMemory
{
	// 두리번거리기 시작 시점의 몸 방향(Yaw). 이 각도를 기준으로 좌우로 왔다갔다한다.
	float baseYaw = 0.0f;

	// 이 태스크가 시작된 뒤 지금까지 지난 시간(초).
	float elapsedTime = 0.0f;
};

UBTTask_LookAround::UBTTask_LookAround()
{
	NodeName = "Look Around";

	// bNotifyTick을 true로 켜야 TickTask()가 실제로 매 프레임 호출된다. 기본값은 false(꺼짐)다.
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_LookAround::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	AAIController* aiController = ownerComp.GetAIOwner();
	APawn* enemyPawn = aiController ? aiController->GetPawn() : nullptr;
	if (!enemyPawn)
	{
		return EBTNodeResult::Failed;
	}

	// nodeMemory(원시 바이트 메모리)를 우리 구조체로 해석해서 초기값을 채운다.
	FBTLookAroundMemory* memory = reinterpret_cast<FBTLookAroundMemory*>(nodeMemory);
	memory->baseYaw = enemyPawn->GetActorRotation().Yaw;
	memory->elapsedTime = 0.0f;

	// InProgress를 반환하면 "아직 안 끝났다"는 뜻으로, 이후 TickTask가 매 프레임 호출되기 시작한다.
	// Wait 같은 즉시 완료 태스크는 여기서 Succeeded/Failed를 바로 반환하지만, 이 태스크는 시간이
	// 걸리는 동작이라 InProgress로 시작한다.
	return EBTNodeResult::InProgress;
}

void UBTTask_LookAround::TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
	AAIController* aiController = ownerComp.GetAIOwner();
	APawn* enemyPawn = aiController ? aiController->GetPawn() : nullptr;
	if (!aiController || !enemyPawn)
	{
		// 컨트롤러나 폰이 사라졌으면(예: 그 사이 사망) 더 진행할 수 없으니 실패로 마무리한다.
		FinishLatentTask(ownerComp, EBTNodeResult::Failed);
		return;
	}

	FBTLookAroundMemory* memory = reinterpret_cast<FBTLookAroundMemory*>(nodeMemory);
	memory->elapsedTime += deltaSeconds;

	// FMath::Sin은 -1~1 사이를 오가는 물결 모양 값을 준다. 여기에 lookAngleDegrees를 곱하면
	// "기준 방향에서 좌우로 최대 lookAngleDegrees도까지 왔다갔다하는 오프셋"이 된다.
	// lookSpeed가 클수록 sin 함수 안의 각도가 더 빨리 증가해서, 더 빠르게 좌우로 움직인다.
	const float angleOffset = FMath::Sin(memory->elapsedTime * lookSpeed) * lookAngleDegrees;

	FRotator newRotation = aiController->GetControlRotation();
	newRotation.Yaw = memory->baseYaw + angleOffset;

	// SetControlRotation은 이 컨트롤러가 바라보는 방향을 직접 지정한다. AEnemyCharacter 생성자에서
	// bUseControllerRotationYaw = true로 설정해뒀기 때문에, 이 회전이 그대로 몸 회전에 반영된다.
	aiController->SetControlRotation(newRotation);

	// 정해진 시간이 다 지났으면 태스크를 성공으로 마무리한다. FinishLatentTask를 호출해야
	// InProgress 상태에서 벗어나 다음 노드로 넘어간다.
	if (memory->elapsedTime >= lookDuration)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTask_LookAround::GetInstanceMemorySize() const
{
	return sizeof(FBTLookAroundMemory);
}
