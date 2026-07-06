// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"



// Sets default values
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// FPS Camera
	firstPersonCamera =CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	firstPersonCamera->SetupAttachment(GetCapsuleComponent());
	firstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	firstPersonCamera->bUsePawnControlRotation = true;

	// Spring Arm
	springArm =CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	springArm->SetupAttachment(GetCapsuleComponent());
	springArm->TargetArmLength = 300.f;
	springArm->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	springArm->bUsePawnControlRotation = true;

	// TPS Camera
	thirdPersonCamera =CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	thirdPersonCamera->SetupAttachment(springArm);
	thirdPersonCamera->bUsePawnControlRotation = false;

	isFirstPerson = false;

	walkSpeed = 500.0f;
	runSpeed = 1300.0f;
	isRunning = false;

	maxHealth = 100.0f;
	currentHealth = maxHealth;
	isDead = false;

	maxAmmo = 30;
	currentAmmo = maxAmmo;
	reserveAmmo = 90;

	attackDamage = 10.0f;
	reloadTime = 2.0f;
	isReloading = false;

	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	

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
}


// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

		if (attackAction)
		{
			enhancedInputComponent->BindAction(attackAction, ETriggerEvent::Started, this, &APlayerCharacter::Attack);
		}

		if (reloadAction)
		{
			enhancedInputComponent->BindAction(reloadAction, ETriggerEvent::Started, this, &APlayerCharacter::Reload);
		}
		if (toggleCameraAction)
		{
			enhancedInputComponent->BindAction(toggleCameraAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleCamera);
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& value)
{
	if (isDead)
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
	if (isDead)
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


//여기서 부터 전투관련 상태
void APlayerCharacter::Attack()
{
	if (!CanAttack())
	{
		return;
	}

	currentAmmo--;

	UE_LOG(LogTemp, Warning, TEXT("Attack! Ammo: %d / %d"), currentAmmo, reserveAmmo);

	//총알 발사 구현
}

void APlayerCharacter::Reload()
{
	if (!CanReload())
	{
		return;
	}

	isReloading = true;
	isRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	UE_LOG(LogTemp, Warning, TEXT("Reload Start"));

	GetWorldTimerManager().SetTimer(
		reloadTimerHandle,
		this,
		&APlayerCharacter::FinishReload,
		reloadTime,
		false
	);
}

void APlayerCharacter::FinishReload()
{
	int neededAmmo = maxAmmo - currentAmmo;
	int reloadAmmo = FMath::Min(neededAmmo, reserveAmmo);

	currentAmmo += reloadAmmo;
	reserveAmmo -= reloadAmmo;

	isReloading = false;

	UE_LOG(LogTemp, Warning, TEXT("Reload Finish! Ammo: %d / %d"), currentAmmo, reserveAmmo);
}

bool APlayerCharacter::CanAttack() const
{
	return !isDead && !isReloading && currentAmmo > 0;
}

bool APlayerCharacter::CanReload() const
{
	return !isDead && !isReloading && currentAmmo < maxAmmo && reserveAmmo > 0;
}

bool APlayerCharacter::CanRun() const
{
	return !isDead && !isReloading;
}

float APlayerCharacter::TakeDamage(
	float damageAmount,
	FDamageEvent const& damageEvent,
	AController* eventInstigator,
	AActor* damageCauser
)
{
	if (isDead)
	{
		return 0.0f;
	}

	currentHealth -= damageAmount;

	UE_LOG(LogTemp, Warning, TEXT("Take Damage: %f / Health: %f"), damageAmount, currentHealth);

	if (currentHealth <= 0.0f)
	{
		currentHealth = 0.0f;
		Die();
	}

	return damageAmount;
}

void APlayerCharacter::Die()
{
	isDead = true;
	isRunning = false;
	isReloading = false;

	GetCharacterMovement()->DisableMovement();

	UE_LOG(LogTemp, Warning, TEXT("Player Dead"));
}


//z키 누를시 카메라 시점 변경
void APlayerCharacter::ToggleCamera()
{
	isFirstPerson = !isFirstPerson;

	firstPersonCamera->SetActive(isFirstPerson);
	thirdPersonCamera->SetActive(!isFirstPerson);

	GetMesh()->SetOwnerNoSee(isFirstPerson);

	if (isFirstPerson)
	{
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}