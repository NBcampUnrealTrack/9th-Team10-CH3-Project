// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTTask_AttackPlayer.h"

// 블랙보드 값을 읽는 UBlackboardComponent의 함수(GetValueAsObject)를 쓰기 위해 필요.
#include "BehaviorTree/BlackboardComponent.h"

// 이 태스크를 실행 중인 AI 컨트롤러(AAIController)를 가져오기 위해 필요.
#include "AIController.h"

// enemy->attackRange, enemy->attackDamage 등 우리가 만든 캐릭터의 값을 읽으려면 실제 정의가 필요하다.
#include "EnemyCharacter.h"

// 전투 로직 담당 팀원이 만든 체력 컴포넌트. TakeDamage() 함수를 호출하려면 실제 정의가 필요하다.
#include "HealthComponent.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
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

// 생성자 정의. 이 태스크 노드가 만들어질 때 한 번 실행된다.
UBTTask_AttackPlayer::UBTTask_AttackPlayer()
{
	// BT 에디터 그래프에서 이 노드 위에 표시될 이름
	NodeName = "Attack Player If In Range";

	// targetActorKey로 고를 수 있는 블랙보드 키를 AActor 타입의 Object 키로 제한한다.
	// (BTService_DetectPlayer의 생성자와 같은 이유)
	targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AttackPlayer, targetActorKey), AActor::StaticClass());
}

// 이 태스크가 Behavior Tree에서 실행될 때 호출되는 함수의 실제 구현.
// 반환값(EBTNodeResult::Type)에 따라 상위 노드(Selector 등)가 다음 행동을 결정한다:
// Succeeded면 "이 가지의 목표를 달성했다"로 처리되고, Failed면 "이 가지는 실패했으니 다른 자식을 시도하라"로 처리된다.
EBTNodeResult::Type UBTTask_AttackPlayer::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	const FName resolvedTargetActorKey = targetActorKey.SelectedKeyName.IsNone()
		? FName(TEXT("targetActor"))
		: targetActorKey.SelectedKeyName;

	// 이 Behavior Tree가 쓰는 블랙보드 컴포넌트를 가져온다.
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	// 블랙보드가 없으면(설정 오류 등) 더 진행할 수 없으므로 실패 처리.
	if (!blackboard)
	{
		return EBTNodeResult::Failed;
	}

	// 이 Behavior Tree를 실행 중인 AI 컨트롤러를 가져온다.
	AAIController* aiController = ownerComp.GetAIOwner();
	// 컨트롤러가 조종 중인 폰을 AEnemyCharacter로 안전하게 형변환한다. 실패하면 nullptr이 된다.
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	// enemy가 없으면 공격을 실행할 대상 자체가 없는 것이므로 실패 처리.
	if (!enemy)
	{
		return EBTNodeResult::Failed;
	}

	// 블랙보드에 저장된 "공격 대상"을 가져온다. GetValueAsObject는 UObject* 타입을 반환하므로,
	// 우리가 원하는 AActor* 타입으로 다시 Cast한다.
	AActor* target = Cast<AActor>(blackboard->GetValueAsObject(resolvedTargetActorKey));
	// target이 없다는 건 아직 플레이어가 탐지되지 않았다는 뜻이므로 실패 처리.
	if (!target)
	{
		return EBTNodeResult::Failed;
	}

	// FVector::Dist(A, B)는 두 위치 사이의 직선 거리를 계산해주는 함수다.
	const float distance = FVector::Dist(enemy->GetActorLocation(), target->GetActorLocation());
	// 대상이 공격 사거리(attackRange)보다 멀리 있으면 지금은 공격할 수 없다.
	if (distance > enemy->attackRange)
	{
		// Failed를 반환하면, 이 태스크의 상위 Selector가 "다음 자식"(보통 엔진 기본 Move To 태스크)을
		// 대신 시도하게 된다. 즉 사거리 밖이면 자동으로 "추적 이동"으로 넘어가는 구조다.
		return EBTNodeResult::Failed;
	}

	FVector traceStart;
	FRotator enemyEyeRotation;
	enemy->GetActorEyesViewPoint(traceStart, enemyEyeRotation);

	FVector targetPoint;
	FRotator targetEyeRotation;
	target->GetActorEyesViewPoint(targetPoint, targetEyeRotation);
	// 적의 정면 방향(GetActorForwardVector)이 아니라, "적 위치 -> 대상 위치" 벡터를 직접 계산해서 쓴다.
	// 이렇게 해야 적의 몸이 어느 방향을 보고 있든 상관없이 항상 대상을 정확히 조준한 방향이 나온다.
	const FVector idealDirection = (targetPoint - traceStart).GetSafeNormal();

	// FMath::Lerp(A, B, 비율)은 비율이 0이면 A, 1이면 B, 그 사이는 둘을 선형으로 섞은 값을 반환한다.
	// attackAccuracy가 1(완벽한 명중률)이면 spreadDegrees가 0이 되고, 0(부정확)이면
	// attackMaxSpreadDegrees만큼 조준 각도가 흔들릴 수 있게 된다.
	const float spreadDegrees = FMath::Lerp(enemy->attackMaxSpreadDegrees, 0.f, enemy->attackAccuracy);
	const float spreadRadians = FMath::DegreesToRadians(spreadDegrees);

	// VRandCone(방향, 원뿔 반각)은 주어진 방향을 중심으로 한 원뿔 범위 안에서 무작위 방향 하나를 뽑아준다.
	// spreadRadians가 0이면 항상 idealDirection 그대로 나가고, 커질수록 더 크게 빗나갈 수 있다.
	const FVector fireDirection = FMath::VRandCone(idealDirection, spreadRadians);
	// 조준 방향으로 attackRange만큼 뻗은 지점을 트레이스 종료점으로 삼는다.
	const FVector traceEnd = traceStart + fireDirection * enemy->attackRange;

	// 트레이스 결과(맞은 지점, 맞은 액터 등)를 담을 구조체.
	FHitResult hitResult;
	// 트레이스 세부 옵션을 담는 구조체.
	TArray<AActor*> actorsToIgnore;
	// 자기 자신은 트레이스 대상에서 제외한다. 안 그러면 자기 캡슐에 바로 막혀버린다.
	actorsToIgnore.Add(enemy);

	// 플레이어 쪽 BattleSystem::FireLineTrace와 동일한 채널(ECC_Visibility)을 써서,
	// 벽 같은 장애물에 똑같이 가로막히도록 한다. 이게 없으면 벽 뒤의 대상도 맞출 수 있게 된다.
	const bool isHit = UKismetSystemLibrary::LineTraceSingle(
		enemy,
		traceStart,
		traceEnd,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		actorsToIgnore,
		EDrawDebugTrace::ForDuration,
		hitResult,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		enemy->attackTraceDrawDuration);
	UHealthComponent* hitHealth = isHit
		? FindHealthComponentFromHit(hitResult.GetActor())
		: nullptr;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("AI attack trace executed: Enemy=%s Target=%s Hit=%s HitActor=%s HasHealth=%s"),
		*GetNameSafe(enemy),
		*GetNameSafe(target),
		isHit ? TEXT("true") : TEXT("false"),
		*GetNameSafe(hitResult.GetActor()),
		hitHealth ? TEXT("true") : TEXT("false"));

	// 트레이스가 뭔가에 맞았을 때만 데미지 여부를 판단한다. (벽에 막혔든, 조준이 빗나가서 다른 걸 맞췄든,
	// 원래 목표를 정확히 맞췄든 일단 hitResult에 정보가 담긴다.)
	if (isHit)
	{
		// 맞은 액터에 체력 컴포넌트가 있는지 확인한다. 벽처럼 체력 컴포넌트가 없는 걸 맞았으면
		// hitHealth가 nullptr이 되어 데미지가 적용되지 않는다 (벽 관통 방지).
		if (hitHealth)
		{
			// TakeDamage(데미지량, 공격자, 피격 위치)를 호출해서 실제로 체력을 깎는다.
			// 대상의 GetActorLocation() 대신, 실제로 트레이스가 맞은 지점(hitResult.ImpactPoint)을 넘긴다.
			hitHealth->TakeDamage(enemy->attackDamage, enemy, hitResult.ImpactPoint);
		}
	}
	// 트레이스가 아무것도 못 맞췄으면(오차 때문에 완전히 빗나갔으면) 데미지 없이 그냥 넘어간다.

	// 명중했든 빗나갔든 "공격 행동 자체"는 정상적으로 실행했으므로 Succeeded를 반환한다.
	// (명중 여부까지 실패로 처리하면 빗나갈 때마다 Move To로 폴백해서 계속 대상에게 다가가버린다.)
	return EBTNodeResult::Succeeded;
}
