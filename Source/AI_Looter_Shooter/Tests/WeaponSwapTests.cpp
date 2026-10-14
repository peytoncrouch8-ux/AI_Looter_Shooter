#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "Player/WeaponSwapMotion.h"
#include "Tests/PlayerAnimTestKit.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "TwoBoneIK.h"

// Weapon swaps (Player/WeaponSwapMotion.h, Inventory/WeaponManagerSwap.cpp). The user (2026-10-14): "I need weapon swapping
// animations when we are going from weapon 1 in the equipped slot to weapon 2". Borderlands' way: the gun in hand drops and
// tips away out of view, the guns change places out of sight, the next swings up past its hold a touch and settles; the
// six-gun quicker with a flick, long guns heavier; the body lowers and raises its arm in third person.

namespace WeaponSwapTest
{
	const EWeaponKind Kinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver };

	FString KindName(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Revolver ? TEXT("Revolver") : Kind == EWeaponKind::Shotgun ? TEXT("Shotgun") : TEXT("Rifle");
	}

	float Degrees(const FQuat& A, const FQuat& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(A.AngularDistance(B)));
	}

	UWeaponDefinition* LoadGun(const TCHAR* Path)
	{
		return LoadObject<UWeaponDefinition>(nullptr, Path);
	}

	FWeaponInstanceData Roll(UWeaponDefinition* Definition, EWeaponRarity Rarity = EWeaponRarity::Common)
	{
		return UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 1);
	}

	// --- The body (as Tests/PlayerCarryTests.cpp poses it: the mannequin's reference pose, a gun shouldered in the right hand) ---

	/** The mannequin's front and its right in its component space (it faces +Y; its right is -X), and the facing's yaw there. */
	const FVector BodyAhead(0.0, 1.0, 0.0);
	const FVector BodyRight(-1.0, 0.0, 0.0);
	constexpr float BodyFacingYaw = 90.f;
	/** The left palm off the hand_l bone; the shouldered right wrist from the right shoulder; a bullpup's foregrip from its grip. */
	const FVector BodyPalmFromWrist(7.5, -2.5, 0.0);
	const FVector BodyShoulderedWrist(14.0, 16.0, -18.0);
	const FVector BodyBullpupForegrip(21.5, 0.0, 5.5);
	const FVector BodySixGunForegrip(0.7, -3.2, -0.8);

	FTransform BodyBone(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Name)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Rig.Bones);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	/** The reference pose with the right wrist brought up to Wrist in front of the chest, as the armed upper body holds a gun. */
	FCompactPose BodyRightHandAt(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Rest, const FVector& Wrist)
	{
		const auto Index = [&Rig](const TCHAR* Name)
		{
			FBoneReference Bone(Name);
			Bone.Initialize(Rig.Bones);
			return Bone.GetCompactPoseIndex(Rig.Bones);
		};
		const FCompactPoseBoneIndex UpperArm = Index(TEXT("upperarm_r"));
		const FCompactPoseBoneIndex LowerArm = Index(TEXT("lowerarm_r"));
		const FCompactPoseBoneIndex Hand = Index(TEXT("hand_r"));
		FCompactPose Posed = Rest;
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			return Posed;
		}
		FCSPose<FCompactPose> Space;
		Space.InitPose(Rest);
		FTransform Shoulder = Space.GetComponentSpaceTransform(UpperArm);
		FTransform Elbow = Space.GetComponentSpaceTransform(LowerArm);
		FTransform Grip = Space.GetComponentSpaceTransform(Hand);
		const FQuat HandRotation = Grip.GetRotation();
		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Grip, Elbow.GetLocation() + BodyRight * 10.0 - FVector(0.0, 0.0, 15.0), Wrist,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Grip.SetRotation(HandRotation);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Grip) };
		Space.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Space), Posed);
		return Posed;
	}

	/** A gun in the right hand of Shouldered as the anim instance hands it over: grip 6 cm ahead of the wrist, 2 under, facing ahead. */
	FLooterStanceInput BodyGunIn(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, const FVector& Foregrip, float HoldReach)
	{
		const FTransform Hand = BodyBone(Rig, Shouldered, TEXT("hand_r"));
		FLooterStanceInput Held;
		Held.bAimWithTorso = true;
		Held.bHandOnForegrip = true;
		Held.HoldBone = TEXT("hand_r");
		Held.HoldSocketLocal = FTransform(Hand.GetLocation() + BodyAhead * 6.0 - FVector(0.0, 0.0, 2.0)).GetRelativeTransform(Hand);
		Held.WeaponForegrip = Foregrip;
		Held.HoldReach = HoldReach;
		Held.AimRotation = FRotator(0.f, BodyFacingYaw, 0.f).Quaternion();
		Held.CarryRotation = Held.AimRotation;
		Held.WeaponRotation = Held.AimRotation;
		return Held;
	}

	struct FBodyPosed
	{
		FCompactPose Pose;
		FQuat Gun = FQuat::Identity;
		FTransform HandR;
		FTransform HandL;
		FVector Grip = FVector::ZeroVector;
		FVector ShoulderR = FVector::ZeroVector;

		FQuat InRightHand() const { return HandR.GetRotation().Inverse() * Gun; }
		FVector Palm() const { return HandL.TransformPosition(BodyPalmFromWrist); }
		FVector OnGun(const FVector& Spot) const { return Grip + Gun.RotateVector(Spot); }
	};

	/** Held lowered by a swap's share, the gun turned as the view turns it, posed on Shouldered. */
	FBodyPosed BodyPoseOf(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, FLooterStanceInput Held, float Swap, bool bShortGun)
	{
		FLooterCarry Carry;
		Carry.bShortGun = bShortGun;
		Carry.Swap = Swap;
		Held.Carry = Carry;
		Held.CarryRotation = LooterCarry::CarriedGun(Held.AimRotation, BodyFacingYaw, Carry);
		Held.WeaponRotation = Held.CarryRotation;
		FBodyPosed Posed;
		Posed.Pose = Shouldered;
		LooterStancePose::Apply(Posed.Pose, Held);
		Posed.Gun = Held.WeaponRotation;
		Posed.HandR = BodyBone(Rig, Posed.Pose, TEXT("hand_r"));
		Posed.HandL = BodyBone(Rig, Posed.Pose, TEXT("hand_l"));
		Posed.Grip = (Held.HoldSocketLocal * Posed.HandR).GetLocation();
		Posed.ShoulderR = BodyBone(Rig, Posed.Pose, TEXT("upperarm_r")).GetLocation();
		return Posed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSwapCurvesTest, "Looter.Weapons.Swap.Curves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponSwapCurvesTest::RunTest(const FString& Parameters)
{
	// Each kind's path: a draw from out of view swings past its hold a touch and settles, never in a jump; down is out of
	// view and muzzle-down; the pose has no kink at the hold; the six-gun flicks as it arrives.
	for (const EWeaponKind Kind : WeaponSwapTest::Kinds)
	{
		const FString Name = WeaponSwapTest::KindName(Kind);
		const FWeaponSwapStyle& Style = WeaponSwapMotion::StyleOf(Kind);
		TestNearlyEqual(*(Name + TEXT(": a draw starts out of view")), WeaponSwapMotion::DrawLowered(Kind, 0.f), 1.f, 1.e-4f);
		TestNearlyEqual(*(Name + TEXT(": ...and ends on its hold")), WeaponSwapMotion::DrawLowered(Kind, 1.f), 0.f, 1.e-4f);
		float Lowest = 1.f;
		float Step = 0.f;
		float Last = 1.f;
		for (int32 Sample = 1; Sample <= 240; ++Sample)
		{
			const float Lowered = WeaponSwapMotion::DrawLowered(Kind, Sample / 240.f);
			Lowest = FMath::Min(Lowest, Lowered);
			Step = FMath::Max(Step, FMath::Abs(Lowered - Last));
			Last = Lowered;
		}
		TestTrue(FString::Printf(TEXT("%s: past its hold a touch (%.3f), then back"), *Name, Lowest),
			Lowest < -0.03f && Lowest >= -Style.Overshoot - 1.e-4f && Lowest >= -0.15f);
		TestTrue(FString::Printf(TEXT("%s: never a jump (%.3f of the way in a 240th at most)"), *Name, Step), Step <= 0.05f);

		FVector Offset;
		FRotator Turn;
		WeaponSwapMotion::ViewPose(Kind, 1.f, Offset, Turn);
		TestTrue(FString::Printf(TEXT("%s: down is out of view, muzzle down (%.0f cm, %.0f degrees)"), *Name, Offset.Z, Turn.Pitch),
			Offset.Z <= -30.f && Turn.Pitch <= -35.f);
		WeaponSwapMotion::ViewPose(Kind, 0.f, Offset, Turn);
		TestTrue(*(Name + TEXT(": up is the hold itself")), Offset.IsNearlyZero(1.e-3) && Turn.IsNearlyZero(1.e-3f));

		// A hundredth of the way at a time, from past the hold to out of view: no step moves or turns it much.
		FVector LastOffset;
		FRotator LastTurn;
		WeaponSwapMotion::ViewPose(Kind, -0.2f, LastOffset, LastTurn);
		float WorstMove = 0.f;
		float WorstTurn = 0.f;
		for (int32 Hundredth = -19; Hundredth <= 100; ++Hundredth)
		{
			WeaponSwapMotion::ViewPose(Kind, Hundredth / 100.f, Offset, Turn);
			WorstMove = FMath::Max(WorstMove, static_cast<float>(FVector::Dist(Offset, LastOffset)));
			WorstTurn = FMath::Max(WorstTurn, WeaponSwapTest::Degrees(Turn.Quaternion(), LastTurn.Quaternion()));
			LastOffset = Offset;
			LastTurn = Turn;
		}
		TestTrue(FString::Printf(TEXT("%s: smooth through the hold (%.2f cm, %.2f degrees a hundredth at most)"), *Name, WorstMove, WorstTurn),
			WorstMove <= 1.5f && WorstTurn <= 3.5f);
	}
	FVector Offset;
	FRotator SixGun;
	FRotator LongGun;
	WeaponSwapMotion::ViewPose(EWeaponKind::Revolver, -0.1f, Offset, SixGun);
	WeaponSwapMotion::ViewPose(EWeaponKind::Rifle, -0.1f, Offset, LongGun);
	TestTrue(FString::Printf(TEXT("The six-gun flicks past its hold (a %.1f degree cant; the rifle's %.1f)"), SixGun.Roll, LongGun.Roll),
		FMath::Abs(SixGun.Roll) >= FMath::Abs(LongGun.Roll) + 3.f);

	// The holster answers the key at once and gathers pace out of view; from part way it carries on from there.
	TestNearlyEqual(TEXT("A holster sets off from the hold"), WeaponSwapMotion::HolsterLowered(0.f, 0.f), 0.f, 1.e-5f);
	TestTrue(TEXT("...moving at once"), WeaponSwapMotion::HolsterLowered(0.f, 0.1f) >= 0.03f);
	TestNearlyEqual(TEXT("...and ends out of view"), WeaponSwapMotion::HolsterLowered(0.f, 1.f), 1.f, 1.e-5f);
	TestNearlyEqual(TEXT("From part way, from there"), WeaponSwapMotion::HolsterLowered(0.6f, 0.f), 0.6f, 1.e-5f);

	// How long: each kind's holster about a fifth of a second at its usual handling, the six-gun's the quickest, the shotgun's
	// the slowest; better handling quicker, within bounds. The draw is the gun's own ready time (its first shot waits for it).
	const float Rifle = WeaponSwapMotion::HolsterSeconds(EWeaponKind::Rifle, 1.f);
	const float Shotgun = WeaponSwapMotion::HolsterSeconds(EWeaponKind::Shotgun, 1.f);
	const float Revolver = WeaponSwapMotion::HolsterSeconds(EWeaponKind::Revolver, 1.3f);
	TestTrue(FString::Printf(TEXT("Holsters %.3f, %.3f and %.3f s (rifle, shotgun, six-gun)"), Rifle, Shotgun, Revolver),
		Revolver < Rifle && Rifle < Shotgun && Revolver >= 0.17f && Shotgun <= 0.25f);
	TestTrue(TEXT("Better handling is quicker"), WeaponSwapMotion::HolsterSeconds(EWeaponKind::Rifle, 1.2f) < Rifle);
	TestNearlyEqual(TEXT("...within bounds"), WeaponSwapMotion::HolsterSeconds(EWeaponKind::Rifle, 10.f), WeaponSwapMotion::MinHolsterSeconds, 1.e-5f);
	TestNearlyEqual(TEXT("...both ways"), WeaponSwapMotion::HolsterSeconds(EWeaponKind::Rifle, 0.1f), WeaponSwapMotion::MaxHolsterSeconds, 1.e-5f);
	const float LongDraw = AWeaponBase::BaseReadySeconds;
	const float SixGunDraw = AWeaponBase::BaseReadySeconds / 1.3f;
	TestTrue(FString::Printf(TEXT("Draws: %.2f s for a long gun, %.2f s for the six-gun"), LongDraw, SixGunDraw),
		SixGunDraw < LongDraw && SixGunDraw >= 0.2f && LongDraw >= 0.3f && LongDraw <= 0.45f);
	TestTrue(FString::Printf(TEXT("A rifle to a shotgun, away and up: %.2f s"), Rifle + LongDraw), Rifle + LongDraw >= 0.45f && Rifle + LongDraw <= 0.75f);

	TestEqual(TEXT("The body's share: none past the hold"), WeaponSwapMotion::BodyShare(-0.1f), 0.f);
	TestEqual(TEXT("...all of it out of view"), WeaponSwapMotion::BodyShare(1.f), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSwapClockTest, "Looter.Weapons.Swap.Clock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponSwapClockTest::RunTest(const FString& Parameters)
{
	const float Whole = 0.2f;
	FWeaponSwapClock Clock;
	TestTrue(TEXT("Idle: nothing runs, nothing lowered"), !Clock.IsRunning() && Clock.GetLowered(0.0) == 0.f);

	// A whole holster from the hold.
	Clock.Lower(10.0, 0.f, Whole);
	TestTrue(TEXT("Going down"), Clock.IsLowering() && Clock.GetLowered(10.0) == 0.f);
	TestNearlyEqual(TEXT("...a whole holster to go"), Clock.SecondsLeft(10.0), Whole, 1.e-4f);
	const float Half = Clock.GetLowered(10.1);
	TestTrue(FString::Printf(TEXT("...part way half way through (%.2f)"), Half), Half > 0.2f && Half < 0.6f);
	TestTrue(TEXT("...not done before its time"), !Clock.IsDone(10.19));
	TestTrue(TEXT("...out of view on time"), Clock.IsDone(10.2) && FMath::IsNearlyEqual(Clock.GetLowered(10.2), 1.f, 1.e-4f));

	// Mid-draw it carries on from where the gun is, for only the rest of the way; a gun just drawn is out of view already.
	Clock.Lower(20.0, 0.6f, Whole);
	TestNearlyEqual(TEXT("From part way through a draw: from there"), Clock.GetLowered(20.0), 0.6f, 1.e-4f);
	TestNearlyEqual(TEXT("...for the rest of the way's share"), Clock.SecondsLeft(20.0), Whole * 0.4f, 1.e-4f);
	Clock.Lower(30.0, 1.f, Whole);
	TestTrue(TEXT("Just drawn: down already"), Clock.IsDone(30.0));

	// Called off: back up from where it is, in its share of a holster, settling on the hold.
	Clock.Lower(40.0, 0.f, Whole);
	const float Mid = Clock.GetLowered(40.1);
	Clock.Raise(40.1);
	TestTrue(TEXT("Called off: coming back up"), Clock.IsRaising());
	TestNearlyEqual(TEXT("...from where it was"), Clock.GetLowered(40.1), Mid, 1.e-4f);
	TestNearlyEqual(TEXT("...in its share of a holster"), Clock.SecondsLeft(40.1), Whole * FMath::Max(Mid, WeaponSwapMotion::MinRaiseShare), 1.e-4f);
	const double Up = 40.1 + Clock.SecondsLeft(40.1);
	TestTrue(TEXT("...and up on time"), Clock.IsDone(Up) && FMath::IsNearlyZero(Clock.GetLowered(Up), 1.e-4f));

	// A 240th of a second at a time through down, called off, down again, out of view: never a jump.
	Clock.Lower(50.0, 0.f, Whole);
	float Last = 0.f;
	float Worst = 0.f;
	for (int32 Frame = 1; Frame <= 240; ++Frame)
	{
		const double Now = 50.0 + Frame / 240.0;
		if (Frame == 24)
		{
			Clock.Raise(Now);
		}
		else if (Frame == 40)
		{
			Clock.Lower(Now, Clock.GetLowered(Now), Whole);
		}
		const float Lowered = Clock.GetLowered(Now);
		Worst = FMath::Max(Worst, FMath::Abs(Lowered - Last));
		Last = Lowered;
	}
	TestTrue(FString::Printf(TEXT("Down, back, down again: smooth (%.3f a 240th at most)"), Worst), Worst <= 0.05f);
	TestTrue(TEXT("...and out of view in the end"), Clock.IsLowering() && FMath::IsNearlyEqual(Last, 1.f, 1.e-4f));
	Clock.Reset();
	TestFalse(TEXT("Reset: nothing runs"), Clock.IsRunning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSwapManagerTest, "Looter.Weapons.Swap.Manager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponSwapManagerTest::RunTest(const FString& Parameters)
{
	// A real weapon manager with three real guns, its world's clock set by hand (a test world never ticks).
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = WeaponSwapTest::LoadGun(TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	UWeaponDefinition* Shotgun = WeaponSwapTest::LoadGun(TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	UWeaponDefinition* Revolver = WeaponSwapTest::LoadGun(TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver"));
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview))
		|| !TestTrue(TEXT("The three guns load"), Rifle && Shotgun && Revolver))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	APawn* Holder = World->SpawnActor<APawn>();
	UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder);
	Weapons->RegisterComponent();
	Weapons->MaxWeapons = 3;
	Weapons->BackpackCapacity = 4;
	FWeaponInstanceData RifleRoll = WeaponSwapTest::Roll(Rifle);
	RifleRoll.SavedMagazine = 5;
	AWeaponBase* RifleGun = Weapons->GiveWeapon(RifleRoll);
	AWeaponBase* ShotgunGun = Weapons->GiveWeapon(WeaponSwapTest::Roll(Shotgun));
	AWeaponBase* RevolverGun = Weapons->GiveWeapon(WeaponSwapTest::Roll(Revolver));
	if (!TestTrue(TEXT("Three guns, the rifle in hand"), RifleGun && ShotgunGun && RevolverGun && Weapons->GetActiveWeapon() == RifleGun))
	{
		return false;
	}
	Weapons->AddAmmo(RifleGun->GetAmmoType(), 90);
	World->TimeSeconds = 10.0;

	// Where no one sees it (an editor world, no player), guns change at once, as before.
	Weapons->EquipSlot(1);
	TestTrue(TEXT("Unseen: the change at once"), Weapons->GetActiveWeapon() == ShotgunGun && !Weapons->IsSwapping());
	Weapons->EquipSlot(0);
	World->TimeSeconds = 20.0;

	// In view: the rifle goes down first, locked, its reload cut; the shotgun waits.
	Weapons->bSwapMotionAnywhere = true;
	RifleGun->Reload();
	const int32 Rounds = RifleGun->GetCurrentMagazine();
	TestTrue(TEXT("Reloading"), RifleGun->IsReloading());
	Weapons->EquipSlot(1);
	TestTrue(TEXT("A swap: the rifle still in hand, weapon 2 coming"), Weapons->IsSwapping() && Weapons->GetActiveWeapon() == RifleGun
		&& Weapons->GetSwapTargetSlot() == 1);
	TestTrue(TEXT("...its reload cut, the gun locked"), !RifleGun->IsReloading() && RifleGun->IsMeleeSwinging());
	Weapons->StartFire();
	Weapons->StopFire();
	Weapons->Reload();
	TestTrue(TEXT("...no shot and no reload on its way down"), RifleGun->GetCurrentMagazine() == Rounds && !RifleGun->IsReloading());

	// Part way down, another turn of the wheel only changes what comes up.
	World->TimeSeconds = 20.05;
	const float Part = Weapons->GetGunLowered();
	TestTrue(FString::Printf(TEXT("Part way down (%.2f)"), Part), Part > 0.05f && Part < 0.7f);
	Weapons->NextWeapon();
	TestEqual(TEXT("The wheel counts on from the gun coming: weapon 3"), Weapons->GetSwapTargetSlot(), 2);
	TestNearlyEqual(TEXT("...and the holster carries on, never starting over"), Weapons->GetGunLowered(), Part, 1.e-4f);
	Weapons->EquipSlot(2);
	TestNearlyEqual(TEXT("Its key again changes nothing"), Weapons->GetGunLowered(), Part, 1.e-4f);

	// The rifle's own key calls it off: back up from where it got to, then free again, its reload started over.
	Weapons->EquipSlot(0);
	TestTrue(TEXT("Called off: coming back up"), Weapons->IsSwapping() && Weapons->GetSwapTargetSlot() == INDEX_NONE);
	TestNearlyEqual(TEXT("...from where it was"), Weapons->GetGunLowered(), Part, 1.e-3f);
	World->TimeSeconds = 21.0;
	Weapons->UpdateSwap();
	TestTrue(TEXT("Back up: the rifle in hand at its hold"), !Weapons->IsSwapping() && Weapons->GetActiveWeapon() == RifleGun
		&& FMath::IsNearlyZero(Weapons->GetGunLowered(), 1.e-4f));
	TestTrue(TEXT("...free, its reload started over"), !RifleGun->IsMeleeSwinging() && RifleGun->IsReloading());

	// Through to the change: the rifle put away and let go, the revolver drawn from out of view.
	Weapons->EquipSlot(2);
	TestFalse(TEXT("The reload is cut again"), RifleGun->IsReloading());
	World->TimeSeconds = 21.0 + 0.5 * WeaponSwapMotion::HolsterSeconds(EWeaponKind::Rifle, RifleGun->GetStats().Handling);
	Weapons->UpdateSwap();
	TestTrue(TEXT("Half way down: still the rifle"), Weapons->GetActiveWeapon() == RifleGun && Weapons->IsSwapping());
	World->TimeSeconds = 22.0;
	Weapons->UpdateSwap();
	TestTrue(TEXT("Down: the change"), Weapons->GetActiveWeapon() == RevolverGun && !Weapons->IsSwapping());
	TestTrue(TEXT("...the rifle put away and let go"), RifleGun->IsHidden() && !RifleGun->IsMeleeSwinging());
	TestNearlyEqual(TEXT("...the revolver drawn from out of view"), Weapons->GetGunLowered(), 1.f, 1.e-3f);

	// Still below the view (just drawn), a gun has nothing to put away: the change is at once.
	Weapons->EquipSlot(1);
	TestTrue(TEXT("Just drawn: the change at once"), Weapons->GetActiveWeapon() == ShotgunGun && !Weapons->IsSwapping());

	// Dropped on its way down: the gun the swap was for comes up.
	World->TimeSeconds = 23.0;
	Weapons->EquipSlot(0);
	TestTrue(TEXT("Going down for the rifle"), Weapons->IsSwapping());
	TestTrue(TEXT("Dropped mid-swap"), Weapons->DropActiveWeapon() == ShotgunGun);
	TestTrue(TEXT("...the rifle it was for in hand"), Weapons->GetActiveWeapon() == RifleGun && !Weapons->IsSwapping());

	// Loot taken in hand with every slot full: the gun in hand goes down first and they change places out of sight.
	TestNotNull(TEXT("A third gun again"), Weapons->GiveWeapon(WeaponSwapTest::Roll(Shotgun)));
	World->TimeSeconds = 24.0;
	AWeaponBase* Loot = UWeaponRollLibrary::SpawnWeapon(World, WeaponSwapTest::Roll(Shotgun, EWeaponRarity::Rare), FTransform::Identity);
	if (!TestNotNull(TEXT("Loot"), Loot))
	{
		return false;
	}
	Loot->OnDropped();
	TestTrue(TEXT("Taken in hand"), Weapons->EquipPickup(Loot));
	TestTrue(TEXT("...the rifle going down first, the loot waiting"), Weapons->IsSwapping() && Weapons->GetActiveWeapon() == RifleGun && Loot->IsPickup());
	const float Going = Weapons->GetGunLowered();
	TestTrue(TEXT("Held again on its way: nothing new"), Weapons->EquipPickup(Loot) && FMath::IsNearlyEqual(Weapons->GetGunLowered(), Going, 1.e-4f));
	World->TimeSeconds = 25.0;
	Weapons->UpdateSwap();
	TestTrue(TEXT("Down: the loot in hand, in the rifle's slot"), Weapons->GetActiveWeapon() == Loot && Weapons->GetActiveSlot() == 0 && !Loot->IsPickup());
	TestTrue(TEXT("...the rifle in the backpack"), Weapons->GetBackpack().Num() == 1 && Weapons->GetBackpack()[0].Definition == Rifle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSwapBodyTest, "Looter.Weapons.Swap.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponSwapBodyTest::RunTest(const FString& Parameters)
{
	// Third person: a swap lowers the gun to the side in the right hand, muzzle to the ground, the left hand let go; the
	// guns change places there and the next comes up the same way. On the mannequin's reference pose, bone by bone.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const FCompactPose Shouldered = WeaponSwapTest::BodyRightHandAt(Rig, Rest, Rig.Where(Rest, TEXT("upperarm_r")) + WeaponSwapTest::BodyShoulderedWrist);

	struct FBodyCase
	{
		const TCHAR* Name;
		FVector Foregrip;
		float HoldReach;
		bool bShortGun;
	};
	const FBodyCase Cases[] = {
		{ TEXT("Bullpup"), WeaponSwapTest::BodyBullpupForegrip, 0.f, false },
		{ TEXT("Six-gun"), WeaponSwapTest::BodySixGunForegrip, 14.f, true },
	};
	for (const FBodyCase& Case : Cases)
	{
		const FLooterStanceInput Gun = WeaponSwapTest::BodyGunIn(Rig, Shouldered, Case.Foregrip, Case.HoldReach);
		const WeaponSwapTest::FBodyPosed Held = WeaponSwapTest::BodyPoseOf(Rig, Shouldered, Gun, 0.f, Case.bShortGun);
		const WeaponSwapTest::FBodyPosed Down = WeaponSwapTest::BodyPoseOf(Rig, Shouldered, Gun, 1.f, Case.bShortGun);
		TestTrue(FString::Printf(TEXT("%s: lowered, sound"), Case.Name), PlayerAnimTestKit::FPoseRig::IsSound(Down.Pose));
		const FVector Side = Down.ShoulderR + WeaponSwapTest::BodyAhead * LooterCarry::SideAhead + WeaponSwapTest::BodyRight * LooterCarry::SideOut
			- FVector(0.0, 0.0, LooterCarry::SideDown);
		TestTrue(FString::Printf(TEXT("%s: the right hand low at the side (%.1f cm from the spot, %.1f under the shoulder)"), Case.Name,
			FVector::Dist(Down.HandR.GetLocation(), Side), Down.ShoulderR.Z - Down.HandR.GetLocation().Z),
			FVector::Dist(Down.HandR.GetLocation(), Side) <= 2.0 && Down.ShoulderR.Z - Down.HandR.GetLocation().Z >= 40.0);
		TestTrue(FString::Printf(TEXT("%s: the gun turns with the hand (%.2f degrees in the palm)"), Case.Name,
			WeaponSwapTest::Degrees(Held.InRightHand(), Down.InRightHand())), WeaponSwapTest::Degrees(Held.InRightHand(), Down.InRightHand()) <= 1.f);
		TestTrue(FString::Printf(TEXT("%s: the muzzle at the ground (%.2f down)"), Case.Name, -Down.Gun.GetForwardVector().Z),
			Down.Gun.GetForwardVector().Z <= -0.8);
		TestTrue(FString::Printf(TEXT("%s: the left hand let go (%.1f cm off the gun)"), Case.Name, FVector::Dist(Down.Palm(), Down.OnGun(Case.Foregrip))),
			FVector::Dist(Down.Palm(), Down.OnGun(Case.Foregrip)) >= 15.0);

		// Down and up a twentieth at a time: every pose sound, the hand never jumping.
		FVector LastWrist = Held.HandR.GetLocation();
		float WorstStep = 0.f;
		bool bSound = true;
		for (int32 Step = 1; Step <= 20; ++Step)
		{
			const WeaponSwapTest::FBodyPosed Frame = WeaponSwapTest::BodyPoseOf(Rig, Shouldered, Gun, Step / 20.f, Case.bShortGun);
			bSound &= PlayerAnimTestKit::FPoseRig::IsSound(Frame.Pose);
			WorstStep = FMath::Max(WorstStep, static_cast<float>(FVector::Dist(Frame.HandR.GetLocation(), LastWrist)));
			LastWrist = Frame.HandR.GetLocation();
		}
		TestTrue(FString::Printf(TEXT("%s: lowering, every pose sound"), Case.Name), bSound);
		TestTrue(FString::Printf(TEXT("%s: ...the hand moving smoothly (%.1f cm a twentieth at most)"), Case.Name, WorstStep), WorstStep <= 6.f);
	}
	return true;
}

#endif
