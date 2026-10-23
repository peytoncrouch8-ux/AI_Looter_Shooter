#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSoundCues.h"
#include "UI/HUD/HudHitMarker.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The gun feel pass's impacts and hit markers (Docs/Work/2026-10-22-Gunfeel/PLAN.md, GF-IMPACT): a world hit's weight by its
// gun's kind (Looter.Vfx.Impact.*), a shotgun blast's one dust sheet, a crit's look by what the body spills, a heavy round's
// cracked marks, and the HUD's hit-marker rules (Looter.UI.HUD.HitMarker.*). The rules with the cvars' defaults (default-made
// tunings); the effects spawned in a test world and counted, never judged by eye (the play-check does that).

namespace
{
	constexpr float ImpactTestFrame = 1.f / 60.f;

	void RunImpactEffects(FWeaponFX& Effects, float Seconds, const FVector& Camera)
	{
		for (float Time = 0.f; Time < Seconds; Time += ImpactTestFrame)
		{
			Effects.Tick(ImpactTestFrame, Camera);
		}
	}

	/** A console variable set for the scope, put back after. */
	struct FImpactTestVariable
	{
		FImpactTestVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetFloat();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FImpactTestVariable()
		{
			if (Variable)
			{
				Variable->Set(*FString::SanitizeFloat(Was), ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		float Was = 0.f;
	};

	/** Pieces one round of Kind throws hitting Material at Where (the effects emptied first, so the count is its own). */
	int32 ImpactPieces(FWeaponFX& Effects, const FVector& Camera, EWeaponKind Kind, EImpactMaterial Material, const FVector& Where, int32 Volley = 1)
	{
		RunImpactEffects(Effects, 4.f, Camera);
		const int32 Before = Effects.NumParticles();
		FVfxRound Round;
		Round.Kind = Kind;
		Round.RoundsLeft = 30;
		Round.Volley = Volley;
		const FWeaponFX::FLandingScope Landing(Effects, Round);
		Effects.SpawnSurfaceImpact(Where, FVector::UpVector, FVector(1.0, 0.0, -0.5), Material, FBulletMarks::GunScale(Kind), false);
		return Effects.NumParticles() - Before;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelImpactRulesTest, "Looter.Vfx.Impact.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelImpactRulesTest::RunTest(const FString& Parameters)
{
	const FVfxTuning Tuning;
	// The plan's weights.
	TestEqual(TEXT("SMG 0.6"), VfxRules::ImpactWeight(EWeaponKind::SMG, Tuning), 0.6f);
	TestEqual(TEXT("Rifle 1"), VfxRules::ImpactWeight(EWeaponKind::Rifle, Tuning), 1.f);
	TestEqual(TEXT("LMG 1.05"), VfxRules::ImpactWeight(EWeaponKind::LMG, Tuning), 1.05f);
	TestEqual(TEXT("Revolver 1.3"), VfxRules::ImpactWeight(EWeaponKind::Revolver, Tuning), 1.3f);
	TestEqual(TEXT("Sniper 1.8"), VfxRules::ImpactWeight(EWeaponKind::Sniper, Tuning), 1.8f);
	TestEqual(TEXT("A pellet 0.5"), VfxRules::ImpactWeight(EWeaponKind::Shotgun, Tuning), 0.5f);
	TestEqual(TEXT("No gun: 1"), VfxRules::ImpactWeight(EWeaponKind::None, Tuning), 1.f);
	for (const TCHAR* Name : { TEXT("Looter.Vfx.Impact.Weight.SMG"), TEXT("Looter.Vfx.Impact.Weight.Sniper"), TEXT("Looter.Vfx.Impact.LMGFullEvery"),
		TEXT("Looter.Vfx.Hit.CritPop"), TEXT("Looter.Vfx.Impact.Sheet") })
	{
		TestNotNull(*FString::Printf(TEXT("%s exists"), Name), IConsoleManager::Get().FindConsoleVariable(Name));
	}

	// Looks by kind.
	const FVfxImpactLook None = VfxRules::ImpactLook(EWeaponKind::None, 0, EImpactMaterial::Stone, Tuning);
	TestTrue(TEXT("No gun: today's look, nothing added"), None.Count == 1.f && None.Size == 1.f && !None.bPellet && !None.bPunch && !None.bChunks);
	const FVfxImpactLook Rifle = VfxRules::ImpactLook(EWeaponKind::Rifle, 30, EImpactMaterial::Stone, Tuning);
	TestTrue(TEXT("A rifle round is the reference"), Rifle.Count == 1.f && FMath::IsNearlyEqual(Rifle.Size, 1.f) && !Rifle.bPunch && !Rifle.bChunks);
	const FVfxImpactLook SMG = VfxRules::ImpactLook(EWeaponKind::SMG, 30, EImpactMaterial::Dirt, Tuning);
	TestTrue(TEXT("An SMG's fewer and smaller"), SMG.Count < 1.f && SMG.Size < 1.f && !SMG.bPellet);
	const FVfxImpactLook Sniper = VfxRules::ImpactLook(EWeaponKind::Sniper, 5, EImpactMaterial::Dirt, Tuning);
	TestTrue(TEXT("A sniper's punches, bigger"), Sniper.bPunch && Sniper.Size > 1.3f && Sniper.Count > 1.7f);
	TestTrue(TEXT("Revolver: a chunk burst on stone"), VfxRules::ImpactLook(EWeaponKind::Revolver, 6, EImpactMaterial::Stone, Tuning).bChunks);
	TestFalse(TEXT("...not into dirt"), VfxRules::ImpactLook(EWeaponKind::Revolver, 6, EImpactMaterial::Dirt, Tuning).bChunks);
	TestFalse(TEXT("...and no punch"), VfxRules::ImpactLook(EWeaponKind::Revolver, 6, EImpactMaterial::Stone, Tuning).bPunch);
	const FVfxImpactLook Full = VfxRules::ImpactLook(EWeaponKind::LMG, 99, EImpactMaterial::Dirt, Tuning);
	const FVfxImpactLook Light = VfxRules::ImpactLook(EWeaponKind::LMG, 98, EImpactMaterial::Dirt, Tuning);
	TestTrue(TEXT("LMG: every third round full, with chunks"), Full.bChunks && FMath::IsNearlyEqual(Full.Count, 1.05f));
	TestTrue(TEXT("...the rest lighter, none"), !Light.bChunks && FMath::IsNearlyEqual(Light.Count, 1.05f * Tuning.ImpactLMGLight));
	const FVfxImpactLook Pellet = VfxRules::ImpactLook(EWeaponKind::Shotgun, 6, EImpactMaterial::Stone, Tuning);
	TestTrue(TEXT("A pellet: half a hit, its mark's size, nothing added"), Pellet.bPellet && Pellet.Count == 0.5f && Pellet.Size == 1.f
		&& !Pellet.bPunch && !Pellet.bChunks);

	// The sheet's rules.
	TestTrue(TEXT("Sparks off stone and metal"), VfxRules::SheetOf(EImpactMaterial::Stone) == EVfxSheet::Hard
		&& VfxRules::SheetOf(EImpactMaterial::Metal) == EVfxSheet::Hard);
	TestTrue(TEXT("A fan off dirt and wood"), VfxRules::SheetOf(EImpactMaterial::Dirt) == EVfxSheet::Soft && VfxRules::SheetOf(EImpactMaterial::Wood) == EVfxSheet::Soft);
	TestTrue(TEXT("Spray off water, nothing off the unknown"), VfxRules::SheetOf(EImpactMaterial::Water) == EVfxSheet::Splash
		&& VfxRules::SheetOf(EImpactMaterial::Unknown) == EVfxSheet::None);
	TestEqual(TEXT("A fist-sized pattern: the least sheet"), VfxRules::SheetRadius(0.f), 40.f);
	TestEqual(TEXT("...a huge one: the most"), VfxRules::SheetRadius(1000.f), 220.f);
	TestTrue(TEXT("Puffs 3 to 5"), VfxRules::SheetPuffs(40.f) == 3 && VfxRules::SheetPuffs(220.f) == 5);

	FVfxBlastSheet Sheet;
	Sheet.Add(FVector(0.0, -30.0, 0.0), FVector::UpVector, FVector::ForwardVector, EImpactMaterial::Dirt);
	Sheet.Add(FVector(0.0, 30.0, 0.0), FVector::UpVector, FVector::ForwardVector, EImpactMaterial::Dirt);
	Sheet.Add(FVector(0.0, 0.0, 0.0), FVector::UpVector, FVector::ForwardVector, EImpactMaterial::Stone);
	TestTrue(TEXT("The sheet's middle"), Sheet.Centroid().Equals(FVector::ZeroVector, 0.01));
	TestEqual(TEXT("...its spread (root mean square)"), Sheet.Spread(), FMath::Sqrt(600.f), 0.05f);
	TestTrue(TEXT("...in what most struck"), Sheet.Material() == EImpactMaterial::Dirt);
	TestTrue(TEXT("...facing up"), Sheet.Normal().Equals(FVector::UpVector, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelImpactKindsTest, "Looter.Vfx.Impact.Kinds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelImpactKindsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Camera(-600.0, 0.0, 170.0);
	// Each round in its own spot, so none is thinned as a stream into another's.
	const int32 RifleDirt = ImpactPieces(Effects, Camera, EWeaponKind::Rifle, EImpactMaterial::Dirt, FVector(0.0, 0.0, 1.0));
	const int32 SMGDirt = ImpactPieces(Effects, Camera, EWeaponKind::SMG, EImpactMaterial::Dirt, FVector(0.0, 600.0, 1.0));
	const int32 SniperDirt = ImpactPieces(Effects, Camera, EWeaponKind::Sniper, EImpactMaterial::Dirt, FVector(0.0, -600.0, 1.0));
	TestTrue(FString::Printf(TEXT("An SMG round throws less than a rifle's (%d < %d)"), SMGDirt, RifleDirt), SMGDirt < RifleDirt);
	TestTrue(FString::Printf(TEXT("A sniper's punch far more (%d > 1.5 x %d)"), SniperDirt, RifleDirt), SniperDirt * 2 > RifleDirt * 3);
	const int32 RifleStone = ImpactPieces(Effects, Camera, EWeaponKind::Rifle, EImpactMaterial::Stone, FVector(300.0, 0.0, 1.0));
	const int32 RevolverStone = ImpactPieces(Effects, Camera, EWeaponKind::Revolver, EImpactMaterial::Stone, FVector(300.0, 600.0, 1.0));
	TestTrue(FString::Printf(TEXT("A revolver's slug chips more off stone (%d > %d)"), RevolverStone, RifleStone), RevolverStone > RifleStone);
	const int32 NoGun = [&]()
	{
		RunImpactEffects(Effects, 4.f, Camera);
		const int32 Before = Effects.NumParticles();
		Effects.SpawnSurfaceImpact(FVector(300.0, -600.0, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Stone, 1.f, false);
		return Effects.NumParticles() - Before;
	}();
	TestTrue(FString::Printf(TEXT("No gun handed in: a rifle round's look (%d, %d)"), NoGun, RifleStone), FMath::Abs(NoGun - RifleStone) <= 3);

	// A blast: its pellets gather, its sheet waits for the last, then lays once.
	RunImpactEffects(Effects, 4.f, Camera);
	for (int32 Pellet = 0; Pellet < 6; ++Pellet)
	{
		FVfxRound Round;
		Round.Kind = EWeaponKind::Shotgun;
		Round.Volley = 7;
		const FWeaponFX::FLandingScope Landing(Effects, Round);
		Effects.SpawnSurfaceImpact(FVector(500.0, -60.0 + 24.0 * Pellet, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Dirt,
			FBulletMarks::GunScale(EWeaponKind::Shotgun), false);
	}
	TestEqual(TEXT("The blast's pellets gather"), Effects.PendingSheetVolley(), 7);
	TestFalse(TEXT("...its sheet waits while a pellet still flies"), Effects.SettleBlastSheet(true));
	const int32 BeforeSheet = Effects.NumParticles();
	TestTrue(TEXT("...and lays when the last is down"), Effects.SettleBlastSheet(false));
	TestTrue(FString::Printf(TEXT("...a sheet of pieces (%d)"), Effects.NumParticles() - BeforeSheet), Effects.NumParticles() >= BeforeSheet + 6);
	TestEqual(TEXT("...once"), Effects.PendingSheetVolley(), 0);
	// A scope gone, the round is forgotten: the next hit has no gun.
	{
		FVfxRound Round;
		Round.Kind = EWeaponKind::Shotgun;
		Round.Volley = 8;
		const FWeaponFX::FLandingScope Landing(Effects, Round);
		Effects.SpawnSurfaceImpact(FVector(800.0, 0.0, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Stone, 0.6f, false);
	}
	Effects.SpawnSurfaceImpact(FVector(800.0, 300.0, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Stone, 0.6f, false);
	TestEqual(TEXT("Only the scoped pellet gathered"), Effects.PendingSheetVolley(), 8);
	{
		FVfxRound Round;
		Round.Kind = EWeaponKind::Shotgun;
		Round.Volley = 9;
		const FWeaponFX::FLandingScope Landing(Effects, Round);
		Effects.SpawnSurfaceImpact(FVector(900.0, 0.0, 1.0), FVector::UpVector, FVector(1.0, 0.0, -0.5), EImpactMaterial::Stone, 0.6f, false);
	}
	TestEqual(TEXT("A new blast lets the last one go"), Effects.PendingSheetVolley(), 9);
	RunImpactEffects(Effects, 6.f, Camera);
	TestEqual(TEXT("All of it gone in time"), Effects.NumParticles(), 0);
	Effects.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelCritLooksTest, "Looter.Vfx.Hit.CritLooks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelCritLooksTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Camera(-600.0, 0.0, 170.0);
	auto CritPieces = [&](ECreatureBlood Blood, const FVector& Where)
	{
		RunImpactEffects(Effects, 4.f, Camera);
		const int32 Before = Effects.NumParticles() + Effects.NumShapes();
		Effects.SpawnCreatureHit(Where, FVector(-1.0, 0.0, 0.0), FVector(1.0, 0.0, 0.0), Blood, CreatureBlood::DefaultTint(Blood), 1.f, true);
		return Effects.NumParticles() + Effects.NumShapes() - Before;
	};
	int32 Spot = 0;
	for (const ECreatureBlood Blood : { ECreatureBlood::Ichor, ECreatureBlood::DarkIchor, ECreatureBlood::Gel, ECreatureBlood::Embers,
		ECreatureBlood::MinerEmbers, ECreatureBlood::Clay })
	{
		int32 Plain = 0;
		{
			const FImpactTestVariable Off(TEXT("Looter.Vfx.Hit.CritPop"), 0.f);
			Plain = CritPieces(Blood, FVector(0.0, 600.0 * Spot++, 80.0));
		}
		const int32 Own = CritPieces(Blood, FVector(0.0, 600.0 * Spot++, 80.0));
		TestTrue(FString::Printf(TEXT("%s: its own crit adds to every crit's (%d > %d)"), CreatureBlood::Name(Blood), Own, Plain), Own > Plain);
	}
	RunImpactEffects(Effects, 6.f, Camera);
	TestEqual(TEXT("All of it gone in time"), Effects.NumParticles(), 0);
	Effects.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelHeavyMarksTest, "Looter.Vfx.Impact.HeavyMarks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelHeavyMarksTest::RunTest(const FString& Parameters)
{
	const FImpactTestVariable On(TEXT("Looter.BulletMarks.HeavyCracks"), 1.f);
	TestEqual(TEXT("Stone's cracked style"), FBulletMarks::HeavyLookOf(EImpactMaterial::Stone).Style, 13);
	TestEqual(TEXT("Wood's split style"), FBulletMarks::HeavyLookOf(EImpactMaterial::Wood).Style, 14);
	TestTrue(TEXT("None for dirt"), FBulletMarks::HeavyLookOf(EImpactMaterial::Dirt).Style < 0);
	TestEqual(TEXT("A sniper's round on stone cracks it"), FBulletMarks::LookFor(EImpactMaterial::Stone, FBulletMarks::GunScale(EWeaponKind::Sniper)).Style, 13);
	TestEqual(TEXT("...a revolver's on wood splits it"), FBulletMarks::LookFor(EImpactMaterial::Wood, FBulletMarks::GunScale(EWeaponKind::Revolver)).Style, 14);
	TestEqual(TEXT("...a rifle's chips it as ever"), FBulletMarks::LookFor(EImpactMaterial::Stone, FBulletMarks::GunScale(EWeaponKind::Rifle)).Style,
		FBulletMarks::LookOf(EImpactMaterial::Stone).Style);
	TestEqual(TEXT("...a sniper's into dirt as ever"), FBulletMarks::LookFor(EImpactMaterial::Dirt, 1.5f).Style, FBulletMarks::LookOf(EImpactMaterial::Dirt).Style);
	TestTrue(TEXT("The cracks reach past the chip"), FBulletMarks::HeavyLookOf(EImpactMaterial::Stone).Radius > FBulletMarks::LookOf(EImpactMaterial::Stone).Radius);
	TestTrue(TEXT("...within the decal's 16 style slots"), FBulletMarks::Code(14, 0.5f).R < 1.f);
	{
		const FImpactTestVariable Off(TEXT("Looter.BulletMarks.HeavyCracks"), 0.f);
		TestEqual(TEXT("Switched off: the plain chip"), FBulletMarks::LookFor(EImpactMaterial::Stone, 1.5f).Style, FBulletMarks::LookOf(EImpactMaterial::Stone).Style);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudHitMarkerRulesTest, "Looter.UI.HUD.HitMarker.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudHitMarkerRulesTest::RunTest(const FString& Parameters)
{
	using namespace HudHitMarker;
	const FTuning Tuning;
	// Which mark.
	TestTrue(TEXT("A kill whatever else"), Decide(true, true, true, 1.f, 0.f) == EHudHitMark::Kill);
	TestTrue(TEXT("A ward that stands took it (a crit too)"), Decide(false, true, true, 1.f, 0.4f) == EHudHitMark::Ward);
	TestTrue(TEXT("A ward up before and gone now broke"), Decide(false, false, true, 0.2f, 0.f) == EHudHitMark::WardBreak);
	TestTrue(TEXT("A ward long gone: the body's hit"), Decide(false, true, true, 0.f, 0.f) == EHudHitMark::Crit);
	TestTrue(TEXT("No ward: crit or plain"), Decide(false, true, false, 0.f, 0.f) == EHudHitMark::Crit && Decide(false, false, false, 1.f, 1.f) == EHudHitMark::Body);
	// How big.
	TestEqual(TEXT("A scratch: its own size"), Size(0.f, Tuning), 1.f);
	TestEqual(TEXT("A quarter of its health: the biggest"), Size(0.25f, Tuning), 1.4f, 0.001f);
	TestEqual(TEXT("...and no bigger"), Size(2.f, Tuning), 1.4f, 0.001f);
	TestEqual(TEXT("Half a blast's pellets: halfway"), BlastSize(5, 10, Tuning), 1.2f, 0.001f);
	TestEqual(TEXT("...all of them: the biggest"), BlastSize(12, 10, Tuning), 1.4f, 0.001f);
	// Order, sound, time and colour.
	TestTrue(TEXT("A crit outranks a plain pellet, a kill everything"), Outranks(EHudHitMark::Crit, EHudHitMark::Body) && Outranks(EHudHitMark::Kill, EHudHitMark::WardBreak));
	TestTrue(TEXT("A plain pellet doesn't take a crit's place"), !Outranks(EHudHitMark::Body, EHudHitMark::Crit));
	TestTrue(TEXT("A crit pellet sounds again after plain ones"), SoundsAgain(EHudHitMark::Crit, EHudHitMark::Body));
	TestFalse(TEXT("...a plain one doesn't"), SoundsAgain(EHudHitMark::Body, EHudHitMark::Body));
	TestTrue(TEXT("The kill's sound is the voice's"), Cue(EHudHitMark::Kill).IsNone());
	TestEqual(TEXT("The crit's ping"), Cue(EHudHitMark::Crit), FName(LooterSoundCue::Gunfeel::HitMarkerCrit));
	TestEqual(TEXT("The ward's tick"), Cue(EHudHitMark::Ward), FName(LooterSoundCue::Gunfeel::HitMarkerWard));
	TestEqual(TEXT("The break's crack"), Cue(EHudHitMark::WardBreak), FName(LooterSoundCue::Gunfeel::WardBreak));
	TestEqual(TEXT("The plain tick"), Cue(EHudHitMark::Body), FName(LooterSoundCue::HitMarker));
	TestTrue(TEXT("A kill and a break show longer and pop bigger"), SecondsOf(EHudHitMark::Kill, Tuning) > SecondsOf(EHudHitMark::Body, Tuning)
		&& SecondsOf(EHudHitMark::WardBreak, Tuning) > SecondsOf(EHudHitMark::Ward, Tuning) && PopOf(EHudHitMark::WardBreak, Tuning) > PopOf(EHudHitMark::Crit, Tuning));
	TestTrue(TEXT("Each mark its own colour"), !Tint(EHudHitMark::Body).Equals(Tint(EHudHitMark::Crit)) && !Tint(EHudHitMark::Crit).Equals(Tint(EHudHitMark::Ward))
		&& !Tint(EHudHitMark::Ward).Equals(Tint(EHudHitMark::Kill)));

	// The wards remembered.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	FHudWardMemory Memory;
	AActor* First = World->SpawnActor<AActor>();
	TestEqual(TEXT("Never seen: a full ward"), Memory.Before(First), 1.f);
	Memory.Note(First, nullptr, 0.4f);
	TestEqual(TEXT("Seen: as it was"), Memory.Before(First), 0.4f);
	for (int32 Index = 0; Index < FHudWardMemory::Kept + 2; ++Index)
	{
		Memory.Note(World->SpawnActor<AActor>(), nullptr, 0.5f);
	}
	TestEqual(TEXT("Only the last few kept"), Memory.Num(), FHudWardMemory::Kept);
	TestEqual(TEXT("...the oldest forgotten"), Memory.Before(First), 1.f);
	Memory.Refresh();
	TestEqual(TEXT("One with no ward to read is let go"), Memory.Num(), 0);
	return true;
}

#endif
