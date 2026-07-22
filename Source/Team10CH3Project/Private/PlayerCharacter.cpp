	// Fill out your copyright notice in the Description page of Project Settings.

	#include "PlayerCharacter.h"

	#include "GameFramework/CharacterMovementComponent.h"
	#include "GameFramework/PlayerController.h"
	#include "Camera/PlayerCameraManager.h"
	#include "Components/CapsuleComponent.h"
	#include "Components/SceneComponent.h"
	#include "Camera/CameraComponent.h"
	#include "GameFramework/SpringArmComponent.h"
	#include "Animation/AnimInstance.h"
	#include "Animation/AnimSequenceBase.h"
	#include "Engine/SkeletalMesh.h"

	#include "HealthComponent.h"
	#include "WeaponComponent.h"
	#include "BattleSystem.h"
	#include "Engine/World.h"

	// Sets default values
	APlayerCharacter::APlayerCharacter()
	{
		// 앉기/일어서기 시 카메라 높이를 매 프레임 보간해야 해서 Tick을 켜둔다.
		PrimaryActorTick.bCanEverTick = true;

		// True FPS에서는 몸 전체가 카메라의 좌우 회전을 따라가고 캡슐은 항상 수직을 유지한다.
		bUseControllerRotationYaw = true;
		bUseControllerRotationPitch = false;
		bUseControllerRotationRoll = false;
		GetCharacterMovement()->bOrientRotationToMovement = false;

		weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
		weaponMesh->SetupAttachment(GetMesh(),TEXT("HandGrip_R"));
		weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		weaponMesh->SetGenerateOverlapEvents(false);

		// 총의 조준경 시점용 카메라
		adsCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("AdsCamera"));
		adsCamera->SetupAttachment(weaponMesh, TEXT("SightSocket"));
		adsCamera->SetRelativeLocation(FVector::ZeroVector);
		adsCamera->SetRelativeRotation(FRotator::ZeroRotator);
		adsCamera->bUsePawnControlRotation = true;
		adsCamera->SetFieldOfView(adsFieldOfView);
		adsCamera->SetActive(false);

		// FPS Camera. 숨겨진 head 본의 자식으로 두면 본 스케일과 갱신 순서의
		// 영향을 받을 수 있으므로 루트는 캡슐에 두고 Tick에서 최종 머리 위치를 적용한다.
		firstPersonCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonCameraRoot"));
		firstPersonCameraRoot->SetupAttachment(GetCapsuleComponent());
		firstPersonCameraRoot->SetRelativeLocation(firstPersonEyeOffset);
		firstPersonCameraRoot->SetUsingAbsoluteRotation(true);

		firstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
		firstPersonCamera->SetupAttachment(firstPersonCameraRoot);
		firstPersonCamera->SetRelativeLocation(FVector::ZeroVector);
		firstPersonCamera->SetRelativeRotation(FRotator::ZeroRotator);
		// FPS 시점의 상하/좌우 입력은 항상 Control Rotation을 사용한다.
		// 머리 본은 위치 추적에만 사용해야 Aim Offset과 카메라 Pitch가 서로 막지 않는다.
		firstPersonCamera->bUsePawnControlRotation = true;

		// FPS 전용 팔은 카메라 공간에서만 보이며 전신(TPS) 메시와 독립적으로 움직인다.
		firstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
		firstPersonArmsMesh->SetupAttachment(firstPersonCamera);
		firstPersonArmsMesh->SetRelativeLocation(firstPersonArmsLocation);
		firstPersonArmsMesh->SetRelativeRotation(firstPersonArmsRotation);
		firstPersonArmsMesh->SetRelativeScale3D(FVector(firstPersonArmsScale));
		// FPS/TPS 전환 함수에서 직접 표시 여부를 관리하므로 Owner 판정에 의존하지 않는다.
		firstPersonArmsMesh->SetOnlyOwnerSee(false);
		firstPersonArmsMesh->SetOwnerNoSee(false);
		firstPersonArmsMesh->SetCastShadow(false);
		firstPersonArmsMesh->SetBoundsScale(10.0f);
		firstPersonArmsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		firstPersonArmsMesh->SetGenerateOverlapEvents(false);
		firstPersonArmsMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		firstPersonArmsMesh->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

		firstPersonWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonWeapon"));
		firstPersonWeaponMesh->SetupAttachment(firstPersonArmsMesh, firstPersonWeaponSocketName);
		firstPersonWeaponMesh->SetOnlyOwnerSee(false);
		firstPersonWeaponMesh->SetOwnerNoSee(false);
		firstPersonWeaponMesh->SetCastShadow(false);
		firstPersonWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		firstPersonWeaponMesh->SetGenerateOverlapEvents(false);

		// Spring Arm
		springArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
		springArm->SetupAttachment(GetCapsuleComponent());
		springArm->TargetArmLength = 300.f;
		springArm->SetRelativeLocation(FVector(0.f, 20.f, standingCameraHeight));
		springArmBaseHeight = standingCameraHeight;
		springArm->bUsePawnControlRotation = true;

		// TPS Camera
		thirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
		thirdPersonCamera->SetupAttachment(springArm);
		thirdPersonCamera->bUsePawnControlRotation = false;


		thirdPersonCamera->SetFieldOfView(normalFieldOfView);

		normalArmLength = springArm->TargetArmLength;
		normalSocketOffset = springArm->SocketOffset;


		isFirstPerson = true;

		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
		GetCharacterMovement()->SetCrouchedHalfHeight(44.0f);

		healthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
		weaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

		battleSystemClass = ABattleSystem::StaticClass();
	}

	// Called when the game starts or when spawned
	void APlayerCharacter::BeginPlay()
	{
		Super::BeginPlay();

		// 일반 FPS 카메라: 전신 머리 애니메이션을 따라가지 않고 캡슐의 고정 눈높이를
		// 사용한다. 팔/총만 카메라 자식으로 움직여 화면 흔들림과 몸 관통을 방지한다.
		firstPersonCameraRoot->AttachToComponent(
			GetCapsuleComponent(),
			FAttachmentTransformRules::KeepRelativeTransform
		);
		followHeadPosition = false;
		firstPersonCameraRoot->SetRelativeLocation(
			FVector(firstPersonEyeOffset.X, firstPersonEyeOffset.Y, standingCameraHeight)
		);
		firstPersonCameraRoot->SetUsingAbsoluteRotation(true);
		firstPersonCamera->bUsePawnControlRotation = true;

		if (APlayerController* playerController = Cast<APlayerController>(Controller))
		{
			if (playerController->PlayerCameraManager)
			{
				playerController->PlayerCameraManager->ViewPitchMin = firstPersonMinViewPitch;
				playerController->PlayerCameraManager->ViewPitchMax = firstPersonMaxViewPitch;
			}
		}

		// 캐릭터 Tick을 스켈레탈 메시 애니메이션 평가 이후에 실행해 최종 head 위치를 읽는다.
		PrimaryActorTick.TickGroup = TG_PostUpdateWork;
		AddTickPrerequisiteComponent(GetMesh());


		// 실제 게임은 FPS로 시작하고 TPS 카메라는 테스트 전환용으로만 둔다.
		firstPersonCamera->SetActive(true);
		thirdPersonCamera->SetActive(false);
		adsCamera->SetActive(false);

		isFirstPerson = true;
		isAiming = false;
		isAdsAiming = false;

		springArm->TargetArmLength = normalArmLength;
		springArm->SocketOffset = normalSocketOffset;
		thirdPersonCamera->SetFieldOfView(normalFieldOfView);
		firstPersonCamera->SetFieldOfView(normalFieldOfView);

		// TPS와 FPS에 남아 있는 기존 기본 소총을 AKS74U 패키지의 총기로 통일한다.
		// BP 컴포넌트에 예전 메시/머티리얼이 저장되어 있어도 실행 시 확실히 덮어쓴다.
		USkeletalMesh* sharedAks74uMesh = LoadObject<USkeletalMesh>(
			nullptr,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Meshes/SK_AK74U.SK_AK74U")
		);
		if (sharedAks74uMesh)
		{
			if (weaponMesh)
			{
				weaponMesh->EmptyOverrideMaterials();
				weaponMesh->SetSkeletalMeshAsset(sharedAks74uMesh);
			}
			if (firstPersonWeaponMesh)
			{
				firstPersonWeaponMesh->EmptyOverrideMaterials();
				firstPersonWeaponMesh->SetSkeletalMeshAsset(sharedAks74uMesh);
			}
		}
		else
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Failed to load shared AKS74U weapon mesh")
			);
		}

		// 이동된 AKS74U 패키지의 FPS 전용 액션을 기본값으로 연결한다.
		// BP에서 개별 자산을 지정하면 그 값을 우선 사용한다.
		auto loadFirstPersonAnimation = [](UAnimSequenceBase*& target, const TCHAR* path)
		{
			if (!target)
			{
				target = LoadObject<UAnimSequenceBase>(nullptr, path);
			}
		};

		loadFirstPersonAnimation(firstPersonEquipAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Equipe.A_FP_AKS74U_Equipe"));
		loadFirstPersonAnimation(firstPersonFireAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Fire.A_FP_AKS74U_Fire"));
		loadFirstPersonAnimation(firstPersonAimedFireAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Fire_Aimed.A_FP_AKS74U_Fire_Aimed"));
		loadFirstPersonAnimation(firstPersonReloadAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Reload.A_FP_AKS74U_Reload"));
		loadFirstPersonAnimation(firstPersonAimedReloadAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Reload_Aimed.A_FP_AKS74U_Reload_Aimed"));
		loadFirstPersonAnimation(firstPersonEmptyReloadAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Reload_Empty.A_FP_AKS74U_Reload_Empty"));
		loadFirstPersonAnimation(firstPersonAimedEmptyReloadAnimation,
			TEXT("/Game/Character/Animation/Arms/AKS74U/Animations/A_FP_AKS74U_Reload_Empty_Aimed.A_FP_AKS74U_Reload_Empty_Aimed"));

		// FPS/TPS가 같은 무기 자산을 사용하되 렌더링 컴포넌트는 서로 분리한다.
		if (weaponMesh && firstPersonWeaponMesh)
		{
			firstPersonWeaponMesh->SetSkeletalMeshAsset(weaponMesh->GetSkeletalMeshAsset());
		}

		// BP에서 Anim Class를 빠뜨렸더라도 전신이 사용하는 AnimBP를 자동으로 공유한다.
		// 각 컴포넌트는 별도의 AnimInstance를 가지므로 FPS 팔 몽타주도 독립 재생할 수 있다.
		if (firstPersonArmsMesh && firstPersonArmsMesh->GetSkeletalMeshAsset())
		{
			// BP에 남아 있는 초기 컴포넌트 Transform과 무관하게 FPS 기준값을 적용한다.
			firstPersonArmsMesh->SetRelativeLocation(firstPersonArmsLocation);
			firstPersonArmsMesh->SetRelativeRotation(firstPersonArmsRotation);
			firstPersonArmsMesh->SetRelativeScale3D(FVector(firstPersonArmsScale));
			firstPersonArmsMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			if (!firstPersonArmsMesh->GetAnimClass() && GetMesh()->GetAnimClass())
			{
				firstPersonArmsMesh->SetAnimInstanceClass(GetMesh()->GetAnimClass());
			}
		}

		if (firstPersonArmsMesh && firstPersonArmsMesh->GetSkeletalMeshAsset()
			&& firstPersonArmsMesh->DoesSocketExist(firstPersonWeaponSocketName))
		{
			firstPersonWeaponMesh->AttachToComponent(
				firstPersonArmsMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				firstPersonWeaponSocketName
			);
		}
		else
		{
			// 전용 팔 자산이 지정되기 전에도 FPS 총은 사용할 수 있다.
			firstPersonWeaponMesh->AttachToComponent(
				firstPersonCamera,
				FAttachmentTransformRules::KeepRelativeTransform
			);
			firstPersonWeaponMesh->SetRelativeLocation(firstPersonWeaponFallbackLocation);
			firstPersonWeaponMesh->SetRelativeRotation(firstPersonWeaponFallbackRotation);
		}

		UpdateCameraPresentation();

		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

		// BattleSystem은 AActor라 컴포넌트로 붙일 수 없어서, 이 캐릭터 전용으로 하나 스폰해서 들고 있는다.
		if (battleSystemClass)
		{
			FActorSpawnParameters spawnParams;
			spawnParams.Owner = this;
			battleSystem = GetWorld()->SpawnActor<ABattleSystem>(battleSystemClass, spawnParams);
		}

		if (weaponComponent)
		{
			weaponComponent->Init(battleSystem, healthComponent);
		}

		if (healthComponent)
		{
			healthComponent->onDeath.AddDynamic(this, &APlayerCharacter::HandleDeath);
		}

		PlayUpperBodyAnimation(equipAnimation);
		PlayFirstPersonUpperBodyAnimation(firstPersonEquipAnimation);
	}

	// Called every frame
	void APlayerCharacter::Tick(float DeltaTime)
	{
		Super::Tick(DeltaTime);

		if (Controller)
		{
			const FRotator aimDelta =
				(Controller->GetControlRotation() - GetActorRotation()).GetNormalized();

			const float targetAimPitch = FMath::Clamp(aimDelta.Pitch, minAimPitch, maxAimPitch);
			const float targetAimYaw = FMath::Clamp(aimDelta.Yaw, -maxAimYaw, maxAimYaw);

			aimPitch = FMath::FInterpTo(aimPitch, targetAimPitch, DeltaTime, aimRotationInterpSpeed);
			aimYaw = FMath::FInterpTo(aimYaw, targetAimYaw, DeltaTime, aimRotationInterpSpeed);
		}

		const float targetHeight = isCrouching ? crouchedCameraHeight : standingCameraHeight;
		springArmBaseHeight = FMath::FInterpTo(springArmBaseHeight, targetHeight, DeltaTime, cameraInterpSpeed);

		cameraOffsetCompensation = FMath::FInterpTo(cameraOffsetCompensation, 0.0f, DeltaTime, cameraInterpSpeed);

		FVector springArmLocation = springArm->GetRelativeLocation();
		springArmLocation.Z = springArmBaseHeight + cameraOffsetCompensation;
		springArm->SetRelativeLocation(springArmLocation);

		if (followHeadPosition && GetMesh()->DoesSocketExist(firstPersonViewBoneName))
		{
			const FTransform headWorldTransform =
				GetMesh()->GetSocketTransform(firstPersonViewBoneName, RTS_World);

			// 카메라 오프셋은 head 회전이 아니라 캐릭터의 수직인 로컬 축으로 적용한다.
			// 그래야 위/아래 조준 시 카메라가 몸 안쪽으로 원을 그리며 들어가지 않는다.
			const FVector cameraWorldLocation =
				headWorldTransform.GetLocation()
				+ GetActorQuat().RotateVector(firstPersonEyeOffset);

			firstPersonCameraRoot->SetWorldLocation(cameraWorldLocation);

		}
		else
		{
			// 머리 본을 찾지 못했거나 추적을 끈 경우 캡슐 기준 높이를 사용한다.
			FVector firstPersonRootLocation = firstPersonCameraRoot->GetRelativeLocation();
			firstPersonRootLocation.X = firstPersonEyeOffset.X;
			firstPersonRootLocation.Y = firstPersonEyeOffset.Y;
			firstPersonRootLocation.Z = springArmBaseHeight + cameraOffsetCompensation;
			firstPersonCameraRoot->SetRelativeLocation(firstPersonRootLocation);
		}

		// 일반 FPS ADS는 카메라를 총에 붙이지 않는다. 고정된 FPS 카메라를 기준으로
		// 팔/총 전체를 이동 및 회전시켜 SightSocket의 위치와 전방축을 화면 중앙에 맞춘다.
		if (isFirstPerson && firstPersonArmsMesh && firstPersonWeaponMesh)
		{
			FVector targetArmsLocation = firstPersonArmsLocation;
			FRotator targetArmsRotation = firstPersonArmsRotation;
			if (isAdsAiming && firstPersonWeaponMesh->DoesSocketExist(adsSightSocketName))
			{
				if (alignAdsSightRotation)
				{
					const FTransform sightRelativeToArms =
						firstPersonWeaponMesh->GetSocketTransform(adsSightSocketName, RTS_World)
							.GetRelativeTransform(firstPersonArmsMesh->GetComponentTransform());

					const FTransform desiredSightRelativeToCamera(
						adsSightViewRotation,
						adsSightViewOffset
					);

					const FTransform targetArmsRelativeToCamera =
						sightRelativeToArms.Inverse() * desiredSightRelativeToCamera;

					targetArmsLocation = targetArmsRelativeToCamera.GetLocation();
					targetArmsRotation = targetArmsRelativeToCamera.Rotator();
				}
				else
				{
					// 소켓 축 대신 후방-전방 가늠쇠 두 점으로 총열 방향을 계산한다.
					// FrontSightSocket이 있으면 두 점을 카메라 전방축에 정확히 맞춘다.
					if (firstPersonWeaponMesh->DoesSocketExist(adsFrontSightSocketName))
					{
						const FTransform armsWorldTransform =
							firstPersonArmsMesh->GetComponentTransform();
						const FVector rearSightInArmsSpace =
							armsWorldTransform.InverseTransformPosition(
								firstPersonWeaponMesh->GetSocketLocation(adsSightSocketName)
							);
						const FVector frontSightInArmsSpace =
							armsWorldTransform.InverseTransformPosition(
								firstPersonWeaponMesh->GetSocketLocation(adsFrontSightSocketName)
							);

						const FQuat currentArmsRotation =
							firstPersonArmsMesh->GetRelativeRotation().Quaternion();
						const FVector currentSightDirection =
							currentArmsRotation.RotateVector(
								(frontSightInArmsSpace - rearSightInArmsSpace).GetSafeNormal()
							).GetSafeNormal();
						const FVector desiredSightDirection =
							adsSightViewRotation.Quaternion().RotateVector(FVector::ForwardVector);

						const FQuat targetArmsRotationQuat =
							FQuat::FindBetweenNormals(currentSightDirection, desiredSightDirection)
							* currentArmsRotation;

						const FTransform rotatedArmsTransform(
							targetArmsRotationQuat,
							FVector::ZeroVector,
							FVector(firstPersonArmsScale)
						);
						targetArmsLocation =
							adsSightViewOffset
							- rotatedArmsTransform.TransformPosition(rearSightInArmsSpace);
						targetArmsRotation = targetArmsRotationQuat.Rotator();
					}
					else
					{
					// FrontSightSocket이 없을 때는 위치와 수동 회전만 사용하는 대체 방식.
					const FVector sightLocationInCameraSpace =
						firstPersonCamera->GetComponentTransform().InverseTransformPosition(
							firstPersonWeaponMesh->GetSocketLocation(adsSightSocketName)
						);

					targetArmsLocation =
						firstPersonArmsMesh->GetRelativeLocation()
						+ (adsSightViewOffset - sightLocationInCameraSpace);
					// 팔 메시에는 기본 -90도 회전이 있으므로 Euler 값을 단순히 더하면
					// 카메라 Yaw가 화면상 Roll처럼 보인다. 카메라(부모) 공간 회전을
					// 기본 팔 회전 앞에 Quaternion으로 합성해 축을 올바르게 유지한다.
					targetArmsRotation = FRotator(
						adsSightViewRotation.Quaternion()
						* firstPersonArmsRotation.Quaternion()
					);
					}
				}
			}

			firstPersonArmsMesh->SetRelativeLocation(
				FMath::VInterpTo(
					firstPersonArmsMesh->GetRelativeLocation(),
					targetArmsLocation,
					DeltaTime,
					adsWeaponInterpSpeed
				)
			);
			firstPersonArmsMesh->SetRelativeRotation(
				FMath::RInterpTo(
					firstPersonArmsMesh->GetRelativeRotation(),
					targetArmsRotation,
					DeltaTime,
					adsWeaponInterpSpeed
				)
			);
		}

		const float targetArmLength =
			isAiming && !isAdsAiming
			? shoulderArmLength
			: normalArmLength;

		const FVector targetSocketOffset =
			isAiming && !isAdsAiming
			? shoulderSocketOffset
			: normalSocketOffset;

		const float targetFieldOfView =
			isAiming && !isAdsAiming
			? shoulderFieldOfView
			: normalFieldOfView;

		springArm->TargetArmLength = FMath::FInterpTo(
			springArm->TargetArmLength,
			targetArmLength,
			DeltaTime,
			aimInterpSpeed
		);

		springArm->SocketOffset = FMath::VInterpTo(
			springArm->SocketOffset,
			targetSocketOffset,
			DeltaTime,
			aimInterpSpeed
		);

		const float currentFieldOfView = thirdPersonCamera->FieldOfView;

		thirdPersonCamera->SetFieldOfView(
			FMath::FInterpTo(
				currentFieldOfView,
				targetFieldOfView,
				DeltaTime,
				aimInterpSpeed
			)
		);

		const float firstPersonTargetFieldOfView = isAiming ? adsFieldOfView : normalFieldOfView;
		firstPersonCamera->SetFieldOfView(
			FMath::FInterpTo(
				firstPersonCamera->FieldOfView,
				firstPersonTargetFieldOfView,
				DeltaTime,
				aimInterpSpeed
			)
		);
	}

	// Called to bind functionality to input

	void APlayerCharacter::Move(const FInputActionValue& value)
	{
		if (healthComponent && healthComponent->isDead)
		{
			return;
		}

		FVector2D moveValue = value.Get<FVector2D>();

		if (Controller != nullptr)
		{
			FRotator controlRotation = Controller->GetControlRotation();
			FRotator yawRotation(0.0f, controlRotation.Yaw, 0.0f);

			FVector forwardDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
			FVector rightDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);

			AddMovementInput(forwardDirection, moveValue.Y);
			AddMovementInput(rightDirection, moveValue.X);
		}
	}

	void APlayerCharacter::Look(const FInputActionValue& value)
	{
		if (healthComponent && healthComponent->isDead)
		{
			return;
		}

		FVector2D lookValue = value.Get<FVector2D>();

		if (Controller != nullptr)
		{
			AddControllerYawInput(lookValue.X);
			AddControllerPitchInput(lookValue.Y);
		}
	}

	bool APlayerCharacter::CanRun() const
	{
		const bool isAlive = !healthComponent || !healthComponent->isDead;
		const bool isReloading = weaponComponent && weaponComponent->isReloading;

		return isAlive
			&& !isReloading
			&& !isCrouching
			&& !isAiming
			&& !isAdsAiming;
	}

	void APlayerCharacter::StartRun()
	{
		if (!CanRun())
		{
			return;
		}

		isRunning = true;
		GetCharacterMovement()->MaxWalkSpeed = runSpeed;
	}

	void APlayerCharacter::StopRun()
	{
		isRunning = false;
		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	}

	bool APlayerCharacter::CanCrouch() const
	{
		const bool isAlive = !healthComponent || !healthComponent->isDead;
		const bool isReloading = weaponComponent && weaponComponent->isReloading;
		return isAlive && !isReloading && !GetCharacterMovement()->IsFalling();
	}

	void APlayerCharacter::StartCrouch()
	{
		if (!CanCrouch())
		{
			return;
		}

		// Crouch()가 캡슐 크기를 줄이면서 발 위치를 유지하려고 캡슐(springArm의 부모)을 그 프레임에
		// 즉시 아래로 이동시킨다. 그 이동량을 미리/이후 캡슐 반높이 차이로 계산해 보정값에 더해두면,
		// Tick에서 그 보정값이 서서히 0으로 줄어들면서 순간이동 없이 부드럽게 이어진다.
		const float previousHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		isCrouching = true;
		isRunning = false;

		Crouch();

		const float newHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		cameraOffsetCompensation += (previousHalfHeight - newHalfHeight);

		GetCharacterMovement()->MaxWalkSpeed = crouchSpeed;
	}

	void APlayerCharacter::StopCrouch()
	{
		const float previousHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		isCrouching = false;

		UnCrouch();

		const float newHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		cameraOffsetCompensation += (previousHalfHeight - newHalfHeight);

		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	}


	void APlayerCharacter::StartSpecialSkill()
	{
		// G 입력을 누르고 있는 동안에는 수류탄 시전 가능 여부와 관계없이
		// 총기 발사 입력을 차단한다. 키를 떼면 ReleaseSpecialSkill에서 해제된다.
		isGrenadeKeyHeld = true;

		// 이미 시전 중이거나 손에 수류탄을 들고 있으면 다시 시작하지 않는다.
		if (isCastingGrenade || isGrenadeReady)
		{
			return;
		}

		// 사망 상태에서는 사용할 수 없다.
		if (healthComponent && healthComponent->isDead)
		{
			return;
		}

		// 재장전 중에는 사용할 수 없다.
		if (weaponComponent && weaponComponent->isReloading)
		{
			return;
		}

		if (!battleSystem)
		{
			return;
		}

		isCastingGrenade = true;
		isGrenadeReady = false;
		isGrenadeKeyHeld = true;
		shouldThrowAfterCast = false;

		PlayUpperBodyAnimation(grenadeReadyAnimation);

		GetWorldTimerManager().SetTimer(
			grenadeCastTimerHandle,
			this,
			&APlayerCharacter::FinishGrenadeCast,
			grenadeCastTime,
			false
		);

		UE_LOG(LogTemp, Warning, TEXT("Grenade Cast Start"));
	}

	void APlayerCharacter::ReleaseSpecialSkill()
	{
		isGrenadeKeyHeld = false;

		// 최소 시전시간이 이미 끝났다면 즉시 던진다.
		if (isGrenadeReady)
		{
			ThrowGrenade();
			return;
		}

		// 아직 시전 중이라면 취소하지 않고,
		// 시전 완료 직후 던지도록 예약한다.
		if (isCastingGrenade)
		{
			shouldThrowAfterCast = true;

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Grenade key released during cast - throw after cast")
			);
		}
	}

	void APlayerCharacter::FinishGrenadeCast()
	{
		if (!isCastingGrenade)
		{
			return;
		}

		isCastingGrenade = false;
		isGrenadeReady = true;

		UE_LOG(LogTemp, Warning, TEXT("Grenade Ready"));

		// 시전 도중 G키를 뗐거나 현재 키를 누르고 있지 않으면
		// 준비가 끝나는 즉시 던진다.
		if (shouldThrowAfterCast || !isGrenadeKeyHeld)
		{
			ThrowGrenade();
		}
	}

	void APlayerCharacter::ThrowGrenade()
	{
		if (!isGrenadeReady)
		{
			return;
		}

		// 중복 호출 방지를 위해 먼저 상태를 변경한다.
		isGrenadeReady = false;
		isCastingGrenade = false;

		PlayUpperBodyAnimation(grenadeThrowAnimation);

		if (battleSystem)
		{
			FVector viewLocation;
			FVector viewDirection;

			GetAttackView(viewLocation, viewDirection);

			FVector throwLocation =
				GetMesh()->GetSocketLocation(TEXT("HandGrip_R"));

			battleSystem->RequestSkillAttackByView(
				this,
				viewLocation,
				viewDirection,
				throwLocation
			);
		}

		UE_LOG(LogTemp, Warning, TEXT("Grenade Throw"));

		ResetGrenadeState();
	}

	void APlayerCharacter::ResetGrenadeState()
	{
		GetWorldTimerManager().ClearTimer(grenadeCastTimerHandle);

		isCastingGrenade = false;
		isGrenadeReady = false;
		isGrenadeKeyHeld = false;
		shouldThrowAfterCast = false;
	}


	void APlayerCharacter::HandleDeath(AActor* deadActor)
	{
		isRunning = false;

		GetCharacterMovement()->DisableMovement();

		UE_LOG(LogTemp, Warning, TEXT("Player Dead"));
	}

	//z키 누를시 카메라 시점 변경
	void APlayerCharacter::ToggleCamera()
	{
		isFirstPerson = !isFirstPerson;

		// 카메라만 바꾼다. 이동/조준/애니메이션 상태는 그대로 유지한다.
		firstPersonCamera->SetActive(isFirstPerson);
		thirdPersonCamera->SetActive(!isFirstPerson);
		adsCamera->SetActive(false);


		UpdateCameraPresentation();
	}

	void APlayerCharacter::UpdateCameraPresentation()
	{
		// FPS에서는 전신과 TPS 총을 로컬 플레이어에게 숨기고 전용 팔/총만 보인다.
		// TPS에서는 반대로 전신과 기존 총만 보인다.
		GetMesh()->SetOwnerNoSee(isFirstPerson);
		weaponMesh->SetOwnerNoSee(isFirstPerson);

		firstPersonArmsMesh->SetVisibility(isFirstPerson, true);
		firstPersonWeaponMesh->SetVisibility(isFirstPerson, true);
		firstPersonArmsMesh->SetHiddenInGame(!isFirstPerson, true);
		firstPersonWeaponMesh->SetHiddenInGame(!isFirstPerson, true);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("FPS presentation: FirstPerson=%s ArmsMesh=%s ArmsVisible=%s ArmsAnim=%s"),
			isFirstPerson ? TEXT("true") : TEXT("false"),
			firstPersonArmsMesh->GetSkeletalMeshAsset()
				? *firstPersonArmsMesh->GetSkeletalMeshAsset()->GetName()
				: TEXT("None"),
			firstPersonArmsMesh->IsVisible() ? TEXT("true") : TEXT("false"),
			firstPersonArmsMesh->GetAnimClass()
				? *firstPersonArmsMesh->GetAnimClass()->GetName()
				: TEXT("None")
		);
	}

	void APlayerCharacter::Attack()
	{
		// 수류탄 키를 누르는 동안 좌클릭이 들어와도 총을 발사하지 않는다.
		if (isGrenadeKeyHeld)
		{
			return;
		}

		// Sprint 중 공격 입력이 들어오면 먼저 걷기 상태로 전환한 다음 발사한다.
		// 따라서 달리기 모션과 발사 모션이 동시에 재생되지 않는다.
		if (isRunning)
		{
			StopRun();
		}

		if (!weaponComponent || !weaponComponent->CanAttack())
		{
			return;
		}

		FVector viewLocation;
		FVector viewDirection;

		if (!GetAttackView(viewLocation, viewDirection))
		{
			return;
		}

		PlayUpperBodyAnimation(fireAnimation);
		UAnimSequenceBase* selectedFirstPersonFire = isAdsAiming
			? firstPersonAimedFireAnimation
			: firstPersonFireAnimation;
		PlayFirstPersonUpperBodyAnimation(
			selectedFirstPersonFire,
			1.0f,
			firstPersonFireBlendInTime,
			firstPersonFireBlendOutTime
		);

		USkeletalMeshComponent* activeWeaponMesh =
			isFirstPerson ? firstPersonWeaponMesh : weaponMesh;

		FVector fireLocation =
			activeWeaponMesh->GetSocketLocation(TEXT("Muzzle"));

		weaponComponent->Attack(
			this,
			viewLocation,
			viewDirection,
			fireLocation
		);
	}

	bool APlayerCharacter::GetAttackView(
		FVector& outViewLocation,
		FVector& outViewDirection
	) const
	{
		if (!Controller)
		{
			return false;
		}

		FRotator viewRotation;

		Controller->GetPlayerViewPoint(
			outViewLocation,
			viewRotation
		);

		outViewDirection = viewRotation.Vector();

		return true;
	}

	bool APlayerCharacter::GetLeftHandIKTransform(FTransform& outTransform) const
	{
		// AKS74U FPS 전용 애니메이션은 양손 동작이 이미 제작되어 있다.
		// 여기에 FABRIK을 다시 적용하면 Aim/Fire/Reload 전환마다 왼손 제어권이
		// 바뀌며 포즈가 튀므로, IK는 테스트용 TPS 전신 메시에서만 사용한다.
		if (isFirstPerson)
		{
			return false;
		}

		outTransform = FTransform::Identity;

		// 재장전 애니메이션이 왼손을 탄창으로 이동시킬 수 있도록 IK를 잠시 해제한다.
		if (weaponComponent && weaponComponent->isReloading)
		{
			return false;
		}

		USkeletalMeshComponent* activeWeaponMesh = weaponMesh;
		USkeletalMeshComponent* activeCharacterMesh = GetMesh();

		if (!activeWeaponMesh
			|| !activeCharacterMesh
			|| !activeWeaponMesh->DoesSocketExist(leftHandIKSocketName)
			|| activeCharacterMesh->GetBoneIndex(rightHandBoneName) == INDEX_NONE)
		{
			return false;
		}

		const FTransform socketWorldTransform =
			activeWeaponMesh->GetSocketTransform(leftHandIKSocketName, RTS_World);

		FVector boneSpaceLocation;
		FRotator boneSpaceRotation;
		activeCharacterMesh->TransformToBoneSpace(
			rightHandBoneName,
			socketWorldTransform.GetLocation(),
			socketWorldTransform.Rotator(),
			boneSpaceLocation,
			boneSpaceRotation
		);

		outTransform = FTransform(boneSpaceRotation, boneSpaceLocation);
		return true;
	}

	void APlayerCharacter::Reload()
	{
		if (weaponComponent && weaponComponent->CanReload())
		{
			const bool wasMagazineEmpty = weaponComponent->currentAmmo <= 0;
			UAnimSequenceBase* selectedFirstPersonReload = nullptr;
			if (wasMagazineEmpty)
			{
				selectedFirstPersonReload = isAdsAiming
					? firstPersonAimedEmptyReloadAnimation
					: firstPersonEmptyReloadAnimation;
			}
			else
			{
				selectedFirstPersonReload = isAdsAiming
					? firstPersonAimedReloadAnimation
					: firstPersonReloadAnimation;
			}

			float reloadPlayRate = 1.0f;
			if (reloadAnimation && weaponComponent->reloadTime > UE_SMALL_NUMBER)
			{
				reloadPlayRate = reloadAnimation->GetPlayLength() / weaponComponent->reloadTime;
			}

			PlayUpperBodyAnimation(reloadAnimation, reloadPlayRate);

			float firstPersonReloadPlayRate = 1.0f;
			if (selectedFirstPersonReload && weaponComponent->reloadTime > UE_SMALL_NUMBER)
			{
				firstPersonReloadPlayRate =
					selectedFirstPersonReload->GetPlayLength() / weaponComponent->reloadTime;
			}
			PlayFirstPersonUpperBodyAnimation(
				selectedFirstPersonReload,
				firstPersonReloadPlayRate
			);
			weaponComponent->Reload();
		}
	}

	bool APlayerCharacter::PlayUpperBodyAnimation(UAnimSequenceBase* animation, float playRate)
	{
		if (!animation || !GetMesh())
		{
			return false;
		}

		UAnimInstance* animInstance = GetMesh()->GetAnimInstance();
		if (!animInstance)
		{
			return false;
		}

		animInstance->PlaySlotAnimationAsDynamicMontage(
			animation,
			upperBodySlotName,
			animationBlendInTime,
			animationBlendOutTime,
			FMath::Max(playRate, UE_SMALL_NUMBER)
		);

		return true;
	}

	bool APlayerCharacter::PlayFirstPersonUpperBodyAnimation(
		UAnimSequenceBase* animation,
		float playRate,
		float blendInTime,
		float blendOutTime
	)
	{
		if (!animation || !firstPersonArmsMesh)
		{
			return false;
		}

		UAnimInstance* animInstance = firstPersonArmsMesh->GetAnimInstance();
		if (!animInstance)
		{
			return false;
		}

		animInstance->PlaySlotAnimationAsDynamicMontage(
			animation,
			upperBodySlotName,
			blendInTime >= 0.0f ? blendInTime : animationBlendInTime,
			blendOutTime >= 0.0f ? blendOutTime : animationBlendOutTime,
			FMath::Max(playRate, UE_SMALL_NUMBER)
		);

		return true;
	}

	bool APlayerCharacter::CanAim() const
	{
		const bool isAlive = !healthComponent || !healthComponent->isDead;
		const bool isReloading = weaponComponent && weaponComponent->isReloading;

		return isAlive && !isReloading;
	}

	void APlayerCharacter::StartAim()
	{
		if (!CanAim())
		{
			return;
		}

		isAiming = true;

		// FPS에서는 우클릭 즉시 ADS. TPS는 테스트용 숄더 뷰를 유지한다.
		if (isFirstPerson)
		{
			EnterAdsAim();
		}
		else
		{
			EnterShoulderAim();
		}

	}

	void APlayerCharacter::StopAim()
	{
		if (!isAiming && !isAdsAiming)
		{
			return;
		}

		ExitAim();
	}

	void APlayerCharacter::EnterShoulderAim()
	{
		isAiming = true;
		isAdsAiming = false;

		adsCamera->SetActive(false);
		firstPersonCamera->SetActive(isFirstPerson);
		thirdPersonCamera->SetActive(!isFirstPerson);

		isRunning = false;
		GetCharacterMovement()->MaxWalkSpeed = aimingMoveSpeed;

		UE_LOG(LogTemp, Warning, TEXT("Shoulder Aim"));
	}

	void APlayerCharacter::EnterAdsAim()
	{
		isAiming = true;
		isAdsAiming = true;

		// 일반 FPS처럼 카메라는 고정한다. Tick에서 팔/총을 보간해 AKS74U의
		// SightSocket이 카메라 중앙으로 오도록 이동한다.
		const bool canUseWeaponSight =
			isFirstPerson
			&& firstPersonWeaponMesh
			&& firstPersonWeaponMesh->DoesSocketExist(adsSightSocketName);

		thirdPersonCamera->SetActive(!isFirstPerson);
		firstPersonCamera->SetActive(isFirstPerson);
		adsCamera->SetActive(false);

		if (!canUseWeaponSight)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("ADS fallback: FirstPersonWeapon has no socket named %s"),
				*adsSightSocketName.ToString()
			);
		}

		isRunning = false;
		GetCharacterMovement()->MaxWalkSpeed = aimingMoveSpeed;

		UE_LOG(LogTemp, Warning, TEXT("ADS Aim"));
	}

	void APlayerCharacter::ExitAim()
	{
		isAiming = false;
		isAdsAiming = false;

		adsCamera->SetActive(false);
		firstPersonCamera->SetActive(isFirstPerson);
		thirdPersonCamera->SetActive(!isFirstPerson);

		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

		UE_LOG(LogTemp, Warning, TEXT("Aim End"));
	}
