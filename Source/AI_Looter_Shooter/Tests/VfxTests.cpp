#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureKillLook.h"
#include "Embers/Powers/EmberVfx.h"
#include "Weapons/DeathSplats.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The VFX pass (Saved/Vfx/PLAN.md): its plain rules (detail by distance and in a stream, kill weights, tiers, muzzles, the
// shapes' curves, blasts, splats) read with the cvars' defaults (a default-made FVfxTuning), never what the user has set; and
// the code-drawn effects' new pieces spawned in a test world (shapes alive and gone in time, kills by kind, blasts, muzzles,
// a stream thinned). Looks are judged in play, not here.

namespace
{
	constexpr float VfxFrame = 1.f / 60.f;

	void RunEffects(FWeaponFX& Effects, float Seconds, const FVector& Camera)
	{
		for (float Time = 0.f; Time < Seconds; Time += VfxFrame)
		{
			Effects.Tick(VfxFrame, Camera);
		}
	}

	FVfxKill TestKill(ECreatureBlood Blood, ECreatureRank Rank = ECreatureRank::Basic, EWeaponKind Gun = EWeaponKind::Rifle, bool bCritical = false)
	{
		FVfxKill Kill;
		Kill.Blood = Blood;
		Kill.Tint = CreatureBlood::DefaultTint(Blood);
		Kill.Spill = Kill.Tint;
		Kill.Center = FVector(0.0, 0.0, 60.0);
		Kill.Ground = FVector::ZeroVector;
		Kill.GroundNormal = FVector::UpVector;
		Kill.bGround = true;
		Kill.Direction = FVector(1.0, 0.0, -0.1);
		Kill.HitAt = FVector(-20.0, 0.0, 70.0);
		Kill.Rank = Rank;
		Kill.TierColor = FLinearColor(0.3f, 0.5f, 1.f);
		Kill.Gun = Gun;
		Kill.bCritical = bCritical;
		Kill.Share = 0.4f;
		return Kill;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxDetailTest, "Looter.Vfx.Rules.Detail", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxDetailTest::RunTest(const FString& Parameters)
{
	const FVfxTuning Tuning;
	TestEqual(TEXT("Full detail up close"), VfxRules::DetailAt(0.f, Tuning), 1.f);
	TestEqual(TEXT("...and to the near edge"), VfxRules::DetailAt(Tuning.DetailNear, Tuning), 1.f);
	TestEqual(TEXT("The least past the far edge"), VfxRules::DetailAt(Tuning.DetailFar * 2.f, Tuning), Tuning.DetailMin, 0.001f);
	float Last = 1.f;
	bool bFalls = true;
	for (float Distance = Tuning.DetailNear; Distance <= Tuning.DetailFar; Distance += 250.f)
	{
		const float Detail = VfxRules::DetailAt(Distance, Tuning);
		bFalls &= Detail <= Last + 0.0001f;
		Last = Detail;
	}
	TestTrue(TEXT("It only falls with distance"), bFalls);

	TestEqual(TEXT("Count: none at no amount"), VfxRules::Count(8, 0.f), 0);
	TestEqual(TEXT("...at least one at any"), VfxRules::Count(8, 0.01f), 1);
	TestEqual(TEXT("...rounded"), VfxRules::Count(8, 0.5f), 4);
	TestEqual(TEXT("...none of none"), VfxRules::Count(0, 1.f), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxStreamTest, "Looter.Vfx.Rules.Stream", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxStreamTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A lone hit in full"), VfxRules::StreamDetail(1), 1.f);
	TestEqual(TEXT("...a pair too"), VfxRules::StreamDetail(2), 1.f);
	TestTrue(TEXT("The third thinner"), VfxRules::StreamDetail(3) < 1.f);
	TestTrue(TEXT("...the tenth thinner still"), VfxRules::StreamDetail(10) < VfxRules::StreamDetail(3));
	TestEqual(TEXT("...never under a quarter"), VfxRules::StreamDetail(1000), 0.25f);

	const FVfxTuning Tuning;
	FVfxHitStream Stream;
	Stream.Reset();
	TestEqual(TEXT("The first hit is alone"), Stream.Note(FVector::ZeroVector, 10.0, Tuning.StreamWindow, Tuning.StreamReach), 1);
	TestEqual(TEXT("A second close by counts the first"), Stream.Note(FVector(30.0, 0.0, 0.0), 10.05, Tuning.StreamWindow, Tuning.StreamReach), 2);
	TestEqual(TEXT("One far off is alone"), Stream.Note(FVector(5000.0, 0.0, 0.0), 10.1, Tuning.StreamWindow, Tuning.StreamReach), 1);
	TestEqual(TEXT("One long after is alone"), Stream.Note(FVector::ZeroVector, 20.0, Tuning.StreamWindow, Tuning.StreamReach), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxKillRulesTest, "Looter.Vfx.Rules.Kills", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxKillRulesTest::RunTest(const FString& Parameters)
{
	const float Rifle = VfxRules::KillWeight(EWeaponKind::Rifle, false, 0.3f);
	TestEqual(TEXT("A rifle's body kill is the measure"), Rifle, 1.f);
	TestTrue(TEXT("A shotgun's kill throws more"), VfxRules::KillWeight(EWeaponKind::Shotgun, false, 0.3f) > Rifle);
	TestTrue(TEXT("...a rocket's most of the guns"), VfxRules::KillWeight(EWeaponKind::Launcher, false, 0.3f) > VfxRules::KillWeight(EWeaponKind::Shotgun, false, 0.3f));
	TestTrue(TEXT("...an SMG's a little less"), VfxRules::KillWeight(EWeaponKind::SMG, false, 0.3f) < Rifle);
	TestTrue(TEXT("A crit more"), VfxRules::KillWeight(EWeaponKind::Rifle, true, 0.3f) > Rifle);
	TestTrue(TEXT("Overkill more"), VfxRules::KillWeight(EWeaponKind::Rifle, false, 3.f) > Rifle);
	TestTrue(TEXT("...but at most 0.3 more"), VfxRules::KillWeight(EWeaponKind::Rifle, false, 100.f) <= Rifle + 0.3f + 0.001f);
	TestTrue(TEXT("Never past the cap"), VfxRules::KillWeight(EWeaponKind::Launcher, true, 100.f) <= 2.2f);
	TestTrue(TEXT("A shotgun carries its gore along the shot most"), VfxRules::KillCarry(EWeaponKind::Shotgun) > VfxRules::KillCarry(EWeaponKind::Rifle));
	TestTrue(TEXT("Heavy rounds: the revolver's and the sniper's"), VfxRules::IsHeavyRound(1.25f) && VfxRules::IsHeavyRound(1.5f) && !VfxRules::IsHeavyRound(1.f));
	TestTrue(TEXT("A pellet's pop is smaller than a rifle round's, a sniper's bigger"),
		VfxRules::HitWeight(0.6f) < VfxRules::HitWeight(1.f) && VfxRules::HitWeight(1.5f) > VfxRules::HitWeight(1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxTierRulesTest, "Looter.Vfx.Rules.Tiers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxTierRulesTest::RunTest(const FString& Parameters)
{
	const FVfxTierLayers Regular = VfxRules::TierLayers(ECreatureRank::Basic);
	TestTrue(TEXT("A Regular adds nothing"), Regular.Rings == 0 && Regular.Star == 0.f && Regular.Motes == 0);
	const FVfxTierLayers Restless = VfxRules::TierLayers(ECreatureRank::Rare);
	const FVfxTierLayers Gravebound = VfxRules::TierLayers(ECreatureRank::Epic);
	TestEqual(TEXT("Restless: a ring"), Restless.Rings, 1);
	TestTrue(TEXT("Gravebound: more than Restless"), Gravebound.Rings > Restless.Rings && Gravebound.Star > Restless.Star && Gravebound.Motes > Restless.Motes);
	for (const ECreatureRank Top : { ECreatureRank::Hallowed, ECreatureRank::Ethereal })
	{
		const FVfxTierLayers Layers = VfxRules::TierLayers(Top);
		TestTrue(TEXT("The top tiers keep to a star under their own show"), Layers.bOwnShow && Layers.Rings == 0 && Layers.Star > 0.f);
	}
	TestEqual(TEXT("A boss's death is its own show"), VfxRules::TierLayers(ECreatureRank::Boss).Rings, 0);
	TestTrue(TEXT("A Soulfed monster's rings reach farthest"), VfxRules::TierLayers(ECreatureRank::Legendary).RingReach > Gravebound.RingReach);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxMuzzleRulesTest, "Looter.Vfx.Rules.Muzzles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxMuzzleRulesTest::RunTest(const FString& Parameters)
{
	for (const EWeaponKind Kind : { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper,
		EWeaponKind::LMG, EWeaponKind::Launcher })
	{
		const FVfxMuzzleLook Look = VfxRules::MuzzleLook(Kind);
		TestTrue(FString::Printf(TEXT("Kind %d: a flash with a size, spikes and a length of time"), static_cast<int32>(Kind)),
			Look.CoreSize > 0.f && Look.StarSize > Look.CoreSize && Look.StarSpikes >= 3 && Look.Duration > 0.f && Look.Every >= 1);
	}
	const FVfxMuzzleLook Rifle = VfxRules::MuzzleLook(EWeaponKind::Rifle);
	const FVfxMuzzleLook Sniper = VfxRules::MuzzleLook(EWeaponKind::Sniper);
	const FVfxMuzzleLook Smg = VfxRules::MuzzleLook(EWeaponKind::SMG);
	TestTrue(TEXT("The SMG's is small and quick, the sniper's big"), Smg.StarSize < Rifle.StarSize && Sniper.StarSize > Rifle.StarSize
		&& Smg.Duration < Rifle.Duration);
	TestTrue(TEXT("The sniper's brake blows side jets and a ring"), Sniper.SideJets > 0.f && Sniper.Ring > 0.f);
	TestTrue(TEXT("The revolver spits from its cylinder"), VfxRules::MuzzleLook(EWeaponKind::Revolver).GapSpit > 0.f);
	TestTrue(TEXT("The fast guns thin their sparks and smoke"), Smg.Every > 1 && VfxRules::MuzzleLook(EWeaponKind::LMG).Every > 1);
	TestTrue(TEXT("The launcher's ring is the widest"), VfxRules::MuzzleLook(EWeaponKind::Launcher).Ring > Sniper.Ring);
	TestTrue(TEXT("The shotgun's tongues are the widest of the guns"), VfxRules::MuzzleLook(EWeaponKind::Shotgun).TongueWidth > Rifle.TongueWidth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxShapeRulesTest, "Looter.Vfx.Rules.Shapes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxShapeRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A ring starts at its start"), VfxRules::RingGrowth(0.f), 0.f);
	TestEqual(TEXT("...and ends at its end"), VfxRules::RingGrowth(1.f), 1.f);
	TestTrue(TEXT("...most of the way out by half its life"), VfxRules::RingGrowth(0.5f) > 0.8f);
	TestEqual(TEXT("A ring at full strength at once"), VfxRules::RingFade(0.f), 1.f);
	TestEqual(TEXT("...gone at the end"), VfxRules::RingFade(1.f), 0.f);
	TestEqual(TEXT("A fireball white-hot at first"), VfxRules::FireballHeat(0.f), 1.f);
	TestEqual(TEXT("...cold at the end"), VfxRules::FireballHeat(1.f), 0.f);
	TestTrue(TEXT("...past half hot at a quarter"), VfxRules::FireballHeat(0.25f) > 0.5f);
	const FLinearColor Orange(1.f, 0.4f, 0.1f);
	TestTrue(TEXT("Its heat colour: its own at half"), VfxRules::HeatColor(Orange, 0.5f).Equals(Orange, 0.001f));
	TestTrue(TEXT("...whiter hotter"), VfxRules::HeatColor(Orange, 1.f).B > Orange.B);
	TestTrue(TEXT("...darker cooler"), VfxRules::HeatColor(Orange, 0.f).R < Orange.R);
	TestEqual(TEXT("A small ring's stand-in: 10 pieces"), VfxRules::RingPieces(20.f), 10);
	TestEqual(TEXT("...a huge one's: 28"), VfxRules::RingPieces(5000.f), 28);

	const FVfxExplosionLook Small = VfxRules::ExplosionLook(150.f);
	const FVfxExplosionLook Rocket = VfxRules::ExplosionLook(350.f);
	const FVfxExplosionLook Huge = VfxRules::ExplosionLook(5000.f);
	TestTrue(TEXT("A bigger blast has more fireballs and embers"), Rocket.Fireballs >= Small.Fireballs && Rocket.Embers > Small.Embers);
	TestTrue(TEXT("...never past the caps"), Huge.Fireballs <= 8 && Huge.Embers <= 32 && Huge.SmokeRing <= 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxSplatRulesTest, "Looter.Vfx.Rules.Splats", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxSplatRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A splat starts as a first gout"), FDeathSplats::SpreadAt(0.f, 1.f) > 0.f && FDeathSplats::SpreadAt(0.f, 1.f) < 0.5f);
	TestEqual(TEXT("...and is spread by its time"), FDeathSplats::SpreadAt(1.f, 1.f), 1.f);
	TestEqual(TEXT("...a scorch is there at once"), FDeathSplats::SpreadAt(0.f, 0.f), 1.f);
	TestTrue(TEXT("...seeping out fast, then slowly"), FDeathSplats::SpreadAt(0.5f, 1.f) > 0.7f);
	const FLinearColor Green(0.2f, 0.8f, 0.3f);
	TestTrue(TEXT("Ichor and gel are wet, ash and scorch dry"), FDeathSplats::LookFor(EVfxSplat::Ichor, Green).Wet > 0.5f
		&& FDeathSplats::LookFor(EVfxSplat::Gel, Green).Wet > 0.5f && FDeathSplats::LookFor(EVfxSplat::Ash, Green).Wet == 0.f
		&& FDeathSplats::LookFor(EVfxSplat::Scorch, Green).Wet == 0.f);
	TestTrue(TEXT("A gel puddle takes the slime's colour"), FDeathSplats::LookFor(EVfxSplat::Gel, Green).Color.G > FDeathSplats::LookFor(EVfxSplat::Gel, Green).Color.R);
	TestTrue(TEXT("Each kind leaves its own"), CreatureKillLook::SplatFor(ECreatureBlood::Ichor) == EVfxSplat::Ichor
		&& CreatureKillLook::SplatFor(ECreatureBlood::Gel) == EVfxSplat::Gel && CreatureKillLook::SplatFor(ECreatureBlood::Embers) == EVfxSplat::Ash
		&& CreatureKillLook::SplatFor(ECreatureBlood::DarkIchor) == EVfxSplat::DarkIchor);
	TestEqual(TEXT("A power's blow: a rifle round's at 20 damage"), EmberVfx::BlowScale(20.f), 1.f);
	TestTrue(TEXT("...within a pellet's and a heavy round's"), EmberVfx::BlowScale(1.f) >= 0.6f && EmberVfx::BlowScale(500.f) <= 1.6f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxEffectsTest, "Looter.Vfx.Effects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxEffectsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Camera(-600.0, 0.0, 170.0);

	// The shapes: each spawned, alive, and gone once its life is over (with or without M_FX_Shape in the game).
	Effects.SpawnShockRing(FVector::ZeroVector, FVector::UpVector, 10.f, 200.f, 8.f, FLinearColor::White, 10.f, 0.3f);
	Effects.SpawnFireball(FVector(0.0, 0.0, 50.0), FVector::ZeroVector, 20.f, 80.f, FLinearColor(1.f, 0.4f, 0.1f), 10.f, 0.4f);
	Effects.SpawnStarburst(FVector(0.0, 0.0, 80.0), 40.f, FLinearColor::White, 20.f, 0.08f, 8);
	TestEqual(TEXT("Three shapes"), Effects.NumShapes(), 3);
	RunEffects(Effects, 0.1f, Camera);
	TestEqual(TEXT("The star's gone first"), Effects.NumShapes(), 2);
	RunEffects(Effects, 0.5f, Camera);
	TestEqual(TEXT("...all gone after their lives"), Effects.NumShapes(), 0);
	TestTrue(TEXT("A ring with nothing to it isn't made"), [&Effects]()
	{
		Effects.SpawnShockRing(FVector::ZeroVector, FVector::UpVector, 10.f, 200.f, 8.f, FLinearColor::White, 0.f, 0.3f);
		return Effects.NumShapes() == 0;
	}());

	// Kills: every kind throws its layers; a crit and a tier add shapes; a boss's would be the caller's to skip.
	for (const ECreatureBlood Blood : { ECreatureBlood::Ichor, ECreatureBlood::DarkIchor, ECreatureBlood::Gel, ECreatureBlood::Embers,
		ECreatureBlood::MinerEmbers })
	{
		RunEffects(Effects, 4.f, Camera);
		const int32 Before = Effects.NumParticles();
		Effects.SpawnKillLayers(TestKill(Blood));
		TestTrue(FString::Printf(TEXT("%s: a kill's layers (%d)"), CreatureBlood::Name(Blood), Effects.NumParticles() - Before),
			Effects.NumParticles() > Before + 8);
	}
	RunEffects(Effects, 4.f, Camera);
	Effects.SpawnKillLayers(TestKill(ECreatureBlood::Ichor));
	const int32 PlainShapes = Effects.NumShapes();
	RunEffects(Effects, 4.f, Camera);
	Effects.SpawnKillLayers(TestKill(ECreatureBlood::Ichor, ECreatureRank::Epic, EWeaponKind::Sniper, true));
	TestTrue(FString::Printf(TEXT("A Gravebound crit kill flares more (%d shapes, %d plain)"), Effects.NumShapes(), PlainShapes),
		Effects.NumShapes() >= PlainShapes + 4);

	// A blast: shapes, smoke and embers, all gone in a few seconds.
	RunEffects(Effects, 4.f, Camera);
	FVfxExplosion Blast;
	Blast.Center = FVector(300.0, 0.0, 0.0);
	Blast.Radius = 350.f;
	Effects.SpawnExplosion(Blast);
	TestTrue(FString::Printf(TEXT("A rocket's blast (%d pieces, %d shapes)"), Effects.NumParticles(), Effects.NumShapes()),
		Effects.NumParticles() > 40 && Effects.NumShapes() >= 6);
	RunEffects(Effects, 5.f, Camera);
	TestEqual(TEXT("...gone a few seconds on"), Effects.NumParticles(), 0);

	// Muzzles: each kind throws something; the sniper's ring is left out in the player's own view.
	for (const EWeaponKind Kind : { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper,
		EWeaponKind::LMG, EWeaponKind::Launcher })
	{
		const int32 Before = Effects.NumParticles();
		FVfxMuzzle Muzzle;
		Muzzle.Kind = Kind;
		Effects.SpawnMuzzleBurst(Muzzle);
		TestTrue(FString::Printf(TEXT("Kind %d's muzzle throws something"), static_cast<int32>(Kind)), Effects.NumParticles() > Before);
	}
	RunEffects(Effects, 4.f, Camera);
	FVfxMuzzle Sniper;
	Sniper.Kind = EWeaponKind::Sniper;
	Effects.SpawnMuzzleBurst(Sniper);
	TestEqual(TEXT("The sniper's ring from behind"), Effects.NumShapes(), 1);
	RunEffects(Effects, 4.f, Camera);
	Sniper.bFirstPerson = true;
	Effects.SpawnMuzzleBurst(Sniper);
	TestEqual(TEXT("...none in the player's own view"), Effects.NumShapes(), 0);

	// A stream of fire into one spot is thinned: ten SMG rounds throw less than ten lone ones.
	RunEffects(Effects, 4.f, Camera);
	int32 Before = Effects.NumParticles();
	Effects.SpawnSurfaceImpact(FVector(0.0, 0.0, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Dirt, 0.8f, false);
	const int32 Lone = Effects.NumParticles() - Before;
	Before = Effects.NumParticles();
	for (int32 Round = 0; Round < 10; ++Round)
	{
		Effects.SpawnSurfaceImpact(FVector(500.0 + Round, 0.0, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Dirt, 0.8f, false);
	}
	const int32 Stream = Effects.NumParticles() - Before;
	TestTrue(FString::Printf(TEXT("Ten rounds into a spot throw %d, under ten lone hits' %d"), Stream, Lone * 10), Stream < Lone * 8);
	TestTrue(TEXT("...and still something each"), Stream >= 10);
	Effects.Shutdown();
	return true;
}

#endif
