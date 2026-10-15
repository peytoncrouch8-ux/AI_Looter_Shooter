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
		// The Bullpup: its grip low on the right, level as it always was (the user: the 8 degree muzzle-down tilt read as
		// tipped forward, 2026-10-14).
		{ EWeaponKind::Rifle, FVector(50.f, 23.f, -23.5f), FRotator::ZeroRotator },
		// The Ranchhand: its grip is on the stock behind the receiver. Level too (its 7 degree tilt read as the rifle's did).
		// 20 cm nearer than the rifle (the user, 2026-10-15: "too far forward making the arm look silly"): the stock leaves
		// the frame past the bottom right, the left arm's reach to the pump is short and its sleeve's end stays under the
		// frame. The trigger glove sits below the frame there, as a full stock hid it anyway.
		{ EWeaponKind::Shotgun, FVector(30.f, 22.5f, -23.f), FRotator(0.f, -3.f, -5.f) },
	};

#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<int32> CVarHipKind(TEXT("Looter.HipHold.Kind"), 0,
		TEXT("Which gun the Looter.HipHold.* nudges move: 0 every gun, 1 rifle, 2 shotgun, 3 revolver (tuning only)."));
	TAutoConsoleVariable<float> CVarHipAhead(TEXT("Looter.HipHold.Ahead"), 0.f, TEXT("Moves the first-person hip hold ahead (cm; tuning only)."));
	TAutoConsoleVariable<float> CVarHipRight(TEXT("Looter.HipHold.Right"), 0.f, TEXT("Moves the first-person hip hold right (cm; tuning only)."));
	TAutoConsoleVariable<float> CVarHipUp(TEXT("Looter.HipHold.Up"), 0.f, TEXT("Moves the first-person hip hold up (cm; tuning only)."));
	TAutoConsoleVariable<float> CVarHipPitch(TEXT("Looter.HipHold.Pitch"), 0.f, TEXT("Turns the first-person hip hold muzzle up (degrees; tuning only)."));
	TAutoConsoleVariable<float> CVarHipYaw(TEXT("Looter.HipHold.Yaw"), 0.f, TEXT("Turns the first-person hip hold muzzle right (degrees; tuning only)."));
	TAutoConsoleVariable<float> CVarHipRoll(TEXT("Looter.HipHold.Roll"), 0.f, TEXT("Rolls the first-person hip hold (degrees; tuning only)."));

	bool TunesKind(EWeaponKind Kind)
	{
		const int32 Wanted = CVarHipKind.GetValueOnGameThread();
		return Wanted == 0 || Wanted == static_cast<int32>(Kind);
	}
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

FVector WeaponHipHold::TuningNudge(EWeaponKind Kind)
{
#if !UE_BUILD_SHIPPING
	if (TunesKind(Kind))
	{
		return FVector(CVarHipAhead.GetValueOnGameThread(), CVarHipRight.GetValueOnGameThread(), CVarHipUp.GetValueOnGameThread());
	}
#endif
	return FVector::ZeroVector;
}

FRotator WeaponHipHold::TuningTurn(EWeaponKind Kind)
{
#if !UE_BUILD_SHIPPING
	if (TunesKind(Kind))
	{
		return FRotator(CVarHipPitch.GetValueOnGameThread(), CVarHipYaw.GetValueOnGameThread(), CVarHipRoll.GetValueOnGameThread());
	}
#endif
	return FRotator::ZeroRotator;
}
