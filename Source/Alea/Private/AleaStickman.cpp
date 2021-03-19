#include "AleaStickman.h"
#include "AleaTeamRow.h"
#include "AleaStickmanRow.h"
#include "AleaActorPool.h"
#include "AleaStickmanPool.h"
#include "AleaProjectileInPool.h"
#include "AleaAleo.h"
#include "AleaPawn.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Net/UnrealNetwork.h"

AAleaStickman::AAleaStickman()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBar->SetupAttachment(RootComponent);

	ConstructorHelpers::FObjectFinder<UDataTable> DataTable(TEXT("/Game/DataTable/Team/TeamTable"));
	TeamTable = DataTable.Object;
}

void AAleaStickman::Release()
{
	if (!bActive)
	{
		return;
	}

	SetActorTickEnabled(false);
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	bActive = false;
	Pool->Release(this);

	bDead = false;
	TrgtAutoAttacked = nullptr;
	AutoAttackCoolDownTime = 0.f;
	bMoving = false;
	bAttacking = false;
	Health = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetHealth();
}

void AAleaStickman::Activate()
{
	if (bActive)
	{
		return;
	}

	SetActorTickEnabled(true);
	SetActorEnableCollision(true);
	SetActorHiddenInGame(false);

	bActive = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("PhysicsActor"));
	GetMesh()->SetCollisionProfileName(TEXT("PhysicsActor"));
}

void AAleaStickman::BeginPlay()
{
	AAleaCharacter::BeginPlay();

	StickmanTable = TeamTable->FindRow<FAleaTeamRow>(Team, TEXT(""))->GetStickmanTable();

	MaxHealth = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetHealth();
	Health = MaxHealth;
}

void AAleaStickman::Tick(float DeltaSeconds)
{
	if (bActive)
	{
		AAleaCharacter::Tick(DeltaSeconds);

		bMoving = GetVelocity().Size() > 0.f;

		if (bMoving)
		{
			bAttacking = false;
		}

		Detect();

		if (TrgtAutoAttacked)
		{
			TargetOn();
		}
		else
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), DestLoc);
		}
	}
}

void AAleaStickman::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	AAleaCharacter::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAleaStickman, bMoving);
	DOREPLIFETIME(AAleaStickman, bAttacking);
}

float AAleaStickman::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	Health -= FinalDamage;

	if (Health <= 0.f)
	{
		Die(DamageCauser);
	}

	return FinalDamage;
}

void AAleaStickman::Die(AActor* DamageCauser)
{
	bDead = true;
	MulticastDie();

	if (AAleaAleo* Enemy = Cast<AAleaAleo>(DamageCauser))
	{
		Enemy->ServerAddExp(StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetExperience());
		Enemy->ServerAddChips(StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetChips());
	}
	else
	{
		TArray<FOverlapResult> OutOverlaps;
		const ECollisionChannel& AleaTrace = ECC_GameTraceChannel1;
		float ExperienceRange = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetExperienceRange();
		FCollisionQueryParams Params(NAME_None, false, this);

		if (GetWorld()->OverlapMultiByChannel(OutOverlaps, GetActorLocation(), FQuat::Identity, AleaTrace, FCollisionShape::MakeSphere(ExperienceRange), Params))
		{
			for (FOverlapResult OutOverlap : OutOverlaps)
			{
				if (AAleaAleo* Target = Cast<AAleaAleo>(OutOverlap.Actor))
				{
					if (!Target->IsDead() && Team != Target->Team)
					{
						Target->ServerAddExp(StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetExperience());
					}
				}
			}
		}
	}

	Release();
}

void AAleaStickman::MulticastDie_Implementation()
{
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Die"));
	GetMesh()->SetCollisionProfileName(TEXT("Die"));
}

void AAleaStickman::FollowPath(class UNavigationPath* FollowPath)
{
	FVector ToPoint(FollowPath->PathPoints[1] - GetActorLocation());
	ToPoint.Z = 0.f;

	SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToPoint.Rotation(), GetWorld()->GetDeltaSeconds(), 640.f));
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), FollowPath->PathPoints[1]);

	if (!bMoving && FollowPath->PathPoints.Num() > 2)
	{
		FVector ToNextPoint(FollowPath->PathPoints[2] - GetActorLocation());
		ToNextPoint.Z = 0.f;

		SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToNextPoint.Rotation(), GetWorld()->GetDeltaSeconds(), 640.f));
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), FollowPath->PathPoints[2]);
	}
}

void AAleaStickman::Detect()
{
	TArray<FOverlapResult> OutOverlaps;
	const ECollisionChannel& AleaTrace = ECC_GameTraceChannel1;
	float DetectRange = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetDetectRange();
	FCollisionQueryParams Params(NAME_None, false, this);

	if (GetWorld()->OverlapMultiByChannel(OutOverlaps, GetActorLocation(), FQuat::Identity, AleaTrace, FCollisionShape::MakeSphere(DetectRange), Params))
	{
		TArray<AAleaCharacter*> Enemies, EnemiesTargetedAleo, EnemiesTargetedStickman;

		for (FOverlapResult OutOverlap : OutOverlaps)
		{
			if (AAleaCharacter* Target = Cast<AAleaCharacter>(OutOverlap.Actor))
			{
				if (Team != Target->Team)
				{
					Enemies.Add(Target);
				}
			}
		}

		if (!Enemies.Num())
		{
			TrgtAutoAttacked = nullptr;
			return;
		}

		for (AAleaCharacter* Enemy : Enemies)
		{
			if (Cast<AAleaAleo>(Enemy->GetTarget()))
			{
				EnemiesTargetedAleo.Add(Enemy);
			}
			else if (Cast<AAleaStickman>(Enemy->GetTarget()))
			{
				EnemiesTargetedStickman.Add(Enemy);
			}
		}

		TArray<AAleaCharacter*> Aleos, Stickmen, Pawns;

		if (EnemiesTargetedAleo.Num())
		{
			for (AAleaCharacter* EnemyTargetedAleo : EnemiesTargetedAleo)
			{
				if (AAleaAleo* Aleo = Cast<AAleaAleo>(EnemyTargetedAleo))
				{
					Aleos.Add(Aleo);
				}
				else if (AAleaStickman* Stickman = Cast<AAleaStickman>(EnemyTargetedAleo))
				{
					Stickmen.Add(Stickman);
				}
			}

			if (Aleos.Num())
			{
				FindNearestEnemy(Aleos);
			}
			else if (Stickmen.Num())
			{
				FindNearestEnemy(Stickmen);
			}
		}
		else if (EnemiesTargetedStickman.Num())
		{
			for (AAleaCharacter* EnemyTargetedStickman : EnemiesTargetedStickman)
			{
				if (AAleaAleo* Aleo = Cast<AAleaAleo>(EnemyTargetedStickman))
				{
					Aleos.Add(Aleo);
				}
				else if (AAleaStickman* Stickman = Cast<AAleaStickman>(EnemyTargetedStickman))
				{
					Stickmen.Add(Stickman);
				}
				else if (AAleaPawn* Pawn = Cast<AAleaPawn>(EnemyTargetedStickman))
				{
					Pawns.Add(Pawn);
				}
			}

			if (Stickmen.Num())
			{
				FindNearestEnemy(Stickmen);
			}
			else if (Pawns.Num())
			{
				FindNearestEnemy(Pawns);
			}
			else if (Aleos.Num())
			{
				FindNearestEnemy(Aleos);
			}
		}
		else
		{
			for (AAleaCharacter* Enemy : Enemies)
			{
				if (AAleaAleo* Aleo = Cast<AAleaAleo>(Enemy))
				{
					Aleos.Add(Aleo);
				}
				else if (AAleaStickman* Stickman = Cast<AAleaStickman>(Enemy))
				{
					Stickmen.Add(Stickman);
				}
			}

			if (Stickmen.Num())
			{
				FindNearestEnemy(Stickmen);
			}
			else if (Aleos.Num())
			{
				FindNearestEnemy(Aleos);
			}
		}
	}
	else
	{
		TrgtAutoAttacked = nullptr;
	}
}

void AAleaStickman::FindNearestEnemy(const TArray<AAleaCharacter*>& Enemies)
{
	float ShortestRange = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetDetectRange();

	for (AAleaCharacter* Enemy : Enemies)
	{
		if (GetDistanceTo(Enemy) <= ShortestRange)
		{
			ShortestRange = GetDistanceTo(Enemy);
			TrgtAutoAttacked = Enemy;
		}
	}
}

void AAleaStickman::TargetOn()
{
	TArray<FHitResult> OutHits;
	const ECollisionChannel& AleaTrace = ECC_GameTraceChannel1;
	FCollisionQueryParams Params(NAME_None, false, this);
	FVector ToEnemy;

	if (GetWorld()->LineTraceMultiByChannel(OutHits, GetActorLocation(), TrgtAutoAttacked->GetActorLocation(), AleaTrace, Params))
	{
		for (const FHitResult& OutHit : OutHits)
		{
			if (TrgtAutoAttacked == Cast<AAleaCharacter>(OutHit.Actor))
			{
				ToEnemy = OutHit.ImpactPoint;
				break;
			}
		}
	}

	ToEnemy -= GetActorLocation();
	ToEnemy.Z = 0.f;

	float AttackRange = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetAttackRange();

	if (ToEnemy.Size() <= AttackRange)
	{
		Attack();
	}
	else
	{
		if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), TrgtAutoAttacked))
		{
			if (NavPath->PathPoints.Num() > 1)
			{
				FollowPath(NavPath);
			}
		}
	}
}

void AAleaStickman::Attack()
{
	bAttacking = true;

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), GetActorLocation());

	FVector ToEnemy(TrgtAutoAttacked->GetActorLocation() - GetActorLocation());
	ToEnemy.Z = 0.f;

	SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToEnemy.Rotation(), GetWorld()->GetDeltaSeconds(), 640.f));

	if (AutoAttackCoolDownTime <= 0.f)
	{
		bool bMeleeAttack = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->IsMeleeAttack();
		float AttackDamage = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetAttackDamage();

		if (bMeleeAttack)
		{
			UGameplayStatics::ApplyDamage(TrgtAutoAttacked, AttackDamage, GetController(), this, nullptr);
		}
		else
		{
			UParticleSystem* AttackEffect = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetAttackEffect();
			UParticleSystem* HitEffect = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetHitEffect();

			MulticastAttack(AttackEffect, nullptr, HitEffect, this, TrgtAutoAttacked, AttackDamage);
		}

		AutoAttackCoolDownTime = StickmanTable->FindRow<FAleaStickmanRow>(Game, TEXT(""))->GetCoolDownTime();
	}
}
