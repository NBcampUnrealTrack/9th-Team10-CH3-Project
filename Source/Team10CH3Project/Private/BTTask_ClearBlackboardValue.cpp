#include "BTTask_ClearBlackboardValue.h"
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
	blackboard->ClearValue(keyToClear.SelectedKeyName);

	return EBTNodeResult::Succeeded;
}
