// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTTask_AttackPlayer.h"

// 블랙보드 값을 읽는 UBlackboardComponent의 함수(GetValueAsObject)를 쓰기 위해 필요.
#include "BehaviorTree/BlackboardComponent.h"

// 이 태스크를 실행 중인 AI 컨트롤러(AAIController)를 가져오기 위해 필요.
#include "AIController.h"

// enemy->attackRange, enemy->attackDamage 등 우리가 만든 캐릭터의 값을 읽으려면 실제 정의가 필요하다.
#include "EnemyCharacter.h"

// 전투 로직 담당 팀원이 만든 체력 컴포넌트. TakeDamage() 함수를 호출하려면 실제 정의가 필요하다.
#include "HealthComponent.h"

// 이 태스크 노드(UBTTask_AttackPlayer) 객체 자체는 Behavior Tree 에셋 하나당 딱 하나만 만들어지고,
// 그 BT를 실행하는 모든 적들이 "같은 노드 객체"를 공유해서 쓴다. 만약 "마지막 공격 시각"을
// 이 클래스의 평범한 멤버 변수로 저장하면, 적이 여러 마리일 때 서로의 쿨타임 기록이 뒤섞여버린다
// (한 마리가 공격하면 다른 마리들도 전부 방금 공격한 것처럼 기록되는 문제).
// 그래서 언리얼 Behavior Tree는 "NodeMemory"라는, 각 적(각 Behavior Tree 인스턴스)마다
// 따로 할당되는 별도의 메모리 공간을 제공한다. 이 구조체가 바로 그 메모리에 저장할 데이터의 형태다.
struct FBTAttackPlayerMemory
{
	// lastAttackTime은 이 적이 마지막으로 공격에 성공한 게임 시간(초)이다.
	// -1000.f로 초기화해두는 이유는, 게임 시작 직후 첫 공격이 쿨타임 때문에 막히지 않게 하기 위함이다
	// (현재 시간 - (-1000)은 항상 attackCooldown보다 커지므로 첫 공격은 항상 허용된다).
	float lastAttackTime = -1000.f;
};

// 생성자 정의. 이 태스크 노드가 만들어질 때 한 번 실행된다.
UBTTask_AttackPlayer::UBTTask_AttackPlayer()
{
	// BT 에디터 그래프에서 이 노드 위에 표시될 이름
	NodeName = "Attack Player If In Range";

	// targetActorKey로 고를 수 있는 블랙보드 키를 AActor 타입의 Object 키로 제한한다.
	// (BTService_DetectPlayer의 생성자와 같은 이유)
	targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AttackPlayer, targetActorKey), AActor::StaticClass());
}

// 이 태스크가 Behavior Tree에서 실행될 때 호출되는 함수의 실제 구현.
// 반환값(EBTNodeResult::Type)에 따라 상위 노드(Selector 등)가 다음 행동을 결정한다:
// Succeeded면 "이 가지의 목표를 달성했다"로 처리되고, Failed면 "이 가지는 실패했으니 다른 자식을 시도하라"로 처리된다.
EBTNodeResult::Type UBTTask_AttackPlayer::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	// 이 Behavior Tree가 쓰는 블랙보드 컴포넌트를 가져온다.
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	// 블랙보드가 없으면(설정 오류 등) 더 진행할 수 없으므로 실패 처리.
	if (!blackboard)
	{
		return EBTNodeResult::Failed;
	}

	// 이 Behavior Tree를 실행 중인 AI 컨트롤러를 가져온다.
	AAIController* aiController = ownerComp.GetAIOwner();
	// 컨트롤러가 조종 중인 폰을 AEnemyCharacter로 안전하게 형변환한다. 실패하면 nullptr이 된다.
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	// enemy가 없으면 공격을 실행할 대상 자체가 없는 것이므로 실패 처리.
	if (!enemy)
	{
		return EBTNodeResult::Failed;
	}

	// 블랙보드에 저장된 "공격 대상"을 가져온다. GetValueAsObject는 UObject* 타입을 반환하므로,
	// 우리가 원하는 AActor* 타입으로 다시 Cast한다.
	AActor* target = Cast<AActor>(blackboard->GetValueAsObject(targetActorKey.SelectedKeyName));
	// target이 없다는 건 아직 플레이어가 탐지되지 않았다는 뜻이므로 실패 처리.
	if (!target)
	{
		return EBTNodeResult::Failed;
	}

	// FVector::Dist(A, B)는 두 위치 사이의 직선 거리를 계산해주는 함수다.
	const float distance = FVector::Dist(enemy->GetActorLocation(), target->GetActorLocation());
	// 대상이 공격 사거리(attackRange)보다 멀리 있으면 지금은 공격할 수 없다.
	if (distance > enemy->attackRange)
	{
		// Failed를 반환하면, 이 태스크의 상위 Selector가 "다음 자식"(보통 엔진 기본 Move To 태스크)을
		// 대신 시도하게 된다. 즉 사거리 밖이면 자동으로 "추적 이동"으로 넘어가는 구조다.
		return EBTNodeResult::Failed;
	}

	// nodeMemory는 uint8*(단순한 바이트 배열의 시작 주소) 형태로 넘어온다.
	// reinterpret_cast는 "이 메모리를 다른 타입으로 취급해서 읽어라"라고 컴파일러에게 지시하는,
	// 상당히 강한 형변환이다. 여기서는 이 메모리를 우리가 정의한 FBTAttackPlayerMemory 구조체로
	// 해석해서 쓰겠다는 뜻이다. GetInstanceMemorySize()에서 이 구조체 크기만큼의 공간을
	// 미리 요청해뒀기 때문에 안전하게 쓸 수 있다.
	FBTAttackPlayerMemory* memory = reinterpret_cast<FBTAttackPlayerMemory*>(nodeMemory);
	// GetWorld()는 현재 게임 월드에 대한 접근을 제공하고, GetTimeSeconds()는 "게임이 시작된 뒤로
	// 지난 시간(초)"을 반환한다.
	const float currentTime = enemy->GetWorld()->GetTimeSeconds();
	// 마지막 공격 시각으로부터 얼마나 시간이 지났는지 계산해서, 아직 쿨타임(attackCooldown)이
	// 다 지나지 않았으면 공격할 수 없다.
	if (currentTime - memory->lastAttackTime < enemy->attackCooldown)
	{
		// 쿨타임 중이라 실패 반환. 다음 BT 틱에 다시 이 태스크가 시도되며 조건을 재확인한다.
		return EBTNodeResult::Failed;
	}

	// FindComponentByClass<T>()는 대상 액터에 붙어있는 컴포넌트 중, T 타입(또는 T의 자식 타입)인
	// 컴포넌트를 찾아서 반환한다. 없으면 nullptr을 반환한다.
	UHealthComponent* targetHealth = target->FindComponentByClass<UHealthComponent>();
	// 체력 컴포넌트를 실제로 찾은 경우에만 데미지를 적용한다(플레이어가 아닌, 체력이 없는 액터를
	// 잘못 공격 대상으로 잡는 경우를 방지).
	if (targetHealth)
	{
		// TakeDamage(데미지량, 공격자, 피격 위치)를 호출해서 실제로 체력을 깎는다.
		// enemy를 공격자로 넘겨주면, 나중에 히트 인디케이터 쪽에서 "누가 공격했는지" 알 수 있다.
		targetHealth->TakeDamage(enemy->attackDamage, enemy, target->GetActorLocation());
	}

	// 방금 공격을 실행했으니, 다음 쿨타임 계산의 기준이 될 마지막 공격 시각을 지금으로 갱신한다.
	memory->lastAttackTime = currentTime;
	// 공격을 성공적으로 실행했으므로 Succeeded를 반환한다.
	return EBTNodeResult::Succeeded;
}

// 이 태스크가 개체별로 필요로 하는 메모리 크기를 언리얼에게 알려주는 함수.
// sizeof(FBTAttackPlayerMemory)는 이 구조체가 차지하는 바이트 수를 컴파일 시점에 계산해주는 연산자다.
// 언리얼은 이 값을 보고, 이 Behavior Tree를 실행하는 적 하나당 그만큼의 메모리를 따로 마련해준다.
uint16 UBTTask_AttackPlayer::GetInstanceMemorySize() const
{
	return sizeof(FBTAttackPlayerMemory);
}
