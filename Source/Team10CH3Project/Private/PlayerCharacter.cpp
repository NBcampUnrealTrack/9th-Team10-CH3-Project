	// Fill out your copyright notice in the Description page of Project Settings.

	#include "PlayerCharacter.h"

	#include "GameFramework/CharacterMovementComponent.h"
	#include "Components/CapsuleComponent.h"
	#include "Camera/CameraComponent.h"
	#include "GameFramework/SpringArmComponent.h"

	#include "HealthComponent.h"
	#include "WeaponComponent.h"
	#include "BattleSystem.h"
	#include "Engine/World.h"

	// Sets default values
	APlayerCharacter::APlayerCharacter()
	{
		// 앉기/일어서기 시 카메라 높이를 매 프레임 보간해야 해서 Tick을 켜둔다.
		PrimaryActorTick.bCanEverTick = true;

		weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
		weaponMesh->SetupAttachment(GetMesh(),TEXT("WeaponSocket"));
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

		// FPS Camera
		firstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
		firstPersonCamera->SetupAttachment(GetMesh(), TEXT("head"));
		firstPersonCamera->SetRelativeLocation(FVector(20.0f, -5.0f, 65.0f));
		firstPersonCamera->SetRelativeRotation(FRotator::ZeroRotator);
		firstPersonCamera->bUsePawnControlRotation = true;

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


		isFirstPerson = false;

		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
		GetCharacterMovement()->CrouchedHalfHeight = 44.0f;

		healthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
		weaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

		battleSystemClass = ABattleSystem::StaticClass();
	}

	// Called when the game starts or when spawned
	void APlayerCharacter::BeginPlay()
	{
		Super::BeginPlay();


		// 시작은 TPS 카메라 활성화
		firstPersonCamera->SetActive(false);
		thirdPersonCamera->SetActive(true);
		adsCamera->SetActive(false);

		isFirstPerson = false;
		isAiming = false;
		isAdsAiming = false;

		springArm->TargetArmLength = normalArmLength;
		springArm->SocketOffset = normalSocketOffset;
		thirdPersonCamera->SetFieldOfView(normalFieldOfView);

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
	}

	// Called every frame
	void APlayerCharacter::Tick(float DeltaTime)
	{
		Super::Tick(DeltaTime);

		const float targetHeight = isCrouching ? crouchedCameraHeight : standingCameraHeight;
		springArmBaseHeight = FMath::FInterpTo(springArmBaseHeight, targetHeight, DeltaTime, cameraInterpSpeed);

		cameraOffsetCompensation = FMath::FInterpTo(cameraOffsetCompensation, 0.0f, DeltaTime, cameraInterpSpeed);

		FVector springArmLocation = springArm->GetRelativeLocation();
		springArmLocation.Z = springArmBaseHeight + cameraOffsetCompensation;
		springArm->SetRelativeLocation(springArmLocation);

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

		// 필요하면 여기서 수류탄을 꺼내는 애니메이션을 재생한다.
		// PlayAnimMontage(grenadeReadyMontage);

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

		// 필요하면 여기서 투척 애니메이션을 재생한다.
		// PlayAnimMontage(grenadeThrowMontage);

		if (battleSystem)
		{
			FVector viewLocation;
			FVector viewDirection;

			GetAttackView(viewLocation, viewDirection);

			FVector throwLocation =
				GetMesh()->GetSocketLocation(TEXT("GrenadeSocket"));

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
		if (isAiming || isAdsAiming)
		{
			ExitAim();
		}

		isFirstPerson = !isFirstPerson;

		firstPersonCamera->SetActive(isFirstPerson);
		thirdPersonCamera->SetActive(!isFirstPerson);
		adsCamera->SetActive(false);

		if (isFirstPerson)
		{
			GetMesh()->HideBoneByName(
				TEXT("head"),
				EPhysBodyOp::PBO_None
			);
		}
		else
		{
			GetMesh()->UnHideBoneByName(TEXT("head"));
		}
	}

	void APlayerCharacter::Attack()
	{
		if (!weaponComponent)
		{
			return;
		}

		FVector viewLocation;
		FVector viewDirection;

		if (!GetAttackView(viewLocation, viewDirection))
		{
			return;
		}

		FVector fireLocation =
			weaponMesh->GetSocketLocation(TEXT("Muzzle"));

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

	void APlayerCharacter::Reload()
	{
		if (weaponComponent)
		{
			weaponComponent->Reload();
		}
	}

	bool APlayerCharacter::CanAim() const
	{
		const bool isAlive = !healthComponent || !healthComponent->isDead;
		const bool isReloading = weaponComponent && weaponComponent->isReloading;

		return isAlive && !isReloading && !isFirstPerson;
	}

	void APlayerCharacter::StartAim()
	{
		if (!CanAim())
		{
			return;
		}

		const float currentTime = GetWorld()->GetTimeSeconds();
		const float timeSinceLastPress = currentTime - lastAimPressTime;

		isAiming = true;

		// 이전 우클릭으로부터 1초 이내라면 ADS 진입
		if (timeSinceLastPress <= doubleClickTime)
		{
			EnterAdsAim();
		}
		else
		{
			EnterShoulderAim();
		}

		lastAimPressTime = currentTime;
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
		firstPersonCamera->SetActive(false);
		thirdPersonCamera->SetActive(true);

		isRunning = false;
		GetCharacterMovement()->MaxWalkSpeed = aimingMoveSpeed;

		UE_LOG(LogTemp, Warning, TEXT("Shoulder Aim"));
	}

	void APlayerCharacter::EnterAdsAim()
	{
		isAiming = true;
		isAdsAiming = true;

		thirdPersonCamera->SetActive(false);
		firstPersonCamera->SetActive(false);
		adsCamera->SetActive(true);

		adsCamera->SetFieldOfView(adsFieldOfView);

		isRunning = false;
		GetCharacterMovement()->MaxWalkSpeed = aimingMoveSpeed;

		UE_LOG(LogTemp, Warning, TEXT("ADS Aim"));
	}

	void APlayerCharacter::ExitAim()
	{
		isAiming = false;
		isAdsAiming = false;

		adsCamera->SetActive(false);
		firstPersonCamera->SetActive(false);
		thirdPersonCamera->SetActive(true);

		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

		UE_LOG(LogTemp, Warning, TEXT("Aim End"));
	}