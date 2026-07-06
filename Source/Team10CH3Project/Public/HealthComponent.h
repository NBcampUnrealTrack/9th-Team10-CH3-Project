#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TEAM10CH3PROJECT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float maxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float currentHealth = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath onDeath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	bool isDead = false;
	
	UFUNCTION(BlueprintCallable)
	void TakeDamage(float damageAmount);

	UFUNCTION()
	void HandleDeath();	
};