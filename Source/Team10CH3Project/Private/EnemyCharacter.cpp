// cpp 파일은 관례적으로 자기 자신의 헤더를 가장 먼저 include한다.
// 이렇게 하면 "이 헤더 파일이 그 자체로 완전한지"를 컴파일러가 검증해줄 수 있다.
#include "EnemyCharacter.h"

// AIControllerClass에 실제 클래스를 대입하려면(아래 7번째 줄), 그 클래스의 실제 정의가 필요하다.
// 헤더에서는 전방 선언도 없이 그냥 썼는데, 여기 cpp에서 include하는 것으로 충분하다.
#include "EnemyAIController.h"

// healthComponent를 실제로 생성하고(CreateDefaultSubobject), onDeath 델리게이트를 구독하려면
// UHealthComponent의 실제 정의가 필요해서 여기서 include한다. (헤더에서는 포인터만 쓰므로 전방 선언으로 충분했다.)
#include "HealthComponent.h"

// GetCharacterMovement()가 반환하는 UCharacterMovementComponent의 회피(Avoidance) 관련
// 설정값(bUseRVOAvoidance 등)을 쓰려면 실제 정의가 필요해서 include한다.
#include "GameFramework/CharacterMovementComponent.h"

// 생성자 정의. 이 액터가 스폰될 때 딱 한 번 실행된다.
AEnemyCharacter::AEnemyCharacter()
{
	// PrimaryActorTick은 이 액터의 Tick() 함수(매 프레임 자동으로 호출되는 함수)를 제어하는 설정이다.
	// bCanEverTick = false로 설정하면 Tick() 자체가 아예 호출되지 않는다.
	// 우리는 탐지/이동/공격 로직을 전부 Behavior Tree 쪽에서 처리하기 때문에, 캐릭터 자체의 Tick은
	// 필요 없다. 불필요한 Tick을 꺼두는 것은 성능을 아끼는 좋은 습관이다.
	PrimaryActorTick.bCanEverTick = false;

	// AIControllerClass는 "이 폰(캐릭터)을 자동으로 조종할 컨트롤러로 어떤 클래스를 쓸지" 지정하는 변수다.
	// AEnemyAIController::StaticClass()는 "AEnemyAIController라는 클래스 자체에 대한 정보(UClass)"를 가져오는 함수다.
	// (StaticClass는 특정 객체가 아니라 클래스 타입 자체를 나타내는 리플렉션 객체를 반환한다.)
	AIControllerClass = AEnemyAIController::StaticClass();

	// AutoPossessAI는 "AI 컨트롤러가 언제 자동으로 이 폰을 빙의(Possess, 조종을 시작)할지" 설정하는 열거형(enum) 값이다.
	// PlacedInWorldOrSpawned는 "레벨에 미리 배치해뒀든, 게임 중에 SpawnActor로 만들었든 상관없이 항상 자동으로 빙의한다"는 뜻이다.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// CreateDefaultSubobject<T>(TEXT("이름"))은 언리얼에서 "컴포넌트를 생성자 안에서 만드는 표준 방법"이다.
	// 일반 C++처럼 new UHealthComponent()를 직접 쓰면 안 되고, 반드시 이 함수를 통해 만들어야
	// 언리얼의 객체 관리 시스템(가비지 컬렉션, 직렬화 등)에 제대로 등록된다.
	// 결과적으로 healthComponent 포인터가 이제 실제 컴포넌트 객체를 가리키게 된다.
	healthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// GetCharacterMovement()는 ACharacter가 기본으로 가지고 있는 이동 담당 컴포넌트를 가져온다.
	// NavMesh 경로탐색(정적 지형/장애물 우회)은 Move To 태스크가 이미 자동으로 처리해주지만,
	// "다른 액터(다른 적, 플레이어)와 서로 부딪히지 않고 피해서 이동하는 것"은 별도 기능이라
	// 이렇게 직접 켜줘야 한다. 이 기능을 RVO(Reciprocal Velocity Obstacles) 회피라고 부른다.
	if (UCharacterMovementComponent* movement = GetCharacterMovement())
	{
		// bUseRVOAvoidance를 true로 켜면, 이동 중에 다른 회피 대상과 겹칠 것 같으면
		// 스스로 경로를 살짝 틀어서 피해간다.
		movement->bUseRVOAvoidance = true;

		// AvoidanceConsiderationRadius는 "이 반경 안에 있는 다른 회피 대상들만 신경 쓴다"는 기준 거리다.
		// 캡슐 크기보다 조금 넉넉하게 잡아야 미리 피할 수 있어서, 여기서는 attackRange 정도로 설정한다.
		movement->AvoidanceConsiderationRadius = attackRange;
	}
}

// BeginPlay()는 이 액터가 실제로 월드에 존재하고 플레이가 시작될 때, 언리얼이 자동으로 호출해주는 함수다.
// (생성자는 "객체가 만들어질 때", BeginPlay는 "그 객체가 실제로 플레이에 참여하기 시작할 때"로 시점이 다르다.)
void AEnemyCharacter::BeginPlay()
{
	// Super는 "부모 클래스"를 가리키는 키워드다. Super::BeginPlay()는 부모 클래스(ACharacter)의
	// BeginPlay 로직을 먼저 실행해달라는 뜻이다. 이걸 빼먹으면 부모가 하던 초기화가 스킵되어
	// 예상치 못한 버그가 생길 수 있으므로, override한 함수의 맨 처음에 항상 호출해주는 것이 규칙이다.
	Super::BeginPlay();

	// if (healthComponent)는 "healthComponent 포인터가 nullptr이 아닌지" 확인하는 코드다.
	// 포인터가 유효한 객체를 가리키고 있을 때만 그 객체의 함수를 호출해야 안전하다(안 그러면 크래시 위험).
	if (healthComponent)
	{
		// onDeath는 healthComponent 안에 있는 "델리게이트(이벤트)"다.
		// AddDynamic(this, &AEnemyCharacter::HandleDeath)는 "이 델리게이트가 방송(Broadcast)될 때,
		// this(지금 이 적 캐릭터)의 HandleDeath 함수를 호출해달라"고 등록(구독)하는 코드다.
		// &AEnemyCharacter::HandleDeath는 HandleDeath 함수 자체를 가리키는 "함수 포인터" 문법이다.
		healthComponent->onDeath.AddDynamic(this, &AEnemyCharacter::HandleDeath);
	}
}

// 사망 처리 함수의 실제 구현. healthComponent의 체력이 0이 되어 onDeath가 방송되면 이 함수가 호출된다.
// deadActor 파라미터는 델리게이트가 넘겨주는 값인데, 지금 구현에서는 굳이 쓰지 않았다
// (이미 this로 자기 자신에 접근할 수 있어서). 하지만 델리게이트 시그니처를 맞추기 위해 파라미터 자체는 꼭 있어야 한다.
void AEnemyCharacter::HandleDeath(AActor* deadActor)
{
	// GetController()는 지금 이 폰(적 캐릭터)을 조종하고 있는 컨트롤러(우리 경우 AEnemyAIController)를 가져온다.
	// if (AController* aiController = ...)는 "변수를 선언하면서 동시에 조건도 검사하는" C++ 문법이다.
	// aiController가 nullptr이 아니면 중괄호 블록 안으로 들어간다.
	if (AController* aiController = GetController())
	{
		// UnPossess()는 컨트롤러와 폰 사이의 연결을 끊는다.
		// 연결이 끊기면 이 폰을 대상으로 하던 Behavior Tree 실행도 사실상 의미가 없어져서, AI가 멈춘 것과 같은 효과가 난다.
		aiController->UnPossess();
	}

	// SetActorEnableCollision(false)는 이 액터의 모든 충돌 처리를 꺼버린다.
	// 죽은 시체에 총알이 계속 맞거나, 플레이어가 시체에 걸려서 이동이 막히는 문제를 방지한다.
	SetActorEnableCollision(false);
}
