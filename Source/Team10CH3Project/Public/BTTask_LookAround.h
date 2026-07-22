// 헤더 가드
#pragma once

// 언리얼 기본 타입 전체 포함
#include "CoreMinimal.h"

// UBTTaskNode(태스크 노드의 부모 클래스) 정의 포함
#include "BehaviorTree/BTTaskNode.h"

// UHT 리플렉션 코드, 항상 마지막에 include
#include "BTTask_LookAround.generated.h"

UCLASS()
// 정해진 시간 동안 제자리에서 좌우로 부드럽게 회전하며 "두리번거리는" 태스크.
// Wait처럼 한 틱에 끝나지 않고 여러 틱에 걸쳐 실행되는 "latent task"라서 TickTask를 오버라이드한다.
class TEAM10CH3PROJECT_API UBTTask_LookAround : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LookAround();

	// 두리번거리는 데 걸리는 총 시간(초).
	UPROPERTY(EditAnywhere, Category = "LookAround")
	float lookDuration = 3.0f;

	// 시작 방향 기준으로 좌우 최대 몇 도까지 고개(몸)를 돌릴지.
	UPROPERTY(EditAnywhere, Category = "LookAround")
	float lookAngleDegrees = 60.0f;

	// 좌우로 왕복하는 속도. 클수록 더 빨리 두리번거린다.
	UPROPERTY(EditAnywhere, Category = "LookAround")
	float lookSpeed = 1.5f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;

	// TickTask는 이 태스크가 "진행 중(InProgress)"인 동안 매 프레임 호출된다.
	// Wait 같은 즉시 끝나는 태스크와 달리, 좌우 회전을 매 프레임 갱신해야 해서 필요하다.
	virtual void TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override;
};
