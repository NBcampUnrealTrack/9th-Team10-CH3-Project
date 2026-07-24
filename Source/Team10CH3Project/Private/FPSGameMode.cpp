#include "FPSGameMode.h"
#include "EnemyCharacter.h"
#include "HealthComponent.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AFPSGameMode::AFPSGameMode()
{
	easyDifficultySettings.attackDamageMultiplier = 0.30f;
	easyDifficultySettings.attackAccuracyMultiplier = 0.75f;
	easyDifficultySettings.attackSpreadMultiplier = 1.60f;
	easyDifficultySettings.minBurstShots = 3;
	easyDifficultySettings.maxBurstShots = 5;
	easyDifficultySettings.burstShotInterval = 0.12f;
	easyDifficultySettings.initialDetectionDelay = 1.20f;
	easyDifficultySettings.bodyDamageMultiplier = 1.0f;
	easyDifficultySettings.headDamageMultiplier = 1.0f;
	easyDifficultySettings.headBoneNames = { TEXT("head") };
	easyDifficultySettings.aimBoneName = TEXT("spine_02");
	easyDifficultySettings.aimVerticalOffset = 0.0f;

	normalDifficultySettings.attackDamageMultiplier = 0.40f;
	normalDifficultySettings.attackAccuracyMultiplier = 1.0f;
	normalDifficultySettings.attackSpreadMultiplier = 1.70f;
	normalDifficultySettings.minBurstShots = 3;
	normalDifficultySettings.maxBurstShots = 5;
	normalDifficultySettings.burstShotInterval = 0.12f;
	normalDifficultySettings.initialDetectionDelay = 0.65f;
	normalDifficultySettings.bodyDamageMultiplier = 1.0f;
	normalDifficultySettings.headDamageMultiplier = 1.1f;
	normalDifficultySettings.headBoneNames = { TEXT("head") };
	normalDifficultySettings.aimBoneName = TEXT("spine_02");
	normalDifficultySettings.aimVerticalOffset = 0.0f;

	hardDifficultySettings.attackDamageMultiplier = 0.40f;
	hardDifficultySettings.attackAccuracyMultiplier = 1.08f;
	hardDifficultySettings.attackSpreadMultiplier = 1.75f;
	hardDifficultySettings.minBurstShots = 4;
	hardDifficultySettings.maxBurstShots = 7;
	hardDifficultySettings.burstShotInterval = 0.10f;
	hardDifficultySettings.initialDetectionDelay = 0.25f;
	hardDifficultySettings.bodyDamageMultiplier = 1.0f;
	hardDifficultySettings.headDamageMultiplier = 1.40f;
	hardDifficultySettings.headBoneNames = { TEXT("head") };
	hardDifficultySettings.aimBoneName = TEXT("spine_03");
	hardDifficultySettings.aimVerticalOffset = 8.0f;
}

const FEnemyDifficultySettings& AFPSGameMode::GetCurrentEnemyDifficultySettings() const
{
	switch (difficulty)
	{
	case EGameDifficulty::Easy:
		return easyDifficultySettings;
	case EGameDifficulty::Hard:
		return hardDifficultySettings;
	case EGameDifficulty::Normal:
	default:
		return normalDifficultySettings;
	}
}

bool AFPSGameMode::IsHeadBone(FName boneName) const
{
	return !boneName.IsNone()
		&& GetCurrentEnemyDifficultySettings().headBoneNames.Contains(boneName);
}

FText AFPSGameMode::GetDifficultyDisplayName() const
{
	switch (difficulty)
	{
	case EGameDifficulty::Easy:
		return FText::FromString(TEXT("Easy"));
	case EGameDifficulty::Hard:
		return FText::FromString(TEXT("Hard"));
	case EGameDifficulty::Normal:
	default:
		return FText::FromString(TEXT("Normal"));
	}
}

void AFPSGameMode::StartGame()
{
	if (isGameStarted)
	{
		return;
	}

	isGameStarted = true;
	isGameOver = false;
	isGameCleared = false;
	isGamePaused = false;

	currentKillCount = 0;
	score = 0;
	targetKillCount = 0;

	remainingTime = timeLimit;
	onGameStarted.Broadcast();

	RefreshTargetKillCount();
	GetWorldTimerManager().SetTimer(
		targetCountRefreshTimerHandle,
		this,
		&AFPSGameMode::RefreshTargetKillCount,
		0.1f,
		false
	);

	onScoreChanged.Broadcast(score, currentKillCount);
	onTimeChanged.Broadcast(remainingTime);

	GetWorldTimerManager().SetTimer(
		gameTimerHandle,
		this,
		&AFPSGameMode::UpdateGameTimer,
		1.0f,
		true
	);

}

void AFPSGameMode::AddKillScore(int addScore)
{
	if (!isGameStarted || isGameOver || isGameCleared)
	{
		return;
	}

	currentKillCount++;
	score += addScore;

	onScoreChanged.Broadcast(score, currentKillCount);

	if (currentKillCount >= targetKillCount)
	{
		ClearGame();
	}
}

void AFPSGameMode::RefreshTargetKillCount()
{
	if (!isGameStarted || isGameOver || isGameCleared)
	{
		return;
	}

	targetKillCount = 0;
	for (TActorIterator<AEnemyCharacter> enemyIterator(GetWorld()); enemyIterator; ++enemyIterator)
	{
		AEnemyCharacter* enemy = *enemyIterator;
		if (IsValid(enemy) && (!enemy->healthComponent || !enemy->healthComponent->isDead))
		{
			++targetKillCount;
		}
	}

	onScoreChanged.Broadcast(score, currentKillCount);
}

void AFPSGameMode::RegisterSpawnedEnemy()
{
	if (!isGameStarted || isGameOver || isGameCleared)
	{
		return;
	}

	++targetKillCount;
	onScoreChanged.Broadcast(score, currentKillCount);
}

void AFPSGameMode::ClearGame()
{
	if (isGameOver || isGameCleared)
	{
		return;
	}

	isGameCleared = true;
	isGameStarted = false;
	isGamePaused = false;

	GetWorldTimerManager().ClearTimer(gameTimerHandle);
	GetWorldTimerManager().ClearTimer(targetCountRefreshTimerHandle);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Game cleared after %d/%d kills."),
		currentKillCount,
		targetKillCount);
	onGameCleared.Broadcast();
}

void AFPSGameMode::GameOver()
{
	if (!isGameStarted || isGameOver || isGameCleared)
	{
		return;
	}

	isGameOver = true;
	isGameStarted = false;
	isGamePaused = false;

	GetWorldTimerManager().ClearTimer(gameTimerHandle);
	GetWorldTimerManager().ClearTimer(targetCountRefreshTimerHandle);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Game over with %.1f seconds remaining after %d/%d kills."),
		remainingTime,
		currentKillCount,
		targetKillCount);
	onGameOver.Broadcast();
}

void AFPSGameMode::PauseGame()
{
	if (!isGameStarted || isGameOver || isGameCleared || isGamePaused)
	{
		return;
	}

	isGamePaused = true;
	GetWorldTimerManager().ClearTimer(gameTimerHandle);
	UGameplayStatics::SetGamePaused(this, true);
	onGamePaused.Broadcast();
}

void AFPSGameMode::ResumeGame()
{
	if (!isGameStarted || isGameOver || isGameCleared || !isGamePaused)
	{
		return;
	}

	isGamePaused = false;
	GetWorldTimerManager().SetTimer(
		gameTimerHandle,
		this,
		&AFPSGameMode::UpdateGameTimer,
		1.0f,
		true
	);
	UGameplayStatics::SetGamePaused(this, false);
	onGameResumed.Broadcast();
}

void AFPSGameMode::UpdateGameTimer()
{
	if (!isGameStarted || isGameOver || isGameCleared)
	{
		GetWorldTimerManager().ClearTimer(gameTimerHandle);
		return;
	}

	remainingTime -= 1.0f;

	if (remainingTime <= 0.0f)
	{
		remainingTime = 0.0f;
		onTimeChanged.Broadcast(remainingTime);
		GameOver();
		return;
	}

	onTimeChanged.Broadcast(remainingTime);
}
