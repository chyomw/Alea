#include "AleaCharacter.h"
#include "AleaActorPool.h"
#include "AleaGameInstance.h"
#include "AleaProjectileInPool.h"

#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

AAleaCharacter::AAleaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("PhysicsActor"));
	GetMesh()->SetCollisionProfileName(TEXT("PhysicsActor"));
}

void AAleaCharacter::BeginPlay()
{
	Super::BeginPlay();

	ActorPool = CastChecked<UAleaGameInstance>(GetGameInstance())->GetActorPool();
}

void AAleaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AutoAttackCoolDownTime -= DeltaSeconds;
}

void AAleaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAleaCharacter, Team);
	DOREPLIFETIME(AAleaCharacter, bDead);
	DOREPLIFETIME(AAleaCharacter, MaxHealth);
	DOREPLIFETIME(AAleaCharacter, Health);
	DOREPLIFETIME(AAleaCharacter, TrgtAutoAttacked);
}

void AAleaCharacter::MulticastAttack_Implementation(UParticleSystem* AttackEffect, class USoundCue* AttackSound, UParticleSystem* Hit, class AAleaCharacter* Spwnr, AAleaCharacter* Trgt, float Damage)
{
	AAleaProjectileInPool* Projectile = Cast<AAleaProjectileInPool>(ActorPool->Spawn(AAleaProjectileInPool::StaticClass(), GetActorTransform()));
	Projectile->Initialize(AttackEffect, AttackSound, Hit, Spwnr, Trgt, Damage);
}
