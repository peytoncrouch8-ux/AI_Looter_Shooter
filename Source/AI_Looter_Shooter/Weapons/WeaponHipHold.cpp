#include "Weapons/WeaponHipHold.h"
#include "HAL/IConsoleManager.h"

// The long guns' hip holds. At the view model's 90 degrees (16:9: the frame's bottom edge is 0.5625 cm down per cm
// ahead) the old hold put the grip at 50, 20, -26, half under the bottom edge with the trigger hand below the frame, and
// the Bullpup's magazine (behind and under its grip) was worked out of sight. Raised and brought in, the trigger hand sits
// about a fifth of the frame above the bottom edge and the support hand about a third, with the guns at their drawn
// size (UWeaponModelComponent::ScaleFor).

namespace
{
	struct FHipHold
	{
		EWeaponKind Kind;
		FVector Grip;
		FRotator Turn;
	};

	// Tuned in game (2026-10-14, the user's Borderlands framing, the hands at 0.88): the first raise put the Bullpup's butt
	// in the middle of the view and stretched the forearms; 6 ahead, 7 right and 7 down from it sits each gun low on the
	// right with the support glove on its fore-end and both hands in view.
	const FHipHold HipHolds[] = {
		// The Bullpup: its grip low on the right, the handguard rising toward the middle, the muzzle 8 degrees down.
		{ EWeaponKind::Rifle, FVector(50.f, 23.f, -23.5f), FRotator(-8.f, -4.f, -5.f) },
		// The Ranchhand: its long forend reaches further in; its grip is on the stock behind the receiver.
		{ EWeaponKind::Shotgun, FVector(50.f, 22.5f, -23.f), FRotator(-7.f, -3.f, -5.f) },
	};

#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<float> CVarHipAhead(TEXT("Looter.HipHold.Ahead"), 0.f, TEXT("Moves every first-person hip hold ahead (cm; tuning only)."));
	TAutoConsoleVariable<float> CVarHipRight(TEXT("Looter.HipHold.Right"), 0.f, TEXT("Moves every first-person hip hold right (cm; tuning only)."));
	TAutoConsoleVariable<float> CVarHipUp(TEXT("Looter.HipHold.Up"), 0.f, TEXT("Moves every first-person hip hold up (cm; tuning only)."));
#endif
}

bool WeaponHipHold::For(EWeaponKind Kind, FVector& OutGrip, FRotator& OutTurn)
{
	for (const FHipHold& Hold : HipHolds)
	{
		if (Hold.Kind == Kind)
		{
			OutGrip = Hold.Grip;
			OutTurn = Hold.Turn;
			return true;
		}
	}
	return false;
}

FVector WeaponHipHold::TuningNudge()
{
#if !UE_BUILD_SHIPPING
	return FVector(CVarHipAhead.GetValueOnGameThread(), CVarHipRight.GetValueOnGameThread(), CVarHipUp.GetValueOnGameThread());
#else
	return FVector::ZeroVector;
#endif
}
