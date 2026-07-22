// 헤더 가드
#pragma once

// 언리얼 기본 타입 전체 포함
#include "CoreMinimal.h"

// UBTService는 "Behavior Tree 서비스 노드"의 부모 클래스다.
// 서비스 노드는 태스크와 달리 한 번 실행되고 끝나는 게 아니라, 자신이 붙어있는 브랜치(가지)가
// 활성화돼 있는 동안 정해진 시간 간격마다 계속 반복 실행된다. 그래서 "주기적으로 확인해야 하는
// 로직"(우리 경우: 플레이어를 감지했는지 확인)에 적합하다.
#include "BehaviorTree/BTService.h"

// FBlackboardKeySelector라는 구조체가 정의된 헤더. 이 구조체는 "블랙보드의 어떤 키를 쓸지"를
// 에디터에서 드롭다운으로 고를 수 있게 해주는 타입이다. (참고: 이 언리얼 버전에서는
// BlackboardKeySelector.h가 아니라 BehaviorTreeTypes.h에 정의돼 있다.)
#include "BehaviorTree/BehaviorTreeTypes.h"

// UHT가 리플렉션 코드를 채워 넣는 자리, 항상 마지막에 include
#include "BTService_DetectPlayer.generated.h"

// 리플렉션 시스템에 등록
UCLASS()
// UBTService_DetectPlayer는 UBTService를 상속하는, "플레이어를 감지하는 서비스 노드" 클래스다.
// 클래스 이름 앞에 U가 붙은 건 언리얼 컨벤션으로 "이 클래스는 UObject를 상속한다"는 뜻이다.
class TEAM10CH3PROJECT_API UBTService_DetectPlayer : public UBTService
{
	GENERATED_BODY()

public:
	// 생성자 선언
	UBTService_DetectPlayer();

	// 에디터에서 이 값을 지정할 수 있게 등록
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	// targetActorKey는 "감지된 플레이어를 블랙보드의 어느 키에 저장할지"를 담는 선택기(selector)다.
	// 실제 값(문자열 이름)은 BT 에셋을 만들 때 에디터에서 드롭다운으로 골라서 지정한다.
	FBlackboardKeySelector targetActorKey;

	// 감지되는 동안(매 틱) 플레이어의 현재 위치로 계속 갱신되는 Vector 키.
	// 놓치는 순간부터 갱신을 멈추기 때문에, 그 값이 자연스럽게 "마지막으로 본 위치"로 남는다.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector lastKnownLocationKey;

	// Reset when the target is lost so a newly detected player must be aimed at again.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector combatReadyKey;

protected:
	// virtual/override: 부모 클래스(UBTService)의 TickNode 함수를 재정의한다.
	// TickNode는 이 서비스가 활성화된 상태에서, 정해진 시간 간격(Interval)마다 자동으로 호출되는 함수다.
	// UBehaviorTreeComponent& ownerComp: 이 서비스를 실행 중인 Behavior Tree 컴포넌트에 대한 참조.
	//   참조(&)는 포인터와 비슷하게 "원본 객체를 직접 다룬다"는 뜻이지만, nullptr이 될 수 없다는 점이 다르다.
	// uint8* nodeMemory: 이 노드가 개체별로 저장해둘 수 있는 raw 메모리 공간. 우리 서비스는 안 쓴다.
	// float deltaSeconds: 마지막 TickNode 호출 이후 지난 시간(초).
	virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;
};
