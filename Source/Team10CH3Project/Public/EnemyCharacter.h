// #pragma once는 "헤더 가드"라고 부르는 지시문이다.
// 이 헤더 파일이 여러 cpp 파일에서 동시에 include되더라도, 컴파일러가 내용을 딱 한 번만 읽도록 막아준다.
// 이게 없으면 같은 클래스가 두 번 정의됐다는 컴파일 에러가 난다.
#pragma once

// CoreMinimal.h는 언리얼의 거의 모든 코드에서 공통으로 쓰는 기본 타입들
// (FVector, FString, TArray, float 관련 매크로 등)을 한꺼번에 모아서 포함시켜주는 헤더다.
// 언리얼 C++ 파일은 관례적으로 항상 이 헤더를 맨 위에서 include한다.
#include "CoreMinimal.h"

// 우리가 상속(inherit)받을 부모 클래스인 ACharacter의 실제 정의가 담긴 헤더다.
// ACharacter는 이미 이동, 점프, 충돌(캡슐), 3D 모델(스켈레탈 메시) 기능을 기본으로 가지고 있어서
// 이걸 상속하면 그 기능들을 그대로 물려받는다.
#include "GameFramework/Character.h"

// generated.h는 우리가 직접 쓰는 코드가 아니라, 언리얼의 "언리얼 헤더 툴(UHT)"이라는 프로그램이
// 컴파일 전에 자동으로 만들어주는 코드가 들어가는 자리다. UCLASS, UPROPERTY, UFUNCTION 같은
// 매크로들이 실제로 동작하려면 이 파일이 필요하다. 규칙상 항상 include 목록의 맨 마지막에 와야 한다.
#include "EnemyCharacter.generated.h"

// UHealthComponent 클래스를 통째로 include하지 않고, "이런 이름의 클래스가 존재한다"라고만
// 컴파일러에게 알려주는 것을 "전방 선언(forward declaration)"이라고 한다.
// 지금 이 헤더 파일에서는 UHealthComponent를 포인터(주소만 가리키는 변수)로만 사용하기 때문에
// 실제 클래스 내용을 몰라도 문제없다. 이렇게 하면 헤더 파일 간 의존성이 줄어들어 컴파일이 빨라진다.
class UHealthComponent;

// UCLASS()는 이 클래스를 언리얼의 "리플렉션 시스템"에 등록하는 매크로다.
// 리플렉션 시스템에 등록되면: 블루프린트에서 이 클래스를 상속할 수 있고, 에디터의 Details 패널에
// 프로퍼티가 노출되고, 언리얼의 가비지 컬렉터(자동 메모리 관리)가 이 클래스의 포인터들을 추적할 수 있게 된다.
UCLASS()
// AEnemyCharacter는 ACharacter를 상속(:)하는 새로운 클래스다.
// 클래스 이름 앞에 A가 붙은 건 언리얼 컨벤션으로 "이 클래스는 AActor를 상속한다"는 뜻이다(예: AActor, APawn, ACharacter 모두 A로 시작).
// TEAM10CH3PROJECT_API는 다른 모듈에서 이 클래스를 참조할 수 있게 export/import를 처리해주는 매크로.
// 지금은 모듈이 하나뿐이라 크게 의미는 없지만, 언리얼 프로젝트에서는 관례적으로 항상 붙인다.
class TEAM10CH3PROJECT_API AEnemyCharacter : public ACharacter
{
	// GENERATED_BODY()는 UHT가 이 클래스에 필요한 리플렉션 코드(생성자 관련 처리 등)를
	// 실제로 채워 넣는 매크로다. 클래스 본문의 맨 첫 줄에 반드시 있어야 한다.
	GENERATED_BODY()

// public: 아래에 선언된 멤버들은 이 클래스 밖의 다른 코드(다른 클래스, 블루프린트 등)에서도 접근할 수 있다.
public:
	// 생성자(Constructor) 선언. 클래스와 이름이 같고 반환형이 없는 특별한 함수로,
	// 이 클래스의 객체(액터)가 스폰될 때 딱 한 번 자동으로 호출된다.
	AEnemyCharacter();

	// UPROPERTY(...)는 아래에 있는 변수를 리플렉션 시스템에 등록하는 매크로다.
	// EditAnywhere: 언리얼 에디터의 Details 패널에서 이 값을 자유롭게 수정할 수 있다는 뜻.
	// BlueprintReadOnly: 블루프린트(비주얼 스크립팅)에서는 이 값을 읽을 수는 있지만 바꿀 수는 없다는 뜻.
	// Category = "AI|Detection": Details 패널에서 이 값이 "AI > Detection" 카테고리 아래에 묶여서 보인다는 뜻.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Detection")
	// float은 소수점을 가질 수 있는 숫자 타입이다. 뒤에 붙은 f는 "이 숫자는 float이다"라는 표시(리터럴 접미사).
	// sightRadius는 "이 적이 플레이어를 감지할 수 있는 최대 거리"를 의미하며, 단위는 언리얼 기본 단위인 cm다.
	// 즉 1200.f는 12미터를 뜻한다.
	float sightRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Detection")
	// sightHalfAngleDegrees는 "적이 정면을 바라볼 때, 좌우로 몇 도까지 볼 수 있는지"의 절반값이다.
	// 예를 들어 60이면, 정면 기준 왼쪽 60도 + 오른쪽 60도 = 총 120도 시야각을 의미한다.
	float sightHalfAngleDegrees = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	// attackRange는 "이 적이 공격을 실행할 수 있는 최대 거리"다. 대상이 이 거리보다 멀면 공격 대신 추적한다.
	float attackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	// attackDamage는 공격 한 번에 상대방 체력을 얼마나 깎을지 정하는 값이다.
	float attackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	// attackCooldown은 공격 한 번을 실행한 뒤, 다시 공격할 수 있게 되기까지 기다려야 하는 시간(초)이다.
	// 이게 없으면 사거리 안에 있는 동안 매 순간 계속 공격해버리는 문제가 생긴다.
	float attackCooldown = 1.5f;

	// VisibleAnywhere: 에디터에서 이 값을 "볼 수는" 있지만 직접 수정은 못 하게 막는다.
	// (컴포넌트는 보통 코드에서 직접 만들어서 연결하기 때문에, 에디터에서 손대면 오히려 꼬일 수 있어서 이렇게 설정한다.)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	// UHealthComponent*는 "UHealthComponent 객체가 메모리 어딘가에 있고, 그 위치(주소)를 가리키는 변수"라는 뜻이다.
	// 이런 변수를 포인터라고 부른다. = nullptr은 "지금은 아무것도 가리키고 있지 않다"는 초기값이다.
	// 실제 컴포넌트는 .cpp의 생성자에서 만들어져서 이 변수에 연결(대입)된다.
	UHealthComponent* healthComponent = nullptr;

// protected: 아래 멤버들은 이 클래스 자신과, 이 클래스를 상속하는 자식 클래스에서만 접근할 수 있다.
// 외부의 다른 클래스에서는 접근 불가능하다.
protected:
	// virtual: 이 함수는 부모 클래스(ACharacter)에도 이미 정의돼 있는데, 그걸 "재정의"할 수 있게 열어둔다는 뜻.
	// override: 우리가 실제로 부모의 함수를 재정의하고 있다는 걸 컴파일러에게 명시해서, 오타 등으로
	// 실수로 새로운 함수를 만들어버리는 실수를 컴파일 에러로 잡아준다.
	// BeginPlay()는 이 액터가 실제로 게임 월드에 존재하며 플레이가 시작되는 순간, 언리얼이 자동으로 호출해주는 함수다.
	virtual void BeginPlay() override;

	// UFUNCTION()은 이 함수를 리플렉션 시스템에 등록한다.
	// 델리게이트(이벤트)에 함수를 연결(바인딩)하려면, 그 함수가 반드시 UFUNCTION()으로 등록돼 있어야 한다.
	UFUNCTION()
	// HandleDeath는 "사망 처리"를 담당하는 함수다.
	// AActor* deadActor 파라미터는 healthComponent의 onDeath 델리게이트가 "누가 죽었는지"를
	// 알려주기 위해 넘겨주는 값이다. 델리게이트에 바인딩하는 함수는 델리게이트가 정의한 파라미터와
	// 정확히 똑같은 형태여야 한다.
	void HandleDeath(AActor* deadActor);
};
