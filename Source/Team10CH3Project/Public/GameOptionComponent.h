#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameOptionComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM10CH3PROJECT_API UGameOptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGameOptionComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting|Mouse")
	float mouseSensitivity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting|Volume")
	float soundVolume = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting|FOV")
	float fieldOfView = 103.0f;
	
	UFUNCTION(BlueprintCallable, Category = "Setting|Mouse")
	float ApplySensitivity(float RawInputAxisValue);
	UFUNCTION(BlueprintCallable, Category = "Setting|Mouse")
	void SetMouseSensitivity(float NewSensitivity);
	UFUNCTION(BlueprintCallable, Category = "Setting|Volume")
	void SetSoundVolume(float NewVolume);
	UFUNCTION(BlueprintCallable, Category = "Setting|FOV")
	void SetFOV(float NewFOV);
};
