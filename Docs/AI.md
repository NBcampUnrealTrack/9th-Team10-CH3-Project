# Enemy AI 설계 문서

## 목적과 범위

이 문서는 `AEnemyCharacter`, `AEnemyAIController`, Blackboard, Behavior Tree 커스텀 노드가 만드는 적 AI의 동작을 설명한다. 코드에는 유지보수 판단에 필요한 짧은 주석만 남기고, Unreal API 사용법과 설계 배경은 이 문서에서 관리한다.

## 구성

| 구성 요소 | 책임 |
| --- | --- |
| `AEnemyCharacter` | 체력, 이동/회전 속도, 피격 반응, 사망 처리 |
| `AEnemyAIController` | Behavior Tree 실행과 Focus 제어 |
| `BTService_DetectPlayer` | 시야 탐지, LOS, 마지막 위치 갱신 |
| `BTTask_AimAtPlayer` | 전투 준비 상태와 대상 Focus |
| `BTTask_AttackPlayer` | 눈높이 Hitscan 공격과 데미지 적용 |
| `BTTask_FindPatrolPoint` | NavMesh 위 순찰 목적지 선정 |
| `BTTask_LookAround` | 마지막 위치 도착 후 주변 탐색 회전 |
| `BTTask_ClearBlackboardValue` | 수색 종료 후 Blackboard 값 정리 |

## Blackboard 규약

| 키 | 형식 | 의미 |
| --- | --- | --- |
| `targetActor` | Object / Actor | 현재 직접 추적·조준할 대상 |
| `lastKnownLocation` | Vector | 마지막으로 확인한 대상 위치 |
| `patrolLocation` | Vector | 다음 순찰 목적지 |
| `isCombatReady` | Bool | 발견 후 조준 대기가 완료됐는지 여부 |

키 이름과 타입은 BT, Blackboard 에셋, C++ 태스크 사이의 계약이다. 이름이나 타입을 바꾸면 세 곳을 함께 수정해야 한다.

## 탐지와 시야 판정

탐지 서비스는 0.2초마다 실행된다.

1. 적과 플레이어의 거리가 `sightRadius` 안인지 확인한다.
2. FOV는 수평(XY) 벡터로만 계산한다. 계단·발판 때문에 높이가 달라도 수평으로 정면이면 시야각 밖으로 잘못 판정하지 않기 위함이다.
3. 적과 플레이어의 `GetActorEyesViewPoint` 사이를 `ECC_Visibility`로 Trace한다.
4. Trace가 벽을 맞으면 탐지하지 않고, 플레이어를 직접 맞거나 아무것도 맞지 않으면 탐지한다.

LOS Trace에는 적 자신을 Ignore한다. 적 Capsule은 `ECC_Visibility`를 Block해야 하며, Blueprint 충돌 설정이 C++ 생성자 값을 덮어쓸 수 있으므로 `BeginPlay`에서도 다시 적용한다.

## 발견, 추적, 수색

플레이어가 보이면 `targetActor`와 `lastKnownLocation`을 갱신하고 AI Controller가 플레이어에 Focus한다. 시야를 잃으면 직접 대상은 비우지만 마지막 위치는 유지한다.

Behavior Tree는 마지막 위치로 이동한 뒤 `Look Around`을 실행하고, 완료되면 마지막 위치를 지운다. 피격된 적은 `HandleDamaged`에서 공격자를 임시 경계 대상으로 설정해, 시야 밖에서 먼저 공격받아도 추적을 시작한다.

플레이어의 `HealthComponent::isDead`가 true면 탐지 서비스는 `targetActor`, `lastKnownLocation`, `isCombatReady`, Focus를 모두 비우고 순찰 이동 모드로 돌아간다. 게임 종료 뒤 적이 시체를 추적하거나 사격하는 것을 막기 위한 처리다.

## 이동과 회전

| 상태 | 속도 | 회전 |
| --- | --- | --- |
| 순찰 | `patrolMoveSpeed` | `patrolRotationSpeed` |
| 발견·추적·수색·사격 | `alertMoveSpeed` | `alertRotationSpeed` |

`bRequestedMoveUseAcceleration`을 켜서 AI Move To가 실제 가속도를 사용하게 한다. Animation Blueprint가 속도와 가속도를 정상적으로 읽어 Idle 자세로 미끄러지는 현상을 줄이기 위함이다. RVO 회피 반경은 공격 거리와 분리된 `avoidanceRadius`를 사용한다.

상체 조준 오프셋은 `GetUpperBodyAimOffset()`이 Controller 회전과 Actor 회전의 차이를 계산해 제공한다. 이 값은 Animation Blueprint에서 spine 본에 적용해야 시각적인 상·하체 분리가 완성된다.

## 조준과 사격

전투 Branch는 다음 순서다.

1. `targetActor`가 설정되면 Aim 태스크가 Controller Focus를 유지한다.
2. 공격 태스크는 대상의 `HealthComponent`와 생존 상태를 확인한다. 무효하거나 사망한 대상이면 전투 Blackboard 값과 Focus를 지우고 순찰 모드로 복귀한다.
3. 조준 대기 후 `isCombatReady`가 true가 된다.
4. 공격 태스크는 적 눈높이에서 대상 눈높이를 향해 Hitscan Trace를 실행한다.
5. 명중 액터, Owner, Attach Parent 순으로 `UHealthComponent`를 찾는다.
6. 체력 컴포넌트를 찾았을 때만 `TakeDamage`를 호출한다.

Trace 채널은 플레이어 사격과 같은 `ECC_Visibility`다. 따라서 적과 플레이어 Capsule은 이 채널을 Block해야 하며, 벽에 먼저 맞았을 때는 데미지가 전달되지 않는다. 현재 디버그 Trace는 `EDrawDebugTrace::ForDuration`으로 표시된다.

공격 간격은 Attack 태스크의 C++ 메모리가 아니라 Behavior Tree의 Wait 노드가 담당한다. 여러 적이 같은 BT 노드를 공유해도 쿨다운 상태가 섞이지 않게 하는 구조다.

## 피격과 사망

`UHealthComponent`는 체력을 먼저 갱신한 뒤 `onDamaged`를 Broadcast한다. 체력이 0이 되면 `onDeath`를 한 번 Broadcast한다.

적 사망 시에는 AI Controller를 UnPossess하고 Collision을 끈 뒤 GameMode에 킬 점수를 알린다. Hit/HUD 델리게이트 체인이 끝날 시간을 위해 즉시 숨긴 뒤 짧은 수명으로 제거한다.

## 검증 체크리스트

- 벽 뒤 플레이어를 탐지하거나 공격하지 않는다.
- 계단 위아래의 플레이어를 수평 FOV와 눈높이 LOS로 정상 탐지한다.
- 시야 상실 후 마지막 위치로 이동하고 주변을 둘러본 뒤 순찰로 복귀한다.
- 피격된 적은 공격자를 향해 경계 상태로 전환한다.
- 여러 적이 동시에 있어도 각자의 공격 간격이 독립적으로 동작한다.
- 플레이어와 적 Capsule이 `ECC_Visibility`를 Block한다.
- 적 사망 시 길막 없이 제거되고 킬 점수가 한 번만 올라간다.
