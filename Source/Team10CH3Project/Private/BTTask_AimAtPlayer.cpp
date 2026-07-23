#include "BTTask_AimAtPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyCharacter.h"

struct FBTAimAtPlayerMemory
{
	float elapsedTime;
	float requiredDuration;
};

UBTTask_AimAtPlayer::UBTTask_AimAtPlayer()
{
	NodeName = "Aim At Player";
	bNotifyTick = true;

	targetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_AimAtPlayer, targetActorKey),
		AActor::StaticClass());
	combatReadyKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_AimAtPlayer, combatReadyKey));
}

EBTNodeResult::Type UBTTask_AimAtPlayer::ExecuteTask(
	UBehaviorTreeComponent& ownerComp,
	uint8* nodeMemory)
{
	const FName resolvedTargetActorKey = targetActorKey.SelectedKeyName.IsNone()
		? FName(TEXT("targetActor"))
		: targetActorKey.SelectedKeyName;
	const FName resolvedCombatReadyKey = combatReadyKey.SelectedKeyName.IsNone()
		? FName(TEXT("isCombatReady"))
		: combatReadyKey.SelectedKeyName;

	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	AAIController* aiController = ownerComp.GetAIOwner();
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	AActor* target = blackboard
		? Cast<AActor>(blackboard->GetValueAsObject(resolvedTargetActorKey))
		: nullptr;

	if (!blackboard || !aiController || !enemy || !IsValid(target))
	{
		return EBTNodeResult::Failed;
	}

	if (FVector::Dist(enemy->GetActorLocation(), target->GetActorLocation()) > enemy->attackRange)
	{
		return EBTNodeResult::Failed;
	}

	blackboard->SetValueAsBool(resolvedCombatReadyKey, false);
	aiController->StopMovement();
	aiController->SetFocus(target);

	FBTAimAtPlayerMemory* memory = reinterpret_cast<FBTAimAtPlayerMemory*>(nodeMemory);
	memory->elapsedTime = 0.0f;
	memory->requiredDuration = FMath::Max(
		0.0f,
		aimDuration + FMath::FRandRange(-randomDeviation, randomDeviation));

	if (memory->requiredDuration <= KINDA_SMALL_NUMBER)
	{
		blackboard->SetValueAsBool(resolvedCombatReadyKey, true);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_AimAtPlayer::TickTask(
	UBehaviorTreeComponent& ownerComp,
	uint8* nodeMemory,
	float deltaSeconds)
{
	const FName resolvedTargetActorKey = targetActorKey.SelectedKeyName.IsNone()
		? FName(TEXT("targetActor"))
		: targetActorKey.SelectedKeyName;
	const FName resolvedCombatReadyKey = combatReadyKey.SelectedKeyName.IsNone()
		? FName(TEXT("isCombatReady"))
		: combatReadyKey.SelectedKeyName;

	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	AAIController* aiController = ownerComp.GetAIOwner();
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	AActor* target = blackboard
		? Cast<AActor>(blackboard->GetValueAsObject(resolvedTargetActorKey))
		: nullptr;

	if (!blackboard || !aiController || !enemy || !IsValid(target) ||
		FVector::Dist(enemy->GetActorLocation(), target->GetActorLocation()) > enemy->attackRange)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Failed);
		return;
	}

	aiController->SetFocus(target);

	FBTAimAtPlayerMemory* memory = reinterpret_cast<FBTAimAtPlayerMemory*>(nodeMemory);
	memory->elapsedTime += deltaSeconds;
	if (memory->elapsedTime >= memory->requiredDuration)
	{
		blackboard->SetValueAsBool(resolvedCombatReadyKey, true);
		FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTask_AimAtPlayer::GetInstanceMemorySize() const
{
	return sizeof(FBTAimAtPlayerMemory);
}
