// 자기 자신의 헤더, 관례상 가장 먼저 include
#include "BTService_DetectPlayer.h"

// 블랙보드의 값을 실제로 읽고 쓰려면 UBlackboardComponent의 함수(SetValueAsObject 등)가 필요하다.
#include "BehaviorTree/BlackboardComponent.h"

// 이 서비스를 실행 중인 AI 컨트롤러(AAIController)를 가져오기 위해 필요.
#include "AIController.h"

// enemy->sightRadius처럼 우리가 만든 캐릭터의 값을 읽으려면 실제 클래스 정의가 필요하다.
#include "EnemyCharacter.h"

// UGameplayStatics::GetPlayerPawn()이라는 "현재 조작 중인 플레이어의 폰을 가져오는" 유틸리티 함수를 쓰기 위해 필요.
#include "Kismet/GameplayStatics.h"

// 생성자 정의. 이 서비스 노드가 만들어질 때 한 번 실행된다.
UBTService_DetectPlayer::UBTService_DetectPlayer()
{
	// NodeName은 Behavior Tree 에디터의 그래프에서 이 노드 위에 표시될 이름이다.
	// 팀원들이 그래프를 볼 때 이름만으로 무슨 노드인지 알 수 있게 해준다.
	NodeName = "Detect Player (FOV + Distance)";

	// Interval은 "TickNode를 몇 초마다 호출할지" 정하는 값이다.
	// 0.2f로 설정하면 매 프레임(보통 1/60초)이 아니라 0.2초에 한 번만 계산하므로 성능을 아낄 수 있다.
	Interval = 0.2f;

	// AddObjectFilter는 "targetActorKey로 고를 수 있는 블랙보드 키의 타입을 제한"하는 함수다.
	// 여기서는 AActor 타입을 담는 Object 키만 선택 가능하게 제한한다.
	// 이렇게 안 해두면 에디터에서 Bool이나 Float 같은 잘못된 타입의 키도 고를 수 있게 되어 버린다.
	// GET_MEMBER_NAME_CHECKED는 "targetActorKey"라는 멤버 변수 이름을 문자열이 아니라
	// 실제 멤버로 검증해서 가져오는 매크로다. 나중에 변수 이름이 바뀌면 컴파일 에러로 바로 알 수 있다.
	targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayer, targetActorKey), AActor::StaticClass());
}

// Interval(0.2초)마다 자동으로 호출되는 함수의 실제 구현.
void UBTService_DetectPlayer::TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
	// 부모 클래스(UBTService)의 기본 처리를 먼저 실행해준다. (관례적으로 항상 먼저 호출)
	Super::TickNode(ownerComp, nodeMemory, deltaSeconds);

	// ownerComp.GetBlackboardComponent()는 이 Behavior Tree가 사용 중인 블랙보드를 가져온다.
	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	// 블랙보드가 아직 준비되지 않았을 수도 있으므로(예: BT 에셋에 블랙보드가 안 지정된 경우), null 체크를 한다.
	// 이 체크가 없으면 nullptr에 대해 함수를 호출해서 크래시가 날 수 있다.
	if (!blackboard)
	{
		// return은 여기서 함수 실행을 즉시 끝낸다는 뜻이다. 더 진행할 수 없으니 그냥 종료.
		return;
	}

	// GetAIOwner()는 이 Behavior Tree를 실행 중인 AI 컨트롤러를 가져온다.
	AAIController* aiController = ownerComp.GetAIOwner();
	// 삼항 연산자(조건 ? A : B): 조건이 참이면 A, 거짓이면 B를 결과로 사용한다.
	// aiController가 있으면 그 컨트롤러가 조종 중인 폰(GetPawn())을 가져와서 AEnemyCharacter로
	// "형변환(Cast)"을 시도한다. Cast<T>는 "이 객체가 실제로 T 타입이거나 T의 자식 타입이면
	// 그 타입의 포인터로 바꿔주고, 아니면 nullptr을 반환"하는 안전한 형변환 함수다.
	AEnemyCharacter* enemy = aiController ? Cast<AEnemyCharacter>(aiController->GetPawn()) : nullptr;
	// enemy가 nullptr이면(컨트롤러가 없거나, 조종 중인 폰이 EnemyCharacter가 아니면) 더 진행할 수 없다.
	if (!enemy)
	{
		return;
	}

	// GetPlayerPawn(월드 컨텍스트, 플레이어 인덱스)는 현재 게임을 조작하는 플레이어의 폰을 가져온다.
	// 인덱스 0은 "첫 번째(유일한) 플레이어"를 의미한다(우리 프로젝트는 싱글플레이라 항상 0).
	APawn* playerPawn = UGameplayStatics::GetPlayerPawn(enemy, 0);
	// 아직 플레이어 폰이 스폰되지 않았을 수도 있으니 null 체크.
	if (!playerPawn)
	{
		return;
	}

	// toPlayer는 "적의 위치에서 플레이어의 위치로 향하는 벡터"다. (도착점 - 출발점)
	const FVector toPlayer = playerPawn->GetActorLocation() - enemy->GetActorLocation();
	// Size()는 벡터의 길이(크기)를 계산한다. 여기서는 적과 플레이어 사이의 실제 거리를 의미한다.
	// const는 "이 변수는 한 번 값이 정해지면 이후 절대 바뀌지 않는다"는 뜻으로, 실수로 값을 바꾸는 버그를 막아준다.
	const float distance = toPlayer.Size();

	// detected는 "이번 틱에 플레이어를 감지했는지 여부"를 담는 변수. 기본값은 false(감지 못함)로 시작.
	bool detected = false;
	// 먼저 거리 조건을 확인한다: 플레이어가 시야 반경(sightRadius) 안에 있는지.
	if (distance <= enemy->sightRadius)
	{
		// GetActorForwardVector()는 이 액터가 현재 "정면"으로 바라보는 방향을 나타내는, 길이가 1인 벡터다.
		const FVector forwardDir = enemy->GetActorForwardVector();
		// GetSafeNormal()은 벡터의 방향은 유지하면서 길이를 1로 만들어주는 함수다("정규화"라고 부른다).
		// 두 방향의 "각도 차이"만 알고 싶을 때는, 길이는 상관없고 방향만 비교해야 하므로 정규화가 필요하다.
		const FVector toPlayerDir = toPlayer.GetSafeNormal();
		// DotProduct(내적)는 두 벡터 사이의 관계를 나타내는 계산으로, 두 벡터가 둘 다 길이 1이면
		// 결과값이 "두 방향 사이 각도의 코사인 값"이 된다(정면으로 완전히 같은 방향이면 1, 정반대면 -1).
		// FMath::Clamp(값, -1, 1)은 부동소수점 계산 오차로 값이 아주 살짝 1을 넘거나 -1보다 작아지는 것을
		// 강제로 -1~1 범위 안으로 눌러준다. 이게 없으면 바로 아래의 Acos가 정의되지 않는 값(NaN)을
		// 반환할 위험이 있다.
		const float dotClamped = FMath::Clamp(FVector::DotProduct(forwardDir, toPlayerDir), -1.f, 1.f);
		// Acos(아크코사인)는 코사인 값을 다시 각도(라디안)로 되돌려주는 함수다.
		// RadiansToDegrees는 라디안 단위를 우리가 익숙한 도(degree) 단위로 바꿔준다.
		const float angleDegrees = FMath::RadiansToDegrees(FMath::Acos(dotClamped));

		// 계산된 각도가 시야각의 절반(sightHalfAngleDegrees) 이내면 감지된 것으로 판정한다.
		detected = angleDegrees <= enemy->sightHalfAngleDegrees;
	}

	// 감지 결과에 따라 블랙보드 값을 갱신한다. 이 값은 Behavior Tree의 다른 노드(태스크, 데코레이터)들이
	// 읽어서 "지금 공격할 대상이 있는지"를 판단하는 데 쓰인다.
	if (detected)
	{
		// SetValueAsObject(키 이름, 저장할 객체)는 블랙보드의 해당 키에 값을 저장한다.
		blackboard->SetValueAsObject(targetActorKey.SelectedKeyName, playerPawn);
	}
	else
	{
		// ClearValue(키 이름)는 블랙보드의 해당 키 값을 비운다(없음 상태로 만든다).
		blackboard->ClearValue(targetActorKey.SelectedKeyName);
	}
}
