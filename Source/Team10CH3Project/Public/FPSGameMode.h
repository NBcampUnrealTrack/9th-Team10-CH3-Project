#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnScoreChanged,
	int,
	currentScore,
	int,
	currentKillCount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnTimeChanged,
	float,
	remainingTime
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);

UCLASS()
class TEAM10CH3PROJECT_API AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode")
	int targetKillCount = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	int currentKillCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	int score = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	bool isGameStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	bool isGameOver = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	bool isGameCleared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Time")
	float timeLimit = 60.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Time")
	float remainingTime = 0.0f;

	FTimerHandle gameTimerHandle;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnScoreChanged onScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnTimeChanged onTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnGameCleared onGameCleared;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnGameOver onGameOver;

	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void AddKillScore(int addScore);

	UFUNCTION(BlueprintCallable)
	void ClearGame();

	UFUNCTION(BlueprintCallable)
	void GameOver();

	void UpdateGameTimer();
};
