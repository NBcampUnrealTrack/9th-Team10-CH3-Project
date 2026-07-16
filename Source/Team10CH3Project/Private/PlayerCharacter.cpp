// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "HealthComponent.h"
#include "WeaponComponent.h"
#include "BattleSystem.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// 앉기/일어서기 시 카메라 높이를 매 프레임 보간해야 해서 Tick을 켜둔다.
	PrimaryActorTick.bCanEverTick = true;

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

	if (APlayerController* playerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(playerController->GetLocalPlayer()))
		{
			if (defaultMappingContext)
			{
				subsystem->AddMappingContext(defaultMappingContext, 0);
			}
		}

		// 실제로 이 폰을 조종하는 로컬 플레이어일 때만 HUD를 띄운다.
		if (playerHudClass)
		{
			playerHudWidget = CreateWidget<UUserWidget>(playerController, playerHudClass);
			if (playerHudWidget)
			{
				playerHudWidget->AddToViewport();
			}
		}
	}

	// 시작은 TPS 카메라 활성화
	firstPersonCamera->SetActive(false);
	thirdPersonCamera->SetActive(true);

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
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (moveAction)
		{
			enhancedInputComponent->BindAction(moveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		}

		if (lookAction)
		{
			enhancedInputComponent->BindAction(lookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}

		if (jumpAction)
		{
			enhancedInputComponent->BindAction(jumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			enhancedInputComponent->BindAction(jumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		if (runAction)
		{
			enhancedInputComponent->BindAction(runAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRun);
			enhancedInputComponent->BindAction(runAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRun);
		}

		if (crouchAction)
		{
			enhancedInputComponent->BindAction(crouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouch);
			enhancedInputComponent->BindAction(crouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);
		}

		// 공격/재장전은 캐릭터를 거치지 않고 weaponComponent로 바로 연결한다.
		if (attackAction && weaponComponent)
		{
			enhancedInputComponent->BindAction(attackAction, ETriggerEvent::Started, weaponComponent, &UWeaponComponent::Attack);
		}

		if (reloadAction && weaponComponent)
		{
			enhancedInputComponent->BindAction(reloadAction, ETriggerEvent::Started, weaponComponent, &UWeaponComponent::Reload);
		}

		if (specialSkillAction)
		{
			enhancedInputComponent->BindAction(specialSkillAction, ETriggerEvent::Started, this, &APlayerCharacter::UseSpecialSkill);
		}

		if (toggleCameraAction)
		{
			enhancedInputComponent->BindAction(toggleCameraAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleCamera);
		}
	}
}

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
	return isAlive && !isReloading && !isCrouching;
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

void APlayerCharacter::UseSpecialSkill()
{
	// 쿨타임 판정과 실패 이벤트(onSkillAttackFailed)는 BattleSystem이 전담하므로,
	// 여기서는 그대로 위임만 한다.
	if (battleSystem)
	{
		battleSystem->RequestSkillAttack(this);
	}
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

	if (isFirstPerson)
	{
		// 1인칭에서는 머리만 숨김
		GetMesh()->HideBoneByName(
			TEXT("head"),
			EPhysBodyOp::PBO_None
		);
	}
	else
	{
		// 3인칭으로 돌아가면 머리 표시
		GetMesh()->UnHideBoneByName(TEXT("head"));
	}
}
