// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerCharacter.generated.h"

class UHealthComponent;
class UWeaponComponent;
class ABattleSystem;
class UUserWidget;

UCLASS()
class TEAM10CH3PROJECT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* defaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* moveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* lookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* jumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* runAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* attackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* reloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* toggleCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* crouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* specialSkillAction;

	// 체력/사망은 HealthComponent가 단일 진실 소스다. PlayerCharacter는 자체적으로
	// currentHealth/isDead 같은 값을 따로 들고 있지 않고, 항상 이 컴포넌트를 참조한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UHealthComponent* healthComponent = nullptr;

	// 탄약/재장전 상태 전담 컴포넌트. 실제 공격 판정은 battleSystem에 위임한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UWeaponComponent* weaponComponent = nullptr;

	// 플레이 시작 시 화면에 띄울 HUD 위젯 클래스. 에디터에서 WBP_PlayerHUD 등을 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> playerHudClass;

	// 실제로 생성돼 화면에 추가된 HUD 위젯 인스턴스.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UUserWidget* playerHudWidget = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float walkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float runSpeed = 1300.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isRunning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float crouchSpeed = 150.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isCrouching = false;

	// 실제 공격 판정(라인 트레이스, 데미지, 헤드샷, 범위 공격)을 담당하는 액터.
	// AActor 파생 클래스라 컴포넌트로 붙일 수 없어서, BeginPlay에서 스폰해서 참조만 들고 있는다.
	UPROPERTY(EditDefaultsOnly, Category = "Battle")
	TSubclassOf<ABattleSystem> battleSystemClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	ABattleSystem* battleSystem = nullptr;

	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);

	void StartRun();
	void StopRun();
	bool CanRun() const;

	void StartCrouch();
	void StopCrouch();
	bool CanCrouch() const;

	void UseSpecialSkill();

	UFUNCTION()
	void HandleDeath(AActor* deadActor);

	// Camera
	void ToggleCamera();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* firstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* springArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* thirdPersonCamera;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool isFirstPerson;

	// 앉기/일어서기 시 springArm이 바로 순간이동하지 않고 Tick에서 서서히 목표 높이로 보간되도록 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float standingCameraHeight = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float crouchedCameraHeight = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float cameraInterpSpeed = 10.0f;

	// standingCameraHeight/crouchedCameraHeight 목표값 쪽으로 순수하게 보간 중인 높이.
	// cameraOffsetCompensation과 분리해서 관리해야 이중으로 더해지는 걸 방지할 수 있다.
	float springArmBaseHeight = 0.0f;

	// Crouch()/UnCrouch() 호출 시 캡슐(springArm의 부모)이 그 프레임에 즉시 이동한 만큼을 담아뒀다가
	// Tick에서 서서히 0으로 줄여서, 캡슐의 순간이동으로 인한 카메라 순간이동을 상쇄한다.
	float cameraOffsetCompensation = 0.0f;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
