#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AleaAleoAnim.generated.h"

UCLASS()
class ALEA_API UAleaAleoAnim : public UAnimInstance
{
	GENERATED_BODY()
	
protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY()
	class AAleaAleo* Aleo;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 bDead : 1;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 bMoving : 1;
};
