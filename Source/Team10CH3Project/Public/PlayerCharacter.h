// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PlayerCharacter.generated.h"

class UHealthComponent;
class UWeaponComponent;
class ABattleSystem;
class APC_PlayerController;
class USceneComponent;
class UAnimSequenceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpecialSkillThrown);

UCLASS()
class TEAM10CH3PROJECT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	APlayerCharacter();

	// 체력/사망은 HealthComponent가 단일 진실 소스다. PlayerCharacter는 자체적으로
	// currentHealth/isDead 같은 값을 따로 들고 있지 않고, 항상 이 컴포넌트를 참조한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UHealthComponent* healthComponent = nullptr;

	// 탄약/재장전 상태 전담 컴포넌트. 실제 공격 판정은 battleSystem에 위임한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UWeaponComponent* weaponComponent = nullptr;
	

	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);

	void StartRun();
	void StopRun();
	bool CanRun() const;

	void StartCrouch();
	void StopCrouch();
	bool CanCrouch() const;

	void StartSpecialSkill();
	void ReleaseSpecialSkill();

	// Camera
	void ToggleCamera();

	void StartAim();
	void StopAim();

	void EnterShoulderAim();
	void EnterAdsAim();
	void ExitAim();

	bool CanAim() const;

	void Attack();
	void Reload();

	bool GetAttackView(
		FVector& outViewLocation,
		FVector& outViewDirection
	) const;

	UFUNCTION(BlueprintPure, Category = "Animation|IK")
	bool GetLeftHandIKTransform(FTransform& outTransform) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float walkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float runSpeed = 650.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isRunning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float crouchSpeed = 150.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isCrouching = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* weaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "First Person")
	USkeletalMeshComponent* firstPersonArmsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "First Person")
	USkeletalMeshComponent* firstPersonWeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "First Person")
	FVector firstPersonArmsLocation = FVector(-10.0f, 0.0f, -150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "First Person")
	FRotator firstPersonArmsRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "First Person", meta = (ClampMin = "0.1"))
	float firstPersonArmsScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "First Person")
	FName firstPersonWeaponSocketName = TEXT("WeaponSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "First Person")
	FVector firstPersonWeaponFallbackLocation = FVector(45.0f, 18.0f, -18.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "First Person")
	FRotator firstPersonWeaponFallbackRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|IK")
	FName leftHandIKSocketName = TEXT("LeftHandIK");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|IK")
	FName rightHandBoneName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Weapon")
	UAnimSequenceBase* equipAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Weapon")
	UAnimSequenceBase* fireAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Weapon")
	UAnimSequenceBase* reloadAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonEquipAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonFireAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonAimedFireAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonReloadAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonAimedReloadAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonEmptyReloadAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon")
	UAnimSequenceBase* firstPersonAimedEmptyReloadAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Skill")
	UAnimSequenceBase* grenadeReadyAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Skill")
	UAnimSequenceBase* grenadeThrowAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FName upperBodySlotName = TEXT("UpperBody");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0"))
	float animationBlendInTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0"))
	float animationBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon", meta = (ClampMin = "0.0"))
	float firstPersonFireBlendInTime = 0.025f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|First Person Weapon", meta = (ClampMin = "0.0"))
	float firstPersonFireBlendOutTime = 0.06f;

	// 실제 공격 판정(라인 트레이스, 데미지, 헤드샷, 범위 공격)을 담당하는 액터.
	// AActor 파생 클래스라 컴포넌트로 붙일 수 없어서, BeginPlay에서 스폰해서 참조만 들고 있는다.
	UPROPERTY(EditDefaultsOnly, Category = "Battle")
	TSubclassOf<ABattleSystem> battleSystemClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	ABattleSystem* battleSystem = nullptr;

	UFUNCTION()
	void HandleDamaged(
		float damageAmount,
		AActor* attackerActor,
		FVector hitLocation,
		FVector attackDirection);

	UFUNCTION()
	void HandleDeath(AActor* deadActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent* firstPersonCameraRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* firstPersonCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson")
	bool followHeadPosition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson")
	FName firstPersonViewBoneName = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson")
	FVector firstPersonEyeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float firstPersonMinViewPitch = -60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float firstPersonMaxViewPitch = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson", meta = (ClampMin = "0.0"))
	float headCameraInterpSpeed = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* springArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* thirdPersonCamera;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool isFirstPerson = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* adsCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|ADS")
	FName adsSightSocketName = TEXT("SightSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|ADS")
	FName adsFrontSightSocketName = TEXT("FrontSightSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	FVector adsSightViewOffset = FVector(10.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	FRotator adsSightViewRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	bool alignAdsSightRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS", meta = (ClampMin = "0.0"))
	float adsWeaponInterpSpeed = 12.0f;

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


	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	bool isAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	bool isAdsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Aim")
	float aimPitch = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Aim")
	float aimYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Aim", meta = (ClampMin = "0.0"))
	float aimRotationInterpSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Aim")
	float minAimPitch = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Aim")
	float maxAimPitch = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Aim")
	float maxAimYaw = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float doubleClickTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float aimInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float normalFieldOfView = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float shoulderFieldOfView = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float adsFieldOfView = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float normalArmLength = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float shoulderArmLength = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	FVector normalSocketOffset = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	FVector shoulderSocketOffset = FVector(0.0f, 30.0f, 15.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float aimingMoveSpeed = 200.0f;

	float lastAimPressTime = -5.0f;

	// 수류탄 준비에 필요한 최소 시전시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Grenade")
	float grenadeCastTime = 0.8f;

	// 현재 수류탄 시전 중인지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Grenade")
	bool isCastingGrenade = false;

	// 최소 시전시간이 완료되어 던질 준비가 됐는지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Grenade")
	bool isGrenadeReady = false;

	// 현재 G키를 누르고 있는지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Grenade")
	bool isGrenadeKeyHeld = false;

	// 시전 도중 G키를 먼저 뗐는지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Grenade")
	bool shouldThrowAfterCast = false;

	UPROPERTY()
	APC_PlayerController* PC;
	
	FTimerHandle grenadeCastTimerHandle;

	void FinishGrenadeCast();
	void ThrowGrenade();
	void ResetGrenadeState();
	bool PlayUpperBodyAnimation(UAnimSequenceBase* animation, float playRate = 1.0f);
	bool PlayFirstPersonUpperBodyAnimation(
		UAnimSequenceBase* animation,
		float playRate = 1.0f,
		float blendInTime = -1.0f,
		float blendOutTime = -1.0f
	);
	void UpdateCameraPresentation();
	

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//Joonhyeong
	UFUNCTION(BlueprintCallable, Category = "FOV")
	void SetNormalFOV(float NewFOV);
	UFUNCTION(BlueprintCallable, Category = "FOV")
	float GetNormalFOV();
	UPROPERTY(BlueprintAssignable, Category = "Skill")
	FOnSpecialSkillThrown onSpecialSkillThrown;
};
