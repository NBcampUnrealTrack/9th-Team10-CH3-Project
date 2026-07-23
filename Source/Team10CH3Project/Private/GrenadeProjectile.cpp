#include "GrenadeProjectile.h"
#include "BattleSystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	collisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(collisionComponent);

	collisionComponent->SetSphereRadius(15.0f);
	collisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	collisionComponent->SetCollisionObjectType(ECC_WorldDynamic);

	collisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	collisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	meshComponent->SetupAttachment(collisionComponent);
	meshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->InitialSpeed = 1200.0f;
	projectileMovement->MaxSpeed = 1200.0f;
	projectileMovement->ProjectileGravityScale = 1.0f;
	projectileMovement->bShouldBounce = true;
	projectileMovement->Bounciness = 0.3f;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionEffectFinder(
		TEXT("/Game/Weapons/Grenade/Explosions/Prefabs/Niagara_Dust_Explosion_01.Niagara_Dust_Explosion_01")
	);

	if (ExplosionEffectFinder.Succeeded())
	{
		explosionEffect = ExplosionEffectFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ExplosionSoundFinder(
		TEXT("/Game/NBC_Folder/Sound/268557__cydon__explosion_001.268557__cydon__explosion_001"));
	if (ExplosionSoundFinder.Succeeded())
	{
		explosionSound = ExplosionSoundFinder.Object;
	}
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
	UE_LOG(LogTemp, Warning, TEXT("Grenade Explode at %s"), *GetActorLocation().ToString());

	if (explosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, explosionSound, GetActorLocation());
	}

	if (explosionEffect)
	{
		FActorSpawnParameters effectSpawnParams;
		effectSpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ANiagaraActor* explosionActor = GetWorld()->SpawnActor<ANiagaraActor>(
			ANiagaraActor::StaticClass(),
			GetActorLocation() + FVector(0.0f, 0.0f, 20.0f),
			FRotator::ZeroRotator,
			effectSpawnParams
		);

		if (explosionActor)
		{
			UNiagaraComponent* spawnedEffect = explosionActor->GetNiagaraComponent();
			if (spawnedEffect)
			{
				spawnedEffect->SetAsset(explosionEffect);
				spawnedEffect->SetWorldScale3D(explosionEffectScale);
				spawnedEffect->SetForceSolo(true);
				spawnedEffect->Activate(true);
			}

			explosionActor->SetLifeSpan(explosionEffectDuration);
			UE_LOG(LogTemp, Warning, TEXT("Grenade explosion Niagara spawned"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn grenade explosion Niagara"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Grenade explosionEffect is null"));
	}

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
