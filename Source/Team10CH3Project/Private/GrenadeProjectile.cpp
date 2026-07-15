#include "GrenadeProjectile.h"
#include "BattleSystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	collisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(collisionComponent);

	collisionComponent->SetSphereRadius(15.0f);
	collisionComponent->SetCollisionProfileName(TEXT("BlockAll"));

	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	meshComponent->SetupAttachment(collisionComponent);
	meshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->InitialSpeed = 1200.0f;
	projectileMovement->MaxSpeed = 1200.0f;
	projectileMovement->ProjectileGravityScale = 1.0f;
	projectileMovement->bShouldBounce = true;
	projectileMovement->Bounciness = 0.3f;
}

void AGrenadeProjectile::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		explodeTimerHandle,
		this,
		&AGrenadeProjectile::Explode,
		explodeDelay,
		false
	);
}

void AGrenadeProjectile::InitGrenade(
	ABattleSystem* inBattleSystem,
	AActor* inAttackerActor,
	FVector throwVelocity
)
{
	battleSystem = inBattleSystem;
	attackerActor = inAttackerActor;

	if (projectileMovement)
	{
		projectileMovement->Velocity = throwVelocity;
	}
}

void AGrenadeProjectile::Explode()
{
	if (battleSystem)
	{
		battleSystem->AttackAroundLocation(
			GetActorLocation(),
			attackerActor,
			damageAmount,
			damageRange
		);
	}

	Destroy();
}