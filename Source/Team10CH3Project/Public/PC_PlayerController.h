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
class UGameOptionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnStartMenu,
	bool,
	bIsStartMenu
	);

UCLASS()
class TEAM10CH3PROJECT_API APC_PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	APC_PlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowGameOverWidget();
	
	//Option Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Option")
	UGameOptionComponent* gameOptionComponent = nullptr;
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsStartMenu = false; 
	
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnStartMenu OnGameStartMenu;
	
	UFUNCTION(BlueprintCallable, Category = "StartUI")
	void OnStartButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "StartUI")
	void OnEndButtonClicked();
	
	UFUNCTION(BlueprintCallable, Category = "StartUI")
	void OnOptionButtonClicked();
	
	UFUNCTION(BlueprintCallable, Category = "StartUI")
	void FOnStartMenu(bool bNewIsStartMenu);

	UFUNCTION(BlueprintCallable, Category = "Option")
	void OnReturnButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "Option")
	void OnGoToHomeButtonClicked();

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

	void StartAttack();
	void StopAttack();
	void ToggleFireMode();
	void Reload();

	void StartSpecialSkill();
	void ReleaseSpecialSkill();

	void ToggleCamera();

	void StartAim();
	void StopAim();
	void OptionMenu();
};
