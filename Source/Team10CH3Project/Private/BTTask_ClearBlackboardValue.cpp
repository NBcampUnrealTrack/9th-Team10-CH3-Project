// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTTask_ClearBlackboardValue.h"

// 블랙보드 값을 지우는 UBlackboardComponent 정의
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearBlackboardValue::UBTTask_ClearBlackboardValue()
{
	NodeName = "Clear Blackboard Value";
}

EBTNodeResult::Type UBTTask_ClearBlackboardValue::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	if (!blackboard)
	{
		return EBTNodeResult::Failed;
	}

	// ClearValue는 키의 타입과 상관없이 항상 동작하므로, keyToClear에 어떤 타입의 키를 지정해도 된다.
	blackboard->ClearValue(keyToClear.SelectedKeyName);

	return EBTNodeResult::Succeeded;
}
