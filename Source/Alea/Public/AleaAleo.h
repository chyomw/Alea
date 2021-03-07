#pragma once

#include "CoreMinimal.h"
#include "AleaCharacter.h"
#include "AleaAleo.generated.h"

UCLASS()
class ALEA_API AAleaAleo : public AAleaCharacter
{
	GENERATED_BODY()
	
public:
	AAleaAleo();

	FORCEINLINE bool DoesMove() const noexcept { return bMoving; }

	UFUNCTION(Server, Reliable)
	void ServerAddExp(float Exp);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerAddChips(int Chips);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerUseSkillPoints();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerConsumeMana(float ManaCost);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerTakeDamage(float FinalDamage, AActor* DamageCauser);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLive();

	void ReqMove();
	void AckMove();

	UFUNCTION(Server, Reliable)
	void ServerRotateTo(const FRotator& Target);

	UFUNCTION(Server, Unreliable)
	void ServerMoveTo(const FVector& Goal);

	void FollowPath(class UNavigationPath* FollowPath);

	void ReqAutoAttack();
	void Detect();
	void AckAutoAttack();

	UFUNCTION(Server, Reliable)
	void ServerAttack(AAleaCharacter* Target, bool bMeleeAttack, float AttackDamage);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastMeleeAttack(UParticleSystem* AttackEffect, USoundCue* AttackSound, UParticleSystem* HitEffect, const FVector& HitPoint);

	UFUNCTION(Server, Reliable)
	void ServerSetTarget(AAleaCharacter* Target);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLevelUp();

	void ToggleCameraLock();
	void CheckMousePosition(bool bXorY, bool& bLorT, bool& bRorB);
	void MoveCamera(bool bForR, bool bPorM);

	UPROPERTY(Replicated)
	uint8 bInitialize : 1;

	UPROPERTY(Replicated)
	uint8 bMoving : 1;

	uint8 bPositioning : 1;

	uint8 bAutoAttacking : 1;

	uint8 bLockingCamera : 1;

	FVector DestLoc;

	class USpringArmComponent* SpringArm;
	class UCameraComponent* LockedCamera;
	UCameraComponent* UnlockedCamera;
	class UParticleSystemComponent* Effect;
	class UAudioComponent* Audio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class UWidgetComponent* ResourceBar;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	class UDecalComponent* CursorHitPoint;

	class UDataTable* AleoTable;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UDataTable* StatTable;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UDataTable* SkillTable;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	FName Color;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 Level = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 AvailableSkillPoints = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float MaxExperience;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float Experience;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float HealthRegen;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float MaxMana;

	UPROPERTY(Replicated, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	float Mana;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float ManaRegen;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float RespawnWaitTime;

	UPROPERTY(Replicated, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	int HoldingChips = 500;
};
