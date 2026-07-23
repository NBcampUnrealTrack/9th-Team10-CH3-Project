#include "BTTask_LookAround.h"
#include "AIController.h"
struct FBTLookAroundMemory
{
	float baseYaw = 0.0f;
	float elapsedTime = 0.0f;
};

UBTTask_LookAround::UBTTask_LookAround()
{
	NodeName = "Look Around";
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
	FBTLookAroundMemory* memory = reinterpret_cast<FBTLookAroundMemory*>(nodeMemory);
	memory->baseYaw = enemyPawn->GetActorRotation().Yaw;
	memory->elapsedTime = 0.0f;
	return EBTNodeResult::InProgress;
}

void UBTTask_LookAround::TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
	AAIController* aiController = ownerComp.GetAIOwner();
	APawn* enemyPawn = aiController ? aiController->GetPawn() : nullptr;
	if (!aiController || !enemyPawn)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Failed);
		return;
	}

	FBTLookAroundMemory* memory = reinterpret_cast<FBTLookAroundMemory*>(nodeMemory);
	memory->elapsedTime += deltaSeconds;
	const float angleOffset = FMath::Sin(memory->elapsedTime * lookSpeed) * lookAngleDegrees;

	FRotator newRotation = aiController->GetControlRotation();
	newRotation.Yaw = memory->baseYaw + angleOffset;
	aiController->SetControlRotation(newRotation);
	if (memory->elapsedTime >= lookDuration)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTask_LookAround::GetInstanceMemorySize() const
{
	return sizeof(FBTLookAroundMemory);
}
