// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTService_DetectPlayer.h"
// 블랙보드 값 읽고 쓰는 UBlackboardComponent 정의
#include "BehaviorTree/BlackboardComponent.h"
// AAIController 정의, 이 서비스를 실행 중인 컨트롤러 + SetFocus/ClearFocus를 쓰기 위해 필요
#include "AIController.h"
// AEnemyCharacter 정의, sightRadius/sightHalfAngleDegrees 값을 읽기 위해 필요
#include "EnemyCharacter.h"
// UGameplayStatics::GetPlayerPawn 함수 사용을 위해 필요
#include "Kismet/GameplayStatics.h"

UBTService_DetectPlayer::UBTService_DetectPlayer()
{
	NodeName = "Detect Player (FOV + Distance)";

	Interval = 0.2f;

	targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayer, targetActorKey), AActor::StaticClass());

	// lastKnownLocationKey는 위치(Vector)를 저장할 키라서 Object가 아니라 Vector 타입으로 제한한다.
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

	APawn* playerPawn = UGameplayStatics::GetPlayerPawn(enemy, 0);
	if (!playerPawn)
	{
		return;
	}

	const FVector toPlayer = playerPawn->GetActorLocation() - enemy->GetActorLocation();
	const float distance = toPlayer.Size();

	bool detected = false;
	if (distance <= enemy->sightRadius)
	{
		// 시야각은 수평면(XY)에서만 계산한다. 기존처럼 3D 벡터를 그대로 비교하면 플레이어가
		// 계단이나 위층으로 올라갔을 때 높이 차이가 시야각에 포함되어, 정면에 있어도 FOV 밖으로
		// 잘못 판정될 수 있다. 수직 방향의 가시성은 아래 눈높이 라인 트레이스가 담당한다.
		FVector forwardDir2D = enemy->GetActorForwardVector();
		forwardDir2D.Z = 0.f;
		forwardDir2D.Normalize();

		FVector toPlayerDir2D = toPlayer;
		toPlayerDir2D.Z = 0.f;

		// 플레이어가 적의 거의 바로 위/아래에 있으면 수평 방향 벡터가 0에 가까워진다.
		// 이 경우 수평 시야각은 0도로 취급하고 실제로 보이는지는 라인 트레이스로 판단한다.
		float angleDegrees = 0.f;
		if (!toPlayerDir2D.IsNearlyZero())
		{
			toPlayerDir2D.Normalize();
			const float dotClamped = FMath::Clamp(FVector::DotProduct(forwardDir2D, toPlayerDir2D), -1.f, 1.f);
			angleDegrees = FMath::RadiansToDegrees(FMath::Acos(dotClamped));
		}

		// 거리와 시야각 조건을 통과한 다음, "적과 플레이어 사이에 벽 같은 장애물이 있는지"를
		// 라인 트레이스로 확인한다. 이게 있어야 플레이어가 물체 뒤로 숨었을 때 시야가 끊긴다.
		if (angleDegrees <= enemy->sightHalfAngleDegrees)
		{
			FHitResult sightHit;
			FCollisionQueryParams sightParams;
			// 자기 자신은 트레이스 대상에서 제외 (안 그러면 자기 캡슐에 바로 막힘).
			sightParams.AddIgnoredActor(enemy);

			// 액터 원점끼리 트레이스하면 보통 발밑 높이를 잇게 되어 계단 단이나 바닥에 쉽게 막힌다.
			// 양쪽의 실제 눈높이를 사용해 위층/아래층을 바라보는 자연스러운 시선을 검사한다.
			FVector traceStart;
			FRotator enemyEyeRotation;
			enemy->GetActorEyesViewPoint(traceStart, enemyEyeRotation);

			FVector traceEnd;
			FRotator playerEyeRotation;
			playerPawn->GetActorEyesViewPoint(traceEnd, playerEyeRotation);

			// 적 -> 플레이어로 라인 트레이스. 중간에 뭔가에 막히면(isBlocked = true) 그게 무엇인지 확인한다.
			const bool isBlocked = enemy->GetWorld()->LineTraceSingleByChannel(sightHit, traceStart, traceEnd, ECC_Visibility, sightParams);

			// 아무것도 안 막았거나(막힘 없음), 막은 게 바로 플레이어 자신이면 = 시야가 뚫려 있음.
			// 막은 게 플레이어가 아닌 다른 것(벽 등)이면 = 시야가 차단됨.
			const bool hasLineOfSight = !isBlocked || sightHit.GetActor() == playerPawn;

			detected = hasLineOfSight;
		}
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
		blackboard->SetValueAsObject(targetActorKey.SelectedKeyName, playerPawn);

		// 보이는 동안 계속 갱신 -> 자연스럽게 "마지막으로 본 위치"가 항상 최신 상태로 유지된다.
		// 놓치는 순간부터는 이 줄이 더 이상 실행되지 않으니, 그 시점의 값이 그대로 남는다.
		blackboard->SetValueAsVector(lastKnownLocationKey.SelectedKeyName, playerPawn->GetActorLocation());

		// SetFocus는 AI 컨트롤러가 매 프레임 이 대상(플레이어) 쪽으로 시선(컨트롤 회전)을 돌리게 만든다.
		// 서비스는 0.2초마다 실행되지만, 실제 회전은 컨트롤러가 매 프레임 부드럽게 갱신해준다.
		// 그래서 플레이어가 옆으로 움직여도 적이 몸을 돌려 시야 안에 계속 붙잡아 둔다.
		aiController->SetFocus(playerPawn);
	}
	else
	{
		blackboard->ClearValue(targetActorKey.SelectedKeyName);
		blackboard->SetValueAsBool(combatReadyKey.SelectedKeyName, false);

		// 더 이상 안 보이면 시선 고정을 해제해서, 마지막으로 보던 방향에 몸이 묶여있지 않게 한다.
		aiController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
