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

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// 이동/카메라 토글 전부 입력 이벤트나 타이머로 처리되고, 매 프레임 갱신해야 하는 로직이
	// 없어서 Tick 자체를 꺼둔다.
	PrimaryActorTick.bCanEverTick = false;

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
	springArm->SetRelativeLocation(FVector(0.f, 20.f, 64.f));
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

	isCrouching = true;
	isRunning = false;

	Crouch();

	GetCharacterMovement()->MaxWalkSpeed = crouchSpeed;
}

void APlayerCharacter::StopCrouch()
{
	isCrouching = false;

	UnCrouch();

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
