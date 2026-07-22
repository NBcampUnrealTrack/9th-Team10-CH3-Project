// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PC_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class APlayerCharacter;

UCLASS()
class TEAM10CH3PROJECT_API APC_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* defaultMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* moveAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* lookAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* jumpAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* runAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* crouchAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* attackAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* reloadAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* specialSkillAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* toggleCameraAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* aimAction = nullptr;

	// UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> playerHudClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UUserWidget* playerHudWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> startWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UUserWidget* startWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> endWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UUserWidget* endWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> optionWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UUserWidget* optionWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* optionAction = nullptr;

	UFUNCTION(BlueprintCallable, Category = "StartUI")
	void OnStartButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "StartUI")
	void OnEndButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "Option")
	void OnReturnButtonClicked();

private:
	UPROPERTY()
	APlayerCharacter* playerCharacter = nullptr;

	APlayerCharacter* GetPlayerCharacter();

	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);

	void StartJump();
	void StopJump();

	void StartRun();
	void StopRun();

	void StartCrouch();
	void StopCrouch();

	void Attack();
	void Reload();

	void StartSpecialSkill();
	void ReleaseSpecialSkill();

	void ToggleCamera();

	void StartAim();
	void StopAim();
	void OptionMenu();
};
