#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Player/FirstPersonHandsFraming.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The first-person framing's plain rules (FPHandsFraming): points onto the frame, the view model's push-out for a
// narrower field of view, the forearms' ways (elbows out of the frame, slanted where they can be, never worse than the
// grip tables'), the kinds' table, the parked hand; then on the real character with all seven guns: elbows out of the
// frame at the hip, the support glove in it (the revolver's parked under it).

namespace
{
	const FPHandsFraming::FFrameView FramingView = FPHandsFraming::FFrameView::FromFieldOfView(90.f, 16.f / 9.f);

	FPHandsFraming::FForearmRules FramingRules()
	{
		FPHandsFraming::FForearmRules Rules;
		Rules.ScreenAngle = 60.f;
		Rules.Back = 0.35f;
		Rules.ElbowMargin = 0.12f;
		Rules.MaxSwing = 45.f;
		Rules.Share = 1.f;
		return Rules;
	}

	/** How far outside the frame a point is (0 on the edge; behind the eye: -1). */
	float FramingOut(const FVector& Point)
	{
		FVector2D Screen;
		if (!FPHandsFraming::Project(Point, FramingView, Screen))
		{
			return -1.f;
		}
		return static_cast<float>(FMath::Max(FMath::Abs(Screen.X), FMath::Abs(Screen.Y))) - 1.f;
	}

	float FramingAngle(const FVector& A, const FVector& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Acos(FMath::Clamp(FVector::DotProduct(A.GetSafeNormal(), B.GetSafeNormal()), -1.0, 1.0))));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPHandsProjectTest, "Looter.FPHands.Project", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPHandsProjectTest::RunTest(const FString& Parameters)
{
	FVector2D Screen;
	TestTrue(TEXT("A point straight ahead is on the frame"), FPHandsFraming::Project(FVector(100.f, 0.f, 0.f), FramingView, Screen));
	TestTrue(TEXT("...in its middle"), Screen.Equals(FVector2D::ZeroVector, 1.e-4));
	FPHandsFraming::Project(FVector(100.f, 100.f, 0.f), FramingView, Screen);
	TestNearlyEqual(TEXT("45 degrees right at 90 across: the right edge"), static_cast<float>(Screen.X), 1.f, 1.e-3f);
	FPHandsFraming::Project(FVector(100.f, 0.f, -56.25f), FramingView, Screen);
	TestNearlyEqual(TEXT("...and 9/16 of that down: the bottom edge"), static_cast<float>(Screen.Y), -1.f, 1.e-3f);
	TestFalse(TEXT("A point behind the eye is nowhere on the frame"), FPHandsFraming::Project(FVector(-5.f, 0.f, 0.f), FramingView, Screen));
	TestTrue(TEXT("Under the frame by a fifth: off it by 0.12"), FPHandsFraming::IsOffFrame(FVector(100.f, 0.f, -56.25f * 1.2f), FramingView, 0.12f));
	TestFalse(TEXT("Inside it: not off it"), FPHandsFraming::IsOffFrame(FVector(100.f, 20.f, -20.f), FramingView, 0.f));

	// What of a forearm shows: all of one inside the frame, none of one outside it.
	const float Inside = FPHandsFraming::VisibleForearm(FVector(100.f, 0.f, 0.f), FVector(0.f, 0.f, -1.f), 28.125f, FramingView);
	TestNearlyEqual(TEXT("A forearm from the middle to halfway down covers a quarter of the frame's height"), Inside, 0.25f, 0.01f);
	TestNearlyEqual(TEXT("One under the frame covers none of it"),
		FPHandsFraming::VisibleForearm(FVector(100.f, 0.f, -80.f), FVector(0.f, 0.f, -1.f), 20.f, FramingView), 0.f, 1.e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPHandsViewDepthTest, "Looter.FPHands.ViewDepth", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPHandsViewDepthTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual(TEXT("The same field of view: no push-out"), FPHandsFraming::DepthScaleFor(90.f, 90.f), 1.f, 1.e-4f);
	const float Push = FPHandsFraming::DepthScaleFor(90.f, 70.f);
	TestTrue(TEXT("Narrower: the holds go out"), Push > 1.f);
	// A point of the gun pushed out along the view lands on the same spot of a frame that much narrower.
	const FPHandsFraming::FFrameView Narrow = FPHandsFraming::FFrameView::FromFieldOfView(70.f, 16.f / 9.f);
	const FVector Points[] = { FVector(50.f, 23.f, -23.5f), FVector(90.f, 15.f, -12.f), FVector(28.f, 24.f, -20.f) };
	for (const FVector& Point : Points)
	{
		FVector2D Before;
		FVector2D After;
		FPHandsFraming::Project(Point, FramingView, Before);
		FPHandsFraming::Project(FVector(Point.X * Push, Point.Y, Point.Z), Narrow, After);
		TestTrue(FString::Printf(TEXT("(%.0f, %.0f, %.0f) keeps its spot on screen"), Point.X, Point.Y, Point.Z), Before.Equals(After, 1.e-3));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPHandsForearmTest, "Looter.FPHands.Forearm", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPHandsForearmTest::RunTest(const FString& Parameters)
{
	const FPHandsFraming::FForearmRules Rules = FramingRules();
	constexpr float Length = 28.7f;
	struct FCase
	{
		const TCHAR* Name;
		EFirstPersonHand Hand;
		FVector Wrist;
		FVector Table;
	};
	// Support wrists low and high in the frame (a pump close in, a rifle's fore-end out), trigger wrists low on the right;
	// the tables' forearms as FirstPersonHandsGrips has them, roughly, in camera space.
	const FCase Cases[] = {
		{ TEXT("support, low"), EFirstPersonHand::Left, FVector(55.f, 6.f, -24.f), FVector(-0.3f, -0.3f, -0.9f) },
		{ TEXT("support, mid"), EFirstPersonHand::Left, FVector(70.f, 10.f, -17.f), FVector(-0.3f, -0.26f, -0.92f) },
		{ TEXT("support, high"), EFirstPersonHand::Left, FVector(80.f, 8.f, -8.f), FVector(-0.3f, -0.26f, -0.92f) },
		{ TEXT("trigger, low"), EFirstPersonHand::Right, FVector(45.f, 22.f, -26.f), FVector(-0.45f, 0.35f, -0.82f) },
	};
	for (const FCase& Case : Cases)
	{
		const FVector Way = FPHandsFraming::FrameForearm(Case.Hand, Case.Wrist, Case.Table, Length, FramingView, Rules);
		const FVector Elbow = Case.Wrist + Way * Length;
		const float TableOut = FramingOut(Case.Wrist + Case.Table.GetSafeNormal() * Length);
		const float Out = FramingOut(Elbow);
		TestTrue(FString::Printf(TEXT("%s: a way"), Case.Name), Way.IsNormalized());
		TestTrue(FString::Printf(TEXT("%s: never swung past %.0f degrees off the table's (%.1f)"), Case.Name, Rules.MaxSwing, FramingAngle(Way, Case.Table)),
			FramingAngle(Way, Case.Table) <= Rules.MaxSwing + 0.5f);
		TestTrue(FString::Printf(TEXT("%s: never ahead of the wrist (%.2f)"), Case.Name, Way.X), Way.X <= 0.05);
		TestTrue(FString::Printf(TEXT("%s: the elbow out of the frame where the table's was, or further (%.2f, the table's %.2f)"), Case.Name, Out, TableOut),
			Out >= FMath::Min(TableOut, Rules.ElbowMargin) - 1.e-3f);
		TestTrue(FString::Printf(TEXT("%s: the elbow never at the lens (%.1f cm ahead)"), Case.Name, Elbow.X), Elbow.X >= 6.f || Out == TableOut);
		FVector2D WristOn;
		FVector2D ElbowOn;
		if (FPHandsFraming::Project(Case.Wrist, FramingView, WristOn) && FPHandsFraming::Project(Elbow, FramingView, ElbowOn))
		{
			const bool bOutward = Case.Hand == EFirstPersonHand::Left ? ElbowOn.X <= WristOn.X + 0.02 : ElbowOn.X >= WristOn.X - 0.02;
			TestTrue(FString::Printf(TEXT("%s: the forearm runs down and out toward its own side"), Case.Name), ElbowOn.Y < WristOn.Y && bOutward);
		}
	}
	// A low support wrist has room: the elbow goes out by the margin on a slant, not straight down.
	{
		const FVector Wrist(55.f, 6.f, -24.f);
		const FVector Way = FPHandsFraming::FrameForearm(EFirstPersonHand::Left, Wrist, FVector(-0.3f, -0.3f, -0.9f), Length, FramingView, Rules);
		TestTrue(TEXT("Low support: the elbow out by the margin"), FPHandsFraming::IsOffFrame(Wrist + Way * Length, FramingView, Rules.ElbowMargin));
		TestTrue(TEXT("...on a slant: some of the way across, not a post"), Way.Y < -0.15);
	}
	// Share 0: the table's way, untouched.
	FPHandsFraming::FForearmRules Off = Rules;
	Off.Share = 0.f;
	const FVector Table = FVector(-0.3f, -0.26f, -0.92f).GetSafeNormal();
	TestTrue(TEXT("Share 0 keeps the table's way"), FPHandsFraming::FrameForearm(EFirstPersonHand::Left, FVector(70.f, 10.f, -17.f), Table, Length,
		FramingView, Off).Equals(Table, 1.e-4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPHandsKindsTest, "Looter.FPHands.Kinds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPHandsKindsTest::RunTest(const FString& Parameters)
{
	const EWeaponKind Kinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper,
		EWeaponKind::LMG, EWeaponKind::Launcher };
	for (const EWeaponKind Kind : Kinds)
	{
		const FPHandsFraming::FKindFrame KindFrame = FPHandsFraming::Defaults(Kind);
		const FString Name = UEnum::GetValueAsString(Kind);
		// Nudges, not new holds: the tables (WeaponHipHold, FirstPersonHandsGrips) stay the holds.
		TestTrue(Name + TEXT(": the hip nudge is under 15 cm (the rifle comes 12 nearer, the launcher goes 9 further out)"),
			FVector(KindFrame.HipAhead, KindFrame.HipRight, KindFrame.HipUp).GetAbsMax() <= 15.f);
		// Never muzzle-down (the user, 2026-10-14: "the AR is tilted forward"); pulled-in guns tip up a little instead.
		TestTrue(Name + TEXT(": the muzzle never tips down"), KindFrame.HipPitch >= 0.f);
		TestTrue(Name + TEXT(": ...and a few degrees"), FVector(KindFrame.HipPitch, KindFrame.HipYaw, KindFrame.HipRoll).GetAbsMax() <= 10.f);
		TestTrue(Name + TEXT(": the support hand slides along the gun, never off it"),
			FMath::Abs(KindFrame.SupportAhead) <= 8.f && FMath::Abs(KindFrame.SupportRight) <= 2.f && FMath::Abs(KindFrame.SupportUp) <= 2.f);
		TestTrue(Name + TEXT(": one-handed is a share"), KindFrame.OneHand >= 0.f && KindFrame.OneHand <= 1.f);
		// Borderlands holds a pistol in one hand, every long gun in two.
		TestTrue(Name + TEXT(": one-handed only for the pistol"), (KindFrame.OneHand >= 1.f) == (Kind == EWeaponKind::Revolver));
	}
	// The Bullpup's fore-end has no room behind its foregrip (the grip loop's front is 6 cm back): its support hand stays.
	TestNearlyEqual(TEXT("The rifle's support hand doesn't slide back"), FPHandsFraming::Defaults(EWeaponKind::Rifle).SupportAhead, 0.f, 1.e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPHandsParkTest, "Looter.FPHands.Park", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPHandsParkTest::RunTest(const FString& Parameters)
{
	const FHandPlacement Parked = FPHandsFraming::ParkedSupport();
	// Out of view at the view model's angle and at the narrowest the framing allows (40: the most it can be pushed out).
	TestTrue(TEXT("The parked support hand is under the frame at 90 degrees"), FPHandsFraming::IsOffFrame(Parked.Hold, FramingView, 0.2f));
	TestTrue(TEXT("...and at 40"), FPHandsFraming::IsOffFrame(Parked.Hold, FPHandsFraming::FFrameView::FromFieldOfView(40.f, 16.f / 9.f), 0.2f));
	TestTrue(TEXT("...on the left"), Parked.Hold.Y < 0.0);
	TestTrue(TEXT("...its forearm going down out of the frame"), Parked.Forearm.Z < -0.5);
	return true;
}

// ---------------------------------------------------------------------------
// On the real character
// ---------------------------------------------------------------------------

using namespace LocomotionTestWorld;

namespace
{
	struct FFramingPlayer
	{
		UPlayerLocomotionComponent* Locomotion = nullptr;
		UPlayerViewComponent* View = nullptr;
		UFirstPersonHandsComponent* Hands = nullptr;
		AWeaponBase* Gun = nullptr;
	};

	FFramingPlayer FramingSpawn(UWorld* World, const FVector& Feet, UWeaponDefinition* Definition)
	{
		FFramingPlayer Player;
		Player.Locomotion = SpawnPlayer(World, Feet);
		ACharacter* Body = Player.Locomotion ? BodyOf(Player.Locomotion) : nullptr;
		if (!Body)
		{
			return Player;
		}
		Player.View = Body->FindComponentByClass<UPlayerViewComponent>();
		Player.Hands = Body->FindComponentByClass<UFirstPersonHandsComponent>();
		UWeaponManagerComponent* Weapons = Body->FindComponentByClass<UWeaponManagerComponent>();
		if (Weapons && Definition)
		{
			Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1));
			Player.Gun = Weapons->GetActiveWeapon();
		}
		return Player;
	}

	void FramingFrame(const FFramingPlayer& Player)
	{
		PlayFrame(Player.Locomotion);
		static_cast<UActorComponent*>(Player.View)->TickComponent(Frame, LEVELTICK_All, nullptr);
		Player.Hands->UpdateHands(Frame);
	}

	/** A bone of the arms in the camera's space (posed this frame). */
	FVector FramingBone(const FFramingPlayer& Player, const TCHAR* Bone, const UCameraComponent& Camera)
	{
		const int32 Index = Player.Hands->GetBoneIndex(FName(Bone));
		const FVector World = Index != INDEX_NONE ? Player.Hands->GetBoneTransform(Index).GetLocation() : FVector::ZeroVector;
		return Camera.GetComponentTransform().InverseTransformPosition(World);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPHandsPlayOnScreenTest, "Looter.FPHands.Play.OnScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPHandsPlayOnScreenTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	if (!TestNotNull(TEXT("A floor"), SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(20000.0, 8000.0, 100.0))))
	{
		return false;
	}
	const TCHAR* const Paths[] = {
		TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"), TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"),
		TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver"), TEXT("/Game/Weapons/Data/DA_SMG.DA_SMG"), TEXT("/Game/Weapons/Data/DA_Sniper.DA_Sniper"),
		TEXT("/Game/Weapons/Data/DA_LMG.DA_LMG"), TEXT("/Game/Weapons/Data/DA_Launcher.DA_Launcher") };
	FFramingPlayer Players[UE_ARRAY_COUNT(Paths)];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Paths); ++Index)
	{
		UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, Paths[Index]);
		Players[Index] = FramingSpawn(World, FVector(-6000.0 + 2000.0 * Index, 0.0, 0.0), Definition);
		if (!TestTrue(FString::Printf(TEXT("%s: the player, hands and gun"), Paths[Index]),
			Definition && Players[Index].Locomotion && Players[Index].View && Players[Index].Hands && Players[Index].Gun))
		{
			return false;
		}
	}
	World->TimeSeconds += 10.0;

	for (const FFramingPlayer& Player : Players)
	{
		// Long enough for a one-handed gun's support hand to have left it.
		for (int32 Frames = 0; Frames < 30; ++Frames)
		{
			FramingFrame(Player);
		}
		const UCameraComponent* Camera = CameraOf(Player.Locomotion);
		const EWeaponKind Kind = Player.Gun->GetInstance().Definition->Kind;
		const FString Name = UEnum::GetValueAsString(Kind);
		if (!TestTrue(Name + TEXT(": the hands show"), Camera && Player.Hands->IsShowingHands()))
		{
			continue;
		}
		const FPHandsFraming::FFrameView View = FPHandsFraming::FFrameView::FromFieldOfView(
			Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView, 16.f / 9.f);
		const TCHAR* const Sides[2][2] = { { TEXT("lowerarm_l"), TEXT("hand_l") }, { TEXT("lowerarm_r"), TEXT("hand_r") } };
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FVector Elbow = FramingBone(Player, Sides[Side][0], *Camera);
			const FVector Wrist = FramingBone(Player, Sides[Side][1], *Camera);
			const float Shows = FPHandsFraming::VisibleForearm(Wrist, Elbow - Wrist, static_cast<float>(FVector::Dist(Wrist, Elbow)), View);
			AddInfo(FString::Printf(TEXT("%s, %s forearm: %.0f%% of the frame's height shows"), *Name, Side == 0 ? TEXT("support") : TEXT("trigger"), Shows * 100.f));
			// The sleeve's capped cut end (the elbow) never in view.
			TestTrue(FString::Printf(TEXT("%s: the %s elbow is out of the frame"), *Name, Side == 0 ? TEXT("support") : TEXT("trigger")),
				FPHandsFraming::IsOffFrame(Elbow, View, 0.f));
		}
		const FVector Support = Camera->GetComponentTransform().InverseTransformPosition(Player.Hands->GetHoldPoint(EFirstPersonHand::Left));
		if (FPHandsFraming::OneHandShare(Kind) >= 1.f)
		{
			TestTrue(Name + TEXT(": one-handed, the support hand is parked under the frame"), FPHandsFraming::IsOffFrame(Support, View, 0.1f));
		}
		else
		{
			FVector2D Screen;
			TestTrue(Name + TEXT(": the support glove is in the frame"), FPHandsFraming::Project(Support, View, Screen)
				&& FMath::Abs(Screen.X) <= 0.9 && FMath::Abs(Screen.Y) <= 0.95);
		}
	}
	return true;
}

#endif
