// Fill out your copyright notice in the Description page of Project Settings.

#include "PC_PlayerController.h"
#include "PlayerCharacter.h"
#include "FPSGameMode.h"
#include "GameOptionComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputCoreTypes.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 GameClearedWidgetState = 0;
	constexpr int32 PlayerDiedWidgetState = 1;
	constexpr int32 TimeOverWidgetState = 2;
	const FName GameOverWidgetName(TEXT("GAMEOVER"));
	const FName MissionCompleteWidgetName(TEXT("MISSIONCOMPLETE"));
	const FName TimeOverWidgetName(TEXT("TIMEOVER"));
	const FName DamageOverlayWidgetName(TEXT("DamageOverlay"));
	const FName ShowDamageEventName(TEXT("ShowDamageEvent"));
	const FName HeadSocketName(TEXT("head"));
	const FVector2D DamageWidgetScreenOffset(0.0f, -20.0f);
}

APC_PlayerController::APC_PlayerController()
{
	gameOptionComponent = CreateDefaultSubobject<UGameOptionComponent>(TEXT("gameOptionComponent"));

	static ConstructorHelpers::FClassFinder<UUserWidget> damageWidgetFinder(
		TEXT("/Game/JH_OCK/UI/WBP_Damage"));
	if (damageWidgetFinder.Succeeded())
	{
		damageWidgetClass = damageWidgetFinder.Class;
	}
}

void APC_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	playerCharacter = Cast<APlayerCharacter>(GetPawn());

	if (ULocalPlayer* localPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem =
			localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (defaultMappingContext)
			{
				subsystem->AddMappingContext(defaultMappingContext, 0);
			}
		}
	}

	if (!IsLocalController())
	{
		return;
	}

	if (playerHudClass)
	{
		playerHudWidget = CreateWidget<UUserWidget>(this, playerHudClass);
		if (playerHudWidget)
		{
			if (UWidget* damageOverlay =
				playerHudWidget->GetWidgetFromName(DamageOverlayWidgetName))
			{
				damageOverlay->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	if (startWidgetClass)
	{
		startWidget = CreateWidget<UUserWidget>(this, startWidgetClass);
	}
	if (endWidgetClass)
	{
		endWidget = CreateWidget<UUserWidget>(this, endWidgetClass);
	}
	if (optionWidgetClass)
	{
		optionWidget = CreateWidget<UUserWidget>(this, optionWidgetClass);
	}

	if (AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>())
	{
		gameMode->onGameOver.AddDynamic(this, &APC_PlayerController::ShowGameOverWidget);
		gameMode->onGameCleared.AddDynamic(this, &APC_PlayerController::ShowGameClearedWidget);
	}

	if (startWidget)
	{
		startWidget->AddToViewport();
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
		UGameplayStatics::SetGamePaused(this, true);
	}
	else if (playerHudWidget)
	{
		playerHudWidget->AddToViewport();
	}
}

void APC_PlayerController::ShowGameOverWidget()
{
	int32 endState = PlayerDiedWidgetState;
	if (const AFPSGameMode* gameMode = GetWorld()->GetAuthGameMode<AFPSGameMode>())
	{
		if (gameMode->isGameCleared)
		{
			endState = GameClearedWidgetState;
		}
		else if (gameMode->remainingTime <= 0.0f)
		{
			endState = TimeOverWidgetState;
		}
	}

	ShowEndWidget(endState);
}

void APC_PlayerController::ShowGameClearedWidget()
{
	ShowEndWidget(GameClearedWidgetState);
}

void APC_PlayerController::ShowDamageNumber(
	AActor* targetActor,
	float finalDamage,
	bool isHeadShot,
	FVector hitLocation)
{
	if (!IsLocalController() || !IsValid(targetActor) || !damageWidgetClass)
	{
		return;
	}

	FVector damageWorldLocation = hitLocation;
	if (const ACharacter* targetCharacter = Cast<ACharacter>(targetActor))
	{
		if (const USkeletalMeshComponent* targetMesh = targetCharacter->GetMesh();
			targetMesh && targetMesh->DoesSocketExist(HeadSocketName))
		{
			damageWorldLocation = targetMesh->GetSocketLocation(HeadSocketName);
		}
		else
		{
			FVector boundsOrigin;
			FVector boundsExtent;
			targetActor->GetActorBounds(false, boundsOrigin, boundsExtent);
			damageWorldLocation =
				FVector(boundsOrigin.X, boundsOrigin.Y, boundsOrigin.Z + boundsExtent.Z);
		}
	}

	FVector2D widgetPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		this,
		damageWorldLocation,
		widgetPosition,
		true))
	{
		return;
	}

	UUserWidget* damageWidget = CreateWidget<UUserWidget>(this, damageWidgetClass);
	if (!damageWidget)
	{
		return;
	}

	damageWidget->AddToViewport(20);
	damageWidget->SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
	damageWidget->SetPositionInViewport(
		widgetPosition + DamageWidgetScreenOffset,
		false);

	if (UFunction* showDamageFunction =
		damageWidget->FindFunction(ShowDamageEventName))
	{
		struct FShowDamageParameters
		{
			double Damage;
		};

		FShowDamageParameters parameters{static_cast<double>(finalDamage)};
		damageWidget->ProcessEvent(showDamageFunction, &parameters);
	}
}

void APC_PlayerController::ShowEndWidget(int32 endState)
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Showing end widget with state %d."), endState);

	if (playerHudWidget)
	{
		playerHudWidget->RemoveFromParent();
	}

	if (optionWidget)
	{
		optionWidget->RemoveFromParent();
	}

	if (!endWidget && endWidgetClass)
	{
		endWidget = CreateWidget<UUserWidget>(this, endWidgetClass);
	}

	if (endWidget && !endWidget->IsInViewport())
	{
		endWidget->AddToViewport();
	}

	if (endWidget)
	{
		const auto setEndStateVisibility =
			[this, endState](const FName widgetName, const int32 widgetState)
		{
			if (UWidget* stateWidget = endWidget->GetWidgetFromName(widgetName))
			{
				stateWidget->SetVisibility(
					endState == widgetState
						? ESlateVisibility::Visible
						: ESlateVisibility::Hidden);
			}
		};

		setEndStateVisibility(GameOverWidgetName, PlayerDiedWidgetState);
		setEndStateVisibility(MissionCompleteWidgetName, GameClearedWidgetState);
		setEndStateVisibility(TimeOverWidgetName, TimeOverWidgetState);
	}

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	UGameplayStatics::SetGamePaused(this, true);
}

void APC_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* enhancedInputComponent =
		Cast<UEnhancedInputComponent>(InputComponent);

	if (!enhancedInputComponent)
	{
		return;
	}

	if (moveAction)
	{
		enhancedInputComponent->BindAction(moveAction, ETriggerEvent::Triggered, this, &APC_PlayerController::Move);
	}

	if (lookAction)
	{
		enhancedInputComponent->BindAction(lookAction, ETriggerEvent::Triggered, this, &APC_PlayerController::Look);
	}

	if (jumpAction)
	{
		enhancedInputComponent->BindAction(jumpAction, ETriggerEvent::Started, this, &APC_PlayerController::StartJump);
		enhancedInputComponent->BindAction(jumpAction, ETriggerEvent::Completed, this, &APC_PlayerController::StopJump);
	}

	if (runAction)
	{
		enhancedInputComponent->BindAction(runAction, ETriggerEvent::Started, this, &APC_PlayerController::StartRun);
		enhancedInputComponent->BindAction(runAction, ETriggerEvent::Completed, this, &APC_PlayerController::StopRun);
	}

	if (crouchAction)
	{
		enhancedInputComponent->BindAction(crouchAction, ETriggerEvent::Started, this, &APC_PlayerController::StartCrouch);
		enhancedInputComponent->BindAction(crouchAction, ETriggerEvent::Completed, this, &APC_PlayerController::StopCrouch);
	}

	if (attackAction)
	{
		enhancedInputComponent->BindAction(attackAction, ETriggerEvent::Started, this, &APC_PlayerController::StartAttack);
		enhancedInputComponent->BindAction(attackAction, ETriggerEvent::Completed, this, &APC_PlayerController::StopAttack);
		enhancedInputComponent->BindAction(attackAction, ETriggerEvent::Canceled, this, &APC_PlayerController::StopAttack);
	}

	InputComponent->BindKey(
		EKeys::B,
		IE_Pressed,
		this,
		&APC_PlayerController::ToggleFireMode
	);

	if (reloadAction)
	{
		enhancedInputComponent->BindAction(reloadAction, ETriggerEvent::Started, this, &APC_PlayerController::Reload);
	}

	if (specialSkillAction)
	{
		enhancedInputComponent->BindAction(specialSkillAction, ETriggerEvent::Started, this, &APC_PlayerController::StartSpecialSkill);
		enhancedInputComponent->BindAction(specialSkillAction, ETriggerEvent::Completed, this, &APC_PlayerController::ReleaseSpecialSkill);
		enhancedInputComponent->BindAction(specialSkillAction, ETriggerEvent::Canceled, this, &APC_PlayerController::ReleaseSpecialSkill);
	}

	if (toggleCameraAction)
	{
		enhancedInputComponent->BindAction(toggleCameraAction, ETriggerEvent::Started, this, &APC_PlayerController::ToggleCamera);
	}

	if (aimAction)
	{
		enhancedInputComponent->BindAction(aimAction, ETriggerEvent::Started, this, &APC_PlayerController::StartAim);
		enhancedInputComponent->BindAction(aimAction, ETriggerEvent::Completed, this, &APC_PlayerController::StopAim);
		enhancedInputComponent->BindAction(aimAction, ETriggerEvent::Canceled, this, &APC_PlayerController::StopAim);
	}

	if (optionAction)
	{
		optionAction->bTriggerWhenPaused = true;
		enhancedInputComponent->BindAction(optionAction, ETriggerEvent::Started, this, &APC_PlayerController::OptionMenu);
	}
}

APlayerCharacter* APC_PlayerController::GetPlayerCharacter()
{

	if (!IsValid(playerCharacter))
	{
		playerCharacter = Cast<APlayerCharacter>(GetPawn());
	}

	return playerCharacter;
}

void APC_PlayerController::Move(const FInputActionValue& value)
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->Move(value);
	}
}

void APC_PlayerController::Look(const FInputActionValue& value)
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->Look(value);
	}
}

void APC_PlayerController::StartJump()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->Jump();
	}
}

void APC_PlayerController::StopJump()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StopJumping();
	}
}

void APC_PlayerController::StartRun()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StartRun();
	}
}

void APC_PlayerController::StopRun()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StopRun();
	}
}

void APC_PlayerController::StartCrouch()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StartCrouch();
	}
}

void APC_PlayerController::StopCrouch()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StopCrouch();
	}
}

void APC_PlayerController::StartAttack()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StartAttack();
	}
}

void APC_PlayerController::StopAttack()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StopAttack();
	}
}

void APC_PlayerController::ToggleFireMode()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->ToggleFireMode();
	}
}

void APC_PlayerController::Reload()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->Reload();
	}
}

void APC_PlayerController::StartSpecialSkill()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StartSpecialSkill();
	}
}

void APC_PlayerController::ReleaseSpecialSkill()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->ReleaseSpecialSkill();
	}
}

void APC_PlayerController::ToggleCamera()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->ToggleCamera();
	}
}

void APC_PlayerController::StartAim()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StartAim();
	}
}

void APC_PlayerController::StopAim()
{
	if (APlayerCharacter* character = GetPlayerCharacter())
	{
		character->StopAim();
	}
}

void APC_PlayerController::OnStartButtonClicked()
{
	if (startWidget)
	{
		startWidget->RemoveFromParent();
	}

	FOnStartMenu(false);

	if (AFPSGameMode* gameMode = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode()))
	{
		gameMode->StartGame();
	}

	UGameplayStatics::SetGamePaused(this, false);

	if (playerHudWidget && !playerHudWidget->IsInViewport())
	{
		playerHudWidget->AddToViewport();
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void APC_PlayerController::OnEndButtonClicked()
{
	AFPSGameMode* gameMode = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode());
	if (gameMode)
	{
		gameMode->GameOver();
	}
	UKismetSystemLibrary::QuitGame(
		GetWorld(),
		this,
		EQuitPreference::Quit,
		false
		);
}

void APC_PlayerController::OnOptionButtonClicked()
{
	OptionMenu();
}

void APC_PlayerController::OnRestartButtonClicked()
{
	ReloadCurrentLevel();
}

void APC_PlayerController::OptionMenu()
{
	
	if (!optionWidget)
	{
		return;
	}

	AFPSGameMode* gameMode = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode());
	if (optionWidget->IsInViewport())
	{
		optionWidget->RemoveFromParent();
		if (bIsStartMenu)
		{
			bShowMouseCursor = true;
			SetInputMode(FInputModeUIOnly());
		}
		else
		{
			bShowMouseCursor = false;
			SetInputMode(FInputModeGameOnly());
			if (gameMode)
			{
				gameMode->ResumeGame();
			}
		}
		return;
	}
	
	if (startWidget && startWidget->IsInViewport())
	{
		FOnStartMenu(true);
		optionWidget->AddToViewport(10);
		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
	}

	if (playerHudWidget && playerHudWidget->IsInViewport())
	{
		if (APlayerCharacter* character = GetPlayerCharacter())
		{
			character->StopAttack();
		}

		FOnStartMenu(false);
		optionWidget->AddToViewport(10);
		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
		if (gameMode)
		{
			gameMode->PauseGame();
		}
	}
}

void APC_PlayerController::OnReturnButtonClicked()
{
	if (optionWidget && optionWidget->IsInViewport())
	{
		optionWidget->RemoveFromParent();
		if (bIsStartMenu)
		{
			bShowMouseCursor = true;
			SetInputMode(FInputModeUIOnly());
		}
		else
		{
			bShowMouseCursor = false;
			SetInputMode(FInputModeGameOnly());

			if (AFPSGameMode* gameMode = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode()))
			{
				gameMode->ResumeGame();
			}
		}
	}
}

void APC_PlayerController::OnGoToHomeButtonClicked()
{
	ReloadCurrentLevel();
}

void APC_PlayerController::FOnStartMenu(bool bNewIsStartMenu)
{
	bIsStartMenu = bNewIsStartMenu;

	if (OnGameStartMenu.IsBound())
	{
		OnGameStartMenu.Broadcast(bIsStartMenu);
	}
}

void APC_PlayerController::ReloadCurrentLevel()
{
	UGameplayStatics::SetGamePaused(this, false);

	const FString currentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*currentLevelName));
}
