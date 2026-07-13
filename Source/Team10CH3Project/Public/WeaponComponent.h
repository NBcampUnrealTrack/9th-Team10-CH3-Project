#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class ABattleSystem;
class UHealthComponent;

// 탄약, 재장전, 기본 공격 실행만 담당하는 컴포넌트.
// 실제 판정(라인 트레이스, 데미지 적용)은 BattleSystem이 하고, 이 컴포넌트는 "쏠 수 있는 상태인지,
// 탄창에 몇 발 남았는지"만 관리한다. 플레이어/적 어느 쪽에 붙여도 동작하도록 특정 캐릭터 클래스를
// 직접 참조하지 않는다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TEAM10CH3PROJECT_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	// battleSystem: 실제 공격 판정을 대신 실행해줄 대상.
	// healthComponent: 소유자가 죽었는지 확인하기 위한 참조.
	// 둘 다 소유자(PlayerCharacter 등)가 준비된 뒤 넘겨줘야 하므로, 생성자가 아니라 별도 초기화 함수로 받는다.
	void Init(ABattleSystem* inBattleSystem, UHealthComponent* inHealthComponent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int maxAmmo = 30;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int currentAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int reserveAmmo = 90;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float reloadTime = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool isReloading = false;

	UFUNCTION(BlueprintCallable)
	void Attack();

	UFUNCTION(BlueprintCallable)
	void Reload();

	UFUNCTION(BlueprintCallable)
	bool CanAttack() const;

	UFUNCTION(BlueprintCallable)
	bool CanReload() const;

protected:
	virtual void BeginPlay() override;

private:
	void FinishReload();

	UPROPERTY()
	ABattleSystem* battleSystem = nullptr;

	UPROPERTY()
	UHealthComponent* healthComponent = nullptr;

	FTimerHandle reloadTimerHandle;
};
