#pragma once

#include "CoreMinimal.h"
#include "AleaCharacter.h"
#include "AleaStickman.generated.h"

UCLASS()
class ALEA_API AAleaStickman : public AAleaCharacter
{
	GENERATED_BODY()

public:
	AAleaStickman();

	FORCEINLINE bool DoesMove() const noexcept { return bMoving; }
	FORCEINLINE bool DoesAttack() const noexcept { return bAttacking; }
	FORCEINLINE const FName& GetGame() const noexcept { return Game; }
	FORCEINLINE float GetMaxHealth() const noexcept { return MaxHealth; }
	
	void Release();
	void Activate();

	void SetPool(class AAleaStickmanPool* NewPool) { Pool = NewPool; }

	virtual void LifeSpanExpired() override { Release(); }
	
protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

private:
	void Die(AActor* DamageCauser);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

	void FollowPath(class UNavigationPath* FollowPath);

	void Detect();
	void FindNearestEnemy(const TArray<AAleaCharacter*>& Enemies);
	void TargetOn();
	void Attack();

	uint8 bActive : 1;

	UPROPERTY(Replicated)
	uint8 bMoving : 1;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 bAttacking : 1;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	FVector DestLoc;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	FName Game;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class UWidgetComponent* HealthBar;

	class UDataTable* TeamTable;
	UDataTable* StickmanTable;

	AAleaStickmanPool* Pool;
};
