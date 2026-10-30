#include "Player/FirstPersonHandsMotion.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsMarks.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"

// The support hand's reload spots: where the hand is on a magazine, the charging handle, the loading port, the revolver's
// cylinder, ejector rod, speedloader and batting spot, and the belt pouches it reaches for. The paths between them (timing,
// easing) are in FirstPersonHandsReload.cpp.

namespace
{
	// The revolver's cylinder (Drover.py, the modeled gun's cm; the gun is drawn FHandsGun::Scale bigger): half its length,
	// the rod's tip ahead of its middle, its radius; where it sits shut, for a gun whose model can't say. And the rod's
	// stroke, the speedloader's room behind the cylinder and its push.
	constexpr float CylinderHalfLength = 2.2f;
	constexpr float RodAhead = 6.75f;
	constexpr float CylinderRadius = 2.15f;
	const FVector ShutCylinderRear(2.1f, 0.f, -1.2f);
	constexpr float RodClear = 2.f;
	constexpr float RodPunch = 3.5f;
	constexpr float LoaderBehind = 5.f;
	constexpr float LoaderPush = 2.f;

	/** Where the support hand's forearm leaves the wrist working on the gun (gun space): back, out to the left and down. */
	const FVector WorkForearm(-0.55f, -0.5f, -0.65f);
	/** ...reaching up beside the gun to its top (the charging handle), or to the front of the rod: from below. */
	const FVector UpForearm(-0.15f, -0.5f, -0.85f);

	void CylinderOf(const FHandsGun& Gun, FVector& OutRear, FVector& OutAxis)
	{
		OutRear = Gun.bHasCylinder ? Gun.CylinderRear : ShutCylinderRear * Gun.Scale;
		OutAxis = Gun.bHasCylinder ? Gun.CylinderAxis.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector) : FVector::ForwardVector;
	}

	/** A flat palm with its middle at PalmMiddle (gun space), facing Palm, fingers toward Fingers (the hand's sizes). */
	FHandPlacement PalmOn(const FHandsGun& Gun, const FVector& PalmMiddle, const FVector& Fingers, const FVector& Palm, EHandGrip Grip,
		const FVector& Forearm)
	{
		const FVector Hold = PalmMiddle + Palm.GetSafeNormal() * FirstPersonHands::GripOffPalm + Fingers.GetSafeNormal() * FirstPersonHands::GripPastPalm;
		return FirstPersonHandsGrips::InGun(Gun, Hold, Fingers, Palm, FHandFingers::Of(Grip), Forearm);
	}
}

FVector FirstPersonHandsMotion::MagazinePouch()
{
	// Low on the left, under the view's bottom edge (the view model's frustum ends about 0.56 down per unit ahead).
	return FVector(22.f, -14.f, -46.f);
}

FVector FirstPersonHandsMotion::ShellBelt()
{
	return FVector(24.f, 9.f, -46.f);
}

FHandPlacement FirstPersonHandsMotion::OnMagazine(const FHandsGun& Gun, float Travel)
{
	// Round the magazine from its left, the fingers over its front edge, wherever its travel (the modeled gun's cm, as
	// LooterReload gives it) has it: on the spot the parts mark for it (Reload_L, at the magazine's rest) when they do.
	FHandPlacement Marked;
	if (FirstPersonHandsMarks::Place(Gun, EHandMarkRole::Reload, EFirstPersonHand::Left, Marked, FVector(0.f, 0.f, -Travel)))
	{
		return Marked;
	}
	const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
	const FVector Seat = Gun.bHasMagazineSeat ? Gun.MagazineSeat : FVector(14.f, 0.f, -5.6f) * Gun.Scale;
	const FVector Hold = Seat + (Hands.MagazineGrab - FVector(0.f, 0.f, Travel)) * Gun.Scale;
	return FirstPersonHandsGrips::InGun(Gun, Hold, FVector::ForwardVector, FVector::RightVector, FHandFingers::Of(EHandGrip::Wrap), WorkForearm);
}

FHandPlacement FirstPersonHandsMotion::OnHandle(const FHandsGun& Gun, float Yank)
{
	// Thumb and fingers pinched on the handle from its left and above (the palm against its outer face), pulled straight
	// back along the gun.
	const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
	constexpr float HandlePull = 4.f;
	const FVector Hold = (Hands.Handle + FVector(-HandlePull * Yank, 0.f, 0.4f)) * Gun.Scale + FVector(0.f, 0.5f, 0.f);
	return FirstPersonHandsGrips::InGun(Gun, Hold, FVector(0.2f, 0.f, 1.f), FVector(0.f, 1.f, -0.3f), FHandFingers::Of(EHandGrip::Pinch), UpForearm);
}

FHandPlacement FirstPersonHandsMotion::AtLoadingPort(const FHandsGun& Gun, float Push)
{
	// Palm up under the receiver, the shell between thumb and fingers, thumbed up into the port.
	const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
	constexpr float ShellBelow = 2.f;
	constexpr float ShellPush = 2.f;
	const FVector Hold = (Hands.LoadingPort + FVector(0.f, 0.f, -ShellBelow + ShellPush * FMath::Clamp(Push, 0.f, 1.f))) * Gun.Scale;
	return FirstPersonHandsGrips::InGun(Gun, Hold, FVector(1.f, 0.15f, 0.f), FVector(0.f, 0.25f, 1.f), FHandFingers::Of(EHandGrip::Pinch), WorkForearm);
}

FHandPlacement FirstPersonHandsMotion::OnCylinder(const FHandsGun& Gun)
{
	// Cradled from its left and under it (it swings out to the gun's left), fingers along it, the thumb on its face.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Middle = Rear + Axis * (CylinderHalfLength * Gun.Scale);
	const FVector Palm = Middle + FVector(0.f, -0.6f, -0.8f) * (CylinderRadius * Gun.Scale);
	return PalmOn(Gun, Palm, Axis, FVector(0.f, 0.6f, 0.8f), EHandGrip::Wrap, WorkForearm);
}

FHandPlacement FirstPersonHandsMotion::OnEjectorRod(const FHandsGun& Gun, float Punch)
{
	// The flat of the palm on the rod's tip, fingers up, driving it back along the cylinder.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Tip = Rear + Axis * ((CylinderHalfLength + RodAhead) * Gun.Scale);
	const FVector Palm = Tip + Axis * ((RodClear - (RodClear + RodPunch * 0.5f) * FMath::Clamp(Punch, 0.f, 1.f)) * Gun.Scale);
	return PalmOn(Gun, Palm, FVector::UpVector, -Axis, EHandGrip::Open, UpForearm);
}

FHandPlacement FirstPersonHandsMotion::WithSpeedloader(const FHandsGun& Gun, float Push)
{
	// Fingertips round the loader behind the open chambers, palm toward them, pushing it in along the cylinder.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Palm = Rear - Axis * ((LoaderBehind - LoaderPush * FMath::Clamp(Push, 0.f, 1.f)) * Gun.Scale);
	return PalmOn(Gun, Palm, FVector::UpVector, Axis, EHandGrip::Wrap, WorkForearm);
}

FHandPlacement FirstPersonHandsMotion::BattingCylinder(const FHandsGun& Gun)
{
	// The flat of the hand against the cylinder's left side, riding it as it's batted home into the frame.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Middle = Rear + Axis * (CylinderHalfLength * Gun.Scale);
	const FVector Palm = Middle - FVector(0.f, (CylinderRadius + 0.3f) * Gun.Scale, 0.f);
	return PalmOn(Gun, Palm, FVector(1.f, 0.f, 0.25f), FVector::RightVector, EHandGrip::Open, WorkForearm);
}
