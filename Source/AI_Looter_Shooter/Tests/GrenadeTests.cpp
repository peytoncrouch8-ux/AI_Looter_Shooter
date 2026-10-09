#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GraveSaltRules.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerThrowSave.h"
#include "Player/ViewKick.h"
#include "Progression/LevelRules.h"
#include "Settings/KeyBindingSubsystem.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"

// The grave-salt grenade's rules (Docs/Polish/BorderlandsComparison.md, item 16): the toss's arc, the bounce and roll,
// the fuse, the burst's damage and falloff, the count and its save, when a throw can start, its key and its motion.
// GrenadePlayTests.cpp has it in a level (flight, contact, sight, the knock, the character).

namespace
{
	constexpr float Gravity = -980.f;

	/**
	 * A grenade thrown from Start at Velocity onto flat ground at height 0 (its ball resting on it), stepped at 60 fps as
	 * AGraveSaltGrenade steps (the arc, Contact on the ground, rest), for at most Seconds. Returns where it ended.
	 */
	FVector RollOut(const FVector& Start, const FVector& Velocity, float Seconds, int32& OutHeardBounces, bool& bOutRested, float& OutFirstLanding)
	{
		const float Dt = 1.f / 60.f;
		const float Floor = FGraveSaltRules::CollisionRadius;
		FVector Location = Start;
		FVector Speed = Velocity;
		OutHeardBounces = 0;
		bOutRested = false;
		OutFirstLanding = -1.f;
		for (float Time = 0.f; Time < Seconds; Time += Dt)
		{
			FVector Next = Location;
			FVector NextSpeed = Speed;
			FGraveSaltRules::Fly(Next, NextSpeed, Dt, Gravity);
			if (Next.Z > Floor)
			{
				Location = Next;
				Speed = NextSpeed;
				continue;
			}
			if (OutFirstLanding < 0.f)
			{
				OutFirstLanding = static_cast<float>(FVector::Dist2D(Start, Next));
			}
			// On the ground this frame: where the arc crossed it, then off it.
			const float Into = -static_cast<float>(NextSpeed.Z);
			Location = FVector(Next.X, Next.Y, Floor);
			bool bRolled = false;
			Speed = FGraveSaltRules::Contact(NextSpeed, FVector::UpVector, Dt, bRolled);
			if (Into >= FGraveSaltRules::AudibleSpeed)
			{
				++OutHeardBounces;
			}
			if (FGraveSaltRules::ShouldRest(Speed, FVector::UpVector))
			{
				bOutRested = true;
				break;
			}
		}
		return Location;
	}

	/** A kick and when it starts (seconds into the throw). */
	struct FTimedKick
	{
		FViewKick Kick;
		float At = 0.f;
	};

	/** The most the kicks add up to over a second, before the stack's limits (Read picks which part). */
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeArcTest, "Looter.Grenade.Arc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeArcTest::RunTest(const FString& Parameters)
{
	// The toss: 15 m/s along the look, lifted 8 degrees, with half the thrower's speed.
	const FVector Level = FThrowRules::LaunchVelocity(FVector::ForwardVector, FVector::ZeroVector);
	TestEqual(TEXT("15 m/s standing still"), static_cast<float>(Level.Size()), FThrowRules::ThrowSpeed, 0.5f);
	const float Lift = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Level.Z), static_cast<float>(Level.Size2D())));
	TestEqual(TEXT("Lifted 8 degrees over the look"), Lift, FThrowRules::LiftDegrees, 0.05f);
	TestTrue(TEXT("Along the look"), Level.X > 0.0 && FMath::IsNearlyZero(Level.Y, 0.01));
	const FVector Aside = FThrowRules::LaunchVelocity(FVector::RightVector, FVector::ZeroVector);
	TestTrue(TEXT("The lift is the same whichever way the player faces"), FMath::IsNearlyEqual(Aside.Z, Level.Z, 0.5) && Aside.Y > 0.0);
	const FVector Running = FThrowRules::LaunchVelocity(FVector::ForwardVector, FVector(790.0, 0.0, 0.0));
	TestEqual(TEXT("Half a sprint's speed on top"), static_cast<float>(Running.X - Level.X), 395.f, 0.5f);
	TestTrue(TEXT("Straight up: thrown as it is"), FThrowRules::LaunchVelocity(FVector::UpVector, FVector::ZeroVector).Equals(FVector(0.0, 0.0, 1500.0), 0.5));

	// The arc is the exact fall, whatever the frame rate: a second in one step or sixty lands in the same place.
	FVector OneStep(0.0, 0.0, 140.0);
	FVector OneSpeed = Level;
	FGraveSaltRules::Fly(OneStep, OneSpeed, 1.f, Gravity);
	FVector Steps(0.0, 0.0, 140.0);
	FVector StepSpeed = Level;
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		FGraveSaltRules::Fly(Steps, StepSpeed, 1.f / 60.f, Gravity);
	}
	TestTrue(FString::Printf(TEXT("Frame rate doesn't move the arc (%s vs %s)"), *OneStep.ToCompactString(), *Steps.ToCompactString()), OneStep.Equals(Steps, 0.5));
	const FVector Expected = FVector(0.0, 0.0, 140.0) + Level + FVector(0.0, 0.0, 0.5 * Gravity);
	TestTrue(TEXT("...and is the ballistic one"), OneStep.Equals(Expected, 0.5));
	TestEqual(TEXT("Gravity slows the rise"), static_cast<float>(OneSpeed.Z), static_cast<float>(Level.Z) + Gravity, 0.5f);

	// From the eye, a level throw comes down 9-14 m out; at 45 degrees it carries about 23 m.
	int32 Bounces = 0;
	bool bRested = false;
	float Landing = 0.f;
	RollOut(FVector(0.0, 0.0, 140.0), Level, 6.f, Bounces, bRested, Landing);
	TestTrue(FString::Printf(TEXT("A level throw lands 9-14 m out (%.0f cm)"), Landing), Landing > 900.f && Landing < 1400.f);
	const FVector Lob = FThrowRules::LaunchVelocity(FRotator(45.f - FThrowRules::LiftDegrees, 0.f, 0.f).Vector(), FVector::ZeroVector);
	RollOut(FVector(0.0, 0.0, 140.0), Lob, 6.f, Bounces, bRested, Landing);
	TestTrue(FString::Printf(TEXT("A 45 degree lob carries 20-26 m (%.0f cm)"), Landing), Landing > 2000.f && Landing < 2600.f);

	// The hand: low and left of the eye, ahead of it.
	const FVector Hand = FThrowRules::ReleasePoint(FVector(0.0, 0.0, 140.0), FVector::ForwardVector);
	TestTrue(FString::Printf(TEXT("Released ahead, left and low (%s)"), *Hand.ToCompactString()), Hand.X > 0.0 && Hand.Y < 0.0 && Hand.Z < 140.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeBounceTest, "Looter.Grenade.Bounce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeBounceTest::RunTest(const FString& Parameters)
{
	// Into the ground at 45 degrees: back up at 30% of the speed into it, half the speed along it kept.
	const FVector Bounced = FGraveSaltRules::Bounce(FVector(500.0, 0.0, -500.0), FVector::UpVector);
	TestTrue(FString::Printf(TEXT("Off the ground (%s)"), *Bounced.ToCompactString()),
		Bounced.Equals(FVector(500.0 * FGraveSaltRules::SlideKeep, 0.0, 500.0 * FGraveSaltRules::Restitution), 0.5));
	TestTrue(TEXT("It loses speed every bounce"), Bounced.Size() < FVector(500.0, 0.0, -500.0).Size());
	// Off a wall: back the way it came, damped; a wall is never ground to roll on.
	const FVector Wall = FGraveSaltRules::Bounce(FVector(800.0, 0.0, 100.0), -FVector::ForwardVector);
	TestTrue(FString::Printf(TEXT("Back off a wall (%s)"), *Wall.ToCompactString()), Wall.X < 0.0
		&& FMath::IsNearlyEqual(Wall.X, -800.0 * FGraveSaltRules::Restitution, 0.5) && FMath::IsNearlyEqual(Wall.Z, 100.0 * FGraveSaltRules::SlideKeep, 0.5));
	bool bRolled = true;
	FGraveSaltRules::Contact(FVector(800.0, 0.0, -20.0), -FVector::ForwardVector, 1.f / 60.f, bRolled);
	TestFalse(TEXT("A wall always bounces it"), bRolled);
	TestTrue(TEXT("Leaving a surface already: untouched"), FGraveSaltRules::Bounce(FVector(0.0, 0.0, 50.0), FVector::UpVector).Equals(FVector(0.0, 0.0, 50.0)));

	// A hop too small to leave the ground is a roll: only its way along is left, slowed by the drag.
	const FVector Roll = FGraveSaltRules::Contact(FVector(300.0, 0.0, -100.0), FVector::UpVector, 0.1f, bRolled);
	TestTrue(TEXT("Rolls when the hop would be small"), bRolled);
	TestTrue(FString::Printf(TEXT("...along the ground, slowed (%s)"), *Roll.ToCompactString()), FMath::IsNearlyZero(Roll.Z, 0.01) && FMath::IsNearlyEqual(Roll.X, 300.0 - FGraveSaltRules::RollFriction * 0.1, 0.5));
	FGraveSaltRules::Contact(FVector(300.0, 0.0, -600.0), FVector::UpVector, 0.1f, bRolled);
	TestFalse(TEXT("A hard landing still hops"), bRolled);
	TestTrue(TEXT("Slow on the ground: it lies still"), FGraveSaltRules::ShouldRest(FVector(40.0, 0.0, 0.0), FVector::UpVector));
	TestFalse(TEXT("Slow against a wall: it doesn't"), FGraveSaltRules::ShouldRest(FVector(40.0, 0.0, 0.0), FVector::ForwardVector));

	// A real throw onto flat ground: two or three clinks, a short roll, and it lies still about as its fuse runs out, so
	// it bursts near where it was thrown, not skidding off into the next field.
	int32 Bounces = 0;
	bool bRested = false;
	float Landing = 0.f;
	const FVector Start(0.0, 0.0, 140.0);
	const FVector End = RollOut(Start, FThrowRules::LaunchVelocity(FVector::ForwardVector, FVector::ZeroVector), 4.f, Bounces, bRested, Landing);
	TestTrue(FString::Printf(TEXT("It comes to rest (after %d bounces)"), Bounces), bRested);
	TestTrue(FString::Printf(TEXT("2-4 bounces heard (%d)"), Bounces), Bounces >= 2 && Bounces <= 4);
	const float Rolled = static_cast<float>(FVector::Dist2D(Start, End)) - Landing;
	TestTrue(FString::Printf(TEXT("It skips and rolls on 2-6 m past where it first lands (%.0f cm)"), Rolled), Rolled > 200.f && Rolled < 600.f);
	int32 FuseBounces = 0;
	bool bRestedByFuse = false;
	float Unused = 0.f;
	const FVector AtFuse = RollOut(Start, FThrowRules::LaunchVelocity(FVector::ForwardVector, FVector::ZeroVector), FGraveSaltRules::FuseSeconds,
		FuseBounces, bRestedByFuse, Unused);
	TestTrue(FString::Printf(TEXT("By the fuse it's within a metre of where it stops (%.0f cm)"), FVector::Dist2D(AtFuse, End)), FVector::Dist2D(AtFuse, End) < 100.0);
	// Thrown down at the feet, it stays near.
	RollOut(Start, FThrowRules::LaunchVelocity(FRotator(-70.f, 0.f, 0.f).Vector(), FVector::ZeroVector), 4.f, Bounces, bRested, Landing);
	TestTrue(FString::Printf(TEXT("Thrown at the feet it lands within 2 m (%.0f cm)"), Landing), Landing < 200.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeFuseTest, "Looter.Grenade.Fuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeFuseTest::RunTest(const FString& Parameters)
{
	// 1.5 s from the hand; the throw's release comes well before the next throw can, and the throw is over first.
	TestEqual(TEXT("A 1.5 s fuse"), FGraveSaltRules::FuseSeconds, 1.5f);
	TestTrue(TEXT("The jar leaves the hand inside the throw"), FThrowRules::ReleaseSeconds > 0.f && FThrowRules::ReleaseSeconds < FThrowRules::ThrowSeconds);
	TestTrue(TEXT("The throw is over before the next can start"), FThrowRules::ThrowSeconds < FThrowRules::CooldownSeconds);
	// Three go in under two seconds when it all goes wrong; the first bursts before the last leaves the hand... nearly.
	const float ThreeThrown = 2.f * FThrowRules::CooldownSeconds + FThrowRules::ReleaseSeconds;
	TestTrue(FString::Printf(TEXT("All three thrown in under 2 s (%.2f)"), ThreeThrown), ThreeThrown < 2.f);
	TestEqual(TEXT("A 6 cm ball"), FGraveSaltRules::CollisionRadius, 6.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeDamageTest, "Looter.Grenade.Damage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeDamageTest::RunTest(const FString& Parameters)
{
	// 900 at the heart at level 1: one and a half Common rifle magazines (30 rounds of 20). +/-10% like every hit.
	TestEqual(TEXT("One and a half magazines at the heart"), FGraveSaltRules::BaseDamage, 1.5f * 30.f * 20.f);
	TestEqual(TEXT("900 at the heart"), FGraveSaltRules::BurstDamage(0.f, 1.f, 0.5f, false), 900.f, 0.01f);
	TestEqual(TEXT("...810 at the low end"), FGraveSaltRules::BurstDamage(0.f, 1.f, 0.f, false), 810.f, 0.01f);
	TestEqual(TEXT("...990 at the high end"), FGraveSaltRules::BurstDamage(0.f, 1.f, 1.f, false), 990.f, 0.01f);

	// Full within a metre, straight down to a fifth at 4 m, nothing past.
	TestEqual(TEXT("Full in the core"), FGraveSaltRules::Falloff(FGraveSaltRules::CoreRadius), 1.f);
	TestEqual(TEXT("A fifth at the radius"), FGraveSaltRules::Falloff(FGraveSaltRules::Radius), FGraveSaltRules::EdgeShare, 0.001f);
	TestEqual(TEXT("Nothing past it"), FGraveSaltRules::Falloff(FGraveSaltRules::Radius + 1.f), 0.f);
	TestEqual(TEXT("Nothing past it: no damage"), FGraveSaltRules::BurstDamage(FGraveSaltRules::Radius + 1.f, 1.f, 0.5f, false), 0.f);
	float Last = 2.f;
	bool bFalls = true;
	for (float Distance = 0.f; Distance <= FGraveSaltRules::Radius; Distance += 10.f)
	{
		const float Share = FGraveSaltRules::Falloff(Distance);
		bFalls &= Share <= Last + 1e-5f;
		Last = Share;
	}
	TestTrue(TEXT("It only ever falls with distance"), bFalls);

	// Salt burns the dead: half as much again.
	TestEqual(TEXT("The Unpaid take half again"), FGraveSaltRules::BurstDamage(200.f, 1.f, 0.5f, true) / FGraveSaltRules::BurstDamage(200.f, 1.f, 0.5f, false),
		FGraveSaltRules::UnpaidScale, 0.001f);

	// Growing with the player's level as enemies do, it always takes the same share at equal levels: at the heart a Basic
	// spider (300) and a Restless one go, a Gravebound spider (1500, a level up) is left about half; a Restless Unpaid goes
	// even at the edge.
	const FLevelRules Rules;
	for (const int32 Level : { 1, 10, 30, 70 })
	{
		const float Scale = Rules.EnemyScale(Level);
		const float Heart = FGraveSaltRules::BurstDamage(0.f, Scale, 0.5f, false);
		TestTrue(FString::Printf(TEXT("Level %d: a Basic spider dies at the heart"), Level), Heart >= 300.f * Scale);
		TestTrue(FString::Printf(TEXT("Level %d: a Restless spider dies at the heart"), Level), Heart >= 300.f * 2.5f * Rules.EnemyScale(Level + 1));
		const float Gravebound = 300.f * 5.f * Rules.EnemyScale(Level + 1);
		TestTrue(FString::Printf(TEXT("Level %d: a Gravebound spider lives, at about half"), Level), Heart < Gravebound && Heart > Gravebound * 0.4f);
		const float Edge = FGraveSaltRules::BurstDamage(FGraveSaltRules::Radius, Scale, 0.5f, true);
		TestTrue(FString::Printf(TEXT("Level %d: a Basic Unpaid dies even at the edge"), Level), Edge >= 160.f * Scale);
	}

	// A boss never takes more than 8% of its health from one burst, so three never take a quarter.
	TestEqual(TEXT("A boss's cap"), FGraveSaltRules::CapForBoss(5000.f, 6400.f), 6400.f * FGraveSaltRules::BossShareCap, 0.01f);
	TestEqual(TEXT("Under the cap: the damage as it was"), FGraveSaltRules::CapForBoss(100.f, 6400.f), 100.f, 0.01f);
	TestTrue(TEXT("Three bursts under a quarter"), 3.f * FGraveSaltRules::BossShareCap < 0.25f);

	// Measured to the body's nearest side: a big body is caught by its edge.
	FGraveSaltBody Body;
	Body.Center = FVector(300.0, 0.0, 60.0);
	Body.HalfHeight = 60.f;
	Body.Radius = 40.f;
	TestEqual(TEXT("To the side of a body"), FGraveSaltRules::DistanceToBody(FVector(0.0, 0.0, 60.0), Body), 260.f, 0.5f);
	TestEqual(TEXT("Inside it: 0"), FGraveSaltRules::DistanceToBody(FVector(300.0, 0.0, 60.0), Body), 0.f, 0.01f);
	TestEqual(TEXT("Under its rounded foot"), FGraveSaltRules::DistanceToBody(FVector(300.0, 0.0, -50.0), Body), 50.f, 0.5f);
	TestTrue(TEXT("The nearest point is on its skin"), FGraveSaltRules::NearestPoint(FVector(0.0, 0.0, 60.0), Body).Equals(FVector(260.0, 0.0, 60.0), 0.5));

	// The jolt by distance: full close by, gone at 15 m, only ever falling.
	TestEqual(TEXT("Full jolt close"), FGraveSaltRules::KickShare(100.f), 1.f);
	TestEqual(TEXT("None at 15 m"), FGraveSaltRules::KickShare(1500.f), 0.f);
	TestTrue(TEXT("Half way, well under half"), FGraveSaltRules::KickShare(850.f) < 0.5f && FGraveSaltRules::KickShare(850.f) > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeCountTest, "Looter.Grenade.Count",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeCountTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Three at most"), FThrowRules::MaxGrenades, 3);
	TestEqual(TEXT("Two to start"), FThrowRules::StartingGrenades, 2);
	int32 Taken = 0;
	TestEqual(TEXT("One found with two: three"), FThrowRules::Add(2, 1, &Taken), 3);
	TestEqual(TEXT("...one taken"), Taken, 1);
	TestEqual(TEXT("Three found with two: still three"), FThrowRules::Add(2, 3, &Taken), 3);
	TestEqual(TEXT("...only one taken (the rest stays on the ground)"), Taken, 1);
	TestEqual(TEXT("Full: none taken"), FThrowRules::Add(3, 1, &Taken), 3);
	TestEqual(TEXT("...zero"), Taken, 0);
	TestEqual(TEXT("Thrown: one less"), FThrowRules::Add(1, -1, &Taken), 0);
	TestEqual(TEXT("Never under none"), FThrowRules::Add(0, -1, &Taken), 0);

	// Loot: a kill pays by rank, twice as likely with none left, never when full; a boss always.
	TestTrue(TEXT("Tougher kills pay better"), FThrowRules::DropChance(ECreatureRank::Basic, 1) < FThrowRules::DropChance(ECreatureRank::Rare, 1)
		&& FThrowRules::DropChance(ECreatureRank::Rare, 1) < FThrowRules::DropChance(ECreatureRank::Epic, 1));
	TestEqual(TEXT("A boss always"), FThrowRules::DropChance(ECreatureRank::Boss, 0), 1.f);
	TestEqual(TEXT("Twice as likely with none"), FThrowRules::DropChance(ECreatureRank::Basic, 0), 2.f * FThrowRules::DropChance(ECreatureRank::Basic, 1), 0.0001f);
	TestEqual(TEXT("Never when full"), FThrowRules::DropChance(ECreatureRank::Boss, 3), 0.f);
	TestEqual(TEXT("The Strongbox always gives one"), FThrowRules::ChestChance(true, 2), 1.f);
	TestEqual(TEXT("A chest gives none when full"), FThrowRules::ChestChance(false, 3), 0.f);

	// What the session keeps: both fields are saved properties, and they come back as they went.
	const UScriptStruct* Struct = FThrowablesSave::StaticStruct();
	TestNotNull(TEXT("The count is a saved property"), FindFProperty<FIntProperty>(Struct, TEXT("Grenades")));
	TestNotNull(TEXT("The unlock is a saved property"), FindFProperty<FBoolProperty>(Struct, TEXT("bUnlocked")));
	FThrowablesSave Saved;
	Saved.Grenades = 1;
	Saved.bUnlocked = true;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	FObjectAndNameAsStringProxyArchive WriteProxy(Writer, false);
	FThrowablesSave::StaticStruct()->SerializeItem(WriteProxy, &Saved, nullptr);
	FThrowablesSave Read;
	FMemoryReader Reader(Bytes, true);
	FObjectAndNameAsStringProxyArchive ReadProxy(Reader, true);
	FThrowablesSave::StaticStruct()->SerializeItem(ReadProxy, &Read, nullptr);
	TestTrue(TEXT("Read back as saved"), Read.Grenades == 1 && Read.bUnlocked);
	const FThrowablesSave Fresh;
	TestTrue(TEXT("A save from before grenades: none, locked"), Fresh.Grenades == 0 && !Fresh.bUnlocked);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeGateTest, "Looter.Grenade.Gate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeGateTest::RunTest(const FString& Parameters)
{
	FThrowGateInput Ready;
	Ready.Now = 100.0;
	Ready.Grenades = 2;
	TestTrue(TEXT("Ready: it throws"), FThrowRules::WhyBlocked(Ready) == EThrowBlock::None);

	FThrowGateInput In = Ready;
	In.LastThrow = 100.0 - 0.5;
	TestTrue(TEXT("0.5 s after the last: the cooldown"), FThrowRules::WhyBlocked(In) == EThrowBlock::Cooldown);
	In.LastThrow = 100.0 - 0.81;
	TestTrue(TEXT("0.81 s after: it throws"), FThrowRules::WhyBlocked(In) == EThrowBlock::None);
	In.bThrowing = true;
	TestTrue(TEXT("Still throwing: no"), FThrowRules::WhyBlocked(In) == EThrowBlock::Cooldown);

	In = Ready;
	In.Grenades = 0;
	TestTrue(TEXT("None left: no"), FThrowRules::WhyBlocked(In) == EThrowBlock::Empty);
	In = Ready;
	In.bMeleeing = true;
	TestTrue(TEXT("Mid-strike: no"), FThrowRules::WhyBlocked(In) == EThrowBlock::Busy);
	In = Ready;
	In.bTraversing = true;
	TestTrue(TEXT("Mid-mantle or vault: no"), FThrowRules::WhyBlocked(In) == EThrowBlock::Traversing);
	In = Ready;
	In.bInScene = true;
	TestTrue(TEXT("In a scene: no"), FThrowRules::WhyBlocked(In) == EThrowBlock::Scene);
	In = Ready;
	In.bMenuOpen = true;
	TestTrue(TEXT("Behind a menu or paused: no"), FThrowRules::WhyBlocked(In) == EThrowBlock::Menu);
	In = Ready;
	In.bAlive = false;
	TestTrue(TEXT("Dead (or nobody's): no"), FThrowRules::WhyBlocked(In) == EThrowBlock::NoPlayer);
	// Out of grenades is the last thing weighed: a press behind a menu says nothing about the count.
	In = Ready;
	In.Grenades = 0;
	In.bMenuOpen = true;
	TestTrue(TEXT("Empty behind a menu: the menu"), FThrowRules::WhyBlocked(In) == EThrowBlock::Menu);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKeyTest, "Looter.Grenade.Key",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKeyTest::RunTest(const FString& Parameters)
{
	// G by default (rebindable, "Grenade" in Settings > Controls), the right bumper on a gamepad.
	TestTrue(TEXT("Its binding is \"Grenade\""), UKeyBindingSubsystem::GrenadeBindingId() == FName(TEXT("Grenade")));
	TestTrue(TEXT("G by default"), UKeyBindingSubsystem::DefaultGrenadeKey() == EKeys::G);
	TestTrue(TEXT("The right bumper on a gamepad"), UKeyBindingSubsystem::DefaultGrenadeGamepadKey() == EKeys::Gamepad_RightShoulder);
	// Drop weapon gave G up, and none of the character's other keys is the grenade's.
	TestTrue(TEXT("Drop weapon is off G"), UKeyBindingSubsystem::DefaultDropWeaponKey() != UKeyBindingSubsystem::DefaultGrenadeKey());
	TestTrue(TEXT("...the asset still maps it to G (the binding finds it there)"), UKeyBindingSubsystem::DropWeaponAssetKey() == EKeys::G);
	TestTrue(TEXT("Not the melee's key"), UKeyBindingSubsystem::DefaultMeleeKey() != UKeyBindingSubsystem::DefaultGrenadeKey()
		&& UKeyBindingSubsystem::DefaultMeleeGamepadKey() != UKeyBindingSubsystem::DefaultGrenadeGamepadKey());
	for (const FKey& Taken : { EKeys::Q, EKeys::F, EKeys::E, EKeys::R, EKeys::V, EKeys::G })
	{
		TestTrue(FString::Printf(TEXT("Drop weapon isn't on %s"), *Taken.ToString()), UKeyBindingSubsystem::DefaultDropWeaponKey() != Taken);
	}

	// The mapping the settings menu rebinds has to be there. A fresh outer, so repeated runs never share object names.
	UInputMappingContext* Context = NewObject<UInputMappingContext>(NewObject<ULooterKeyBindingsSave>());
	const UInputAction* Action = UKeyBindingSubsystem::AddGrenadeAction(Context, *Context);
	if (!TestNotNull(TEXT("The grenade action"), Action))
	{
		return false;
	}
	for (const FKey& Key : { UKeyBindingSubsystem::DefaultGrenadeKey(), UKeyBindingSubsystem::DefaultGrenadeGamepadKey() })
	{
		TestTrue(FString::Printf(TEXT("Mapped to %s"), *Key.ToString()), Context->GetMappings().ContainsByPredicate([Action, &Key](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action == Action && Mapping.Key == Key;
		}));
	}

	// The map's key, given out with the grenade's: M and down on the D-pad, clear of the grenade's, the melee's and the
	// inventory's (the View button), rebindable as "Map".
	TestTrue(TEXT("The map's binding is \"Map\""), UKeyBindingSubsystem::MapBindingId() == FName(TEXT("Map")));
	TestTrue(TEXT("The map on M"), UKeyBindingSubsystem::DefaultMapKey() == EKeys::M);
	TestTrue(TEXT("...and down on the D-pad"), UKeyBindingSubsystem::DefaultMapGamepadKey() == EKeys::Gamepad_DPad_Down);
	for (const FKey& Taken : { UKeyBindingSubsystem::DefaultGrenadeGamepadKey(), UKeyBindingSubsystem::DefaultMeleeGamepadKey(),
		EKeys::Gamepad_Special_Left, EKeys::Gamepad_Special_Right, EKeys::Gamepad_DPad_Up })
	{
		TestTrue(FString::Printf(TEXT("The map's pad key isn't %s"), *Taken.ToString()), UKeyBindingSubsystem::DefaultMapGamepadKey() != Taken);
	}
	UInputMappingContext* Global = NewObject<UInputMappingContext>(NewObject<ULooterKeyBindingsSave>());
	const UInputAction* MapAction = UKeyBindingSubsystem::AddMapAction(Global, *Global);
	if (TestNotNull(TEXT("The map action"), MapAction))
	{
		for (const FKey& Key : { UKeyBindingSubsystem::DefaultMapKey(), UKeyBindingSubsystem::DefaultMapGamepadKey() })
		{
			TestTrue(FString::Printf(TEXT("The map mapped to %s"), *Key.ToString()), Global->GetMappings().ContainsByPredicate([MapAction, &Key](const FEnhancedActionKeyMapping& Mapping)
			{
				return Mapping.Action == MapAction && Mapping.Key == Key;
			}));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeMotionTest, "Looter.Grenade.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeMotionTest::RunTest(const FString& Parameters)
{
	using namespace ThrowMotion;

	// The gun starts and ends in its hold, dipped down and away in between.
	TestTrue(TEXT("The gun in its hold at the start"), GunPose(0.f).Offset.IsNearlyZero() && GunPose(0.f).Rotation.IsNearlyZero());
	TestTrue(TEXT("...and at the end"), GunPose(FThrowRules::ThrowSeconds).Offset.IsNearlyZero());
	TestTrue(TEXT("Dipped at the release"), GunPose(FThrowRules::ReleaseSeconds).Offset.Equals(GunDipPose().Offset, 0.01));
	TestTrue(TEXT("The dip lowers it and tips the muzzle down"), GunDipPose().Offset.Z < 0.0 && GunDipPose().Rotation.Pitch < 0.f);

	// The jar: unseen at the start, seen as it's drawn and slung, gone as it leaves the hand.
	bool bShown = true;
	JarPose(0.f, bShown);
	TestFalse(TEXT("No jar before the press"), bShown);
	JarPose(DrawSeconds, bShown);
	TestTrue(TEXT("The jar drawn back"), bShown);
	JarPose(FThrowRules::ReleaseSeconds + 0.001f, bShown);
	TestFalse(TEXT("Gone once it leaves the hand"), bShown);
	TestTrue(TEXT("It comes up from under the view"), JarStartPose().Offset.Z < JarDrawnPose().Offset.Z);
	TestTrue(TEXT("Slung forward"), JarReleasePose().Offset.X > JarDrawnPose().Offset.X);
	TestTrue(TEXT("In the off hand: left of the middle"), JarDrawnPose().Offset.Y < 0.0 && JarReleasePose().Offset.Y < 0.0);

	// Snappy but never a pop: no more than 2.5 degrees or 1 cm in a millisecond, gun and jar alike.
	float WorstTurn = 0.f;
	float WorstMove = 0.f;
	FThrowPose LastGun = GunPose(0.f);
	bool bDummy = false;
	FThrowPose LastJar = JarPose(0.001f, bDummy);
	for (float Time = 0.001f; Time <= FThrowRules::ThrowSeconds + 0.01f; Time += 0.001f)
	{
		const FThrowPose Gun = GunPose(Time);
		const FRotator Turn = Gun.Rotation - LastGun.Rotation;
		WorstTurn = FMath::Max(WorstTurn, FMath::Max3(FMath::Abs(Turn.Pitch), FMath::Abs(Turn.Yaw), FMath::Abs(Turn.Roll)));
		WorstMove = FMath::Max(WorstMove, static_cast<float>((Gun.Offset - LastGun.Offset).Size()));
		LastGun = Gun;
		bool bSeen = false;
		const FThrowPose Jar = JarPose(Time, bSeen);
		if (bSeen)
		{
			const FRotator JarTurn = Jar.Rotation - LastJar.Rotation;
			WorstTurn = FMath::Max(WorstTurn, FMath::Max3(FMath::Abs(JarTurn.Pitch), FMath::Abs(JarTurn.Yaw), FMath::Abs(JarTurn.Roll)));
			WorstMove = FMath::Max(WorstMove, static_cast<float>((Jar.Offset - LastJar.Offset).Size()));
		}
		LastJar = Jar;
	}
	TestTrue(FString::Printf(TEXT("No turn of over 2.5 degrees in a millisecond (%.2f)"), WorstTurn), WorstTurn < 2.5f);
	TestTrue(FString::Printf(TEXT("No move of over 1 cm in a millisecond (%.2f)"), WorstMove), WorstMove < 1.f);

	// The view: drawn back, then leaning into the throw; a burst near by on top. Within the stack's limits together.
	TestTrue(TEXT("The draw and the release turn opposite ways"), WindUp().Yaw * Release().Yaw < 0.f);
	TestTrue(TEXT("The release dips the view"), Release().Pitch < 0.f);
	const TArray<FTimedKick> Kicks = { { WindUp(), 0.f }, { Release(), FThrowRules::ReleaseSeconds }, { Burst(1.f, 1.f), FThrowRules::ReleaseSeconds + 0.2f } };
	const float Pitch = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.Pitch; });
	const float Yaw = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.Yaw; });
	const float Roll = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.Roll; });
	const float Narrow = PeakSum(Kicks, [](const FViewKick& Kick) { return Kick.FieldOfView; });
	TestTrue(FString::Printf(TEXT("Pitch within the limit (%.2f)"), Pitch), Pitch <= FViewKickStack::MaxTurn);
	TestTrue(FString::Printf(TEXT("Yaw within the limit (%.2f)"), Yaw), Yaw <= FViewKickStack::MaxTurn * 0.67f);
	TestTrue(FString::Printf(TEXT("Roll within the limit (%.2f)"), Roll), Roll <= FViewKickStack::MaxTurn);
	TestTrue(FString::Printf(TEXT("Field of view within the limit (%.3f)"), Narrow), Narrow <= FViewKickStack::MaxFieldOfView);
	// The burst's jolt by distance: a far one is a nudge, none at all past the reach.
	TestTrue(TEXT("A far burst jolts less"), Burst(0.2f, 1.f).Pitch < Burst(1.f, 1.f).Pitch);
	TestEqual(TEXT("No jolt from one out of reach"), Burst(0.f, 1.f).Pitch, 0.f);
	TestTrue(TEXT("The burst's jolt rolls the way it's leaned"), Burst(1.f, -1.f).Roll < 0.f && Burst(1.f, 1.f).Roll > 0.f);
	return true;
}

#endif
