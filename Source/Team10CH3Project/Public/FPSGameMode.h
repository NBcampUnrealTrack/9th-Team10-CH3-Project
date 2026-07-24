#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameMode.generated.h"

UENUM(BlueprintType)
enum class EGameDifficulty : uint8
{
	Easy UMETA(DisplayName = "Easy"),
	Normal UMETA(DisplayName = "Normal"),
	Hard UMETA(DisplayName = "Hard")
};

USTRUCT(BlueprintType)
struct FEnemyDifficultySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float attackDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float attackAccuracyMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float attackSpreadMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "1"))
	int32 minBurstShots = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "1"))
	int32 maxBurstShots = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.01"))
	float burstShotInterval = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	FName aimBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float aimVerticalOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float initialDetectionDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float bodyDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float headDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	TArray<FName> headBoneNames;
};

//For Restart
UENUM(BlueprintType)
enum class EGameEndReason : uint8
{
	GameCleared = 0,
	PlayerDied = 1,
	TimeOver = 2
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnGameEnded,
	int32,
	endReason
	);

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnTargetKillCountChanged, 
	int32, 
	newTargetCount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGamePaused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameResumed);

UCLASS()
class TEAM10CH3PROJECT_API AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AFPSGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Difficulty")
	EGameDifficulty difficulty = EGameDifficulty::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Difficulty")
	FEnemyDifficultySettings easyDifficultySettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Difficulty")
	FEnemyDifficultySettings normalDifficultySettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Difficulty")
	FEnemyDifficultySettings hardDifficultySettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode")
	int targetKillCount = 0;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	bool isGamePaused = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Time")
	float timeLimit = 180.0f;

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

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnGameStarted onGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnGamePaused onGamePaused;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnGameResumed onGameResumed;

	//For Restart
	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnGameEnded onGameEnded;
	
	UPROPERTY(BlueprintAssignable, Category = "GameMode|Event")
	FOnTargetKillCountChanged onTargetKillCountChanged;
	
	UFUNCTION(BlueprintCallable)
	void AddTargetKillCount(int32 newTargetKillCount);
	
	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void AddKillScore(int addScore);

	UFUNCTION(BlueprintCallable)
	void ClearGame();

	UFUNCTION(BlueprintCallable)
	void GameOver();

	UFUNCTION(BlueprintCallable)
	void PauseGame();

	UFUNCTION(BlueprintCallable)
	void ResumeGame();

	const FEnemyDifficultySettings& GetCurrentEnemyDifficultySettings() const;
	bool IsHeadBone(FName boneName) const;

	UFUNCTION(BlueprintPure, Category = "GameMode|Difficulty")
	FText GetDifficultyDisplayName() const;
	
	void UpdateGameTimer();
};
