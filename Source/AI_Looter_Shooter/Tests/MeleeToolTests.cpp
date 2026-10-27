#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Weapons/WeaponFXMelee.h"
#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "Engine/StaticMesh.h"

// The heroes' melee tools (the user's note 5 and picks, 2026-10-27): who swings what (UHeroDefinition::MeleeToolFor), each
// swing's arc in first person (MeleeMotion::ToolPose), and the right hand on the tool (FirstPersonHandsMotion::OnMeleeTool).

namespace
{
	const EMeleeSwingStyle AllStyles[] = { EMeleeSwingStyle::Swat, EMeleeSwingStyle::Flick, EMeleeSwingStyle::Chop, EMeleeSwingStyle::Peck,
		EMeleeSwingStyle::Gaff, EMeleeSwingStyle::Jab };

	FVector HandAt(EMeleeSwingStyle Style, float Time, bool bLanded = false)
	{
		return MeleeMotion::ToolPose(Style, EMeleeHeft::Steady, Time, bLanded).Grip.GetLocation();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeToolPicksTest, "Looter.Melee.Tool.Picks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeToolPicksTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* Hero;
		const TCHAR* Mesh;
		EMeleeGripKind Grip;
		EMeleeSwingStyle Swing;
		EMeleeHeft Heft;
	};
	// The user's picks: Ellis A, Odessa A, Crane A, Gauge C, Pike A, with the sheets' swings and feel tags.
	const FCase Cases[] = {
		{ TEXT("Ellis"), TEXT("/Game/Art/Melee/SM_Melee_GraveSpade.SM_Melee_GraveSpade"), EMeleeGripKind::Held, EMeleeSwingStyle::Swat, EMeleeHeft::Steady },
		{ TEXT("Odessa"), TEXT("/Game/Art/Melee/SM_Melee_RazorFan.SM_Melee_RazorFan"), EMeleeGripKind::Held, EMeleeSwingStyle::Flick, EMeleeHeft::Quick },
		{ TEXT("Crane"), TEXT("/Game/Art/Melee/SM_Melee_FerryGaff.SM_Melee_FerryGaff"), EMeleeGripKind::Held, EMeleeSwingStyle::Gaff, EMeleeHeft::Heavy },
		{ TEXT("Gauge"), TEXT("/Game/Art/Melee/SM_Melee_PistonFist.SM_Melee_PistonFist"), EMeleeGripKind::Forearm, EMeleeSwingStyle::Jab, EMeleeHeft::Quick },
		{ TEXT("Pike"), TEXT("/Game/Art/Melee/SM_Melee_RockPick.SM_Melee_RockPick"), EMeleeGripKind::Held, EMeleeSwingStyle::Peck, EMeleeHeft::Quick },
	};
	for (const FCase& Case : Cases)
	{
		const FHeroMeleeTool Tool = UHeroDefinition::MeleeToolFor(FName(Case.Hero));
		const FString Name(Case.Hero);
		TestEqual(Name + TEXT(": their tool's mesh"), Tool.Mesh.ToSoftObjectPath().ToString(), FString(Case.Mesh));
		TestTrue(Name + TEXT(": held or on the forearm"), Tool.Grip == Case.Grip);
		TestTrue(Name + TEXT(": their swing"), Tool.Swing == Case.Swing);
		TestTrue(Name + TEXT(": its heft"), Tool.Heft == Case.Heft);
		TestTrue(Name + TEXT(": a key for its sounds and effects"), !Tool.Kind.IsNone());
		TestEqual(Name + TEXT(": the roster's hero"), UHeroDefinition::RosterIds().Contains(FName(Case.Hero)), true);
		// The hero's data, where it's in the game, says the same unless it was set to its own.
		if (const UHeroDefinition* Hero = UHeroDefinition::Load(FName(Case.Hero)))
		{
			if (Hero->MeleeTool.Mesh.IsNull())
			{
				TestTrue(Name + TEXT(": the data asset uses the picks"), Hero->GetMeleeTool().Swing == Case.Swing);
			}
		}
	}
	TestTrue(TEXT("Anyone else: no tool of their own (the placeholder's)"), UHeroDefinition::MeleeToolFor(TEXT("Nobody")).Mesh.IsNull());
	TestTrue(TEXT("The placeholder's path"), UHeroDefinition::PlaceholderMeleeToolPath().ToString().StartsWith(TEXT("/Game/Art/Melee/")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeToolArcTest, "Looter.Melee.Tool.Arcs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeToolArcTest::RunTest(const FString& Parameters)
{
	using namespace MeleeMotion;
	const float Contact = FMeleeRules::ContactSeconds;
	const float End = FMeleeRules::SwingSeconds;
	for (const EMeleeSwingStyle Style : AllStyles)
	{
		const FString Name = UEnum::GetValueAsString(Style);
		const FMeleeArc Arc = ArcFor(Style);
		TestFalse(Name + TEXT(": hidden before the swing"), ToolPose(Style, EMeleeHeft::Steady, 0.f, false).bShown);
		TestFalse(Name + TEXT(": hidden after it"), ToolPose(Style, EMeleeHeft::Steady, End, false).bShown);
		const FMeleeToolPose AtBlow = ToolPose(Style, EMeleeHeft::Steady, Contact, false);
		TestTrue(Name + TEXT(": shown at the blow"), AtBlow.bShown);
		TestEqual(Name + TEXT(": the hand all on the tool at the blow"), AtBlow.HandShare, 1.f, 0.001f);
		TestTrue(Name + TEXT(": the hand on the gun's grip again at the end"), ToolPose(Style, EMeleeHeft::Steady, End - 0.001f, false).HandShare < 0.05f);
		// At the blow the hand is where the arc says, ahead of the eye and in the middle of the view.
		const FVector Blow = AtBlow.Grip.GetLocation();
		TestTrue(Name + TEXT(": at the arc's blow"), Blow.Equals(Arc.Shoulder + Arc.ContactDir * Arc.Reach, 0.5f));
		TestTrue(Name + TEXT(": ahead of the eye"), Blow.X > 25.f);
		TestTrue(Name + TEXT(": near the middle of the view"), FMath::Abs(Blow.Y) < 12.f);
		// Drawn from under the frame and gone back under it.
		TestTrue(Name + TEXT(": drawn up from under the frame"), HandAt(Style, 0.005f).Z < HandAt(Style, CockSeconds).Z - 10.f);
		TestTrue(Name + TEXT(": back under it at the end"), HandAt(Style, End - 0.005f).Z < Blow.Z - 15.f);

		// Never a jump: under 6 cm and 8 degrees in a millisecond, through each phase's seams, landed or not.
		for (const bool bLanded : { false, true })
		{
			float WorstMove = 0.f;
			float WorstTurn = 0.f;
			FTransform Last = ToolPose(Style, EMeleeHeft::Steady, 0.001f, bLanded).Grip;
			for (float Time = 0.002f; Time < End; Time += 0.001f)
			{
				const FTransform Now = ToolPose(Style, EMeleeHeft::Steady, Time, bLanded && Time > Contact).Grip;
				WorstMove = FMath::Max(WorstMove, static_cast<float>(FVector::Dist(Now.GetLocation(), Last.GetLocation())));
				WorstTurn = FMath::Max(WorstTurn, FMath::RadiansToDegrees(Now.GetRotation().AngularDistance(Last.GetRotation())));
				Last = Now;
			}
			TestTrue(FString::Printf(TEXT("%s%s: no jump of over 6 cm in a millisecond (%.2f)"), *Name, bLanded ? TEXT(" landed") : TEXT(""), WorstMove),
				WorstMove < 6.f);
			TestTrue(FString::Printf(TEXT("%s%s: no turn of over 8 degrees in a millisecond (%.2f)"), *Name, bLanded ? TEXT(" landed") : TEXT(""), WorstTurn),
				WorstTurn < 8.f);
		}
	}

	// Each swing's way, as the sheets drew it (camera space: X ahead, Y right, Z up).
	const float Cock = CockSeconds;
	const float Through = Contact + FollowThroughSeconds;
	const FVector SwatCock = HandAt(EMeleeSwingStyle::Swat, Cock);
	const FVector SwatBlow = HandAt(EMeleeSwingStyle::Swat, Contact);
	const FVector SwatThrough = HandAt(EMeleeSwingStyle::Swat, Through);
	TestTrue(TEXT("Swat: wound up high on the right"), SwatCock.Y > SwatBlow.Y + 15.f && SwatCock.Z > SwatBlow.Z);
	TestTrue(TEXT("...through low on the left"), SwatThrough.Y < SwatBlow.Y - 15.f && SwatThrough.Z < SwatBlow.Z);
	const FVector FlickCock = HandAt(EMeleeSwingStyle::Flick, Cock);
	const FVector FlickBlow = HandAt(EMeleeSwingStyle::Flick, Contact);
	const FVector FlickThrough = HandAt(EMeleeSwingStyle::Flick, Through);
	TestTrue(TEXT("Flick: wound up low on the left"), FlickCock.Y < FlickBlow.Y - 10.f && FlickCock.Z < FlickBlow.Z);
	TestTrue(TEXT("...through high on the right"), FlickThrough.Y > FlickBlow.Y + 10.f && FlickThrough.Z > FlickBlow.Z);
	for (const EMeleeSwingStyle Over : { EMeleeSwingStyle::Chop, EMeleeSwingStyle::Peck, EMeleeSwingStyle::Gaff })
	{
		TestTrue(UEnum::GetValueAsString(Over) + TEXT(": wound up high"), HandAt(Over, Cock).Z > HandAt(Over, Contact).Z + 15.f);
	}
	TestTrue(TEXT("Peck: down and to the left after the blow"), HandAt(EMeleeSwingStyle::Peck, Through).Y < HandAt(EMeleeSwingStyle::Peck, Contact).Y
		&& HandAt(EMeleeSwingStyle::Peck, Through).Z < HandAt(EMeleeSwingStyle::Peck, Contact).Z);
	const FMeleeArc Gaff = ArcFor(EMeleeSwingStyle::Gaff);
	TestTrue(TEXT("Gaff: yanked back toward the body after the blow"),
		FVector::Dist(HandAt(EMeleeSwingStyle::Gaff, Through), Gaff.Shoulder) < FVector::Dist(HandAt(EMeleeSwingStyle::Gaff, Contact), Gaff.Shoulder) - 8.f);
	TestTrue(TEXT("Jab: drawn back, then driven out ahead"), HandAt(EMeleeSwingStyle::Jab, Cock).X < HandAt(EMeleeSwingStyle::Jab, Contact).X - 12.f);
	TestTrue(TEXT("Jab: the back of the fist up at the blow"),
		ToolPose(EMeleeSwingStyle::Jab, EMeleeHeft::Steady, Contact, false).Grip.GetRotation().GetAxisZ().Z > 0.8f);
	TestTrue(TEXT("Jab: the piston kicks out after the blow"), ToolPose(EMeleeSwingStyle::Jab, EMeleeHeft::Steady, Contact + 0.04f, true).Push > 3.f);
	// The striking side leads at the blow: a swat's to the left, a chop's down.
	// The spade's blade turned to the eye through its swat (not edge on, DEMO round 3), and a long tool's head (not its
	// handle) across the middle of the view at the blow (DEMO round 4).
	const FTransform SwatBlowGrip = ToolPose(EMeleeSwingStyle::Swat, EMeleeHeft::Steady, Contact, false).Grip;
	const FVector EyeWay = (-SwatBlowGrip.GetLocation()).GetSafeNormal();
	TestTrue(TEXT("Swat: the blade's face toward the eye"), FMath::Abs(FVector::DotProduct(SwatBlowGrip.GetRotation().GetAxisZ(), EyeWay)) > 0.6f);
	const FTransform Long = ToolPose(EMeleeSwingStyle::Swat, EMeleeHeft::Steady, Contact, false, 76.f).Grip;
	const FVector LongTip = Long.GetLocation() + Long.GetRotation().GetAxisX() * 76.f;
	TestTrue(FString::Printf(TEXT("A 76 cm spade's tip crosses the middle of the view (%.1f, %.1f)"), LongTip.Y, LongTip.Z),
		FMath::Abs(LongTip.Y) < 3.f && FMath::Abs(LongTip.Z + 4.f) < 3.f && LongTip.X > 60.f);
	TestTrue(TEXT("Chop strikes down"), ToolPose(EMeleeSwingStyle::Chop, EMeleeHeft::Steady, Contact, false).Grip.GetRotation().GetAxisZ().Z < -0.4f);
	// A heavy tool winds up farther than a quick one.
	TestTrue(TEXT("Heavy winds up farther"), FMath::Abs(ArcFor(EMeleeSwingStyle::Chop, EMeleeHeft::Heavy).CockDegrees)
		> FMath::Abs(ArcFor(EMeleeSwingStyle::Chop, EMeleeHeft::Quick).CockDegrees));
	// A landed blow stops and bounces back a little, a miss goes on through.
	TestTrue(TEXT("Landed: back toward the wind-up"), HandAt(EMeleeSwingStyle::Swat, Through, true).Y > SwatBlow.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeToolHandTest, "Looter.Melee.Tool.Hand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeToolHandTest::RunTest(const FString& Parameters)
{
	const FMeleeToolPose Pose = MeleeMotion::ToolPose(EMeleeSwingStyle::Swat, EMeleeHeft::Steady, FMeleeRules::ContactSeconds, false);
	const FQuat Turn = Pose.Grip.GetRotation();
	const FHandPlacement Held = FirstPersonHandsMotion::OnMeleeTool(Pose.Grip, 1.6f, false);
	TestTrue(TEXT("Held: the grip point on the tool's"), Held.Hold.Equals(Pose.Grip.GetLocation(), 0.01f));
	TestTrue(TEXT("...the knuckles lead the striking side"), FVector::DotProduct(Held.GetFingersDir(), Turn.GetAxisZ()) > 0.9f);
	TestTrue(TEXT("...the palm onto the handle"), FVector::DotProduct(Held.GetPalmDir(), Turn.GetAxisY()) > 0.9f);
	TestTrue(TEXT("...the fingers close on the handle"), Held.Contact.Handle.bValid && Held.Contact.Share > 0.9f);
	TestEqual(TEXT("...a round handle of its radius"), Held.Contact.Handle.Distance(Pose.Grip.GetLocation() + Turn.GetAxisZ() * 1.6f), 0.f, 0.05f);
	TestTrue(TEXT("...the forearm behind the knuckles"), FVector::DotProduct(Held.Forearm, Held.GetFingersDir()) < 0.f);
	const FHandPlacement Fist = FirstPersonHandsMotion::OnMeleeTool(Pose.Grip, 1.6f, true);
	TestTrue(TEXT("Forearm: the knuckles along the punch"), FVector::DotProduct(Fist.GetFingersDir(), Turn.GetAxisX()) > 0.9f);
	TestFalse(TEXT("...a bare fist (nothing to close on)"), Fist.Contact.Handle.bValid);
	// The empty hands rest under the frame, one each side.
	const FHandPlacement Left = FirstPersonHandsMotion::BareRest(EFirstPersonHand::Left);
	const FHandPlacement Right = FirstPersonHandsMotion::BareRest(EFirstPersonHand::Right);
	TestTrue(TEXT("Resting hands under the frame, either side"), Left.Hold.Z < -20.f && Right.Hold.Z < -20.f && Left.Hold.Y < 0.f && Right.Hold.Y > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeToolFeelTest, "Looter.Melee.Tool.Feel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeToolFeelTest::RunTest(const FString& Parameters)
{
	// Round 4 (2026-10-27): each hero's tool has its own cues, its own sparks, glints or steam, and swings in third person.
	const TCHAR* const Heroes[] = { TEXT("Ellis"), TEXT("Odessa"), TEXT("Crane"), TEXT("Gauge"), TEXT("Pike") };
	TSet<FName> Kinds;
	for (const TCHAR* Hero : Heroes)
	{
		const FName Kind = UHeroDefinition::MeleeToolFor(Hero).Kind;
		Kinds.Add(Kind);
		TestEqual(FString(Hero) + TEXT(": its swing cue"), MeleeFX::SwingCue(Kind).ToString(), FString::Printf(TEXT("Player.Melee.%s.Swing"), *Kind.ToString()));
		TestEqual(FString(Hero) + TEXT(": its hit cue"), MeleeFX::HitCue(Kind).ToString(), FString::Printf(TEXT("Player.Melee.%s.Hit"), *Kind.ToString()));
	}
	TestEqual(TEXT("Five tools, five sounds"), Kinds.Num(), 5);
	TestTrue(TEXT("No key: the shared cues"), MeleeFX::SwingCue(NAME_None).IsNone() && MeleeFX::HitCue(NAME_None).IsNone());
	TestTrue(TEXT("The keys the effects know"), Kinds.Contains(MeleeFX::Spade()) && Kinds.Contains(MeleeFX::Fan()) && Kinds.Contains(MeleeFX::Gaff())
		&& Kinds.Contains(MeleeFX::Piston()) && Kinds.Contains(MeleeFX::Pick()));

	// Sparks: the spade and the pick on stone and iron; never in dirt; the fan's edges on iron alone; the piston never.
	TestTrue(TEXT("Spade on stone: sparks"), MeleeFX::Sparks(MeleeFX::Spade(), EImpactMaterial::Stone));
	TestTrue(TEXT("Pick on stone: sparks"), MeleeFX::Sparks(MeleeFX::Pick(), EImpactMaterial::Stone));
	TestTrue(TEXT("Gaff on iron: sparks"), MeleeFX::Sparks(MeleeFX::Gaff(), EImpactMaterial::Metal));
	TestFalse(TEXT("Spade in dirt: none"), MeleeFX::Sparks(MeleeFX::Spade(), EImpactMaterial::Dirt));
	TestFalse(TEXT("Pick in wood: none"), MeleeFX::Sparks(MeleeFX::Pick(), EImpactMaterial::Wood));
	TestFalse(TEXT("Fan on stone: none"), MeleeFX::Sparks(MeleeFX::Fan(), EImpactMaterial::Stone));
	TestTrue(TEXT("Fan on iron: sparks"), MeleeFX::Sparks(MeleeFX::Fan(), EImpactMaterial::SheetMetal));
	TestFalse(TEXT("Piston: steam, not sparks"), MeleeFX::Sparks(MeleeFX::Piston(), EImpactMaterial::Metal));

	// Third person: the wrist lays the tool back with the coil and snaps it over onto the blow, from and to rest.
	TestEqual(TEXT("At rest: no turn"), LooterStancePose::MeleeWristDegrees(0.f), 0.f);
	TestTrue(TEXT("Coiled: laid back"), LooterStancePose::MeleeWristDegrees(-0.3f) < -20.f);
	TestTrue(TEXT("The blow: snapped over"), LooterStancePose::MeleeWristDegrees(1.f) > 10.f);
	float Worst = 0.f;
	float Last = LooterStancePose::MeleeWristDegrees(LooterStancePose::MeleeJab(0.f));
	for (float Time = 0.001f; Time <= FMeleeRules::SwingSeconds + 0.01f; Time += 0.001f)
	{
		const float Now = LooterStancePose::MeleeWristDegrees(LooterStancePose::MeleeJab(Time));
		Worst = FMath::Max(Worst, FMath::Abs(Now - Last));
		Last = Now;
	}
	TestTrue(FString::Printf(TEXT("The wrist never jumps (%.2f degrees in a millisecond)"), Worst), Worst < 4.f);
	TestEqual(TEXT("...and is back at rest after the swing"), Last, 0.f);
	return true;
}

#endif
