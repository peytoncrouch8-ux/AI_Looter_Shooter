#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/FirstPersonHandsFraming.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsMarks.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Weapons/WeaponHandMarks.h"
#include "Weapons/WeaponModelComponent.h"
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"

// The hands on the grip spots a gun's parts mark (2026-10-29, grip spots: Docs/Work/2026-10-29-GripSpots/CONTRACT.md), as
// plain math on guns made here (no assets): the socket names read as markers, a marked hold's palm facing the part with its
// grip point the palm's skin off the marker at the arms' size, the trigger finger on a marked trigger, the pump and the
// magazine marks riding their parts' travel, and a gun (or a role) not marked holding by the kind's tables as before.

namespace
{
	/** A gun of Kind at a hip-like hold with its default parts' sockets (as the grip tests' guns), the arms at Ellis's 0.88. */
	FHandsGun MarksTestGun(EWeaponKind Kind)
	{
		FHandsGun Gun;
		Gun.ToCamera = FTransform(FRotator(4.f, -3.f, 6.f), FVector(30.f, 18.f, -22.f));
		Gun.Kind = Kind;
		Gun.TriggerReady = 1.f;
		Gun.Scale = UWeaponModelComponent::ScaleFor(Kind);
		Gun.HandScale = 0.88f;
		const bool bShotgun = Kind == EWeaponKind::Shotgun;
		Gun.Grip = (bShotgun ? FVector(-4.5f, 0.f, -6.f) : FVector(22.5f, 0.f, -10.f)) * Gun.Scale;
		Gun.Foregrip = (bShotgun ? FVector(37.f, 0.f, -3.f) : FVector(44.f, 0.f, -4.5f)) * Gun.Scale;
		return Gun;
	}

	/**
	 * A marker at At (cm of the modeled gun; drawn to the gun's scale, as DescribeGun gives them) with its knuckles along
	 * Fingers and its +Z out of the part along Out (the palm then faces -Out).
	 */
	FHandMark MarksTestMark(const FHandsGun& Gun, const FVector& At, const FVector& Fingers, const FVector& Out, const TCHAR* Pose)
	{
		FHandMark Mark;
		Mark.bValid = true;
		Mark.Local = FTransform(FRotationMatrix::MakeFromXZ(Fingers, Out).ToQuat(), At * Gun.Scale);
		Mark.Pose = FName(Pose);
		return Mark;
	}

	/** The rifle's cup under the trigger housing (the thumbhole loop's front-bottom): palm up, knuckles ahead and a little right. */
	FHandMark MarksTestCup(const FHandsGun& Gun)
	{
		return MarksTestMark(Gun, FVector(34.f, 0.3f, -15.f), FVector(0.8f, 0.5f, 0.15f), FVector(0.f, -0.15f, -1.f), TEXT("Cup"));
	}

	/** Sets a console variable for the test's length (as the console would), then back. */
	struct FMarksTestVariable
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;

		FMarksTestVariable(const TCHAR* Name, const TCHAR* Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}

		~FMarksTestVariable()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}

		void Set(const TCHAR* Value)
		{
			if (Variable)
			{
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsMarksParseTest, "Looter.Player.Hands.Marks.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsMarksParseTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* Socket;
		EHandMarkRole Role;
		int32 Side;
		const TCHAR* Pose;
	};
	// As the export leaves them on the mesh (the SOCKET_ prefix dropped) and as Blender's empties are named; any case.
	const FCase Markers[] = {
		{ TEXT("Hold_L_Cup"), EHandMarkRole::Hold, 0, TEXT("Cup") },
		{ TEXT("Hold_R_Pistol"), EHandMarkRole::Hold, 1, TEXT("Pistol") },
		{ TEXT("SOCKET_Rack_L_Pump"), EHandMarkRole::Rack, 0, TEXT("Pump") },
		{ TEXT("Reload_L_Wrap"), EHandMarkRole::Reload, 0, TEXT("Wrap") },
		{ TEXT("hold_r_Revolver2"), EHandMarkRole::Hold, 1, TEXT("Revolver2") },
	};
	for (const FCase& Case : Markers)
	{
		EHandMarkRole Role = EHandMarkRole::Hold;
		int32 Side = -1;
		FName Pose;
		if (TestTrue(FString::Printf(TEXT("%s reads as a marker"), Case.Socket), WeaponHandMarks::ParseSocket(FName(Case.Socket), Role, Side, Pose)))
		{
			TestTrue(FString::Printf(TEXT("%s: its role"), Case.Socket), Role == Case.Role);
			TestEqual(FString::Printf(TEXT("%s: its side"), Case.Socket), Side, Case.Side);
			TestTrue(FString::Printf(TEXT("%s: its pose (%s)"), Case.Socket, *Pose.ToString()), Pose == FName(Case.Pose));
		}
	}
	// Every other socket a part has, and names that are nearly markers.
	const TCHAR* const NotMarkers[] = { TEXT("Grip"), TEXT("Foregrip"), TEXT("Magazine"), TEXT("Trigger"), TEXT("SOCKET_Trigger"), TEXT("Hold"),
		TEXT("Hold_L"), TEXT("Hold_L_"), TEXT("Hold__Cup"), TEXT("Hold_X_Cup"), TEXT("Grab_L_Cup"), TEXT("Hold_L_Cup_2"), TEXT("Hold_LR_Cup"),
		TEXT("Hold_L_2Cup"), TEXT("SOCKET_"), TEXT("") };
	for (const TCHAR* Socket : NotMarkers)
	{
		EHandMarkRole Role = EHandMarkRole::Hold;
		int32 Side = -1;
		FName Pose;
		TestFalse(FString::Printf(TEXT("'%s' is no marker"), Socket), WeaponHandMarks::ParseSocket(FName(Socket), Role, Side, Pose));
	}
	TestTrue(TEXT("Trigger is the trigger's face"), WeaponHandMarks::IsTriggerSocket(FName(TEXT("Trigger"))));
	TestTrue(TEXT("...with its prefix too"), WeaponHandMarks::IsTriggerSocket(FName(TEXT("SOCKET_Trigger"))));
	TestFalse(TEXT("Triggers is not"), WeaponHandMarks::IsTriggerSocket(FName(TEXT("Triggers"))));
	TestFalse(TEXT("Hold_R_Pistol is not"), WeaponHandMarks::IsTriggerSocket(FName(TEXT("Hold_R_Pistol"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsMarksPlacementTest, "Looter.Player.Hands.Marks.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsMarksPlacementTest::RunTest(const FString& Parameters)
{
	FMarksTestVariable Marks(TEXT("Looter.Hands.Marks"), TEXT("1"));
	FMarksTestVariable Cup(TEXT("Looter.Hands.Cup"), TEXT("1"));

	// The support hand's cup under the rifle's trigger housing.
	FHandsGun Gun = MarksTestGun(EWeaponKind::Rifle);
	const FHandMark Mark = MarksTestCup(Gun);
	const FVector Point = Mark.Local.GetLocation();
	const FVector Knuckles = Mark.Local.GetRotation().GetAxisX();
	const FVector Palm = -Mark.Local.GetRotation().GetAxisZ();
	const FHandPlacement Held = FirstPersonHandsMarks::Placement(Gun, Mark, EFirstPersonHand::Left);
	TestTrue(TEXT("The palm faces the part (along the marker's -Z)"), FVector::DotProduct(Held.GetPalmDir(), Gun.ToCameraDir(Palm)) > 0.999);
	TestTrue(TEXT("...up onto the housing's underside"), FVector::DotProduct(Held.GetPalmDir(), Gun.ToCameraDir(FVector::UpVector)) > 0.9);
	TestTrue(TEXT("The knuckles along the marker's +X"), FVector::DotProduct(Held.GetFingersDir(), Gun.ToCameraDir(Knuckles)) > 0.999);
	const FVector Expected = Gun.ToCameraPoint(Point + Palm * (FirstPersonHands::GripOffPalmSkin * Gun.HandScale));
	TestTrue(FString::Printf(TEXT("The grip point is the palm's skin off the marker at the arms' size (%.3f cm off)"), FVector::Dist(Held.Hold, Expected)),
		Held.Hold.Equals(Expected, 0.01));
	// A hand twice the size sits twice as far off the marker: the offset is the hand's, never the gun's.
	FHandsGun Bigger = Gun;
	Bigger.HandScale = Gun.HandScale * 2.f;
	const FVector Skin = Gun.ToCameraPoint(Point);
	TestTrue(TEXT("The offset grows with the hands, not the gun"),
		FMath::IsNearlyEqual(FVector::Dist(FirstPersonHandsMarks::Placement(Bigger, Mark, EFirstPersonHand::Left).Hold, Skin), 2.0 * FVector::Dist(Held.Hold, Skin), 0.01));
	if (TestTrue(TEXT("The fingers close on a handle"), Held.Contact.Handle.bValid && Held.Contact.Share > 0.f))
	{
		const float Gap = Held.Contact.Handle.Distance(Skin);
		TestTrue(FString::Printf(TEXT("The handle's face is at the marker (%.3f cm off)"), Gap), FMath::Abs(Gap) < 0.05f);
		TestTrue(TEXT("...the handle in the part, ahead of the palm"), FVector::DotProduct(Held.Contact.Handle.Center - Skin, Held.GetPalmDir()) > 0.0);
		TestFalse(TEXT("The support hand works no trigger"), Held.Contact.bTrigger);
	}
	TestTrue(TEXT("Cup is a cupping pose"), FirstPersonHandsMarks::PoseCups(FName(TEXT("Cup"))));
	TestTrue(TEXT("The cup's forearm straight back behind the knuckles"), FVector::DotProduct(Held.Forearm, Held.GetFingersDir()) < -0.3f);
	TestTrue(TEXT("...and down from under the part"), FVector::DotProduct(Held.Forearm, Held.GetPalmDir()) < 0.f);

	// The same through the seams: Hold_L is the support hand's rest, the component's cupped test asks its pose.
	Gun.Marks.Hold[FirstPersonHands::Index(EFirstPersonHand::Left)] = Mark;
	TestTrue(TEXT("OnForegrip holds the marked cup"), FirstPersonHandsGrips::OnForegrip(Gun).Hold.Equals(Held.Hold, 0.01));
	TestTrue(TEXT("OnCup too"), FirstPersonHandsGrips::OnCup(Gun).Hold.Equals(Held.Hold, 0.01));
	TestTrue(TEXT("The marked cup cups"), FirstPersonHandsMarks::SupportCups(Gun));
	FHandsGun Forend = Gun;
	Forend.Marks.Hold[FirstPersonHands::Index(EFirstPersonHand::Left)].Pose = FName(TEXT("Forend"));
	TestFalse(TEXT("A marked fore-end hold doesn't cup"), FirstPersonHandsMarks::SupportCups(Forend));
	TestTrue(TEXT("...and leaves its forearm to the framing"), FirstPersonHandsGrips::OnForegrip(Forend).Forearm.IsNearlyZero());

	// The trigger hand on a marked pistol grip (the loop's rear post's right face) with a marked trigger.
	const FHandMark Pistol = MarksTestMark(Gun, FVector(22.5f, 1.8f, -10.f), FVector(1.f, 0.f, 0.15f), FVector(0.f, 1.f, 0.f), TEXT("Pistol"));
	Gun.Marks.Hold[FirstPersonHands::Index(EFirstPersonHand::Right)] = Pistol;
	Gun.Marks.bTrigger = true;
	Gun.Marks.Trigger = FVector(28.f, 0.f, -6.6f) * Gun.Scale;
	const FHandPlacement OnGrip = FirstPersonHandsGrips::OnGrip(Gun);
	TestTrue(TEXT("OnGrip holds the marked grip"), OnGrip.Hold.Equals(FirstPersonHandsMarks::Placement(Gun, Pistol, EFirstPersonHand::Right).Hold, 0.01));
	TestTrue(TEXT("...its palm onto the grip's side (to the gun's left)"), FVector::DotProduct(OnGrip.GetPalmDir(), Gun.ToCameraDir(-FVector::RightVector)) > 0.99);
	if (TestTrue(TEXT("...its trigger finger on the trigger"), OnGrip.Contact.bTrigger))
	{
		TestTrue(TEXT("...at the marked trigger"), OnGrip.Contact.TriggerPoint.Equals(Gun.ToCameraPoint(Gun.Marks.Trigger), 0.01));
		TestTrue(TEXT("...as far as it's wanted there"), OnGrip.Contact.TriggerShare > 0.5f);
	}
	FHandsGun Indexed = Gun;
	Indexed.TriggerReady = 0.f;
	TestTrue(TEXT("Indexed, the trigger finger lets go"), FirstPersonHandsGrips::OnGrip(Indexed).Contact.TriggerShare < 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsMarksFallbackTest, "Looter.Player.Hands.Marks.Fallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsMarksFallbackTest::RunTest(const FString& Parameters)
{
	FMarksTestVariable Marks(TEXT("Looter.Hands.Marks"), TEXT("1"));
	FMarksTestVariable Cup(TEXT("Looter.Hands.Cup"), TEXT("1"));
	const int32 Left = FirstPersonHands::Index(EFirstPersonHand::Left);

	// A gun not marked (every gun today) holds by its kind's tables, as before.
	const FHandsGun Plain = MarksTestGun(EWeaponKind::Rifle);
	FHandPlacement Unused;
	TestFalse(TEXT("No marks: nothing found"), FirstPersonHandsMarks::Place(Plain, EHandMarkRole::Hold, EFirstPersonHand::Left, Unused));
	TestTrue(TEXT("...no trigger mark: the table's spot"), FirstPersonHandsMarks::TriggerAt(Plain, FVector(28.f, 0.f, -6.6f)).Equals(FVector(28.f, 0.f, -6.6f)));
	const FGunHands& Table = FirstPersonHandsGrips::For(EWeaponKind::Rifle);
	const FVector TableCup = Plain.ToCameraPoint(Plain.Grip + (Table.CupHold + FPHandsFraming::SupportNudge(EWeaponKind::Rifle)) * Plain.Scale);
	TestTrue(TEXT("No marks: the table's cup"), FirstPersonHandsGrips::OnForegrip(Plain).Hold.Equals(TableCup, 0.01));
	TestTrue(TEXT("No marks: the table says it cups"), FirstPersonHandsMarks::SupportCups(Plain) == Table.bCup);

	// One role marked: the others keep the tables.
	FHandsGun Marked = Plain;
	Marked.Marks.Hold[Left] = MarksTestCup(Marked);
	TestFalse(TEXT("The marked cup isn't the table's"), FirstPersonHandsGrips::OnForegrip(Marked).Hold.Equals(TableCup, 0.01));
	TestTrue(TEXT("The trigger hand, not marked, on the table's grip"), FirstPersonHandsGrips::OnGrip(Marked).Hold.Equals(FirstPersonHandsGrips::OnGrip(Plain).Hold, 0.01));

	// Looter.Hands.Marks 0: the tables for every gun (before and after).
	Marks.Set(TEXT("0"));
	TestTrue(TEXT("Marks off: the table's cup on a marked gun"), FirstPersonHandsGrips::OnForegrip(Marked).Hold.Equals(TableCup, 0.01));
	TestTrue(TEXT("Marks off: the table says it cups"), FirstPersonHandsMarks::SupportCups(Marked) == Table.bCup);
	Marks.Set(TEXT("1"));
	// Looter.Hands.Cup 0: the fore-end of before 2026-10-27, a marked cup or not.
	Cup.Set(TEXT("0"));
	TestTrue(TEXT("Cup off: the fore-end, not the marked cup"),
		FirstPersonHandsGrips::OnForegrip(Marked).Hold.Equals(FirstPersonHandsGrips::OnForeEnd(Plain).Hold, 0.01));
	TestFalse(TEXT("Cup off: no cup"), FirstPersonHandsMarks::SupportCups(Marked));
	Cup.Set(TEXT("1"));

	// A marked pump (Rack_L, resting forward): the hand rides it back as it's racked, and leaves the cup for it.
	FHandsGun Pump = MarksTestGun(EWeaponKind::Shotgun);
	const FHandMark Rack = MarksTestMark(Pump, FVector(37.f, -1.f, -3.f), FVector(0.92f, -0.4f, 0.f), FVector(0.f, -0.3f, -1.f), TEXT("Pump"));
	Pump.Marks.Rack[Left] = Rack;
	Pump.PumpShare = 1.f;
	FHandMark Racked = Rack;
	Racked.Local.AddToTranslation(FVector(-4.f * Pump.Scale, 0.f, 0.f));
	TestTrue(TEXT("On the marked pump, racked 4 cm back"),
		FirstPersonHandsGrips::OnForegrip(Pump, 4.f).Hold.Equals(FirstPersonHandsMarks::Placement(Pump, Racked, EFirstPersonHand::Left).Hold, 0.01));
	FHandsGun RiflePump = Plain;
	RiflePump.Marks.Rack[Left] = MarksTestMark(RiflePump, FVector(44.f, 0.f, -4.5f), FVector::ForwardVector, -FVector::UpVector, TEXT("Pump"));
	RiflePump.PumpCycle = 0.2f;
	FHandsGun RifleRacking = Plain;
	RifleRacking.PumpCycle = 0.2f;
	TestTrue(TEXT("A marked pump takes the hand off its rest to rack"), FirstPersonHandsGrips::NextPumpShare(RiflePump, 0.f, 0.04f) > 0.f);
	TestEqual(TEXT("...none marked (the rifle's table): it stays"), FirstPersonHandsGrips::NextPumpShare(RifleRacking, 0.f, 0.04f), 0.f);

	// A marked magazine (Reload_L, at its rest): the hand takes it there and goes down with it.
	FHandsGun Reload = Plain;
	const FHandMark Grab = MarksTestMark(Reload, FVector(15.5f, -1.4f, -14.f), FVector::ForwardVector, -FVector::RightVector, TEXT("Wrap"));
	Reload.Marks.Reload[Left] = Grab;
	FHandMark Dropped = Grab;
	Dropped.Local.AddToTranslation(FVector(0.f, 0.f, -5.f * Reload.Scale));
	TestTrue(TEXT("Round the marked magazine, 5 cm out of the well"),
		FirstPersonHandsMotion::OnMagazine(Reload, 5.f).Hold.Equals(FirstPersonHandsMarks::Placement(Reload, Dropped, EFirstPersonHand::Left).Hold, 0.01));
	TestTrue(TEXT("...none marked: the table's grab"), !FirstPersonHandsMotion::OnMagazine(Plain, 5.f).Hold.Equals(FirstPersonHandsMotion::OnMagazine(Reload, 5.f).Hold, 0.01));
	return true;
}

#endif
