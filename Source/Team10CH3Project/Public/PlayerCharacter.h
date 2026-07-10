// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class TEAM10CH3PROJECT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* defaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* moveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* lookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* jumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* runAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* attackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* reloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* toggleCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* crouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* specialSkillAction;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float walkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float runSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isRunning;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float crouchSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isCrouching;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float maxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float currentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool isDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int maxAmmo;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int currentAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int reserveAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float attackDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float reloadTime;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool isReloading;

	FTimerHandle reloadTimerHandle;

	//Skill
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float specialSkillCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool isSpecialSkillReady;

	FTimerHandle specialSkillTimerHandle;

	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);

	void StartRun();
	void StopRun();

	void Attack();
	void Reload();
	void FinishReload();

	bool CanAttack() const;
	bool CanReload() const;
	bool CanRun() const;

	void Die();

	void StartCrouch();
	void StopCrouch();
	bool CanCrouch() const;
	
	void UseSpecialSkill();
	void ResetSpecialSkill();
	bool CanUseSpecialSkill() const;

	// Camera
	void ToggleCamera();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* firstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* springArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* thirdPersonCamera;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool isFirstPerson;

	


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	virtual float TakeDamage(
		float damageAmount,
		struct FDamageEvent const& damageEvent,
		class AController* eventInstigator,
		AActor* damageCauser
	) override;
};
