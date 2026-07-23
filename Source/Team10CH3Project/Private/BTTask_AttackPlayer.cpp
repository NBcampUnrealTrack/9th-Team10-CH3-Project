#include "BTTask_AttackPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "EnemyCharacter.h"
#include "FPSGameMode.h"
#include "HealthComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"

namespace
{
	struct FAttackBurstMemory
	{
		int32 remainingShots = 0;
		float nextShotTime = 0.0f;
	};

	UHealthComponent* FindHealthComponentFromHit(AActor* hitActor)
	{
		TSet<AActor*> visitedActors;
		AActor* currentActor = hitActor;

		while (IsValid(currentActor) && !visitedActors.Contains(currentActor))
		{
			visitedActors.Add(currentActor);

			if (UHealthComponent* health = currentActor->FindComponentByClass<UHealthComponent>())
			{
				return health;
			}

			AActor* nextActor = currentActor->GetOwner();
			if (!IsValid(nextActor))
			{
				nextActor = currentActor->GetAttachParentActor();
			}

			currentActor = nextActor;
		}

		return nullptr;
	}
}
UBTTask_AttackPlayer::UBTTask_AttackPlayer()
{
	NodeName = "Attack Player If In Range";
	bNotifyTick = true;
	targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AttackPlayer, targetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_AttackPlayer::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	AEnemyCharacter* enemy = ownerComp.GetAIOwner()
		? Cast<AEnemyCharacter>(ownerComp.GetAIOwner()->GetPawn())
		: nullptr;
	if (!enemy)
	{
		return EBTNodeResult::Failed;
	}

	const AFPSGameMode* gameMode = enemy->GetWorld()->GetAuthGameMode<AFPSGameMode>();
	const FEnemyDifficultySettings* difficultySettings = gameMode
		? &gameMode->GetCurrentEnemyDifficultySettings()
		: nullptr;
	const int32 minBurstShots = difficultySettings
		? FMath::Max(1, difficultySettings->minBurstShots)
		: 1;
	const int32 maxBurstShots = difficultySettings
		? FMath::Max(minBurstShots, difficultySettings->maxBurstShots)
		: 1;

	FAttackBurstMemory* memory = reinterpret_cast<FAttackBurstMemory*>(nodeMemory);
	memory->remainingShots = FMath::RandRange(minBurstShots, maxBurstShots);

	const EBTNodeResult::Type shotResult = FireShot(ownerComp);
	if (shotResult == EBTNodeResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	--memory->remainingShots;
	if (memory->remainingShots <= 0)
	{
		return EBTNodeResult::Succeeded;
	}

	memory->nextShotTime = enemy->GetWorld()->GetTimeSeconds()
		+ (difficultySettings ? difficultySettings->burstShotInterval : 0.12f);
	return EBTNodeResult::InProgress;
}

void UBTTask_AttackPlayer::TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
	FAttackBurstMemory* memory = reinterpret_cast<FAttackBurstMemory*>(nodeMemory);
	AEnemyCharacter* enemy = ownerComp.GetAIOwner()
		? Cast<AEnemyCharacter>(ownerComp.GetAIOwner()->GetPawn())
		: nullptr;
	if (!enemy)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Failed);
		return;
	}

	if (enemy->GetWorld()->GetTimeSeconds() < memory->nextShotTime)
	{
		return;
	}

	const EBTNodeResult::Type shotResult = FireShot(ownerComp);
	if (shotResult == EBTNodeResult::Failed)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Failed);
		return;
	}

	--memory->remainingShots;
	if (memory->remainingShots <= 0)
	{
		FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);
		return;
	}

	const AFPSGameMode* gameMode = enemy->GetWorld()->GetAuthGameMode<AFPSGameMode>();
	const float burstShotInterval = gameMode
		? gameMode->GetCurrentEnemyDifficultySettings().burstShotInterval
		: 0.12f;
	memory->nextShotTime = enemy->GetWorld()->GetTimeSeconds() + burstShotInterval;
}

uint16 UBTTask_AttackPlayer::GetInstanceMemorySize() const
{
	return sizeof(FAttackBurstMemory);
}

EBTNodeResult::Type UBTTask_AttackPlayer::FireShot(UBehaviorTreeComponent& ownerComp) const
{
	const FName resolvedTargetActorKey = targetActorKey.SelectedKeyName.IsNone()
		? FName(TEXT("targetActor"))
		: targetActorKey.SelectedKeyName;
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	if (!blackboard)
	{
		return EBTNodeResult::Failed;
	}
	AAIController* aiController = ownerComp.GetAIOwner();
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	if (!enemy)
	{
		return EBTNodeResult::Failed;
	}
	AActor* target = Cast<AActor>(blackboard->GetValueAsObject(resolvedTargetActorKey));
	if (!target)
	{
		return EBTNodeResult::Failed;
	}
	UHealthComponent* targetHealth = FindHealthComponentFromHit(target);
	if (!targetHealth || targetHealth->isDead)
	{
		blackboard->ClearValue(resolvedTargetActorKey);
		blackboard->ClearValue(TEXT("lastKnownLocation"));
		blackboard->SetValueAsBool(TEXT("isCombatReady"), false);
		aiController->ClearFocus(EAIFocusPriority::Gameplay);
		enemy->SetAlertMovementMode(false);
		return EBTNodeResult::Failed;
	}
	const float distance = FVector::Dist(enemy->GetActorLocation(), target->GetActorLocation());
	if (distance > enemy->attackRange)
	{
		return EBTNodeResult::Failed;
	}

	FVector traceStart;
	FRotator enemyEyeRotation;
	enemy->GetActorEyesViewPoint(traceStart, enemyEyeRotation);

	const AFPSGameMode* gameMode = enemy->GetWorld()->GetAuthGameMode<AFPSGameMode>();
	const FEnemyDifficultySettings* difficultySettings = gameMode
		? &gameMode->GetCurrentEnemyDifficultySettings()
		: nullptr;
	FVector targetPoint;
	FRotator targetEyeRotation;
	target->GetActorEyesViewPoint(targetPoint, targetEyeRotation);
	if (difficultySettings && !difficultySettings->aimBoneName.IsNone())
	{
		if (const ACharacter* characterTarget = Cast<ACharacter>(target))
		{
			if (const USkeletalMeshComponent* targetMesh = characterTarget->GetMesh();
				targetMesh && targetMesh->GetBoneIndex(difficultySettings->aimBoneName) != INDEX_NONE)
			{
				targetPoint = targetMesh->GetBoneLocation(difficultySettings->aimBoneName)
					+ FVector::UpVector * difficultySettings->aimVerticalOffset;
			}
		}
	}
	const FVector idealDirection = (targetPoint - traceStart).GetSafeNormal();
	const float attackAccuracy = FMath::Clamp(
		enemy->attackAccuracy * (difficultySettings ? difficultySettings->attackAccuracyMultiplier : 1.0f),
		0.0f,
		1.0f);
	const float maxSpreadDegrees = enemy->attackMaxSpreadDegrees
		* (difficultySettings ? difficultySettings->attackSpreadMultiplier : 1.0f);
	const float spreadDegrees = FMath::Lerp(maxSpreadDegrees, 0.f, attackAccuracy);
	const float spreadRadians = FMath::DegreesToRadians(spreadDegrees);
	const FVector fireDirection = FMath::VRandCone(idealDirection, spreadRadians);
	const FVector traceEnd = traceStart + fireDirection * enemy->attackRange;
	FHitResult hitResult;
	FCollisionQueryParams traceParams(SCENE_QUERY_STAT(EnemyAttackTrace), false, enemy);
	if (ACharacter* characterTarget = Cast<ACharacter>(target))
	{
		traceParams.AddIgnoredComponent(characterTarget->GetCapsuleComponent());
	}
	const bool isHit = enemy->GetWorld()->LineTraceSingleByChannel(
		hitResult,
		traceStart,
		traceEnd,
		ECC_Visibility,
		traceParams);
	if (enemy->attackTraceDrawDuration > 0.0f)
	{
		DrawDebugLine(
			enemy->GetWorld(),
			traceStart,
			isHit ? hitResult.ImpactPoint : traceEnd,
			isHit ? FColor::Green : FColor::Red,
			false,
			enemy->attackTraceDrawDuration,
			0,
			1.0f);
	}
	// Hit components can belong to attached actors; resolve their owning health component.
	UHealthComponent* hitHealth = isHit
		? FindHealthComponentFromHit(hitResult.GetActor())
		: nullptr;
	const FString difficultyName = gameMode
		? gameMode->GetDifficultyDisplayName().ToString()
		: TEXT("None");

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("AI attack trace executed: Difficulty=%s Enemy=%s Target=%s Hit=%s HitActor=%s Bone=%s HasHealth=%s"),
		*difficultyName,
		*GetNameSafe(enemy),
		*GetNameSafe(target),
		isHit ? TEXT("true") : TEXT("false"),
		*GetNameSafe(hitResult.GetActor()),
		*hitResult.BoneName.ToString(),
		hitHealth ? TEXT("true") : TEXT("false"));
	if (isHit)
	{
		if (hitHealth)
		{
			float damageMultiplier = difficultySettings
				? difficultySettings->attackDamageMultiplier * difficultySettings->bodyDamageMultiplier
				: 1.0f;
			if (gameMode && gameMode->IsHeadBone(hitResult.BoneName))
			{
				damageMultiplier *= difficultySettings->headDamageMultiplier;
			}

			hitHealth->TakeDamage(enemy->attackDamage * damageMultiplier, enemy, hitResult.ImpactPoint);
		}
	}
	return EBTNodeResult::Succeeded;
}
