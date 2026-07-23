#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AEnemyCharacter;

UCLASS()
class TEAM10CH3PROJECT_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	TSubclassOf<AEnemyCharacter> enemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "1"))
	int32 initialSpawnCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "100.0"))
	float spawnRadius = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0.0"))
	float minimumDistanceFromPlayer = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "1"))
	int32 maxSpawnAttempts = 20;

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner")
	AEnemyCharacter* SpawnEnemy();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void SpawnInitialEnemies();

	bool hasSpawnedInitialEnemies = false;
};
