#include "GameOptionComponent.h"

#include "FPSGameMode.h"
#include "PlayerCharacter.h"

#include "GameFramework/PlayerController.h"
#include "Sound/SoundClass.h"

UGameOptionComponent::UGameOptionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGameOptionComponent::BeginPlay()
{
	Super::BeginPlay();
}

float UGameOptionComponent::ApplySensitivity(float RawInputAxisValue)
{
	return RawInputAxisValue * mouseSensitivity;
}

void UGameOptionComponent::SetMouseSensitivity(float NewSensitivity)
{
	mouseSensitivity = NewSensitivity;
}

void UGameOptionComponent::SetSoundVolume(float NewVolume)
{
	USoundClass* MasterSoundClass = LoadObject<USoundClass>(nullptr, TEXT("/Engine/EngineSounds/Master.Master"));
	if (MasterSoundClass)
	{
		MasterSoundClass->Properties.Volume = (NewVolume / 100.0f);
	}
}

void UGameOptionComponent::SetFOV(float NewFOV)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (PC)
	{
		APlayerCharacter* Character = Cast<APlayerCharacter>(PC->GetPawn());
		if (Character)
		{
			Character->SetNormalFOV(NewFOV);
		}
	}
}

void UGameOptionComponent::SetDifficultyByIndex(int32 DifficultyIndex)
{
	AFPSGameMode* gameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AFPSGameMode>()
		: nullptr;
	if (!gameMode)
	{
		return;
	}

	switch (DifficultyIndex)
	{
	case 0:
		gameMode->difficulty = EGameDifficulty::Easy;
		break;
	case 2:
		gameMode->difficulty = EGameDifficulty::Hard;
		break;
	case 1:
	default:
		gameMode->difficulty = EGameDifficulty::Normal;
		break;
	}
}

int32 UGameOptionComponent::GetDifficultyIndex() const
{
	const AFPSGameMode* gameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AFPSGameMode>()
		: nullptr;
	if (!gameMode)
	{
		return 1;
	}

	switch (gameMode->difficulty)
	{
	case EGameDifficulty::Easy:
		return 0;
	case EGameDifficulty::Hard:
		return 2;
	case EGameDifficulty::Normal:
	default:
		return 1;
	}
}
