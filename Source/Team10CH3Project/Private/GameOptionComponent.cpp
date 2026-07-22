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
	UE_LOG(LogTemp, Warning, TEXT("Calculated Value : %f"), RawInputAxisValue * mouseSensitivity);
	return RawInputAxisValue * mouseSensitivity;
}

void UGameOptionComponent::SetMouseSensitivity(float NewSensitivity)
{
	UE_LOG(LogTemp, Warning, TEXT("Mouse Sensitivity %f"), NewSensitivity);
	mouseSensitivity = NewSensitivity;
}

void UGameOptionComponent::SetSoundVolume(float NewVolume)
{
	UE_LOG(LogTemp, Warning, TEXT("Sound Volume %f"), NewVolume);
	
	USoundClass* MasterSoundClass = LoadObject<USoundClass>(nullptr, TEXT("/Engine/EngineSounds/Master.Master"));
	if (MasterSoundClass)
	{
		MasterSoundClass->Properties.Volume = (NewVolume / 100.0f);
	}
}

void UGameOptionComponent::SetFOV(float NewFOV)
{
	UE_LOG(LogTemp, Warning, TEXT("FOV %f"), NewFOV);
	
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
