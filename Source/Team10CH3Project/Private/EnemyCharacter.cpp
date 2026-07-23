#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "HealthComponent.h"

#include "Components/CapsuleComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSGameMode.h"
AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	healthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_Visibility,
		ECollisionResponse::ECR_Block);
	if (UCharacterMovementComponent* movement = GetCharacterMovement())
	{
		movement->bUseControllerDesiredRotation = true;
		movement->bOrientRotationToMovement = false;
		movement->RotationRate = FRotator(0.0f, patrolRotationSpeed, 0.0f);
		movement->bRequestedMoveUseAcceleration = true;
		movement->bUseRVOAvoidance = true;
		movement->AvoidanceConsiderationRadius = avoidanceRadius;
	}
}
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Blueprint collision overrides are applied after the native constructor.
	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_Visibility,
		ECollisionResponse::ECR_Block);
	if (UCharacterMovementComponent* movement = GetCharacterMovement())
	{
		bUseControllerRotationYaw = false;
		movement->bUseControllerDesiredRotation = true;
		movement->bOrientRotationToMovement = false;
		resolvedAlertMoveSpeed = alertMoveSpeed > 0.0f
			? alertMoveSpeed
			: movement->MaxWalkSpeed;

		SetAlertMovementMode(false);
		// The locomotion graph uses acceleration to distinguish movement from idle.
		movement->bRequestedMoveUseAcceleration = true;
	}
	if (healthComponent)
	{
		healthComponent->onDamaged.AddDynamic(this, &AEnemyCharacter::HandleDamaged);
		healthComponent->onDeath.AddDynamic(this, &AEnemyCharacter::HandleDeath);
	}
}
void AEnemyCharacter::HandleDamaged(
	float damageAmount,
	AActor* attackerActor,
	FVector hitLocation,
	FVector attackDirection)
{
	if (!healthComponent)
	{
		return;
	}

	if (IsValid(attackerActor) && attackerActor != this)
	{
		SetAlertMovementMode(true);
		damageAlertTarget = attackerActor;
		damageAlertEndTime = GetWorld()->GetTimeSeconds() + damageAlertDuration;

		if (AEnemyAIController* aiController = Cast<AEnemyAIController>(GetController()))
		{
			if (UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
			{
				blackboard->SetValueAsObject(TEXT("targetActor"), attackerActor);
				blackboard->SetValueAsVector(TEXT("lastKnownLocation"), attackerActor->GetActorLocation());
				blackboard->SetValueAsBool(TEXT("isCombatReady"), false);
			}

			aiController->SetFocus(attackerActor);
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Enemy damaged: %s | Damage: %.1f | Health: %.1f / %.1f | Hit: %s"),
		*GetName(),
		damageAmount,
		healthComponent->currentHealth,
		healthComponent->maxHealth,
		*hitLocation.ToString());
}

bool AEnemyCharacter::IsDamageAlertActive() const
{
	return IsValid(damageAlertTarget)
		&& GetWorld()
		&& GetWorld()->GetTimeSeconds() < damageAlertEndTime;
}

AActor* AEnemyCharacter::GetDamageAlertTarget() const
{
	return IsDamageAlertActive() ? damageAlertTarget : nullptr;
}

void AEnemyCharacter::SetAlertMovementMode(bool isAlerted)
{
	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (!movement)
	{
		return;
	}

	isUsingAlertMovement = isAlerted;
	movement->RotationRate = FRotator(
		0.0f,
		isAlerted ? alertRotationSpeed : patrolRotationSpeed,
		0.0f);
	movement->MaxWalkSpeed = isAlerted
		? resolvedAlertMoveSpeed
		: patrolMoveSpeed;
}

FRotator AEnemyCharacter::GetUpperBodyAimOffset() const
{
	if (!Controller)
	{
		return FRotator::ZeroRotator;
	}

	const FRotator aimDelta =
		(Controller->GetControlRotation() - GetActorRotation()).GetNormalized();

	return FRotator(
		FMath::Clamp(aimDelta.Pitch, -maxUpperBodyAimPitch, maxUpperBodyAimPitch),
		FMath::Clamp(aimDelta.Yaw, -maxUpperBodyAimYaw, maxUpperBodyAimYaw),
		0.0f);
}

void AEnemyCharacter::HandleDeath(AActor* deadActor)
{
	if (AController* aiController = GetController())
	{
		aiController->UnPossess();
	}
	SetActorEnableCollision(false);
	if (AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>())
	{
		gameMode->AddKillScore(killScoreValue);
	}
	SetActorHiddenInGame(true);
	// Keep the actor valid until hit and HUD delegates finish their current call chain.
	SetLifeSpan(0.2f);
}
