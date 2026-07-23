#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "HealthComponent.h"
#include "PlayerCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "FPSGameMode.h"
#include "UObject/ConstructorHelpers.h"
AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	healthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MuzzleFlashFinder(
		TEXT("/Game/MuzzleFlash/MuzzleFlash/Niagara/NS_MuzzleFlash.NS_MuzzleFlash"));
	if (MuzzleFlashFinder.Succeeded())
	{
		muzzleFlashEffect = MuzzleFlashFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> MuzzleFireSoundFinder(
		TEXT("/Game/NBC_Folder/Sound/SC_Shoot_A_Gun.SC_Shoot_A_Gun"));
	if (MuzzleFireSoundFinder.Succeeded())
	{
		muzzleFireSound = MuzzleFireSoundFinder.Object;
	}
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

	ReceiveDamageAlert(attackerActor);

	if (IsValid(attackerActor) && damageAlertRadius > 0.0f)
	{
		const float alertRadiusSquared = FMath::Square(damageAlertRadius);
		for (TActorIterator<AEnemyCharacter> enemyIterator(GetWorld()); enemyIterator; ++enemyIterator)
		{
			AEnemyCharacter* nearbyEnemy = *enemyIterator;
			if (nearbyEnemy
				&& nearbyEnemy != this
				&& FVector::DistSquared(GetActorLocation(), nearbyEnemy->GetActorLocation()) <= alertRadiusSquared)
			{
				nearbyEnemy->ReceiveDamageAlert(attackerActor);
			}
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

void AEnemyCharacter::ReceiveDamageAlert(AActor* attackerActor)
{
	if (!IsValid(attackerActor)
		|| attackerActor == this
		|| !Cast<APlayerCharacter>(attackerActor)
		|| (healthComponent && healthComponent->isDead))
	{
		return;
	}

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

void AEnemyCharacter::PlayMuzzleFlash() const
{
	if (!muzzleFlashEffect)
	{
		return;
	}

	TArray<UStaticMeshComponent*> staticMeshComponents;
	GetComponents(staticMeshComponents);

	for (UStaticMeshComponent* staticMeshComponent : staticMeshComponents)
	{
		if (!staticMeshComponent || !staticMeshComponent->DoesSocketExist(muzzleSocketName))
		{
			continue;
		}

		UNiagaraFunctionLibrary::SpawnSystemAttached(
			muzzleFlashEffect,
			staticMeshComponent,
			muzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
		return;
	}
}

void AEnemyCharacter::PlayMuzzleSound() const
{
	if (!muzzleFireSound)
	{
		return;
	}

	TArray<UStaticMeshComponent*> staticMeshComponents;
	GetComponents(staticMeshComponents);

	for (UStaticMeshComponent* staticMeshComponent : staticMeshComponents)
	{
		if (!staticMeshComponent || !staticMeshComponent->DoesSocketExist(muzzleSocketName))
		{
			continue;
		}

		UGameplayStatics::PlaySoundAtLocation(
			this,
			muzzleFireSound,
			staticMeshComponent->GetSocketLocation(muzzleSocketName));
		return;
	}
}

bool AEnemyCharacter::GetMuzzleLocation(FVector& outLocation) const
{
	TArray<UStaticMeshComponent*> staticMeshComponents;
	GetComponents(staticMeshComponents);

	for (UStaticMeshComponent* staticMeshComponent : staticMeshComponents)
	{
		if (!staticMeshComponent || !staticMeshComponent->DoesSocketExist(muzzleSocketName))
		{
			continue;
		}

		outLocation = staticMeshComponent->GetSocketLocation(muzzleSocketName);
		return true;
	}

	return false;
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
