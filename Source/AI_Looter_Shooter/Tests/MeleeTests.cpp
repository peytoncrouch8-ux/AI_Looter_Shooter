#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HitReaction.h"
#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "Player/PlayerSize.h"
#include "Player/ViewKick.h"
#include "Progression/LevelRules.h"
#include "Settings/KeyBindingSubsystem.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

// The melee strike's rules (Docs/Polish/BorderlandsComparison.md, item 16): its reach and cone, its damage and knock,
// when it can start, the stagger and hit-stop it gives, its key, and its motion. MeleePlayTests.cpp has it in a level.

namespace
{
	/** The player standing at the origin (their eye 140 cm over their feet, about the player's size), looking along +X. */
	FMeleeAim AimAhead(float PitchDegrees = 0.f)
	{
		FMeleeAim Aim;
		Aim.Eye = FVector(0.0, 0.0, 140.0);
		Aim.Forward = FRotator(PitchDegrees, 0.f, 0.f).Vector();
		Aim.FeetZ = 0.0;
		return Aim;
	}

	/** A body standing on the ground with its middle Distance (cm) from the eye's line, Degrees right of the look. */
	FMeleeBody BodyAt(float Distance, float Degrees, float Radius = 10.f, float HalfHeight = 60.f, float FeetZ = 0.f)
	{
		FMeleeBody Body;
		Body.Center = FRotator(0.f, Degrees, 0.f).Vector() * Distance + FVector(0.0, 0.0, FeetZ + HalfHeight);
		Body.Radius = Radius;
		Body.HalfHeight = HalfHeight;
		return Body;
	}

	bool Reaches(const FMeleeAim& Aim, const FMeleeBody& Body, FMeleeContact* OutContact = nullptr)
	{
		FMeleeContact Contact;
		const bool bReached = FMeleeRules::FindContact(Aim, Body, FMeleeRules::Reach(), Contact);
		if (OutContact)
		{
			*OutContact = Contact;
		}
		return bReached;
	}

	/** A kick and when it starts (seconds into the swing). */
	struct FTimedKick
	{
		FViewKick Kick;
		float At = 0.f;
	};

	/** The most the kicks add up to over the swing, before the stack's limits (Read picks which part). */
	template <typename TRead>
	float PeakSum(const TArray<FTimedKick>& Kicks, TRead Read)
	{
		float Peak = 0.f;
		for (float Time = 0.f; Time < 1.f; Time += 0.001f)
		{
			float Sum = 0.f;
			for (const FTimedKick& Each : Kicks)
			{
				Sum += Read(Each.Kick) * FViewKickStack::Response(Time - Each.At, Each.Kick.Frequency, Each.Kick.Damping);
			}
			Peak = FMath::Max(Peak, FMath::Abs(Sum));
		}
		return Peak;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeReachTest, "Looter.Melee.Reach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeReachTest::RunTest(const FString& Parameters)
{
	// 1.7 m at the player's size: 2 m at full size.
	TestEqual(TEXT("The reach is 2 m times the player's size"), FMeleeRules::Reach(), 200.f * LooterPlayerSize::Scale, 0.01f);
	TestEqual(TEXT("...170 cm"), FMeleeRules::Reach(), 170.f, 0.5f);
	const FMeleeAim Aim = AimAhead();
	const float Reach = FMeleeRules::Reach();

	// The reach is to the body's near side, across the ground.
	TestTrue(TEXT("A body a metre ahead is hit"), Reaches(Aim, BodyAt(100.f, 0.f)));
	TestTrue(TEXT("...its near side just inside the reach"), Reaches(Aim, BodyAt(Reach + 10.f - 1.f, 0.f)));
	TestFalse(TEXT("...just past it, not"), Reaches(Aim, BodyAt(Reach + 10.f + 5.f, 0.f)));
	TestTrue(TEXT("A wide body is hit by its side (its middle 2.2 m off)"), Reaches(Aim, BodyAt(220.f, 0.f, 60.f)));

	// The cone: 50 degrees, to the body's nearest edge.
	TestEqual(TEXT("A 50 degree cone"), FMeleeRules::HalfConeDegrees * 2.f, 50.f);
	TestTrue(TEXT("22 degrees off: hit"), Reaches(Aim, BodyAt(120.f, 22.f)));
	TestTrue(TEXT("22 degrees to the left: hit"), Reaches(Aim, BodyAt(120.f, -22.f)));
	TestFalse(TEXT("32 degrees off: missed"), Reaches(Aim, BodyAt(120.f, 32.f)));
	TestTrue(TEXT("...unless it's wide enough that its edge is in the cone"), Reaches(Aim, BodyAt(120.f, 32.f, 40.f)));
	TestFalse(TEXT("Behind: never"), Reaches(Aim, BodyAt(100.f, 180.f)));
	// Pressed against the player, it's hit anywhere in front, never behind.
	TestTrue(TEXT("Pressed close at the side: hit"), Reaches(Aim, BodyAt(30.f, 70.f, 20.f)));
	TestFalse(TEXT("Pressed close behind: missed"), Reaches(Aim, BodyAt(30.f, 150.f, 20.f)));

	// Up and down: a slime at the feet is hit looking ahead, not looking at the sky; nothing on a ledge overhead.
	const FMeleeBody Slime = BodyAt(150.f, 0.f, 50.f, 50.f);
	TestTrue(TEXT("A slime at the feet, looking ahead: hit"), Reaches(Aim, Slime));
	TestTrue(TEXT("...looking down at it: hit"), Reaches(AimAhead(-40.f), Slime));
	TestFalse(TEXT("...looking up at the sky: missed"), Reaches(AimAhead(60.f), Slime));
	TestFalse(TEXT("A body on a ledge 3 m up: missed"), Reaches(Aim, BodyAt(100.f, 0.f, 10.f, 60.f, 300.f)));
	TestFalse(TEXT("A body 2 m below the feet: missed"), Reaches(Aim, BodyAt(100.f, 0.f, 10.f, 60.f, -200.f)));

	// Where it lands: the near side, at the chest (35 cm under the eye), or the top of a short body.
	FMeleeContact Contact;
	if (TestTrue(TEXT("A tall body ahead"), Reaches(Aim, BodyAt(120.f, 0.f, 30.f, 90.f), &Contact)))
	{
		TestTrue(FString::Printf(TEXT("Lands on its near side at the chest (%s)"), *Contact.Point.ToCompactString()),
			Contact.Point.Equals(FVector(90.0, 0.0, 105.0), 0.5));
		TestEqual(TEXT("...90 cm away"), Contact.Distance, 90.f, 0.5f);
	}
	if (TestTrue(TEXT("The slime"), Reaches(Aim, Slime, &Contact)))
	{
		TestEqual(TEXT("Lands on a slime's top"), static_cast<float>(Contact.Point.Z), 100.f, 0.5f);
	}

	// The nearer, more in-line body is the better target.
	FMeleeContact Near;
	FMeleeContact Far;
	FMeleeContact Aside;
	if (Reaches(Aim, BodyAt(60.f, 0.f), &Near) && Reaches(Aim, BodyAt(150.f, 0.f), &Far) && Reaches(Aim, BodyAt(60.f, 20.f), &Aside))
	{
		TestTrue(TEXT("Nearer scores better"), Near.Score < Far.Score);
		TestTrue(TEXT("In line scores better"), Near.Score < Aside.Score);
	}
	else
	{
		AddError(TEXT("The three bodies should all be in reach"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeDamageTest, "Looter.Melee.Damage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeDamageTest::RunTest(const FString& Parameters)
{
	// 60 at level 1: a tenth of a Common rifle's 30-round magazine at 20 a round. +/-10% like every hit.
	TestEqual(TEXT("60 at level 1"), FMeleeRules::StrikeDamage(1.f, 0.5f), 60.f, 0.01f);
	TestEqual(TEXT("...54 at the low end"), FMeleeRules::StrikeDamage(1.f, 0.f), 54.f, 0.01f);
	TestEqual(TEXT("...66 at the high end"), FMeleeRules::StrikeDamage(1.f, 1.f), 66.f, 0.01f);

	// Growing with the player's level as enemies do, a strike at equal levels always takes the same share of a Basic
	// creature: an Unpaid (160 at level 1) in 3, a Meadow Slime (120) in 2, a spider (300) in 5.
	const FLevelRules Rules;
	for (const int32 Level : { 1, 10, 30, 70 })
	{
		const float Scale = Rules.EnemyScale(Level);
		const float Strike = FMeleeRules::StrikeDamage(Scale, 0.5f);
		const auto Strikes = [Strike, Scale](float LevelOneHealth) { return FMath::CeilToInt(LevelOneHealth * Scale / Strike - 1e-4f); };
		TestEqual(FString::Printf(TEXT("Level %d: an Unpaid in 3"), Level), Strikes(160.f), 3);
		TestEqual(FString::Printf(TEXT("Level %d: a slime in 2"), Level), Strikes(120.f), 2);
		TestEqual(FString::Printf(TEXT("Level %d: a spider in 5"), Level), Strikes(300.f), 5);
	}

	// The knock: a hop back of about 1.3 m for a full-size Basic creature.
	const FVector Basic = FMeleeRules::KnockVelocity(FVector::ForwardVector, 1.f, ECreatureRank::Basic, false);
	TestTrue(TEXT("Away and up"), Basic.X > 0.0 && Basic.Z > 0.0 && FMath::IsNearlyZero(Basic.Y));
	const float Hop = static_cast<float>(Basic.X) * 2.f * static_cast<float>(Basic.Z) / 980.f;
	TestTrue(FString::Printf(TEXT("A Basic creature hops back 1-1.6 m (%.0f cm)"), Hop), Hop > 100.f && Hop < 160.f);
	TestTrue(TEXT("Only the way across the ground counts"),
		FMeleeRules::KnockVelocity(FVector(1.0, 0.0, 5.0), 1.f, ECreatureRank::Basic, false).Equals(Basic, 0.01));
	const FVector Slime = FMeleeRules::KnockVelocity(FVector::ForwardVector, 1.f, ECreatureRank::Basic, true);
	TestEqual(TEXT("A slime goes a quarter harder"), static_cast<float>(Slime.X / Basic.X), FMeleeRules::LightKnockScale, 0.001f);
	const FVector Rare = FMeleeRules::KnockVelocity(FVector::ForwardVector, 1.f, ECreatureRank::Rare, false);
	const FVector Epic = FMeleeRules::KnockVelocity(FVector::ForwardVector, 1.f, ECreatureRank::Epic, false);
	TestTrue(TEXT("Restless gives less ground, Gravebound less still"), Rare.X < Basic.X && Epic.X < Rare.X);
	TestTrue(TEXT("A bigger body gives less"), FMeleeRules::KnockVelocity(FVector::ForwardVector, 2.f, ECreatureRank::Basic, false).X < Basic.X);
	TestTrue(TEXT("A Soulfed monster stands its ground"), FMeleeRules::KnockVelocity(FVector::ForwardVector, 1.f, ECreatureRank::Legendary, false).IsZero());
	TestTrue(TEXT("A boss takes the damage only"), FMeleeRules::KnockVelocity(FVector::ForwardVector, 1.f, ECreatureRank::Boss, false).IsZero());
	TestTrue(TEXT("No way to go: no knock"), FMeleeRules::KnockVelocity(FVector::UpVector, 1.f, ECreatureRank::Basic, false).IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeGateTest, "Looter.Melee.Gate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeGateTest::RunTest(const FString& Parameters)
{
	// The swing's times: the blow 0.12 s into a 0.45 s swing, the next 0.6 s after the last began.
	TestEqual(TEXT("A 0.45 s swing"), FMeleeRules::SwingSeconds, 0.45f);
	TestEqual(TEXT("The blow at 0.12 s"), FMeleeRules::ContactSeconds, 0.12f);
	TestEqual(TEXT("A 0.6 s cooldown"), FMeleeRules::CooldownSeconds, 0.6f);
	TestTrue(TEXT("The swing, its hit-stop and all, ends inside the cooldown"),
		FMeleeRules::SwingSeconds + FMeleeRules::AttackerHitStop < FMeleeRules::CooldownSeconds);

	FMeleeGateInput Ready;
	Ready.Now = 100.0;
	TestTrue(TEXT("Ready: it strikes"), FMeleeRules::WhyBlocked(Ready) == EMeleeBlock::None);

	FMeleeGateInput In = Ready;
	In.LastStrike = 100.0 - 0.3;
	TestTrue(TEXT("0.3 s after the last: the cooldown"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::Cooldown);
	In.LastStrike = 100.0 - 0.59;
	TestTrue(TEXT("0.59 s after: still the cooldown"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::Cooldown);
	In.LastStrike = 100.0 - 0.61;
	TestTrue(TEXT("0.61 s after: it strikes"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::None);
	In.bSwinging = true;
	TestTrue(TEXT("Still swinging: no"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::Cooldown);

	In = Ready;
	In.bTraversing = true;
	TestTrue(TEXT("Mid-mantle or vault: no"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::Traversing);
	In = Ready;
	In.bInScene = true;
	TestTrue(TEXT("In a scene: no"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::Scene);
	In = Ready;
	In.bMenuOpen = true;
	TestTrue(TEXT("Behind a menu or paused: no"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::Menu);
	In = Ready;
	In.bAlive = false;
	TestTrue(TEXT("Dead (or nobody's): no"), FMeleeRules::WhyBlocked(In) == EMeleeBlock::NoPlayer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeStaggerTest, "Looter.Melee.Stagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeStaggerTest::RunTest(const FString& Parameters)
{
	const float Basic = FHitReaction::StaggerSecondsFor(ECreatureRank::Basic);
	const float MeleeStagger = Basic * FHitReaction::MeleeStaggerScale;
	TestTrue(TEXT("A strike's hit-stop is within 40-80 ms, like the others"), FHitReaction::MeleeHitStop >= 0.04f && FHitReaction::MeleeHitStop <= 0.08f);

	// A strike lands heavy whatever its share: a stagger half again as long as a heavy hit's, and a hit-stop.
	FHitReaction Reaction;
	const FHitReaction::FResponse First = Reaction.OnHit(1.0, 5.f, 100.f, false, false, ECreatureRank::Basic, /*bMelee*/ true);
	TestEqual(TEXT("A light strike staggers a Basic one"), First.StaggerSeconds, MeleeStagger);
	TestEqual(TEXT("...and stops it for an instant"), First.HitStopSeconds, FHitReaction::MeleeHitStop);
	// While the stagger runs another strike doesn't stack one, but still lands its hit-stop.
	const FHitReaction::FResponse During = Reaction.OnHit(1.2, 5.f, 100.f, false, false, ECreatureRank::Basic, true);
	TestEqual(TEXT("A strike during the stagger: no more stagger"), During.StaggerSeconds, 0.f);
	TestEqual(TEXT("...the hit-stop still"), During.HitStopSeconds, FHitReaction::MeleeHitStop);
	// After the stagger, inside its cooldown: a gun's crit waits it out, a strike doesn't (the strike's own cooldown rules).
	const double AfterStagger = 1.0 + MeleeStagger + 0.05;
	FHitReaction Copy = Reaction;
	TestEqual(TEXT("A crit in the cooldown: no stagger"), Copy.OnHit(AfterStagger, 5.f, 100.f, true, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
	TestEqual(TEXT("A strike in the cooldown: staggered"), Reaction.OnHit(AfterStagger, 5.f, 100.f, false, false, ECreatureRank::Basic, true).StaggerSeconds,
		MeleeStagger);
	// And a gun's hit right after a strike's stagger can't chain another.
	TestEqual(TEXT("A crit just after: none"), Reaction.OnHit(AfterStagger + MeleeStagger + 0.05, 5.f, 100.f, true, false, ECreatureRank::Basic).StaggerSeconds, 0.f);

	// Tougher ranks less; a Soulfed monster only the hit-stop; a boss nothing; the killing blow no stagger.
	{
		FHitReaction Rare;
		TestEqual(TEXT("Restless"), Rare.OnHit(0.0, 5.f, 100.f, false, false, ECreatureRank::Rare, true).StaggerSeconds,
			FHitReaction::StaggerSecondsFor(ECreatureRank::Rare) * FHitReaction::MeleeStaggerScale);
		FHitReaction Legendary;
		const FHitReaction::FResponse Soulfed = Legendary.OnHit(0.0, 5.f, 100.f, false, false, ECreatureRank::Legendary, true);
		TestTrue(TEXT("A Soulfed monster: the hit-stop, no stagger"), Soulfed.HitStopSeconds > 0.f && Soulfed.StaggerSeconds == 0.f);
		FHitReaction Boss;
		const FHitReaction::FResponse OnBoss = Boss.OnHit(0.0, 5.f, 100.f, false, false, ECreatureRank::Boss, true);
		TestTrue(TEXT("A boss: nothing"), OnBoss.HitStopSeconds == 0.f && OnBoss.StaggerSeconds == 0.f);
		FHitReaction Killed;
		const FHitReaction::FResponse Kill = Killed.OnHit(0.0, 100.f, 100.f, false, true, ECreatureRank::Basic, true);
		TestTrue(TEXT("The killing blow: the kill's hit-stop, no stagger"), Kill.HitStopSeconds >= FHitReaction::KillHitStop && Kill.StaggerSeconds == 0.f);
	}

	// The scope says "a strike" to code that isn't told (the creature's hit reactions hear the health's OnDamaged).
	{
		TestFalse(TEXT("No scope open"), FHitReaction::FMeleeScope::IsOpen());
		FHitReaction Scoped;
		{
			FHitReaction::FMeleeScope Strike;
			TestTrue(TEXT("A scope open"), FHitReaction::FMeleeScope::IsOpen());
			TestEqual(TEXT("A light hit in the scope is a strike's"), Scoped.OnHit(0.0, 5.f, 100.f, false, false, ECreatureRank::Basic).StaggerSeconds,
				MeleeStagger);
		}
		TestFalse(TEXT("Closed again"), FHitReaction::FMeleeScope::IsOpen());
		FHitReaction Plain;
		TestEqual(TEXT("Outside it, a light hit staggers nothing"), Plain.OnHit(0.0, 5.f, 100.f, false, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeKeyTest, "Looter.Melee.Key",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeKeyTest::RunTest(const FString& Parameters)
{
	// V by default (rebindable, "Melee" in Settings > Controls), the right stick's click on a gamepad.
	TestTrue(TEXT("Its binding is \"Melee\""), UKeyBindingSubsystem::MeleeBindingId() == FName(TEXT("Melee")));
	TestTrue(TEXT("V by default"), UKeyBindingSubsystem::DefaultMeleeKey() == EKeys::V);
	TestTrue(TEXT("The right stick's click on a gamepad"), UKeyBindingSubsystem::DefaultMeleeGamepadKey() == EKeys::Gamepad_RightThumbstick);

	// The mapping the settings menu rebinds has to be there. A fresh outer, so repeated runs never share object names.
	UInputMappingContext* Context = NewObject<UInputMappingContext>(NewObject<ULooterKeyBindingsSave>());
	const UInputAction* Action = UKeyBindingSubsystem::AddMeleeAction(Context, *Context);
	if (!TestNotNull(TEXT("The melee action"), Action))
	{
		return false;
	}
	for (const FKey& Key : { UKeyBindingSubsystem::DefaultMeleeKey(), UKeyBindingSubsystem::DefaultMeleeGamepadKey() })
	{
		TestTrue(FString::Printf(TEXT("Mapped to %s"), *Key.ToString()), Context->GetMappings().ContainsByPredicate([Action, &Key](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action == Action && Mapping.Key == Key;
		}));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeMotionTest, "Looter.Melee.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeMotionTest::RunTest(const FString& Parameters)
{
	using namespace MeleeMotion;
	const float Contact = FMeleeRules::ContactSeconds;

	// The gun starts and ends in its hold, and is at the strike pose as the blow lands.
	TestTrue(TEXT("In its hold at the start"), GunPose(0.f, false).Offset.IsNearlyZero() && GunPose(0.f, false).Rotation.IsNearlyZero());
	TestTrue(TEXT("In its hold at the end"), GunPose(FMeleeRules::SwingSeconds, false).Rotation.IsNearlyZero());
	TestTrue(TEXT("Nearly back just before the end"), FMath::Abs(GunPose(FMeleeRules::SwingSeconds - 0.01f, false).Rotation.Yaw) < 1.f);
	TestTrue(TEXT("At the strike pose as the blow lands"), GunPose(Contact, true).Rotation.Equals(StrikePose().Rotation, 0.01f));
	// The stock drives across: the muzzle swung right (the stock out left), after a coil the other way.
	TestTrue(TEXT("The strike swings the muzzle well right"), StrikePose().Rotation.Yaw > 30.f);
	TestTrue(TEXT("The coil turns it the other way"), CockedPose().Rotation.Yaw < 0.f);
	TestTrue(TEXT("The strike drives forward"), StrikePose().Offset.X > CockedPose().Offset.X);

	// A miss carries on past the strike; a landed blow stops dead and comes back.
	const float Through = Contact + FollowThroughSeconds;
	TestTrue(TEXT("A miss follows through past the strike"), GunPose(Through, false).Rotation.Yaw > StrikePose().Rotation.Yaw);
	TestTrue(TEXT("A landed blow doesn't"), GunPose(Through, true).Rotation.Yaw < StrikePose().Rotation.Yaw);

	// Snappy but never a pop: no more than 2.5 degrees or 1 cm in a millisecond.
	float WorstTurn = 0.f;
	float WorstMove = 0.f;
	for (const bool bLanded : { false, true })
	{
		FMeleeSwingPose Last = GunPose(0.f, bLanded);
		for (float Time = 0.001f; Time <= FMeleeRules::SwingSeconds + 0.01f; Time += 0.001f)
		{
			const FMeleeSwingPose Pose = GunPose(Time, bLanded);
			const FRotator Turn = Pose.Rotation - Last.Rotation;
			WorstTurn = FMath::Max(WorstTurn, FMath::Max3(FMath::Abs(Turn.Pitch), FMath::Abs(Turn.Yaw), FMath::Abs(Turn.Roll)));
			WorstMove = FMath::Max(WorstMove, static_cast<float>((Pose.Offset - Last.Offset).Size()));
			Last = Pose;
		}
	}
	TestTrue(FString::Printf(TEXT("No turn of over 2.5 degrees in a millisecond (%.2f)"), WorstTurn), WorstTurn < 2.5f);
	TestTrue(FString::Printf(TEXT("No move of over 1 cm in a millisecond (%.2f)"), WorstMove), WorstMove < 1.f);

	// The view: the coil one way, the lean into the blow the other; a landed blow jolts and punches in, a kill more.
	for (const bool bArmed : { true, false })
	{
		const FString Name = bArmed ? TEXT("Stock") : TEXT("Fist");
		TestTrue(Name + TEXT(": the coil and the blow turn opposite ways"), WindUp(bArmed).Yaw * Strike(bArmed).Yaw < 0.f);
		TestTrue(Name + TEXT(": the blow dips the view"), Strike(bArmed).Pitch < 0.f);
		// Played together, the kicks stay inside the stack's limits, so none is flattened.
		const TArray<FTimedKick> Kicks = { { WindUp(bArmed), 0.f }, { Strike(bArmed), Contact }, { Impact(true, 1.f), Contact } };
		const float Pitch = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.Pitch; });
		const float Yaw = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.Yaw; });
		const float Roll = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.Roll; });
		const float Widen = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.FieldOfView; });
		TestTrue(FString::Printf(TEXT("%s: pitch within the limit (%.2f)"), *Name, Pitch), Pitch <= FViewKickStack::MaxTurn);
		TestTrue(FString::Printf(TEXT("%s: yaw within the limit (%.2f)"), *Name, Yaw), Yaw <= FViewKickStack::MaxTurn * 0.67f);
		TestTrue(FString::Printf(TEXT("%s: roll within the limit (%.2f)"), *Name, Roll), Roll <= FViewKickStack::MaxTurn);
		TestTrue(FString::Printf(TEXT("%s: the field of view within the limit (%.3f)"), *Name, Widen), Widen <= FViewKickStack::MaxFieldOfView);
	}
	// A fist has no model: its punch has to read in the view alone.
	TestTrue(TEXT("The fist's punch pushes the view in at least 2%"), -Strike(false).FieldOfView >= 0.02f);
	TestTrue(TEXT("A landed blow punches in"), Impact(false, 1.f).FieldOfView < 0.f);
	TestTrue(TEXT("...a killing one harder"), Impact(true, 1.f).Pitch > Impact(false, 1.f).Pitch);
	TestTrue(TEXT("The jolt rolls the way it's leaned"), Impact(false, -1.f).Roll < 0.f && Impact(false, 1.f).Roll > 0.f);
	return true;
}

#endif
