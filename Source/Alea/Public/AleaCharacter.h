#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AleaCharacter.generated.h"

UCLASS()
class ALEA_API AAleaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAleaCharacter();

	FORCEINLINE bool IsDead() const noexcept { return bDead; }
	FORCEINLINE AAleaCharacter* GetTarget() const noexcept { return TrgtAutoAttacked; }

	UPROPERTY(Replicated, BlueprintReadWrite)
	FName Team;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttack(UParticleSystem* AttackEffect, class USoundCue* AttackSound, UParticleSystem* Hit, class AAleaCharacter* Spwnr, AAleaCharacter* Trgt, float Damage);

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 bDead : 1;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float MaxHealth;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float Health;

	UPROPERTY(Replicated)
	AAleaCharacter* TrgtAutoAttacked;

	float AutoAttackCoolDownTime;

	class AAleaActorPool* ActorPool;
};
