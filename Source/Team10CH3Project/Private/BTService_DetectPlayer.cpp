#include "BTService_DetectPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "EnemyCharacter.h"
#include "FPSGameMode.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	struct FDetectPlayerMemory
	{
		float firstSightTime = 0.0f;
		bool isWaitingForInitialDetection = false;
	};
}

UBTService_DetectPlayer::UBTService_DetectPlayer()
{
	NodeName = "Detect Player (FOV + Distance)";

	Interval = 0.2f;

	targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayer, targetActorKey), AActor::StaticClass());
	lastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayer, lastKnownLocationKey));
	combatReadyKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayer, combatReadyKey));
}

void UBTService_DetectPlayer::TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
	Super::TickNode(ownerComp, nodeMemory, deltaSeconds);

	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	if (!blackboard)
	{
		return;
	}

	AAIController* aiController = ownerComp.GetAIOwner();
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	if (!enemy)
	{
		return;
	}
	FDetectPlayerMemory* memory = reinterpret_cast<FDetectPlayerMemory*>(nodeMemory);

	APawn* playerPawn = UGameplayStatics::GetPlayerPawn(enemy, 0);
	if (!playerPawn)
	{
		return;
	}

	if (const UHealthComponent* playerHealth = playerPawn->FindComponentByClass<UHealthComponent>();
		playerHealth && playerHealth->isDead)
	{
		blackboard->ClearValue(targetActorKey.SelectedKeyName);
		blackboard->ClearValue(lastKnownLocationKey.SelectedKeyName);
		blackboard->SetValueAsBool(combatReadyKey.SelectedKeyName, false);
		aiController->ClearFocus(EAIFocusPriority::Gameplay);
		enemy->SetAlertMovementMode(false);
		memory->isWaitingForInitialDetection = false;
		return;
	}

	const FVector toPlayer = playerPawn->GetActorLocation() - enemy->GetActorLocation();
	const float distance = toPlayer.Size();

	bool detected = false;
	bool hasDirectSight = false;
	if (distance <= enemy->sightRadius)
	{
		// FOV is horizontal; vertical visibility is handled by the eye-level LOS trace.
		FVector forwardDir2D = enemy->GetActorForwardVector();
		forwardDir2D.Z = 0.f;
		forwardDir2D.Normalize();

		FVector toPlayerDir2D = toPlayer;
		toPlayerDir2D.Z = 0.f;
		float angleDegrees = 0.f;
		if (!toPlayerDir2D.IsNearlyZero())
		{
			toPlayerDir2D.Normalize();
			const float dotClamped = FMath::Clamp(FVector::DotProduct(forwardDir2D, toPlayerDir2D), -1.f, 1.f);
			angleDegrees = FMath::RadiansToDegrees(FMath::Acos(dotClamped));
		}
		if (angleDegrees <= enemy->sightHalfAngleDegrees)
		{
			FHitResult sightHit;
			FCollisionQueryParams sightParams;
			sightParams.AddIgnoredActor(enemy);
			FVector traceStart;
			FRotator enemyEyeRotation;
			enemy->GetActorEyesViewPoint(traceStart, enemyEyeRotation);

			FVector traceEnd;
			FRotator playerEyeRotation;
			playerPawn->GetActorEyesViewPoint(traceEnd, playerEyeRotation);
			const bool isBlocked = enemy->GetWorld()->LineTraceSingleByChannel(sightHit, traceStart, traceEnd, ECC_Visibility, sightParams);
			const bool hasLineOfSight = !isBlocked || sightHit.GetActor() == playerPawn;

			if (hasLineOfSight)
			{
				hasDirectSight = true;
				const bool isAlreadyTrackingPlayer =
					blackboard->GetValueAsObject(targetActorKey.SelectedKeyName) == playerPawn;
				const float detectionDelay = [&enemy]()
				{
					if (const AFPSGameMode* gameMode = enemy->GetWorld()->GetAuthGameMode<AFPSGameMode>())
					{
						return gameMode->GetCurrentEnemyDifficultySettings().initialDetectionDelay;
					}
					return 0.0f;
				}();

				if (isAlreadyTrackingPlayer)
				{
					detected = true;
					memory->isWaitingForInitialDetection = false;
				}
				else
				{
					if (!memory->isWaitingForInitialDetection)
					{
						memory->firstSightTime = enemy->GetWorld()->GetTimeSeconds();
						memory->isWaitingForInitialDetection = true;
					}

					detected = enemy->GetWorld()->GetTimeSeconds() - memory->firstSightTime >= detectionDelay;
				}
			}
		}
	}
	if (!hasDirectSight)
	{
		memory->isWaitingForInitialDetection = false;
	}

	AActor* damageAlertTarget = enemy->GetDamageAlertTarget();
	const bool isSearchingLastKnownLocation =
		blackboard->IsVectorValueSet(lastKnownLocationKey.SelectedKeyName);
	enemy->SetAlertMovementMode(
		detected
		|| IsValid(damageAlertTarget)
		|| isSearchingLastKnownLocation);

	if (!detected && IsValid(damageAlertTarget))
	{
		blackboard->SetValueAsObject(targetActorKey.SelectedKeyName, damageAlertTarget);
		blackboard->SetValueAsVector(lastKnownLocationKey.SelectedKeyName, damageAlertTarget->GetActorLocation());
		aiController->SetFocus(damageAlertTarget);
	}
	else if (detected)
	{
		// Preserve the last observed position when direct sight is lost.
		blackboard->SetValueAsObject(targetActorKey.SelectedKeyName, playerPawn);
		blackboard->SetValueAsVector(lastKnownLocationKey.SelectedKeyName, playerPawn->GetActorLocation());
		aiController->SetFocus(playerPawn);
	}
	else
	{
		blackboard->ClearValue(targetActorKey.SelectedKeyName);
		blackboard->SetValueAsBool(combatReadyKey.SelectedKeyName, false);
		aiController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

uint16 UBTService_DetectPlayer::GetInstanceMemorySize() const
{
	return sizeof(FDetectPlayerMemory);
}
