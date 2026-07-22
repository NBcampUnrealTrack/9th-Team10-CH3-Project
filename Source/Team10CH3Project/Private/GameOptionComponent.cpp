#include "GameOptionComponent.h"

#include "PlayerCharacter.h"

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
