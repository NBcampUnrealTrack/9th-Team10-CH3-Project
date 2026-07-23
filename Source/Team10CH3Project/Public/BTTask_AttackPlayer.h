// 헤더 가드
#pragma once

// 언리얼 기본 타입 전체 포함
#include "CoreMinimal.h"

// UBTTaskNode는 "Behavior Tree 태스크 노드"의 부모 클래스다.
// 태스크는 서비스와 달리 "한 번 실행되고 성공/실패 결과를 반환하며 끝나는" 노드다(리프 노드, 나뭇가지의 끝).
// 우리 경우 "공격을 한 번 시도한다"가 하나의 태스크가 된다.
#include "BehaviorTree/BTTaskNode.h"

// FBlackboardKeySelector 구조체가 정의된 헤더 (이 언리얼 버전에서는 BehaviorTreeTypes.h에 있음)
#include "BehaviorTree/BehaviorTreeTypes.h"

// UHT가 리플렉션 코드를 채워 넣는 자리, 항상 마지막에 include
#include "BTTask_AttackPlayer.generated.h"

UCLASS()
// UBTTask_AttackPlayer는 UBTTaskNode를 상속하는 "공격 시도" 태스크 노드 클래스다.
class TEAM10CH3PROJECT_API UBTTask_AttackPlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 생성자 선언
	UBTTask_AttackPlayer();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	// targetActorKey는 "공격 대상을 블랙보드의 어느 키에서 읽어올지"를 지정하는 선택기다.
	// BTService_DetectPlayer가 저장해둔 것과 같은 키(targetActor)를 가리키도록 에디터에서 설정한다.
	FBlackboardKeySelector targetActorKey;

protected:
	// virtual/override: 부모 클래스(UBTTaskNode)의 ExecuteTask를 재정의한다.
	// 이 함수는 이 태스크 노드가 실제로 "실행"될 때 호출되며, 성공(Succeeded)/실패(Failed)/
	// 진행 중(InProgress) 중 하나를 반환값(EBTNodeResult::Type)으로 알려줘야 한다.
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;

};
