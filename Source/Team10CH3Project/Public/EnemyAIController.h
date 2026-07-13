// 헤더 가드: 이 파일이 여러 곳에서 include돼도 딱 한 번만 처리되도록 막는다.
#pragma once

// 언리얼 기본 타입 전체를 모아서 포함하는 헤더. 언리얼 C++ 파일의 표준 첫 include.
#include "CoreMinimal.h"

// 우리가 상속할 AAIController 클래스의 실제 정의가 담긴 헤더.
// AAIController는 "폰을 대신 조종하는 두뇌 역할"을 하는 클래스로, Behavior Tree를 실행하고
// Blackboard(AI가 기억하는 정보 저장소)를 소유하는 기능이 기본으로 들어있다.
#include "AIController.h"

// UHT가 리플렉션 코드를 자동 생성해서 채워 넣는 자리. 항상 include 목록의 맨 마지막에 온다.
#include "EnemyAIController.generated.h"

// UBehaviorTree 클래스는 포인터로만 사용할 것이므로, 실제 정의를 include하지 않고
// "이런 클래스가 있다"라고만 알려주는 전방 선언으로 충분하다.
class UBehaviorTree;

// 이 클래스를 언리얼 리플렉션 시스템에 등록 (에디터에서 값 편집, 블루프린트 상속 등을 가능하게 함)
UCLASS()
// AEnemyAIController는 AAIController를 상속하는, 우리 프로젝트 전용 적 AI 컨트롤러 클래스다.
class TEAM10CH3PROJECT_API AEnemyAIController : public AAIController
{
	// UHT가 필요한 리플렉션 코드를 채워 넣는 매크로. 클래스 본문의 첫 줄에 항상 위치.
	GENERATED_BODY()

// 외부에서도 접근 가능한 멤버들
public:
	// 생성자 선언
	AEnemyAIController();

	// EditAnywhere: 에디터에서 이 값을 직접 지정할 수 있게 함.
	UPROPERTY(EditAnywhere, Category = "AI")
	// behaviorTreeAsset은 "이 컨트롤러가 실행할 Behavior Tree 에셋이 무엇인지"를 담는 포인터다.
	// 실제 값은 에디터에서 이 클래스의 블루프린트나 디폴트 값에 BT 에셋을 드래그해서 채워넣는다.
	UBehaviorTree* behaviorTreeAsset;

// 이 클래스와, 이 클래스를 상속하는 자식 클래스만 접근 가능한 멤버들
protected:
	// virtual/override: 부모 클래스(AAIController)가 이미 가진 OnPossess 함수를 재정의한다는 뜻.
	// OnPossess는 이 컨트롤러가 어떤 폰을 실제로 "빙의(조종 시작)"하는 바로 그 순간 호출되는 함수다.
	// 여기서 Behavior Tree 실행을 시작시킬 예정이다.
	virtual void OnPossess(APawn* inPawn) override;
};
