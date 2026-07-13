// 자기 자신의 헤더를 가장 먼저 include (관례)
#include "EnemyAIController.h"

// RunBehaviorTree() 함수에 behaviorTreeAsset(UBehaviorTree*)을 넘겨주려면, 그 클래스의
// 실제 정의를 알아야 한다. 헤더에서는 전방 선언만 했으므로, 여기 cpp에서 진짜로 include한다.
#include "BehaviorTree/BehaviorTree.h"

// 지금 이 파일에서 직접 쓰는 코드는 없지만, 나중에 Blackboard에 접근할 일이 생길 걸 대비해
// 미리 include해둔 헤더다. (UBlackboardComponent는 AAIController가 내부적으로 관리한다.)
#include "BehaviorTree/BlackboardComponent.h"

// 생성자 정의. 지금은 특별히 초기화할 값이 없어서 본문이 비어있다.
AEnemyAIController::AEnemyAIController()
{
}

// OnPossess는 이 컨트롤러가 폰을 빙의하는 순간 자동으로 호출된다.
// inPawn은 "지금 빙의하는 대상 폰"을 가리키는 파라미터다.
void AEnemyAIController::OnPossess(APawn* inPawn)
{
	// 부모 클래스(AAIController)의 기본 빙의 처리 로직을 먼저 실행해준다.
	// (내부적으로 폰과 컨트롤러를 서로 연결하는 등의 필수 작업을 한다.)
	Super::OnPossess(inPawn);

	// behaviorTreeAsset이 에디터에서 실제로 지정돼 있는지 확인한다.
	// (지정 안 해놓고 실행하면 nullptr이라 RunBehaviorTree를 호출할 수 없다.)
	if (behaviorTreeAsset)
	{
		// RunBehaviorTree()는 AAIController가 기본으로 제공하는 함수로,
		// 지정된 Behavior Tree 에셋의 실행을 시작한다. 이 순간부터 BT의 Root 노드부터
		// 시작해서 서비스/태스크들이 실제로 동작하게 된다. 필요한 Blackboard도 이 함수가
		// 내부적으로 자동으로 준비해준다.
		RunBehaviorTree(behaviorTreeAsset);
	}
}
