// AChest's opening as it moves: what comes before the lid (the Strongbox's wheel, a coffin pried at, a grave dug), the lid
// swung about its hinge or shoved off, and the poses shut and open.

#include "Loot/Chest.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace
{
	/** A dig's moments (shares of it): dirt off the spade at each of its three bites, and a heave as the coffin comes up. */
	constexpr float DirtAt[] = { 0.08f, 0.33f, 0.58f, 0.86f };
	constexpr float DirtStrength[] = { 0.45f, 0.55f, 0.65f, 1.f };
	/** Where each is thrown from (the grave's frame, cm): the spade's side, the far heap, the near one, the middle. */
	const FVector DirtSpot[] = { FVector(60.0, 40.0, 8.0), FVector(-10.0, -50.0, 10.0), FVector(20.0, 50.0, 10.0), FVector(0.0, 0.0, 12.0) };
	constexpr int32 DirtMoments = UE_ARRAY_COUNT(DirtAt);

	/** The dig: the spade bites three times through this share of it, then goes into the heap. */
	constexpr float BitesShare = 0.75f;
	/** A bite: the blade driven this far down along the shaft (cm), and levered back this much (degrees). */
	constexpr float BiteDepth = 12.f;
	constexpr float BiteLever = 18.f;
	/** The mound flattens to this share of its height as it sinks. */
	constexpr float CoverFlatten = 0.25f;

	/** A pry: two jumps of the lid on its nails (cm up, degrees of tilt), and how proud of the box it's left. */
	constexpr float FirstJump = 2.5f;
	constexpr float SecondJump = 4.f;
	constexpr float FirstTilt = 2.f;
	constexpr float SecondTilt = -3.f;
	constexpr float PriedProud = 1.f;
}

void AChest::Advance(float DeltaSeconds)
{
	const FChestKindInfo Info = GetKindInfo();
	float Left = DeltaSeconds;
	if (State == EChestState::Unlocking)
	{
		Clock += Left;
		Left = 0.f;
		const float Pre = GetPreSeconds();
		if (Pre > 0.f && Clock < Pre)
		{
			SetPreShare(Clock / Pre);
		}
		else
		{
			// Done: what's left of the frame goes to the lid.
			SetPreShare(1.f);
			Left = FMath::Max(Clock - Pre, 0.f);
			Clock = 0.f;
			State = EChestState::Opening;
			if (Info.OpenCue)
			{
				LooterSound::PlayAt(this, Info.OpenCue, Lid ? Lid->GetComponentLocation() : GetActorLocation());
			}
		}
	}
	if (State == EChestState::Opening)
	{
		Clock += Left;
		const float Share = FMath::Clamp(Clock / GetLidSecondsFor(), 0.f, 1.f);
		SetLidShare(Share);
		if (Share >= LootShare)
		{
			DropLoot();
		}
		if (Share >= 1.f)
		{
			State = EChestState::Open;
		}
	}
	if (State == EChestState::Closed || State == EChestState::Open)
	{
		SetActorTickEnabled(false);
	}
}

// ---------------------------------------------------------------------------
// Before the lid
// ---------------------------------------------------------------------------

void AChest::SetPreShare(float Share)
{
	PreShare = FMath::Clamp(Share, 0.f, 1.f);
	const FChestKindInfo Info = GetKindInfo();
	switch (Info.PreMotion)
	{
	case EChestPreMotion::Wheel:
		SetWheelAngle(Info.WheelTurns * 360.f * FMath::SmoothStep(0.f, 1.f, PreShare));
		break;
	case EChestPreMotion::Pry:
	{
		// It jumps on its nails at one end, settles, jumps higher at the other, and is left a hair proud of the box.
		const bool bSecond = PreShare >= 0.5f;
		const float Jump = PreShare >= 1.f ? 0.f : FMath::Sin(UE_PI * FMath::Frac(PreShare * 2.f));
		const float Lift = (bSecond ? SecondJump : FirstJump) * Jump + PriedProud * PreShare;
		const float Tilt = (bSecond ? SecondTilt : FirstTilt) * Jump;
		if (Lid)
		{
			Lid->SetRelativeLocationAndRotation(LidRest + FVector(0.0, 0.0, Lift), FRotator(0.f, 0.f, Tilt).Quaternion());
		}
		break;
	}
	case EChestPreMotion::Dig:
	{
		const float Eased = FMath::SmoothStep(0.f, 1.f, PreShare);
		// The mound sinks and flattens into the ground...
		if (Cover)
		{
			Cover->SetRelativeLocation(FVector(0.0, 0.0, -Info.CoverSink * Eased));
			Cover->SetRelativeScale3D(FVector(1.0, 1.0, FMath::Max(1.f - (1.f - CoverFlatten) * Eased, 0.05f)));
			Cover->SetVisibility(PreShare < 1.f && Cover->GetStaticMesh() != nullptr);
		}
		// ...while the open grave heaves up out of it, slowing as it meets the surface (drawn from here on: shut, it's
		// hidden under the ground, where it would cost a draw for nothing).
		const float Rise = 1.f - FMath::Pow(1.f - PreShare, 2.2f);
		if (Body)
		{
			Body->SetRelativeLocation(FVector(0.0, 0.0, -Info.BodySink * (1.f - Rise)));
			Body->SetVisibility(true, /*bPropagateToChildren*/ false);
		}
		if (Lid)
		{
			Lid->SetVisibility(true, /*bPropagateToChildren*/ false);
		}
		// The spade bites three times (driven down its shaft and levered back), then is stuck in the far heap.
		if (Shovel)
		{
			const float Bites = FMath::Clamp(PreShare / BitesShare, 0.f, 1.f);
			const float Bite = Bites >= 1.f ? 0.f : FMath::Sin(UE_PI * FMath::Frac(Bites * 3.f));
			const FQuat Standing = Info.ShovelBefore.GetRotation();
			FTransform Pose(Standing * FQuat(FVector::RightVector, FMath::DegreesToRadians(-BiteLever * Bite)),
				Info.ShovelBefore.GetLocation() - Standing.GetUpVector() * BiteDepth * Bite);
			if (PreShare > BitesShare)
			{
				const float Away = FMath::SmoothStep(0.f, 1.f, (PreShare - BitesShare) / (1.f - BitesShare));
				Pose.SetLocation(FMath::Lerp(Pose.GetLocation(), Info.ShovelAfter.GetLocation(), static_cast<double>(Away)));
				Pose.SetRotation(FQuat::Slerp(Pose.GetRotation(), Info.ShovelAfter.GetRotation(), Away));
			}
			Shovel->SetRelativeTransform(Pose);
		}
		// Dirt off the spade at each bite, a heave as the coffin comes up (only while it's being dug, never on a restore).
		if (State == EChestState::Unlocking)
		{
			while (DirtThrown < DirtMoments && PreShare >= DirtAt[DirtThrown])
			{
				ThrowDirt(DirtStrength[DirtThrown]);
				++DirtThrown;
			}
		}
		break;
	}
	default:
		break;
	}
}

void AChest::ThrowDirt(float Strength) const
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FVector Spot = GetActorTransform().TransformPosition(DirtSpot[FMath::Clamp(DirtThrown, 0, DirtMoments - 1)]);
	Effects.SpawnDirt(Spot, GetActorUpVector(), Strength);
}

// ---------------------------------------------------------------------------
// The lid
// ---------------------------------------------------------------------------

void AChest::SetLidShare(float Share)
{
	LidShare = FMath::Clamp(Share, 0.f, 1.f);
	const FChestKindInfo Info = GetKindInfo();
	// Easing up off the box and settling at the end.
	const float Eased = FMath::SmoothStep(0.f, 1.f, LidShare);
	if (Info.LidMotion == EChestLidMotion::Hinge)
	{
		SetLidAngle(Info.OpenAngle * Eased);
		return;
	}
	// Shoved off: lifted clear of the box on the way, carried aside, turned to rest where it lands.
	LidAngle = Info.OpenAngle * Eased;
	if (Lid)
	{
		const FVector Offset = Info.SlideOffset * Eased + FVector(0.0, 0.0, Info.SlideLift * FMath::Sin(UE_PI * LidShare));
		Lid->SetRelativeLocationAndRotation(LidRest + Offset, FQuat::Slerp(FQuat::Identity, Info.SlideTurn.Quaternion(), Eased));
	}
}

void AChest::SetLidAngle(float Degrees)
{
	LidAngle = Degrees;
	// Pitched about the hinge: its front (+X) rises toward +Z and goes on over the back past upright (a negative angle
	// swings it down and out: a mailbox's door). A quaternion, so the turn past 90 degrees isn't folded into another rotator.
	if (Lid)
	{
		Lid->SetRelativeLocationAndRotation(LidRest, FRotator(Degrees, 0.f, 0.f).Quaternion());
	}
}

void AChest::SetWheelAngle(float Degrees)
{
	WheelAngle = Degrees;
	// About its front axis, turning the way a hand on its right spoke pulls it down (clockwise to the player in front).
	if (Wheel)
	{
		Wheel->SetRelativeRotation(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees)));
	}
}

// ---------------------------------------------------------------------------
// Shut and open
// ---------------------------------------------------------------------------

void AChest::PoseClosed()
{
	const FChestKindInfo Info = GetKindInfo();
	PreShare = 0.f;
	LidShare = 0.f;
	LidAngle = 0.f;
	SetWheelAngle(0.f);
	if (Lid)
	{
		Lid->SetRelativeLocationAndRotation(LidRest, FQuat::Identity);
	}
	// A grave: its open grave under the ground (not drawn there), its mound over it, its spade standing by.
	const bool bUnderground = Info.BodySink > 0.f;
	if (Body)
	{
		Body->SetRelativeLocation(FVector(0.0, 0.0, -Info.BodySink));
		Body->SetVisibility(!bUnderground, /*bPropagateToChildren*/ false);
	}
	if (Lid)
	{
		Lid->SetVisibility(!bUnderground, /*bPropagateToChildren*/ false);
	}
	if (Cover)
	{
		Cover->SetRelativeLocation(FVector::ZeroVector);
		Cover->SetRelativeScale3D(FVector::OneVector);
		Cover->SetVisibility(Cover->GetStaticMesh() != nullptr);
	}
	if (Shovel)
	{
		Shovel->SetRelativeTransform(Info.ShovelBefore);
		Shovel->SetVisibility(Shovel->GetStaticMesh() != nullptr);
	}
}

void AChest::PoseOpen()
{
	// Everything at its end: the wheel turned, the grave dug and its spade in the heap, the lid off.
	SetPreShare(1.f);
	SetLidShare(1.f);
	if (Body && GetKindInfo().PreMotion != EChestPreMotion::Dig)
	{
		Body->SetRelativeLocation(FVector::ZeroVector);
	}
}
