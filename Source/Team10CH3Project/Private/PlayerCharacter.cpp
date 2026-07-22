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
	#include "PC_PlayerController.h"
	#include "GameOptionComponent.h"
	#include "Engine/World.h"

	// Sets default values
	APlayerCharacter::APlayerCharacter()
	{
		// 앉기/일어서기 시 카메라 높이를 매 프레임 보간해야 해서 Tick을 켜둔다.
		PrimaryActorTick.bCanEverTick = true;

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

		firstPersonCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonCameraRoot"));
		firstPersonCameraRoot->SetupAttachment(GetCapsuleComponent());
		firstPersonCameraRoot->SetRelativeLocation(firstPersonEyeOffset);
		firstPersonCameraRoot->SetUsingAbsoluteRotation(true);

		firstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
		firstPersonCamera->SetupAttachment(firstPersonCameraRoot);
		firstPersonCamera->SetRelativeLocation(FVector::ZeroVector);
		firstPersonCamera->SetRelativeRotation(FRotator::ZeroRotator);
		firstPersonCamera->bUsePawnControlRotation = true;

		firstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
		firstPersonArmsMesh->SetupAttachment(firstPersonCamera);
		firstPersonArmsMesh->SetRelativeLocation(firstPersonArmsLocation);
		firstPersonArmsMesh->SetRelativeRotation(firstPersonArmsRotation);
		firstPersonArmsMesh->SetRelativeScale3D(FVector(firstPersonArmsScale));
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

		PrimaryActorTick.TickGroup = TG_PostUpdateWork;
		AddTickPrerequisiteComponent(GetMesh());


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

		if (weaponMesh && firstPersonWeaponMesh)
		{
			firstPersonWeaponMesh->SetSkeletalMeshAsset(weaponMesh->GetSkeletalMeshAsset());
		}

		if (firstPersonArmsMesh && firstPersonArmsMesh->GetSkeletalMeshAsset())
		{
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

		PC = Cast<APC_PlayerController>(GetController());
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

			const FVector cameraWorldLocation =
				headWorldTransform.GetLocation()
				+ GetActorQuat().RotateVector(firstPersonEyeOffset);

			firstPersonCameraRoot->SetWorldLocation(cameraWorldLocation);

		}
		else
		{
			FVector firstPersonRootLocation = firstPersonCameraRoot->GetRelativeLocation();
			firstPersonRootLocation.X = firstPersonEyeOffset.X;
			firstPersonRootLocation.Y = firstPersonEyeOffset.Y;
			firstPersonRootLocation.Z = springArmBaseHeight + cameraOffsetCompensation;
			firstPersonCameraRoot->SetRelativeLocation(firstPersonRootLocation);
		}

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
					const FVector sightLocationInCameraSpace =
						firstPersonCamera->GetComponentTransform().InverseTransformPosition(
							firstPersonWeaponMesh->GetSocketLocation(adsSightSocketName)
						);

					targetArmsLocation =
						firstPersonArmsMesh->GetRelativeLocation()
						+ (adsSightViewOffset - sightLocationInCameraSpace);
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
			if (APC_PlayerController* playerController = Cast<APC_PlayerController>(Controller))
			{
				if (playerController->gameOptionComponent)
				{
					lookValue.X = playerController->gameOptionComponent->ApplySensitivity(lookValue.X);
					lookValue.Y = playerController->gameOptionComponent->ApplySensitivity(lookValue.Y);
				}
			}

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

			onSpecialSkillThrown.Broadcast();
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

		firstPersonCamera->SetActive(isFirstPerson);
		thirdPersonCamera->SetActive(!isFirstPerson);
		adsCamera->SetActive(false);


		UpdateCameraPresentation();
	}

	void APlayerCharacter::UpdateCameraPresentation()
	{
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
		if (isGrenadeKeyHeld)
		{
			return;
		}

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
		if (isFirstPerson)
		{
			return false;
		}

		outTransform = FTransform::Identity;

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

	void APlayerCharacter::SetNormalFOV(float NewFOV)
	{
		normalFieldOfView = NewFOV;
	}

	float APlayerCharacter::GetNormalFOV()
	{
		return normalFieldOfView;
	}
