#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"
class UHealthComponent;
UCLASS()
class TEAM10CH3PROJECT_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AEnemyCharacter();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Detection")
	float sightRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Detection")
	float sightHalfAngleDegrees = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float attackRange = 4500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float attackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float attackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat", meta = (ClampMin = "0.0"))
	float damageAlertDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat", meta = (ClampMin = "0.0"))
	float damageAlertRadius = 1800.0f;

	bool IsDamageAlertActive() const;
	AActor* GetDamageAlertTarget() const;
	void SetAlertMovementMode(bool isAlerted);
	void ReceiveDamageAlert(AActor* attackerActor);

	UFUNCTION(BlueprintPure, Category = "AI|Animation")
	FRotator GetUpperBodyAimOffset() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	int killScoreValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float attackAccuracy = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float attackMaxSpreadDegrees = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float patrolRotationSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float alertRotationSpeed = 540.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float patrolMoveSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0", DisplayName = "Alert Move Speed (0 = Existing Speed)"))
	float alertMoveSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Animation", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float maxUpperBodyAimYaw = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Animation", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float maxUpperBodyAimPitch = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Debug", meta = (ClampMin = "0.0"))
	float attackTraceDrawDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement")
	float avoidanceRadius = 500.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UHealthComponent* healthComponent = nullptr;
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDamaged(
		float damageAmount,
		AActor* attackerActor,
		FVector hitLocation,
		FVector attackDirection
	);
	UFUNCTION()
	void HandleDeath(AActor* deadActor);

private:
	UPROPERTY(Transient)
	AActor* damageAlertTarget = nullptr;

	float damageAlertEndTime = 0.0f;
	float resolvedAlertMoveSpeed = 0.0f;
	bool isUsingAlertMovement = false;
};
