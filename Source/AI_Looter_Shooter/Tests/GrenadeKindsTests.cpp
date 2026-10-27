#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GraveSaltRules.h"
#include "Combat/GraveSaltTuning.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureRankSettings.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerThrowSave.h"
#include "Progression/LevelRules.h"
#include "Progression/ProgressionSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"

// The grenade kinds' plain rules (round 4, the user's pick 2026-10-27: "a few kinds found as loot"): the table, each
// kind's burst against the salt jar's, the early ramp holding for every kind, the drop roll, the carry rule, the
// scatter's brood, the cinder's bed and the saved kind. The kinds in a test level: GrenadeKindsPlayTests.cpp.

namespace
{
	const EGrenadeKind AllKinds[] = { EGrenadeKind::Salt, EGrenadeKind::Pitch, EGrenadeKind::Scatter, EGrenadeKind::Cinder };
	const EGrenadeKind Specials[] = { EGrenadeKind::Pitch, EGrenadeKind::Scatter, EGrenadeKind::Cinder };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsTableTest, "Looter.Grenade.Kinds.Table",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsTableTest::RunTest(const FString& Parameters)
{
	TSet<FString> Names;
	TSet<FString> Tags;
	TSet<FString> Meshes;
	for (const EGrenadeKind Kind : AllKinds)
	{
		const FString Name = GrenadeKinds::Name(Kind).ToString();
		TestFalse(FString::Printf(TEXT("Kind %d has a name"), static_cast<int32>(Kind)), Name.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s has a HUD word"), *Name), GrenadeKinds::Tag(Kind).IsEmpty());
		TestFalse(FString::Printf(TEXT("%s says what it does"), *Name), GrenadeKinds::Blurb(Kind).IsEmpty());
		TestTrue(FString::Printf(TEXT("%s's model is in /Game/Art/Throwables"), *Name), FString(GrenadeKinds::MeshPath(Kind)).StartsWith(TEXT("/Game/Art/Throwables/SM_")));
		Names.Add(Name);
		Tags.Add(GrenadeKinds::Tag(Kind).ToString());
		Meshes.Add(GrenadeKinds::MeshPath(Kind));
		TestEqual(FString::Printf(TEXT("%s comes back from its saved byte"), *Name), GrenadeKinds::FromSaved(static_cast<uint8>(Kind)), Kind);
	}
	TestEqual(TEXT("Four kinds, four names"), Names.Num(), GrenadeKinds::Num);
	TestEqual(TEXT("...four words"), Tags.Num(), GrenadeKinds::Num);
	TestEqual(TEXT("...four tins"), Meshes.Num(), GrenadeKinds::Num);
	TestEqual(TEXT("The salt jar keeps its own tin"), FString(GrenadeKinds::MeshPath(EGrenadeKind::Salt)),
		FString(TEXT("/Game/Art/Throwables/SM_SaltGrenade.SM_SaltGrenade")));
	TestFalse(TEXT("Salt is the plain kind"), GrenadeKinds::IsSpecial(EGrenadeKind::Salt));
	for (const EGrenadeKind Kind : Specials)
	{
		TestTrue(FString::Printf(TEXT("%s is a special kind"), *GrenadeKinds::Name(Kind).ToString()), GrenadeKinds::IsSpecial(Kind));
	}
	TestEqual(TEXT("An unknown byte reads as salt"), GrenadeKinds::FromSaved(200), EGrenadeKind::Salt);
	TestEqual(TEXT("...and an old save's 0"), GrenadeKinds::FromSaved(0), EGrenadeKind::Salt);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsTuningTest, "Looter.Grenade.Kinds.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsTuningTest::RunTest(const FString& Parameters)
{
	const FGraveSaltTuning Base;
	const FGrenadeKindTuning Kinds;

	// The salt jar is as it was.
	const FGraveSaltTuning Salt = GrenadeKinds::BurstTuning(EGrenadeKind::Salt, Base, Kinds);
	TestTrue(TEXT("Salt: the jar's own numbers"), Salt.BaseDamage == Base.BaseDamage && Salt.Radius == Base.Radius
		&& Salt.CoreRadius == Base.CoreRadius && Salt.BossShareCap == Base.BossShareCap && Salt.KnockShare == Base.KnockShare);

	// Every kind: no heart over the salt jar's (a stronger kind would undo the early ramp: the user, 2026-10-27, "too
	// strong to start with"), a core inside its reach, no reach past the salt jar's (salt is the wide one).
	for (const EGrenadeKind Kind : Specials)
	{
		const FGraveSaltTuning Made = GrenadeKinds::BurstTuning(Kind, Base, Kinds);
		const FString Name = GrenadeKinds::Name(Kind).ToString();
		TestTrue(FString::Printf(TEXT("%s: never a bigger heart than salt (%.0f)"), *Name, Made.BaseDamage), Made.BaseDamage <= Base.BaseDamage + 0.01f);
		TestTrue(FString::Printf(TEXT("%s: a smaller reach than salt (%.0f cm)"), *Name, Made.Radius), Made.Radius < Base.Radius);
		TestTrue(FString::Printf(TEXT("%s: its core inside its reach"), *Name), Made.CoreRadius < Made.Radius);
		TestTrue(FString::Printf(TEXT("%s: the early ramp's numbers untouched"), *Name),
			Made.EarlyShare == Base.EarlyShare && Made.EarlyLevels == Base.EarlyLevels && Made.FullStrengthLevel == Base.FullStrengthLevel);
		TestEqual(FString::Printf(TEXT("%s: salt burns the dead the same"), *Name), Made.UnpaidScale, Base.UnpaidScale);
	}

	// Pitch: the full heart, a wider core, no knock (the tar holds them).
	const FGraveSaltTuning Pitch = GrenadeKinds::BurstTuning(EGrenadeKind::Pitch, Base, Kinds);
	TestEqual(TEXT("Pitch: the salt jar's heart"), Pitch.BaseDamage, Base.BaseDamage, 0.01f);
	TestTrue(TEXT("Pitch: a wider full-damage core than salt's"), Pitch.CoreRadius > Base.CoreRadius);
	TestEqual(TEXT("Pitch: no knock"), Pitch.KnockShare, 0.f);
	TestEqual(TEXT("Pitch: the whole cap (it bursts once)"), Pitch.BossShareCap, Base.BossShareCap);

	// A jar that bursts more than once splits a boss's cap: never more of its bar than the salt jar takes.
	const FGraveSaltTuning Scatter = GrenadeKinds::BurstTuning(EGrenadeKind::Scatter, Base, Kinds);
	const FGraveSaltTuning Brood = GrenadeKinds::BroodTuning(Base, Kinds);
	TestTrue(FString::Printf(TEXT("Scatter: its burst and its brood together within the cap (%.3f)"), Scatter.BossShareCap + Brood.BossShareCap * Kinds.ScatterCount),
		Scatter.BossShareCap + Brood.BossShareCap * Kinds.ScatterCount <= Base.BossShareCap + 1e-5f);
	const FGraveSaltTuning Cinder = GrenadeKinds::BurstTuning(EGrenadeKind::Cinder, Base, Kinds);
	TestTrue(TEXT("Cinder: its burst and its bed together within the cap"), Cinder.BossShareCap + GrenadeKinds::BedBossCap(Base) <= Base.BossShareCap + 1e-5f);
	TestEqual(TEXT("Cinder: no knock (they stay in the fire)"), Cinder.KnockShare, 0.f);
	TestEqual(TEXT("A small jar: no knock"), Brood.KnockShare, 0.f);
	TestTrue(TEXT("A small jar: weaker and smaller than its scatter jar"), Brood.BaseDamage < Scatter.BaseDamage && Brood.Radius < Scatter.Radius);

	// The looks: each kind its own fire; the brood a small pop with its own sound only.
	const FGrenadeBurstLook SaltLook = GrenadeKinds::LookOf(EGrenadeKind::Salt);
	TestTrue(TEXT("Salt's look: the salt burst alone, at full"), SaltLook.SaltVolume == 1.f && SaltLook.LayerCue == nullptr && !SaltLook.bSmall);
	for (const EGrenadeKind Kind : Specials)
	{
		const FGrenadeBurstLook Look = GrenadeKinds::LookOf(Kind);
		TestTrue(FString::Printf(TEXT("%s sounds of itself over the salt"), *GrenadeKinds::Name(Kind).ToString()), Look.LayerCue != nullptr && Look.SaltVolume > 0.f);
		TestFalse(FString::Printf(TEXT("%s burns in its own colour"), *GrenadeKinds::Name(Kind).ToString()), Look.Fire.Equals(SaltLook.Fire));
	}
	const FGrenadeBurstLook Pop = GrenadeKinds::LookOf(EGrenadeKind::Scatter, true);
	TestTrue(TEXT("A small jar's pop: small, its own crack only"), Pop.bSmall && Pop.SaltVolume == 0.f && Pop.LayerCue != nullptr);
	TestTrue(TEXT("...a nudge, not a jolt"), Pop.Kick < 1.f && Pop.Flash < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsEarlyTest, "Looter.Grenade.Kinds.Early",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsEarlyTest::RunTest(const FString& Parameters)
{
	// The early ramp holds for every kind: a jar of any kind at the player's levels 1-3 never kills a Basic Unpaid of
	// that level outright at its heart at the top roll (Looter.Grenade.Early's rule for salt), and the bed burning it
	// the whole of its life as well leaves the Unpaid most of a magazine's work.
	const FGraveSaltTuning Base;
	const FGrenadeKindTuning Kinds;
	const FLevelRules Rules = GetDefault<UProgressionSettings>()->GetLevelRules();
	const float UnpaidBase = 160.f * UCreatureRankSettings::Get(ECreatureRank::Basic).HealthMultiplier;
	for (const int32 Level : { 1, 2, 3 })
	{
		const float Health = UnpaidBase * Rules.EnemyScale(Level);
		const float Scale = FGraveSaltRules::ThrowerScale(Level, Rules.EnemyScale(Level), Base);
		for (const EGrenadeKind Kind : AllKinds)
		{
			const FGraveSaltTuning Made = GrenadeKinds::BurstTuning(Kind, Base, Kinds);
			const float Heart = FGraveSaltRules::BurstDamage(0.f, Scale, 1.f, true, Made);
			TestTrue(FString::Printf(TEXT("Level %d, %s: its heart at the top roll leaves an Unpaid standing (%.0f of %.0f)"), Level,
				*GrenadeKinds::Name(Kind).ToString(), Heart, Health), Heart < Health);
		}
		// The scatter's brood flies apart (2-3 m out, each reaching 2 m): one Unpaid takes its own burst and one small jar's at
		// most, both at their hearts at the top roll: still standing.
		const FGraveSaltTuning Scatter = GrenadeKinds::BurstTuning(EGrenadeKind::Scatter, Base, Kinds);
		const FGraveSaltTuning Brood = GrenadeKinds::BroodTuning(Base, Kinds);
		const float Both = FGraveSaltRules::BurstDamage(0.f, Scale, 1.f, true, Scatter) + FGraveSaltRules::BurstDamage(0.f, Scale, 1.f, true, Brood);
		TestTrue(FString::Printf(TEXT("Level %d: a scatter jar's burst and a small jar on one Unpaid's heart leave it standing (%.0f of %.0f)"),
			Level, Both, Health), Both < Health);
		// The cinder's burst at its heart and its bed all its life (the Unpaid resist fire: x0.5).
		const FGraveSaltTuning Cinder = GrenadeKinds::BurstTuning(EGrenadeKind::Cinder, Base, Kinds);
		const float Burned = FGraveSaltRules::BurstDamage(0.f, Scale, 0.5f, true, Cinder)
			+ GrenadeKinds::BedDamage(Kinds.BedSeconds, Scale, 0.5f, 0.5f, Base, Kinds);
		TestTrue(FString::Printf(TEXT("Level %d: a cinder jar and all its bed leave an Unpaid standing (%.0f of %.0f)"), Level, Burned, Health),
			Burned < Health);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsDropTest, "Looter.Grenade.Kinds.Drop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsDropTest::RunTest(const FString& Parameters)
{
	const FGrenadeKindTuning Kinds;
	// Below level 2 always salt: the tutorial teaches the plain jar.
	for (const float Roll : { 0.f, 0.1f, 0.5f, 0.99f })
	{
		TestEqual(FString::Printf(TEXT("Level 1, roll %.2f: salt"), Roll), GrenadeKinds::RollDrop(1, Roll, Roll, Kinds), EGrenadeKind::Salt);
	}
	// From level 2: a special tin when the roll is under the share, which one by the second roll; salt otherwise.
	TestEqual(TEXT("At the share: salt"), GrenadeKinds::RollDrop(2, Kinds.SpecialShare, 0.5f, Kinds), EGrenadeKind::Salt);
	TestEqual(TEXT("Over it: salt"), GrenadeKinds::RollDrop(5, 0.9f, 0.5f, Kinds), EGrenadeKind::Salt);
	TestEqual(TEXT("Under it, low: pitch"), GrenadeKinds::RollDrop(2, 0.f, 0.1f, Kinds), EGrenadeKind::Pitch);
	TestEqual(TEXT("...middling: scatter"), GrenadeKinds::RollDrop(2, 0.f, 0.5f, Kinds), EGrenadeKind::Scatter);
	TestEqual(TEXT("...high: cinder"), GrenadeKinds::RollDrop(2, 0.f, 0.9f, Kinds), EGrenadeKind::Cinder);
	TestEqual(TEXT("...the very top: cinder"), GrenadeKinds::RollDrop(2, 0.f, 1.f, Kinds), EGrenadeKind::Cinder);

	// Over many rolls: about the share are special, each kind about a third of them.
	int32 Counts[GrenadeKinds::Num] = {};
	FRandomStream Random(1234);
	constexpr int32 Rolls = 6000;
	for (int32 Index = 0; Index < Rolls; ++Index)
	{
		++Counts[static_cast<int32>(GrenadeKinds::RollDrop(4, Random.FRand(), Random.FRand(), Kinds))];
	}
	const float Special = static_cast<float>(Rolls - Counts[0]) / Rolls;
	TestTrue(FString::Printf(TEXT("About %.0f%% special (%.1f%%)"), Kinds.SpecialShare * 100.f, Special * 100.f), FMath::Abs(Special - Kinds.SpecialShare) < 0.03f);
	for (const EGrenadeKind Kind : Specials)
	{
		const float Share = static_cast<float>(Counts[static_cast<int32>(Kind)]) / FMath::Max(Rolls - Counts[0], 1);
		TestTrue(FString::Printf(TEXT("%s about a third of them (%.1f%%)"), *GrenadeKinds::Name(Kind).ToString(), Share * 100.f), FMath::Abs(Share - 1.f / 3.f) < 0.05f);
	}

	// A kind weighed at 0 never comes; all at 0, only salt.
	FGrenadeKindTuning NoPitch = Kinds;
	NoPitch.PitchWeight = 0.f;
	TestEqual(TEXT("No pitch: the low roll goes to scatter"), GrenadeKinds::RollDrop(3, 0.f, 0.f, NoPitch), EGrenadeKind::Scatter);
	FGrenadeKindTuning None = Kinds;
	None.PitchWeight = None.ScatterWeight = None.CinderWeight = 0.f;
	TestEqual(TEXT("None weighed: salt"), GrenadeKinds::RollDrop(3, 0.f, 0.5f, None), EGrenadeKind::Salt);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsCarryTest, "Looter.Grenade.Kinds.Carry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsCarryTest::RunTest(const FString& Parameters)
{
	constexpr int32 Max = FThrowRules::MaxGrenades;
	// A special tin: the kind carried now, the count kept and its jar added.
	FGrenadeCollect Made = GrenadeKinds::Collect(EGrenadeKind::Salt, 2, EGrenadeKind::Pitch, 1, Max);
	TestTrue(TEXT("Salt carried, a pitch tin found: pitch, three carried"), Made.Kind == EGrenadeKind::Pitch && Made.Count == 3 && Made.Taken == 1 && Made.bSwapped);
	// Salt fills any jar: it never changes the kind.
	Made = GrenadeKinds::Collect(EGrenadeKind::Pitch, 1, EGrenadeKind::Salt, 1, Max);
	TestTrue(TEXT("Pitch carried, a salt tin found: still pitch, one more"), Made.Kind == EGrenadeKind::Pitch && Made.Count == 2 && Made.Taken == 1 && !Made.bSwapped);
	// The same kind again: one more, no swap.
	Made = GrenadeKinds::Collect(EGrenadeKind::Cinder, 1, EGrenadeKind::Cinder, 1, Max);
	TestTrue(TEXT("Cinder carried, a cinder tin found: one more, no swap"), Made.Kind == EGrenadeKind::Cinder && Made.Count == 2 && !Made.bSwapped);
	// Full: none taken, but a new kind still swaps (and keeps the three).
	Made = GrenadeKinds::Collect(EGrenadeKind::Pitch, Max, EGrenadeKind::Scatter, 1, Max);
	TestTrue(TEXT("Full of pitch, a scatter tin: scatter, still three, none taken"), Made.Kind == EGrenadeKind::Scatter && Made.Count == Max && Made.Taken == 0 && Made.bSwapped);
	Made = GrenadeKinds::Collect(EGrenadeKind::Pitch, Max, EGrenadeKind::Salt, 1, Max);
	TestTrue(TEXT("Full of pitch, a salt tin: nothing changes"), Made.Kind == EGrenadeKind::Pitch && Made.Count == Max && Made.Taken == 0 && !Made.bSwapped);
	// None carried: the kind is still what a special tin says.
	Made = GrenadeKinds::Collect(EGrenadeKind::Salt, 0, EGrenadeKind::Cinder, 1, Max);
	TestTrue(TEXT("None carried, a cinder tin: one cinder jar"), Made.Kind == EGrenadeKind::Cinder && Made.Count == 1 && Made.bSwapped);
	// A tin of two at two carried: only what fits.
	Made = GrenadeKinds::Collect(EGrenadeKind::Salt, 2, EGrenadeKind::Salt, 2, Max);
	TestTrue(TEXT("Two found at two carried: one taken"), Made.Count == Max && Made.Taken == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsBroodTest, "Looter.Grenade.Kinds.Brood",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsBroodTest::RunTest(const FString& Parameters)
{
	const FGrenadeKindTuning Kinds;
	FRandomStream Random(77);
	TArray<FVector> Launches;
	GrenadeKinds::BroodVelocities(Kinds.ScatterCount, FVector::UpVector, Kinds.BroodSpeed, Kinds.BroodLift, Random, Launches);
	TestEqual(TEXT("The brood: as many as the count"), Launches.Num(), Kinds.ScatterCount);
	FVector Sum = FVector::ZeroVector;
	for (const FVector& Launch : Launches)
	{
		TestTrue(FString::Printf(TEXT("Each flung up (%s)"), *Launch.ToCompactString()), Launch.Z > Kinds.BroodLift * 0.8f);
		const float Across = static_cast<float>(FVector2D(Launch.X, Launch.Y).Size());
		TestTrue(FString::Printf(TEXT("...and out (%.0f cm/s)"), Across), Across > Kinds.BroodSpeed * 0.8f && Across < Kinds.BroodSpeed * 1.2f);
		Sum += FVector(Launch.X, Launch.Y, 0.0);
	}
	// Spread round evenly: their ways across cancel out, near enough.
	TestTrue(FString::Printf(TEXT("Spread all round, not to one side (%.0f left over)"), Sum.Size()), Sum.Size() < Kinds.BroodSpeed * 1.2f);
	// Off a wall facing +X: flung out of the wall, across its face.
	GrenadeKinds::BroodVelocities(3, FVector::ForwardVector, 500.f, 300.f, Random, Launches);
	TestTrue(TEXT("Off a wall: out of it"), Launches.Num() == 3 && Launches[0].X > 0.0 && Launches[1].X > 0.0 && Launches[2].X > 0.0);
	GrenadeKinds::BroodVelocities(0, FVector::UpVector, 500.f, 300.f, Random, Launches);
	TestEqual(TEXT("None asked: none"), Launches.Num(), 0);

	// Their fuses: within the tuning's, each later than the last.
	float Last = 0.f;
	bool bLater = true;
	for (int32 Index = 0; Index < Kinds.ScatterCount; ++Index)
	{
		const float Fuse = GrenadeKinds::BroodFuse(Index, Kinds.ScatterCount, 0.99f, Kinds);
		const float Early = GrenadeKinds::BroodFuse(Index, Kinds.ScatterCount, 0.f, Kinds);
		TestTrue(FString::Printf(TEXT("Small jar %d's fuse within %.2f-%.2f s (%.2f)"), Index, Kinds.BroodFuseMin, Kinds.BroodFuseMax, Fuse),
			Fuse >= Kinds.BroodFuseMin && Fuse <= Kinds.BroodFuseMax);
		bLater &= Early >= Last - 1e-4f;
		Last = Fuse;
	}
	TestTrue(TEXT("Pop, pop, pop: each later than the one before"), bLater);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsBedTest, "Looter.Grenade.Kinds.Bed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsBedTest::RunTest(const FString& Parameters)
{
	const FGraveSaltTuning Base;
	const FGrenadeKindTuning Kinds;
	// A second at full strength: the share of the salt jar's heart; twice as long, twice as much; nothing for nothing.
	const float Second = GrenadeKinds::BedDamage(1.f, 1.f, 1.f, 0.5f, Base, Kinds);
	TestEqual(TEXT("A second's burn at full strength"), Second, Base.BaseDamage * Kinds.BedShare, 0.5f);
	TestEqual(TEXT("Two seconds: twice"), GrenadeKinds::BedDamage(2.f, 1.f, 1.f, 0.5f, Base, Kinds), Second * 2.f, 0.5f);
	TestEqual(TEXT("No time, no burn"), GrenadeKinds::BedDamage(0.f, 1.f, 1.f, 0.5f, Base, Kinds), 0.f);
	// The thrower's level's share (the early ramp), and the weakness table's.
	TestEqual(TEXT("At a tenth of the strength: a tenth"), GrenadeKinds::BedDamage(1.f, 0.1f, 1.f, 0.5f, Base, Kinds), Second * 0.1f, 0.1f);
	TestTrue(TEXT("Weak to fire: more; resisting: less"), GrenadeKinds::BedDamage(1.f, 1.f, 1.75f, 0.5f, Base, Kinds) > Second
		&& GrenadeKinds::BedDamage(1.f, 1.f, 0.5f, 0.5f, Base, Kinds) < Second);
	TestTrue(TEXT("Within the +/-10% spread"), GrenadeKinds::BedDamage(1.f, 1.f, 1.f, 1.f, Base, Kinds) <= Second * 1.1f + 0.5f
		&& GrenadeKinds::BedDamage(1.f, 1.f, 1.f, 0.f, Base, Kinds) >= Second * 0.9f - 0.5f);
	// A boss: never past what's left of its cap.
	TestEqual(TEXT("Under the cap: all of it"), GrenadeKinds::CapLeft(50.f, 0.f, 10000.f, 0.04f), 50.f);
	TestEqual(TEXT("Near it: what's left"), GrenadeKinds::CapLeft(50.f, 380.f, 10000.f, 0.04f), 20.f);
	TestEqual(TEXT("At it: nothing"), GrenadeKinds::CapLeft(50.f, 400.f, 10000.f, 0.04f), 0.f);
	// The bed's whole life at full strength is a real burn but no salt jar's heart (it's for what stays in it).
	const float Life = GrenadeKinds::BedDamage(Kinds.BedSeconds, 1.f, 1.f, 0.5f, Base, Kinds);
	TestTrue(FString::Printf(TEXT("All its life at full: %.0f, under the salt jar's heart"), Life), Life < Base.BaseDamage && Life > Base.BaseDamage * 0.2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsSaveTest, "Looter.Grenade.Kinds.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsSaveTest::RunTest(const FString& Parameters)
{
	// The kind rides in the session's save beside the count, and comes back as it went.
	FThrowablesSave Saved;
	Saved.Grenades = 2;
	Saved.bUnlocked = true;
	Saved.Kind = static_cast<uint8>(EGrenadeKind::Cinder);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	FObjectAndNameAsStringProxyArchive WriteProxy(Writer, false);
	FThrowablesSave::StaticStruct()->SerializeItem(WriteProxy, &Saved, nullptr);
	FThrowablesSave Read;
	FMemoryReader Reader(Bytes, true);
	FObjectAndNameAsStringProxyArchive ReadProxy(Reader, true);
	FThrowablesSave::StaticStruct()->SerializeItem(ReadProxy, &Read, nullptr);
	TestTrue(TEXT("Saved and read: two cinder jars"), Read.Grenades == 2 && Read.bUnlocked && GrenadeKinds::FromSaved(Read.Kind) == EGrenadeKind::Cinder);
	TestNotNull(TEXT("The kind is a saved field"), FindFProperty<FProperty>(FThrowablesSave::StaticStruct(), TEXT("Kind")));
	const FThrowablesSave Fresh;
	TestEqual(TEXT("A save without it (an old one): salt"), GrenadeKinds::FromSaved(Fresh.Kind), EGrenadeKind::Salt);
	return true;
}

#endif
