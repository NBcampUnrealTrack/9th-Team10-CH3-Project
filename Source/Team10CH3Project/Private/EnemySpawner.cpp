#include "EnemySpawner.h"

#include "EnemyCharacter.h"
#include "FPSGameMode.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>())
	{
		gameMode->onGameStarted.AddDynamic(this, &AEnemySpawner::SpawnInitialEnemies);
		if (gameMode->isGameStarted)
		{
			SpawnInitialEnemies();
		}
	}
}

void AEnemySpawner::SpawnInitialEnemies()
{
	if (hasSpawnedInitialEnemies)
	{
		return;
	}

	hasSpawnedInitialEnemies = true;
	
	int32 successfullySpawnedCount = 0;
	
	for (int32 index = 0; index < initialSpawnCount; ++index)
	{
		if (AEnemyCharacter* spawnedEnemy = SpawnEnemy())
		{
			successfullySpawnedCount++;
		}
	}
	
	if (AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>())
	{
		gameMode->AddTargetKillCount(successfullySpawnedCount);
	}
}

AEnemyCharacter* AEnemySpawner::SpawnEnemy()
{
	if (!enemyClass)
	{
		return nullptr;
	}

	UNavigationSystemV1* navigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!navigationSystem)
	{
		return nullptr;
	}

	const APawn* playerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	for (int32 attempt = 0; attempt < maxSpawnAttempts; ++attempt)
	{
		FNavLocation navigationLocation;
		if (!navigationSystem->GetRandomReachablePointInRadius(
			GetActorLocation(),
			spawnRadius,
			navigationLocation))
		{
			continue;
		}

		if (playerPawn
			&& FVector::DistSquared(navigationLocation.Location, playerPawn->GetActorLocation())
			< FMath::Square(minimumDistanceFromPlayer))
		{
			continue;
		}

		FActorSpawnParameters spawnParameters;
		spawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		const FRotator spawnRotation(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
		return GetWorld()->SpawnActor<AEnemyCharacter>(
			enemyClass,
			navigationLocation.Location,
			spawnRotation,
			spawnParameters);
	}

	return nullptr;
}
