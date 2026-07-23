// 헤더 가드
#pragma once

// 언리얼 기본 타입 전체 포함
#include "CoreMinimal.h"

// UBTTaskNode(태스크 노드의 부모 클래스) 정의 포함
#include "BehaviorTree/BTTaskNode.h"

// FBlackboardKeySelector 타입 정의 (이 엔진 버전에서는 BehaviorTreeTypes.h에 있음)
#include "BehaviorTree/BehaviorTreeTypes.h"

// UHT 리플렉션 코드, 항상 마지막에 include
#include "BTTask_FindPatrolPoint.generated.h"

UCLASS()
// 적 주변의 NavMesh에서 "갈 수 있는 랜덤 지점" 하나를 골라 블랙보드에 저장하는 태스크.
// 실제 이동은 이 태스크가 아니라, 뒤이어 실행되는 엔진 기본 Move To가 담당한다.
class TEAM10CH3PROJECT_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();

	// 골라낸 순찰 목적지(위치)를 저장할 블랙보드 키. Vector 타입 키만 고를 수 있게 제한한다.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector patrolLocationKey;

	// 적의 현재 위치를 중심으로, 이 반경(cm) 안에서 랜덤 지점을 찾는다. 1500 = 15미터.
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float patrolRadius = 1500.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
};
