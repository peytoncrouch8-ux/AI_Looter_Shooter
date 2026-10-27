#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltGrenade.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/GraveSaltTuning.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureRankSettings.h"
#include "Player/PlayerThrowRules.h"
#include "Progression/LevelRules.h"
#include "Progression/ProgressionSettings.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The grave-salt grenade after the user's playtest (2026-10-27, note 7: "Grenades are too strong to start with,
// projectiles are too small and lacks explosion visuals"): the early strength and the starting count, the explosion it
// now draws (fire, ring, smoke, debris; on the ground or in the air), and the jar's new size.

namespace
{
	constexpr float BlastFrame = 1.f / 60.f;

	void RunBlast(FWeaponFX& Effects, float Seconds, const FVector& Camera)
	{
		for (float Time = 0.f; Time < Seconds; Time += BlastFrame)
		{
			Effects.Tick(BlastFrame, Camera);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeEarlyTest, "Looter.Grenade.Early",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeEarlyTest::RunTest(const FString& Parameters)
{
	const FGraveSaltTuning Tuning;
	// The game's own level growth, as creatures take it (ACreatureBase::ApplyStats).
	const FLevelRules Rules = GetDefault<UProgressionSettings>()->GetLevelRules();
	constexpr float CommonMagazine = 30.f * 20.f;

	// Weak at the start: 10% of its strength through level 3, then up on a curve to full at level 12, never past it.
	for (const int32 Level : { 0, 1, 2, 3 })
	{
		TestEqual(FString::Printf(TEXT("10%% at level %d"), Level), FGraveSaltRules::EarlyShare(Level, Tuning), 0.1f, 0.0001f);
	}
	TestTrue(TEXT("Slow at first: under a quarter at level 6"), FGraveSaltRules::EarlyShare(6, Tuning) < 0.25f);
	TestEqual(TEXT("Half at level 9"), FGraveSaltRules::EarlyShare(9, Tuning), 0.5f, 0.0001f);
	TestEqual(TEXT("Full at level 12"), FGraveSaltRules::EarlyShare(12, Tuning), 1.f, 0.0001f);
	TestEqual(TEXT("...and after"), FGraveSaltRules::EarlyShare(40, Tuning), 1.f, 0.0001f);
	float Last = 0.f;
	bool bGrows = true;
	for (int32 Level = 1; Level <= 15; ++Level)
	{
		const float Share = FGraveSaltRules::EarlyShare(Level, Tuning);
		bGrows &= Share >= Last - 1e-5f;
		Last = Share;
	}
	TestTrue(TEXT("It only ever grows with the level"), bGrows);
	TestEqual(TEXT("The thrower's scale: the level growth times the early share"), FGraveSaltRules::ThrowerScale(6, Rules.EnemyScale(6), Tuning),
		Rules.EnemyScale(6) * FGraveSaltRules::EarlyShare(6, Tuning), 0.0001f);
	FGraveSaltTuning Flat = Tuning;
	Flat.FullStrengthLevel = 1;
	TestEqual(TEXT("A full level of 1: full from the start"), FGraveSaltRules::EarlyShare(1, Flat), 1.f, 0.0001f);

	// At level 1 the heart is 90, a sixth of a Common rifle's magazine (30 x 20 = 600); it was 900, one and a half.
	const float Heart = FGraveSaltRules::BurstDamage(0.f, FGraveSaltRules::ThrowerScale(1, Rules.EnemyScale(1), Tuning), 0.5f, false, Tuning);
	TestEqual(TEXT("90 at the heart at level 1"), Heart, 90.f, 0.5f);
	TestTrue(FString::Printf(TEXT("A sixth of a Common magazine (%.0f of %.0f)"), Heart, CommonMagazine), Heart < CommonMagazine / 5.f);

	// The Basic Unpaid of Ransom's Rest at levels 1-3, with the health the game gives it (160 grown by its level, times
	// its tier's multiplier), against a player of its level: a jar never kills one outright, even at its heart at the top
	// roll; two metres out (a pack's middle) it leaves about a third; the gun finishes it.
	const float UnpaidBase = 160.f * UCreatureRankSettings::Get(ECreatureRank::Basic).HealthMultiplier;
	for (const int32 Level : { 1, 2, 3 })
	{
		const float Health = UnpaidBase * Rules.EnemyScale(Level);
		const float Scale = FGraveSaltRules::ThrowerScale(Level, Rules.EnemyScale(Level), Tuning);
		const float Core = FGraveSaltRules::BurstDamage(0.f, Scale, 1.f, true, Tuning);
		const float Near = FGraveSaltRules::BurstDamage(200.f, Scale, 0.5f, true, Tuning);
		TestTrue(FString::Printf(TEXT("Level %d: the heart at the top roll leaves it standing (%.0f of %.0f)"), Level, Core, Health), Core < Health);
		TestTrue(FString::Printf(TEXT("...but takes most of it (%.0f of %.0f)"), Core, Health), Core > Health * 0.6f);
		const float Left = 1.f - Near / Health;
		TestTrue(FString::Printf(TEXT("Level %d: two metres out it leaves about a third (%.0f%%)"), Level, Left * 100.f), Left > 0.25f && Left < 0.45f);
	}
	// A Basic spider (300) keeps most of itself; Ransom's Rest's top (level 7) is still well short of full.
	TestTrue(TEXT("A Basic spider at the heart keeps most of itself"), Heart * 1.1f < 300.f * 0.5f);
	TestTrue(TEXT("Ransom's Rest never sees it at full"), FGraveSaltRules::EarlyShare(7, Tuning) < 0.4f);

	// One jar to start (it was two), three carried at most.
	TestEqual(TEXT("One to start"), Tuning.StartingGrenades, 1);
	TestTrue(TEXT("...fewer than can be carried"), Tuning.StartingGrenades < FThrowRules::MaxGrenades);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeBlastTest, "Looter.Grenade.Blast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeBlastTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// A floor with its top at 0, and a wall standing on it.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(1000.0, 0.0, 150.0), FVector(100.0, 1000.0, 300.0));
	const FGraveSaltTuning Tuning;

	// The ground under it: a jar lying on the floor (its heart its ball's radius up) bursts on it; one high in the air
	// doesn't; one against a wall well off the floor is drawn in the air too.
	const FGraveSaltBlast Lying = GraveSaltBurst::FindGround(*World, FVector(0.0, 0.0, FGraveSaltRules::CollisionRadius), Tuning.GroundReach);
	TestTrue(TEXT("Lying on the floor: on the ground"), Lying.bOnGround);
	TestEqual(TEXT("...the ground under it"), static_cast<float>(Lying.Ground.Z), 0.f, 1.f);
	TestTrue(TEXT("...facing up"), Lying.Up.Z > 0.99);
	TestTrue(TEXT("...its surface known (dirt when it says nothing)"), Lying.Surface != EImpactMaterial::Unknown);
	const FGraveSaltBlast Lob = GraveSaltBurst::FindGround(*World, FVector(0.0, 0.0, 300.0), Tuning.GroundReach);
	TestFalse(TEXT("3 m up: in the air"), Lob.bOnGround);
	TestTrue(TEXT("...drawn where it is"), Lob.Ground.Equals(Lob.Center) && Lob.Surface == EImpactMaterial::Unknown);
	const FGraveSaltBlast OnWall = GraveSaltBurst::FindGround(*World, FVector(950.0 - FGraveSaltRules::CollisionRadius, 0.0, 200.0), Tuning.GroundReach);
	TestFalse(TEXT("On a wall 2 m up: in the air"), OnWall.bOnGround);

	// On the ground: the launcher's blast (fire rolling into a soot column), its rings and the salt on top; within the
	// pieces' budget (three in two seconds stay under Looter.Vfx.MaxParticles, 1000); all gone a few seconds on.
	const FVector Camera(-900.0, 0.0, 170.0);
	FWeaponFX Effects;
	Effects.Initialize(World);
	GraveSaltBurst::DrawBlast(Effects, Lying, Tuning);
	const int32 Grounded = Effects.NumParticles();
	const FVfxExplosionLook Look = VfxRules::ExplosionLook(Tuning.Radius);
	TestTrue(FString::Printf(TEXT("Fire and smoke: the core, the puffs and the column (%d flames)"), Effects.NumFlames()),
		Effects.NumFlames() >= Look.Fireballs + 2);
	TestTrue(FString::Printf(TEXT("The crack, the rings and the shock (%d shapes)"), Effects.NumShapes()), Effects.NumShapes() >= 4);
	TestTrue(FString::Printf(TEXT("A real explosion's worth of pieces (%d)"), Grounded), Grounded > 80);
	TestTrue(FString::Printf(TEXT("...within the budget: three under a third of the cap each (%d)"), Grounded), Grounded < 330);
	RunBlast(Effects, 7.f, Camera);
	TestEqual(TEXT("...gone a few seconds on"), Effects.NumParticles(), 0);

	// In the air: no ground thrown up, so fewer pieces; still fire and a ring facing the eye.
	GraveSaltBurst::DrawBlast(Effects, Lob, Tuning);
	TestTrue(FString::Printf(TEXT("In the air: no debris (%d vs %d)"), Effects.NumParticles(), Grounded), Effects.NumParticles() < Grounded);
	TestTrue(TEXT("...still fire"), Effects.NumFlames() >= Look.Fireballs);
	TestTrue(TEXT("...and its rings"), Effects.NumShapes() >= 3);
	RunBlast(Effects, 7.f, Camera);

	// The salt on top is its own knob; the whole look can be turned off for a test of cost.
	FGraveSaltTuning NoSalt = Tuning;
	NoSalt.SaltLayer = 0.f;
	GraveSaltBurst::DrawBlast(Effects, Lying, NoSalt);
	TestTrue(FString::Printf(TEXT("Without the salt: fewer pieces (%d vs %d)"), Effects.NumParticles(), Grounded), Effects.NumParticles() < Grounded);
	RunBlast(Effects, 7.f, Camera);
	FGraveSaltTuning NoLook = Tuning;
	NoLook.LookScale = 0.f;
	GraveSaltBurst::DrawBlast(Effects, Lying, NoLook);
	TestEqual(TEXT("A look scale of 0: nothing drawn"), Effects.NumParticles(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeJarSizeTest, "Looter.Grenade.JarSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeJarSizeTest::RunTest(const FString& Parameters)
{
	// The jar twice its first size (7.5 cm across, 12 to the fuse's top): 15 cm across, 24 to the fuse's top.
	TestEqual(TEXT("Its middle 10 cm up"), FGraveSaltRules::JarMiddle, 10.f);
	TestEqual(TEXT("15 cm across"), FGraveSaltRules::JarRadius * 2.f, 15.f);
	TestTrue(TEXT("The flight's ball between its half width and its middle"),
		FGraveSaltRules::CollisionRadius >= FGraveSaltRules::JarRadius && FGraveSaltRules::CollisionRadius <= FGraveSaltRules::JarMiddle);

	// The imported model (Art/Models/Throwables/SaltGrenade.py, models.ps1 -Only SaltGrenade) matches.
	UStaticMesh* Mesh = AGraveSaltGrenade::FindJarMesh();
	if (!Mesh)
	{
		AddWarning(TEXT("SM_SaltGrenade isn't imported: its size isn't checked."));
		return true;
	}
	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector Size = Bounds.GetSize();
	TestTrue(FString::Printf(TEXT("The model is about 15 cm across (%.1f x %.1f): reimported at its new size"), Size.X, Size.Y),
		Size.X > 14.0 && Size.X < 18.0 && Size.Y > 14.0 && Size.Y < 18.0);
	TestTrue(FString::Printf(TEXT("...and about 24 cm to the fuse's top (%.1f)"), Size.Z), Size.Z > 21.0 && Size.Z < 26.0);
	TestTrue(FString::Printf(TEXT("...standing on its pivot (foot at %.1f)"), Bounds.Min.Z), FMath::Abs(Bounds.Min.Z) < 1.0);
	TestNotNull(TEXT("...with its fuse socket"), Mesh->FindSocket(AGraveSaltGrenade::FuseSocket()));
	return true;
}

#endif
