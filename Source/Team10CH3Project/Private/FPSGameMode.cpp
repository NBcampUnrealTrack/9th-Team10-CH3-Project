#include "FPSGameMode.h"

void AFPSGameMode::StartGame()
{
	isGameStarted = true;
	isGameOver = false;
	isGameCleared = false;

	currentKillCount = 0;
	score = 0;

	remainingTime = timeLimit;

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

void AFPSGameMode::ClearGame()
{
	if (isGameOver || isGameCleared)
	{
		return;
	}

	isGameCleared = true;
	isGameStarted = false;

	GetWorldTimerManager().ClearTimer(gameTimerHandle);

	onGameCleared.Broadcast();
}

void AFPSGameMode::GameOver()
{
	if (isGameCleared)
	{
		return;
	}

	isGameOver = true;
	isGameStarted = false;

	GetWorldTimerManager().ClearTimer(gameTimerHandle);

	onGameOver.Broadcast();
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
