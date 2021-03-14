#include "AleaAleoAnim.h"
#include "AleaAleo.h"

void UAleaAleoAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	Aleo = Cast<AAleaAleo>(TryGetPawnOwner());

	if (Aleo)
	{
		bDead = Aleo->IsDead();
		bMoving = Aleo->DoesMove();
	}
}
