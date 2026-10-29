#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// HITS lane D (2026-10-28; Docs/Work/2026-10-28-Core/HITS.md): the meadow slime's hit-feel (a hop in the air knocked off its line, a
// kill's style, the wounded gel's shrink) and the hit-feel looks' budget (the six FWeaponFX spawns). The effects are counted in a
// test world, never judged by eye (the play-check does that); the slimes are ticked by hand (a test level never ticks).

#include "Combat/HitReaction.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/SlimeCreature.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float HitFxFrame = 1.f / 60.f;

	/** A console variable set for the scope, put back after. */
	struct FHitFxVariable
	{
		FHitFxVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetFloat();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FHitFxVariable()
		{
			if (Variable)
			{
				Variable->Set(*FString::SanitizeFloat(Was), ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		float Was = 0.f;
	};

	ASlimeCreature* HitFxSlime(UWorld* World, const FVector& Feet)
	{
		return Cast<ASlimeCreature>(EncounterTestWorld::SpawnCreature(World, ASlimeCreature::StaticClass(), Feet));
	}

	void HitFxRun(ACreatureBase& Creature, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += HitFxFrame)
		{
			Creature.ConsumeMovementInputVector();
			Creature.Tick(HitFxFrame);
		}
	}

	/** The effects run on until everything they threw is gone, camera at Camera. */
	void HitFxClear(FWeaponFX& Effects, const FVector& Camera)
	{
		for (float Time = 0.f; Time < 6.f; Time += HitFxFrame)
		{
			Effects.Tick(HitFxFrame, Camera);
		}
	}

	/** The tallest of the body bone's scale (the gel's height or width): 1 at rest, 1.25 swollen, 0.15 flat. -1: no rig in this checkout. */
	double HitFxBodyScale(const ASlimeCreature& Slime)
	{
		return Slime.GetBonePose().IsEmpty() ? -1.0 : Slime.GetBonePose()[0].Transform.GetScale3D().GetMax();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelSlimeAirKnockTest, "Looter.Creatures.Hits.Slime.AirKnock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelSlimeAirKnockTest::RunTest(const FString& Parameters)
{
	// The rule: 150 cm/s along the shot, plus 400 for each share of its health the burst took, at most 450, times the root of its size.
	TestNearlyEqual(TEXT("A scratch: 150"), static_cast<float>(ASlimeCreature::AirKnockFor(FVector::ForwardVector, 0.f, 1.f).Size()), 150.f, 0.01f);
	TestNearlyEqual(TEXT("A tenth of its health: 190"), static_cast<float>(ASlimeCreature::AirKnockFor(FVector::ForwardVector, 0.1f, 1.f).Size()), 190.f, 0.01f);
	TestNearlyEqual(TEXT("A big hit: capped at 450"), static_cast<float>(ASlimeCreature::AirKnockFor(FVector::ForwardVector, 1.f, 1.f).Size()), ASlimeCreature::MaxAirKnock, 0.01f);
	TestNearlyEqual(TEXT("A size-4 slime: twice as much"), static_cast<float>(ASlimeCreature::AirKnockFor(FVector::ForwardVector, 0.1f, 4.f).Size()), 380.f, 0.01f);
	TestTrue(TEXT("Flat: a shot from above pushes it sideways only"),
		FMath::IsNearlyZero(ASlimeCreature::AirKnockFor(FVector(0.0, 1.0, -1.0), 0.1f, 1.f).Z));
	TestTrue(TEXT("...and a shot straight down pushes nothing"), ASlimeCreature::AirKnockFor(FVector(0.0, 0.0, -1.0), 0.1f, 1.f).IsNearlyZero());

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	// A wide floor, and a narrow platform (x 6000 to 6600) with nothing beyond its edge.
	const AActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	const AActor* Platform = LocomotionTestWorld::SpawnBlock(World, FVector(6300.0, 0.0, -50.0), FVector(600.0, 2000.0, 100.0));
	ASlimeCreature* Open = HitFxSlime(World, FVector::ZeroVector);
	ASlimeCreature* Edge = HitFxSlime(World, FVector(6100.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A platform"), Platform) || !TestNotNull(TEXT("A slime on the floor"), Open)
		|| !TestNotNull(TEXT("A slime on the platform"), Edge))
	{
		return false;
	}

	// On the ground a hit pushes nothing: a hop in the air is what a shot deflects.
	TestTrue(TEXT("On the ground: no push"), Open->KnockInAir(FVector::ForwardVector, 0.1f).IsNearlyZero() && !Open->IsInAir());
	Open->HopNow(FVector::ForwardVector);
	TestTrue(TEXT("It hops"), Open->IsInAir());
	const FVector Push = Open->KnockInAir(FVector(1.0, 0.0, -0.2), 0.1f);
	TestTrue(FString::Printf(TEXT("In the air: pushed along the shot, 190 cm/s (%.0f)"), Push.Size()),
		FMath::IsNearlyEqual(Push.Size(), 190.0, 1.0) && Push.X > 0.0 && FMath::IsNearlyZero(Push.Z) && FMath::IsNearlyZero(Push.Y, 0.01));

	// Never past a ledge: the hardest push would land it off the platform, so it is halved until it lands on ground.
	Edge->HopNow(FVector::ForwardVector);
	const FVector Hard = Edge->KnockInAir(FVector::ForwardVector, 1.f);
	TestTrue(FString::Printf(TEXT("Near an edge: the push is held back (%.0f of 450)"), Hard.Size()), Hard.Size() < ASlimeCreature::MaxAirKnock - 1.f);
	TestTrue(TEXT("...and the landing it leaves is on the platform"), 6100.0 + 200.0 + Hard.X * 0.76 < 6600.0);

	// Dead: none.
	BossTestWorld::Kill(Open);
	TestTrue(TEXT("Dead: no push"), Open->KnockInAir(FVector::ForwardVector, 0.1f).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelSlimePopTest, "Looter.Creatures.Hits.Slime.Pop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelSlimePopTest::RunTest(const FString& Parameters)
{
	// The kill's style by its hit: a plain kill, a crit through the core (popped), a blast (splattered), an overkill.
	FCreatureHit Plain;
	Plain.Weight = EHitWeight::Kill;
	FCreatureHit Crit = Plain;
	Crit.bCritical = true;
	FCreatureHit Blast = Plain;
	Blast.Kind = EWeaponKind::Shotgun;
	FCreatureHit Big = Plain;
	Big.Weight = EHitWeight::Big;
	FCreatureHit Over = Crit;
	Over.Weight = EHitWeight::Overkill;
	TestEqual(TEXT("A plain kill: today's splat"), ASlimeCreature::DeathStyleFor(Plain), ASlimeCreature::EDeathStyle::Body);
	TestEqual(TEXT("A core crit: popped"), ASlimeCreature::DeathStyleFor(Crit), ASlimeCreature::EDeathStyle::Popped);
	TestEqual(TEXT("A shotgun kill: splattered"), ASlimeCreature::DeathStyleFor(Blast), ASlimeCreature::EDeathStyle::Splattered);
	TestEqual(TEXT("A Big hit: splattered"), ASlimeCreature::DeathStyleFor(Big), ASlimeCreature::EDeathStyle::Splattered);
	TestEqual(TEXT("An overkill beats the crit"), ASlimeCreature::DeathStyleFor(Over), ASlimeCreature::EDeathStyle::Overkill);
	{
		const FHitFxVariable Off(TEXT("Looter.Hits.KillStyles"), 0.f);
		TestEqual(TEXT("Kill styles off: all today's"), ASlimeCreature::DeathStyleFor(Over), ASlimeCreature::EDeathStyle::Body);
	}

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const AActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	ASlimeCreature* Plainly = HitFxSlime(World, FVector::ZeroVector);
	ASlimeCreature* Burst = HitFxSlime(World, FVector(1500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A slime"), Plainly) || !TestNotNull(TEXT("Another"), Burst))
	{
		return false;
	}
	// One taken down in small hits (settling between them and the last: its squash back at rest) and finished off by one (no overkill,
	// nothing Big); one killed outright.
	for (const float Damage : { 29.f, 29.f, 29.f, 29.f })
	{
		BossTestWorld::Hurt(Plainly, Damage);
	}
	HitFxRun(*Plainly, 1.5f);
	BossTestWorld::Hurt(Plainly, 5.f);
	BossTestWorld::Kill(Burst);
	TestTrue(TEXT("Both are dead"), Plainly->IsDead() && Burst->IsDead());
	TestEqual(TEXT("The small finishing hit: today's splat"), Plainly->GetDeathStyle(), ASlimeCreature::EDeathStyle::Body);
	TestEqual(TEXT("The killing blow of 10 million: an overkill"), Burst->GetDeathStyle(), ASlimeCreature::EDeathStyle::Overkill);

	HitFxRun(*Plainly, HitFxFrame);
	HitFxRun(*Burst, HitFxFrame);
	const double PlainScale = HitFxBodyScale(*Plainly);
	const double BurstScale = HitFxBodyScale(*Burst);
	if (PlainScale < 0.0 || BurstScale < 0.0)
	{
		AddWarning(TEXT("SK_Slime isn't imported: the swell and the puddle aren't measured."));
		return true;
	}
	// The overkill is swollen on the killing frame (held by its hit-stop in play), a plain kill is not.
	TestTrue(FString::Printf(TEXT("The overkill swells past its shape (%.2f)"), BurstScale), BurstScale > 1.1);
	TestTrue(FString::Printf(TEXT("...a plain kill does not (%.2f)"), PlainScale), PlainScale < 1.1);
	// Then it bursts into its puddle.
	HitFxRun(*Burst, 0.6f);
	const double Flat = Burst->GetBonePose()[0].Transform.GetScale3D().GetMin();
	TestTrue(FString::Printf(TEXT("...and flattens into its puddle (%.2f)"), Flat), Flat < 0.3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelSlimeShrinkTest, "Looter.Creatures.Hits.Slime.Shrink",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelSlimeShrinkTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const AActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	ASlimeCreature* Whole = HitFxSlime(World, FVector::ZeroVector);
	ASlimeCreature* Hurt = HitFxSlime(World, FVector(1500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A slime"), Whole) || !TestNotNull(TEXT("Another"), Hurt))
	{
		return false;
	}
	// 120 health: three hits and a small one leave it a fifth of its health.
	for (const float Damage : { 29.f, 29.f, 29.f, 8.f })
	{
		BossTestWorld::Hurt(Hurt, Damage);
	}
	HitFxRun(*Whole, 1.5f);
	HitFxRun(*Hurt, 1.5f);
	TestNearlyEqual(TEXT("A whole slime: full size"), Whole->GetWoundShrink(), 1.f, 0.001f);
	const float Shrunk = Hurt->GetWoundShrink();
	TestTrue(FString::Printf(TEXT("A slime at a fifth of its health: about 0.9 (%.3f)"), Shrunk), Shrunk >= 0.88f && Shrunk < 0.93f);
	TestNearlyEqual(TEXT("...its gel scaled so, the hulls with it"), static_cast<float>(Hurt->GetMesh()->GetRelativeScale3D().X), Shrunk, 0.005f);
	const double WholeScale = HitFxBodyScale(*Whole);
	const double HurtScale = HitFxBodyScale(*Hurt);
	if (WholeScale > 0.0 && HurtScale > 0.0)
	{
		// The body bone itself rests at the same shape; what is smaller is the whole mesh under it.
		TestTrue(FString::Printf(TEXT("The body bone still rests near 1 (%.2f)"), HurtScale), HurtScale < 1.4);
	}

	// Switched off (Looter.Hits.Wounds 0): it eases back to full size.
	{
		const FHitFxVariable Off(TEXT("Looter.Hits.Wounds"), 0.f);
		HitFxRun(*Hurt, 1.5f);
		TestNearlyEqual(TEXT("Wounds off: full size again"), Hurt->GetWoundShrink(), 1.f, 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelFxBudgetTest, "Looter.Vfx.Hits.Budget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelFxBudgetTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Camera(-600.0, 0.0, 170.0);
	const int32 Pool = FVfxTuning::Live().MaxParticles;
	const int32 Share = FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Pool) * 0.04f));
	HitFxClear(Effects, Camera);

	// Each of the six looks: the pieces it throws for the count a kill asks (tatters 10-16, an ember tear 10-14), the most it
	// throws asked for far too many, and how many the same look throws at 60 m.
	struct FLook
	{
		const TCHAR* Name;
		TFunction<void(FWeaponFX&, const FVector&, int32)> Spawn;
		int32 Asked;
		bool bThinned;
	};
	const FLinearColor Ember(1.f, 0.62f, 0.32f);
	const FLinearColor Gel(0.4f, 0.9f, 0.4f);
	const FVector Way(1.0, 0.0, 0.0);
	const TArray<FLook> Looks = {
		{ TEXT("Chitin chips"), [&](FWeaponFX& Fx, const FVector& At, int32 Count) { Fx.SpawnChitinChips(At, FVector::UpVector, 100.f, Count); }, 12, true },
		{ TEXT("Goo drip"), [&](FWeaponFX& Fx, const FVector& At, int32) { Fx.SpawnGooDrip(At, Gel, 3.f); }, 1, false },
		{ TEXT("Ember tear"), [&](FWeaponFX& Fx, const FVector& At, int32 Count) { Fx.SpawnEmberTear(At, Way, 300.f, Ember, Count); }, 14, true },
		{ TEXT("Soul tatters"), [&](FWeaponFX& Fx, const FVector& At, int32 Count) { Fx.SpawnSoulTatters(At, Way, Ember, 100.f, Count); }, 16, true },
		{ TEXT("Gel pop"), [&](FWeaponFX& Fx, const FVector& At, int32) { Fx.SpawnGelPop(At, Way, Gel, 1000.f); }, 1, true },
		{ TEXT("Ember puff"), [&](FWeaponFX& Fx, const FVector& At, int32) { Fx.SpawnEmberPuff(At, Ember, 30.f); }, 1, false },
	};
	int32 Spot = 0;
	// (Spots far apart, so one kill's crowd thinning (KillCrowdDetail) never counts the last look's: a test world's clock stands still.)
	auto Throw = [&](const FLook& Look, float AlongX, int32 Count)
	{
		HitFxClear(Effects, Camera);
		const int32 Before = Effects.NumParticles();
		Look.Spawn(Effects, FVector(AlongX, 1500.0 * Spot++, 80.0), Count);
		return Effects.NumParticles() - Before;
	};
	for (const FLook& Look : Looks)
	{
		const int32 Near = Throw(Look, 0.f, Look.Asked);
		TestTrue(FString::Printf(TEXT("%s: it throws something (%d)"), Look.Name, Near), Near > 0);
		TestTrue(FString::Printf(TEXT("%s: within its share of the pool (%d <= %d)"), Look.Name, Near, Share), Near <= Share);
		const int32 Far = Throw(Look, 6000.f, Look.Asked);
		TestTrue(FString::Printf(TEXT("%s: still something at 60 m (%d)"), Look.Name, Far), Far > 0);
		if (Look.bThinned)
		{
			TestTrue(FString::Printf(TEXT("%s: thinned at 60 m (%d < %d)"), Look.Name, Far, Near), Far < Near);
		}
		// Asked for far too many: no more than 40 pieces, and no more than its share of the pool.
		const int32 Most = Throw(Look, 0.f, 500);
		TestTrue(FString::Printf(TEXT("%s: asked for 500, at most 40 pieces and its share (%d)"), Look.Name, Most), Most <= 40 && Most <= Share);
	}

	// Thrown over and over, the pool never grows past its size.
	for (int32 Index = 0; Index < 40; ++Index)
	{
		for (const FLook& Look : Looks)
		{
			Look.Spawn(Effects, FVector(0.0, 40.0 * Index, 80.0), Look.Asked);
		}
	}
	TestTrue(FString::Printf(TEXT("The pool holds (%d <= %d)"), Effects.NumParticles(), Pool), Effects.NumParticles() <= Pool);

	// Switched off (Looter.Vfx.Hit.Scale / Looter.Vfx.Kill.Scale 0): nothing.
	HitFxClear(Effects, Camera);
	{
		const FHitFxVariable HitOff(TEXT("Looter.Vfx.Hit.Scale"), 0.f);
		const FHitFxVariable KillOff(TEXT("Looter.Vfx.Kill.Scale"), 0.f);
		for (const FLook& Look : Looks)
		{
			Look.Spawn(Effects, FVector(0.0, 9000.0, 80.0), Look.Asked);
		}
		TestEqual(TEXT("Both scales at 0: none of them throws anything"), Effects.NumParticles(), 0);
	}
	HitFxClear(Effects, Camera);
	TestEqual(TEXT("All of it gone in time"), Effects.NumParticles(), 0);
	Effects.Shutdown();
	return true;
}

#endif
