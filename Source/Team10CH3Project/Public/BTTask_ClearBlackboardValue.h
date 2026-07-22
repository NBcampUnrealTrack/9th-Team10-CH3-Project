// 헤더 가드
#pragma once

// 언리얼 기본 타입 전체 포함
#include "CoreMinimal.h"

// UBTTaskNode(태스크 노드의 부모 클래스) 정의 포함
#include "BehaviorTree/BTTaskNode.h"

// FBlackboardKeySelector 타입 정의
#include "BehaviorTree/BehaviorTreeTypes.h"

// UHT 리플렉션 코드, 항상 마지막에 include
#include "BTTask_ClearBlackboardValue.generated.h"

UCLASS()
// 지정된 블랙보드 키를 비우기만 하는 범용 태스크. 특정 타입에 묶이지 않아서
// Object/Vector 등 어떤 키 종류에도 재사용할 수 있다 (일부러 AddXFilter를 안 걸었다).
// 우리는 "마지막 목격 위치 조사"가 끝났을 때 lastKnownLocation을 비우는 데 쓴다.
class TEAM10CH3PROJECT_API UBTTask_ClearBlackboardValue : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ClearBlackboardValue();

	// 비울 대상 블랙보드 키.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector keyToClear;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
};
