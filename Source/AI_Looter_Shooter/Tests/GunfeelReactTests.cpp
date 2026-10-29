#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GunfeelReactTuning.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Tests/BossTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"

// Gunfeel step 1 and 3, the target's side (GUNFEEL-REACT; Saved/Gunfeel/CONTRACT.md): every hit flinches by its size, the
// brief slow, staggers more often, heavy hit-stops, the rifle's pressure, the shotgun's power, the revolver's long crit
// stop, kills by gun and the spurts' budget. Rules are read with the cvars' defaults (a default-made FGunfeelReactTuning),
// never what the user has set.

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A hit as the rules see it. */
	FHitReaction::FHit GunHit(float Damage, EWeaponKind Kind, bool bCritical = false, float Distance = 300.f, float Size = 1.f,
		ECreatureRank Rank = ECreatureRank::Basic)
	{
		FHitReaction::FHit Hit;
		Hit.Damage = Damage;
		Hit.MaxHealth = 100.f;
		Hit.bCritical = bCritical;
		Hit.Rank = Rank;
		Hit.Kind = Kind;
		Hit.Distance = Distance;
		Hit.Size = Size;
		return Hit;
	}

	/** A creature on a floor (its feet at 0), started as play starts it, its movement working, dropping nothing. */
	template <typename TCreature>
	TCreature* SpawnOnFloor(UWorld* World, const FVector& Where)
	{
		TCreature* Creature = World->SpawnActor<TCreature>(Where + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Creature)
		{
			return nullptr;
		}
		if (ULootDropComponent* Loot = Creature->template FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Creature->DispatchBeginPlay();
		const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Creature->SetActorLocation(Where + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
		UCharacterMovementComponent* Movement = Creature->GetCharacterMovement();
		Movement->SetUpdatedComponent(Creature->GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Activate(true);
		return Creature;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactFlinchTest, "Looter.Gunfeel.React.Flinch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactFlinchTest::RunTest(const FString& Parameters)
{
	// Every hit flinches, sized by its share of health: Scale x clamp(0.85 + 3 x share, 0.85, 3), a crit x1.4, a boss x
	// BossScale (HITS 2026-10-28 raised the floor 0.6 -> 0.85 and the cap 2.5 -> 3; the far boost is Looter.Gunfeel.Hits.FarBoost's).
	const FGunfeelReactTuning Tuning;
	auto Strength = [&Tuning](float Share, bool bCritical = false, bool bBoss = false, float BurstShare = 0.f)
	{
		FCreatureHit Hit;
		Hit.Share = Share;
		Hit.BurstShare = BurstShare;
		Hit.bCritical = bCritical;
		Hit.bBoss = bBoss;
		Hit.bFresh = true;
		return Hit.FlinchStrength(Tuning);
	};
	TestEqual(TEXT("A scratch still flinches (the floor)"), Strength(0.001f), 0.85f, 0.01f);
	TestEqual(TEXT("A rifle round on a 300-health spider (12): 0.97"), Strength(12.f / 300.f), 0.97f, 0.01f);
	TestEqual(TEXT("A revolver round on it (52): about 1.37"), Strength(52.f / 300.f), 1.37f, 0.01f);
	TestTrue(TEXT("The revolver's rocks it harder than the rifle's"), Strength(52.f / 300.f) > Strength(12.f / 300.f) * 1.4f);
	TestEqual(TEXT("A third of its health: 1.85"), Strength(1.f / 3.f), 1.85f, 0.01f);
	TestEqual(TEXT("Capped at 3"), Strength(0.9f), 3.f, 0.001f);
	TestEqual(TEXT("A crit x1.4"), Strength(0.1f, true), 1.15f * 1.4f, 0.01f);
	TestEqual(TEXT("A boss: today's flinch x BossScale (1), whatever the hit"), Strength(0.1f, true, true), 1.f, 0.01f);
	TestEqual(TEXT("A blast's last pellet carries the blast's share"), Strength(0.05f, false, false, 0.3f), 1.75f, 0.01f);
	FCreatureHit Stale;
	Stale.Share = 0.5f;
	TestEqual(TEXT("A flinch not from a hit (a boss's own jolt): its own size"), Stale.FlinchStrength(Tuning), 1.f);
	FGunfeelReactTuning Doubled;
	Doubled.FlinchScale = 2.f;
	FCreatureHit Plain;
	Plain.Share = 0.1f;
	Plain.bFresh = true;
	TestEqual(TEXT("Flinch.Scale scales them all"), Plain.FlinchStrength(Doubled), 2.3f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactStaggerTest, "Looter.Gunfeel.React.Stagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactStaggerTest::RunTest(const FString& Parameters)
{
	const FGunfeelReactTuning Tuning;
	// Staggers come more often: an eighth of its health is heavy (a fifth before), Basic 0.4 s (0.32), cooldown 0.7 (1.1).
	{
		FHitReaction Reaction;
		TestEqual(TEXT("11%: no stagger"), Reaction.OnHit(0.0, GunHit(11.f, EWeaponKind::None)).StaggerSeconds, 0.f);
		const FHitReaction::FResponse Heavy = Reaction.OnHit(1.0, GunHit(13.f, EWeaponKind::None));
		TestEqual(TEXT("13%: a Basic one staggers 0.4 s"), Heavy.StaggerSeconds, 0.4f, 0.001f);
		TestTrue(TEXT("...a heavy hit"), Heavy.Job == FHitReaction::EJob::Heavy);
		TestEqual(TEXT("...with a heavy hit's stop"), Heavy.HitStopSeconds, Tuning.HitStopHeavy, 0.0001f);
		const double Ready = 1.0 + 0.4 + Tuning.StaggerCooldown;
		TestEqual(TEXT("Not again before the cooldown"), Reaction.OnHit(Ready - 0.05, GunHit(20.f, EWeaponKind::None)).StaggerSeconds, 0.f);
		TestEqual(TEXT("...then again"), Reaction.OnHit(Ready + 0.01, GunHit(20.f, EWeaponKind::None)).StaggerSeconds, 0.4f, 0.001f);
	}
	// The ranks keep their proportions of a Basic one's.
	TestEqual(TEXT("Restless 0.325 s"), FHitReaction::StaggerSecondsFor(ECreatureRank::Rare), 0.4f * 0.26f / 0.32f, 0.001f);
	TestEqual(TEXT("Gravebound 0.25 s"), FHitReaction::StaggerSecondsFor(ECreatureRank::Epic), 0.25f, 0.001f);
	TestEqual(TEXT("Stagger.Seconds tunes them all"), FHitReaction::StaggerSecondsFor(ECreatureRank::Epic, 0.8f), 0.5f, 0.001f);
	TestEqual(TEXT("Soulfed: never"), FHitReaction::StaggerSecondsFor(ECreatureRank::Legendary), 0.f);

	// Hit-stops: a crit 50 ms, a revolver's 90, a kill 85, a crit kill 110 (HITS raised both); a heavy body hit shares the
	// crit's cooldown. The kills are small last hits: a Big kill is an overkill (Looter.Gunfeel.Hits.Overkill).
	{
		FHitReaction Reaction;
		TestEqual(TEXT("A rifle crit: 50 ms"), Reaction.OnHit(0.0, GunHit(5.f, EWeaponKind::Rifle, true)).HitStopSeconds, Tuning.HitStopCrit);
		TestEqual(TEXT("A heavy hit in the crit's cooldown: none"), Reaction.OnHit(0.1, GunHit(20.f, EWeaponKind::None)).HitStopSeconds, 0.f);
		FHitReaction Revolver;
		TestEqual(TEXT("A revolver crit: 90 ms"), Revolver.OnHit(0.0, GunHit(5.f, EWeaponKind::Revolver, true)).HitStopSeconds,
			Tuning.RevolverCritHitStop);
		TestTrue(TEXT("...the longest of the crits"), Tuning.RevolverCritHitStop > Tuning.HitStopCrit);
		FHitReaction Kill;
		FHitReaction::FHit Killing = GunHit(10.f, EWeaponKind::Rifle);
		Killing.bKilled = true;
		TestEqual(TEXT("A kill: 85 ms"), Kill.OnHit(0.0, Killing).HitStopSeconds, Tuning.HitStopKill);
		TestEqual(TEXT("...85 ms by default"), Tuning.HitStopKill, 0.085f, 0.0001f);
		Killing.bCritical = true;
		FHitReaction CritKill;
		TestEqual(TEXT("A crit kill: 110 ms"), CritKill.OnHit(0.0, Killing).HitStopSeconds, Tuning.HitStopCritKill);
		TestEqual(TEXT("...110 ms by default"), Tuning.HitStopCritKill, 0.11f, 0.0001f);
		FHitReaction Boss;
		FHitReaction::FHit OnBoss = GunHit(50.f, EWeaponKind::Revolver, true, 300.f, 1.f, ECreatureRank::Boss);
		const FHitReaction::FResponse BossResponse = Boss.OnHit(0.0, OnBoss);
		TestTrue(TEXT("A boss: nothing new"), BossResponse.HitStopSeconds == 0.f && BossResponse.StaggerSeconds == 0.f && !BossResponse.bKnockBack);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactRiflePressureTest, "Looter.Gunfeel.React.RiflePressure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactRiflePressureTest::RunTest(const FString& Parameters)
{
	// The rifle's job: 5 light hits within 1 s stagger it, with the stagger's cooldown after.
	const FGunfeelReactTuning Tuning;
	FHitReaction Reaction;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		TestEqual(FString::Printf(TEXT("Rifle hit %d: no stagger"), Index + 1),
			Reaction.OnHit(Index * 0.15, GunHit(4.f, EWeaponKind::Rifle)).StaggerSeconds, 0.f);
	}
	const FHitReaction::FResponse Fifth = Reaction.OnHit(0.6, GunHit(4.f, EWeaponKind::Rifle));
	TestEqual(TEXT("The fifth staggers"), Fifth.StaggerSeconds, Tuning.StaggerSeconds, 0.001f);
	TestTrue(TEXT("...the rifle's pressure"), Fifth.Job == FHitReaction::EJob::Pressure);
	double Now = 0.6 + Tuning.StaggerSeconds + 0.05;
	for (int32 Index = 0; Index < 5; ++Index, Now += 0.1)
	{
		TestEqual(TEXT("In the cooldown: pressure builds no stagger"), Reaction.OnHit(Now, GunHit(4.f, EWeaponKind::Rifle)).StaggerSeconds, 0.f);
	}
	// Hits spread out, or another gun's, don't count.
	FHitReaction Slow;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		TestEqual(TEXT("A rifle hit every 0.3 s: never 5 in a second"), Slow.OnHit(Index * 0.3, GunHit(4.f, EWeaponKind::Rifle)).StaggerSeconds, 0.f);
	}
	FHitReaction Revolver;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		TestEqual(TEXT("Revolver hits build no pressure"), Revolver.OnHit(Index * 0.1, GunHit(4.f, EWeaponKind::Revolver)).StaggerSeconds, 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactShotgunPowerTest, "Looter.Gunfeel.React.ShotgunPower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactShotgunPowerTest::RunTest(const FString& Parameters)
{
	// The shotgun's job: a blast worth 35% within 6 m knocks a small one back (a big one staggered), once a blast.
	const FGunfeelReactTuning Tuning;
	{
		FHitReaction Blast;
		bool bKnocked = false;
		int32 Knocks = 0;
		// Eight pellets of 6%: the heavy share (12%) staggers at the second, the power (35%) takes over at the sixth.
		for (int32 Pellet = 0; Pellet < 8; ++Pellet)
		{
			const FHitReaction::FResponse Response = Blast.OnHit(10.0, GunHit(6.f, EWeaponKind::Shotgun));
			Knocks += Response.bKnockBack ? 1 : 0;
			if (Pellet == 5)
			{
				bKnocked = Response.bKnockBack && Response.Job == FHitReaction::EJob::Power;
			}
		}
		TestTrue(TEXT("The pellet that makes 35% knocks it back"), bKnocked);
		TestEqual(TEXT("...once a blast"), Knocks, 1);
		TestFalse(TEXT("...the stagger the blast began gives way to it"), Blast.IsStaggered(10.01));
		const FHitReaction::FResponse Next = Blast.OnHit(11.0, GunHit(40.f, EWeaponKind::Shotgun));
		TestTrue(TEXT("The next blast knocks again (each has taken a third of its health)"), Next.bKnockBack);
	}
	{
		FHitReaction Big;
		const FHitReaction::FResponse Response = Big.OnHit(0.0, GunHit(40.f, EWeaponKind::Shotgun, false, 300.f, 1.5f));
		TestTrue(TEXT("A big one (size 1.5) is staggered instead"), !Response.bKnockBack && Response.StaggerSeconds > 0.f
			&& Response.Job == FHitReaction::EJob::Power);
	}
	{
		FHitReaction Far;
		const FHitReaction::FResponse Response = Far.OnHit(0.0, GunHit(40.f, EWeaponKind::Shotgun, false, Tuning.ShotgunKnockRange + 50.f));
		TestTrue(TEXT("Past 6 m: only a heavy hit's stagger"), !Response.bKnockBack && Response.Job == FHitReaction::EJob::Heavy);
		FHitReaction Unknown;
		TestFalse(TEXT("No shooter known: no knock"), Unknown.OnHit(0.0, GunHit(40.f, EWeaponKind::Shotgun, false, -1.f)).bKnockBack);
		FHitReaction Revolver;
		TestFalse(TEXT("A revolver's 40%: no knock"), Revolver.OnHit(0.0, GunHit(40.f, EWeaponKind::Revolver)).bKnockBack);
		FHitReaction Light;
		TestFalse(TEXT("A blast worth 30%: no knock"), Light.OnHit(0.0, GunHit(30.f, EWeaponKind::Shotgun)).bKnockBack);
	}
	for (const ECreatureRank Rank : { ECreatureRank::Legendary, ECreatureRank::Boss })
	{
		FHitReaction Never;
		const FHitReaction::FResponse Response = Never.OnHit(0.0, GunHit(60.f, EWeaponKind::Shotgun, false, 200.f, 1.f, Rank));
		TestTrue(FString::Printf(TEXT("%s: never knocked or staggered"), *UEnum::GetValueAsString(Rank)), !Response.bKnockBack && Response.StaggerSeconds == 0.f);
	}
	// The knock's distance from its launch speed: 520 cm/s across, 40 cm high, under 980 cm/s^2: about 3 m.
	TestEqual(TEXT("The knock covers about 297 cm"), GunfeelReact::KnockDistance(520.f, 40.f, 980.f), 297.1f, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactKillsTest, "Looter.Gunfeel.React.Kills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactKillsTest::RunTest(const FString& Parameters)
{
	// Kills by gun: a shotgun throws the corpse (650 cm/s, lifted 35 cm), a rifle's crumples (180), the rest as before;
	// a crit keeps its old knock where that's larger.
	using UComponent = UCreatureHitReactionComponent;
	const FGunfeelReactTuning Tuning;
	TestEqual(TEXT("Shotgun kill"), UComponent::CorpseKnockFor(EWeaponKind::Shotgun, false, Tuning), 650.f);
	TestEqual(TEXT("...a crit's no less"), UComponent::CorpseKnockFor(EWeaponKind::Shotgun, true, Tuning), 650.f);
	TestEqual(TEXT("...lifted"), UComponent::CorpseLiftFor(EWeaponKind::Shotgun, Tuning), 35.f);
	TestEqual(TEXT("Rifle kill: crumples"), UComponent::CorpseKnockFor(EWeaponKind::Rifle, false, Tuning), 180.f);
	TestEqual(TEXT("...a rifle headshot keeps the crit's knock"), UComponent::CorpseKnockFor(EWeaponKind::Rifle, true, Tuning),
		UComponent::CritCorpseKnock);
	TestEqual(TEXT("Revolver kill: as before"), UComponent::CorpseKnockFor(EWeaponKind::Revolver, false, Tuning), UComponent::CorpseKnock);
	TestEqual(TEXT("...not lifted"), UComponent::CorpseLiftFor(EWeaponKind::Revolver, Tuning), 0.f);
	TestEqual(TEXT("Not a gun: as before"), UComponent::CorpseKnockFor(EWeaponKind::None, true, Tuning), UComponent::CritCorpseKnock);
	TestEqual(TEXT("Nothing's gun"), UComponent::KindOf(nullptr), EWeaponKind::None);
	TestTrue(TEXT("The shotgun throws farthest"), UComponent::CorpseKnockFor(EWeaponKind::Shotgun, false, Tuning)
		> UComponent::CorpseKnockFor(EWeaponKind::Revolver, true, Tuning));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactSpurtTest, "Looter.Gunfeel.React.Spurts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactSpurtTest::RunTest(const FString& Parameters)
{
	// The budget rule: what's left, no more than wanted.
	TestEqual(TEXT("A pellet gets what it wants while there's budget"), GunfeelReact::BlastSpurtShare(3.f, 0.f, 0.75f), 0.75f);
	TestEqual(TEXT("...what's left near the end"), GunfeelReact::BlastSpurtShare(3.f, 2.7f, 0.75f), 0.3f, 0.001f);
	TestEqual(TEXT("...nothing past it"), GunfeelReact::BlastSpurtShare(3.f, 3.f, 0.75f), 0.f);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const ECreatureBlood Gel = ECreatureBlood::Gel;
	const FLinearColor Tint = CreatureBlood::DefaultTint(Gel);
	const FVector Shot(1.0, 0.0, -0.2);

	// A full hit spills more than before (Spurt.Scale 1.5, with its exit spray).
	int32 Before = Effects.NumParticles();
	Effects.SpawnCreatureHit(FVector(0.0, 0.0, 100.0), -Shot, Shot, Gel, Tint, 1.f, false);
	const int32 Full = Effects.NumParticles() - Before;
	TestTrue(FString::Printf(TEXT("A full hit's spurt (%d particles) is over today's 10"), Full), Full > 10);

	// A blast's nine pellets far from it share one budget: a couple of full hits' worth, not nine half ones.
	Before = Effects.NumParticles();
	int32 Last = 0;
	for (int32 Pellet = 0; Pellet < 9; ++Pellet)
	{
		const int32 At = Effects.NumParticles();
		Effects.SpawnCreatureHit(FVector(5000.0, 0.0, 100.0), -Shot, Shot, Gel, Tint, 0.5f, false);
		Last = Effects.NumParticles() - At;
	}
	const int32 Blast = Effects.NumParticles() - Before;
	TestTrue(FString::Printf(TEXT("Nine pellets spill %d: under three full hits' (%d)"), Blast, Full * 3), Blast < Full * 3);
	TestTrue(FString::Printf(TEXT("...and something (%d)"), Blast), Blast > Full);
	TestEqual(TEXT("The last pellet's past the budget: nothing"), Last, 0);
	Effects.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReactInPlayTest, "Looter.Gunfeel.React.InPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReactInPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// A floor 60 m across with its top at 0.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ASpiderCreature* Hit = SpawnOnFloor<ASpiderCreature>(World, FVector(0.0, 0.0, 0.0));
	ASpiderCreature* Thrown = SpawnOnFloor<ASpiderCreature>(World, FVector(0.0, 1500.0, 0.0));
	if (!TestNotNull(TEXT("A spider"), Hit) || !TestNotNull(TEXT("Another"), Thrown))
	{
		return false;
	}

	// Any hit holds it back a moment (the slow), after the brain's say in its frame.
	BossTestWorld::Hurt(Hit, 1.f);
	const UMovementSlowComponent* Slow = Hit->FindComponentByClass<UMovementSlowComponent>();
	if (TestNotNull(TEXT("A hit slows it"), Slow))
	{
		const FGunfeelReactTuning Tuning;
		TestEqual(TEXT("...to 55%"), Slow->GetMultiplier(), Tuning.FlinchSlow, 0.001f);
		TestTrue(TEXT("...for a moment"), Slow->GetTimeLeft() > 0.f && Slow->GetTimeLeft() <= Tuning.FlinchSlowSeconds + 0.001f);
		TestTrue(TEXT("...ticking after the creature's brain"), Slow->PrimaryComponentTick.TickGroup == TG_PostPhysics);
	}

	// The shotgun's knock-back: rocked back and thrown, no stars.
	UCreatureKnockComponent* Knock = Thrown->GetKnock();
	if (TestNotNull(TEXT("Its knock"), Knock))
	{
		const FVector Launch = Knock->Knock(FVector::ForwardVector, 300.f, 40.f, 0.25f, ECreatureKnockStyle::Shove);
		TestTrue(FString::Printf(TEXT("Thrown back (%s)"), *Launch.ToCompactString()), Launch.X > 0.0 && Launch.Z > 0.0);
		TestTrue(TEXT("...in the shove style"), Knock->IsKnocked() && Knock->GetStyle() == ECreatureKnockStyle::Shove);
		Knock->NoteLanded();
		TestTrue(TEXT("...gathering itself after landing"), Knock->IsDizzy() && Knock->GetStyle() == ECreatureKnockStyle::Shove);
		Knock->Daze(1.f);
		TestTrue(TEXT("A daze on top shows its stars"), Knock->GetStyle() == ECreatureKnockStyle::Fling);
		Knock->Recover();
	}

	// A big hit flinches it farther than a scratch (with SK_Spider imported: its pose is code).
	if (Hit->GetBonePose().IsEmpty())
	{
		AddWarning(TEXT("SK_Spider isn't imported: the flinch's size isn't checked."));
		return true;
	}
	auto PeakAfter = [Hit](float Damage)
	{
		for (float Elapsed = 0.f; Elapsed < 1.f; Elapsed += Frame)
		{
			Hit->Tick(Frame);
		}
		const FVector Before = Hit->GetBonePose()[0].Transform.GetLocation();
		BossTestWorld::Hurt(Hit, Damage);
		float Peak = 0.f;
		for (float Elapsed = 0.f; Elapsed < 0.4f; Elapsed += Frame)
		{
			Hit->Tick(Frame);
			Peak = FMath::Max(Peak, static_cast<float>(FVector::Dist(Hit->GetBonePose()[0].Transform.GetLocation(), Before)));
		}
		return Peak;
	};
	const float Scratch = PeakAfter(1.f);
	const UHealthComponent* Health = Hit->FindComponentByClass<UHealthComponent>();
	const float Heavy = PeakAfter(Health ? Health->GetMaxHealth() * 0.3f : 90.f);
	// The strengths are 1.75 against 0.85 since HITS raised the floor (2.5 against 0.6 before): half as far again at least.
	TestTrue(FString::Printf(TEXT("A third of its health flinches it farther (%.1f cm) than a scratch (%.1f cm)"), Heavy, Scratch),
		Heavy > Scratch * 1.5f);
	return true;
}

#endif
