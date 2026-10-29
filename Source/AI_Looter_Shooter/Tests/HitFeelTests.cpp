#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GunfeelReactTuning.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitPose.h"
#include "Combat/HitReaction.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureHitZones.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Weapons/VfxRules.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"

// HITS lane A (2026-10-28; Docs/Work/2026-10-28-Core/HITS.md): enemy hit reactions you can feel, the core rules: a hit's
// weight class, its zone by bone per kind, its side, poise (break, drain, rank scale, the elites' gap), the stagger pose's
// curve, the far boost, the overkill, and the pose pieces the kinds build on. Rules are read with the cvars' defaults (a
// default-made tuning), never what the user has set.

namespace
{
	/** A hit as the rules see it, on a 100-health creature. */
	FHitReaction::FHit HitOf(float Damage, EWeaponKind Kind = EWeaponKind::None, bool bCritical = false, float Distance = 300.f,
		ECreatureRank Rank = ECreatureRank::Basic)
	{
		FHitReaction::FHit Hit;
		Hit.Damage = Damage;
		Hit.MaxHealth = 100.f;
		Hit.bCritical = bCritical;
		Hit.Kind = Kind;
		Hit.Distance = Distance;
		Hit.Rank = Rank;
		return Hit;
	}

	FHitReaction::FHit KillOf(float Damage, float Overkill, EWeaponKind Kind = EWeaponKind::Rifle, bool bCritical = false)
	{
		FHitReaction::FHit Hit = HitOf(Damage, Kind, bCritical);
		Hit.bKilled = true;
		Hit.Overkill = Overkill;
		return Hit;
	}

	/** Seconds of steady fire (Rate hits a second of Share each) at Rank: how many staggers it brings. */
	int32 StaggersUnderFire(ECreatureRank Rank, EWeaponKind Kind, float Share, float Rate, float Seconds)
	{
		FHitReaction Reaction;
		int32 Staggers = 0;
		const int32 Hits = FMath::FloorToInt32(Seconds * Rate);
		for (int32 Index = 0; Index < Hits; ++Index)
		{
			Staggers += Reaction.OnHit(Index / static_cast<double>(Rate), HitOf(Share * 100.f, Kind, false, 800.f, Rank)).StaggerSeconds > 0.f ? 1 : 0;
		}
		return Staggers;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelWeightTest, "Looter.Gunfeel.Hits.Weight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelWeightTest::RunTest(const FString& Parameters)
{
	const FGunfeelReactTuning Tuning;
	const FHitFeelTuning Feel;
	auto Weight = [&](const FHitReaction::FHit& Hit, float BurstShare) { return FHitReaction::WeightOf(Hit, BurstShare, Tuning, Feel); };
	TestTrue(TEXT("A rifle round (5%): a body hit"), Weight(HitOf(5.f, EWeaponKind::Rifle), 0.05f) == EHitWeight::Body);
	TestTrue(TEXT("A burst of 13%: heavy"), Weight(HitOf(5.f, EWeaponKind::Shotgun), 0.13f) == EHitWeight::Heavy);
	TestTrue(TEXT("A crit: heavy"), Weight(HitOf(5.f, EWeaponKind::Rifle, true), 0.05f) == EHitWeight::Heavy);
	TestTrue(TEXT("One hit of a quarter: Big"), Weight(HitOf(25.f, EWeaponKind::Revolver), 0.25f) == EHitWeight::Big);
	TestTrue(TEXT("A sniper's round, however small: Big"), Weight(HitOf(2.f, EWeaponKind::Sniper), 0.02f) == EHitWeight::Big);
	TestTrue(TEXT("A rocket: Big"), Weight(HitOf(2.f, EWeaponKind::Launcher), 0.02f) == EHitWeight::Big);
	TestTrue(TEXT("A blast worth its power up close: Big"), Weight(HitOf(6.f, EWeaponKind::Shotgun, false, 300.f), 0.36f) == EHitWeight::Big);
	TestTrue(TEXT("...past its power's range: heavy"), Weight(HitOf(6.f, EWeaponKind::Shotgun, false, 700.f), 0.36f) == EHitWeight::Heavy);
	FHitReaction::FHit Strike = HitOf(1.f);
	Strike.bMelee = true;
	TestTrue(TEXT("A melee strike always lands heavy"), Weight(Strike, 0.01f) == EHitWeight::Heavy);
	TestTrue(TEXT("A kill a little past dead: a kill"), Weight(KillOf(10.f, 0.1f), 0.1f) == EHitWeight::Kill);
	TestTrue(TEXT("Half its health past dead: an overkill"), Weight(KillOf(60.f, 0.5f), 0.6f) == EHitWeight::Overkill);
	TestTrue(TEXT("A sniper's kill (a Big kill): an overkill"), Weight(KillOf(10.f, 0.f, EWeaponKind::Sniper), 0.1f) == EHitWeight::Overkill);
	FHitFeelTuning Plain;
	Plain.KillStyles = false;
	TestTrue(TEXT("Looter.Hits.KillStyles 0: never more than a kill"),
		FHitReaction::WeightOf(KillOf(90.f, 0.9f, EWeaponKind::Sniper), 0.9f, Tuning, Plain) == EHitWeight::Kill);

	// The rules hand the weight on, and the Big hit gets its own hit-stop (cooled down as a crit's).
	FHitReaction Reaction;
	const FHitReaction::FResponse Big = Reaction.OnHit(0.0, HitOf(30.f, EWeaponKind::Revolver));
	TestTrue(TEXT("The response's weight"), Big.Weight == EHitWeight::Big);
	TestEqual(TEXT("A Big hit's hit-stop: 60 ms"), Big.HitStopSeconds, Feel.HitStopBig, 0.0001f);
	TestEqual(TEXT("...not again inside the cooldown"), Reaction.OnHit(0.1, HitOf(30.f, EWeaponKind::Revolver)).HitStopSeconds, 0.f);
	FHitReaction Precise;
	TestEqual(TEXT("A revolver crit that's Big keeps the crit's longer stop"),
		Precise.OnHit(0.0, HitOf(30.f, EWeaponKind::Revolver, true)).HitStopSeconds, Tuning.RevolverCritHitStop, 0.0001f);
	FHitReaction Heavy;
	TestEqual(TEXT("A heavy body hit: 45 ms"), Heavy.OnHit(0.0, HitOf(13.f)).HitStopSeconds, 0.045f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelZonesTest, "Looter.Gunfeel.Hits.Zones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelZonesTest::RunTest(const FString& Parameters)
{
	using namespace CreatureHitZones;
	// Each kind's bones by name (the class defaults: the zones go by class and bone names, no model needed).
	const ASpiderCreature& Spider = *GetDefault<ASpiderCreature>();
	TestTrue(TEXT("Spider head"), ZoneOf(Spider, TEXT("head"), true) == ECreatureHitZone::Head);
	TestTrue(TEXT("...its fangs are its head"), ZoneOf(Spider, TEXT("fang_l"), false) == ECreatureHitZone::Head);
	TestTrue(TEXT("...body"), ZoneOf(Spider, TEXT("body"), false) == ECreatureHitZone::Body);
	TestTrue(TEXT("...abdomen"), ZoneOf(Spider, TEXT("abdomen"), false) == ECreatureHitZone::Abdomen);
	TestTrue(TEXT("...a femur is a leg"), ZoneOf(Spider, TEXT("femur_0_l"), false) == ECreatureHitZone::Leg);
	TestTrue(TEXT("...a tibia too"), ZoneOf(Spider, TEXT("tibia_3_r"), false) == ECreatureHitZone::Leg);
	TestTrue(TEXT("...no bone: body"), ZoneOf(Spider, NAME_None, false) == ECreatureHitZone::Body);
	TestTrue(TEXT("...a plain spider has no sac"), ZoneOf(Spider, SpitterLook::SacBone, false) == ECreatureHitZone::Body);
	TestTrue(TEXT("Spitter's sac: weak"), ZoneOf(*GetDefault<ASpitterSpider>(), SpitterLook::SacBone, false) == ECreatureHitZone::Weak);
	TestTrue(TEXT("Gravemother's abdomen"), ZoneOf(*GetDefault<AGravemotherCreature>(), AGravemotherCreature::AbdomenBone, true)
		== ECreatureHitZone::Abdomen);
	TestEqual(TEXT("Leg order: front left 0"), SpiderLegOf(TEXT("femur_0_l")), 0);
	TestEqual(TEXT("...front right 1"), SpiderLegOf(TEXT("tibia_0_r")), 1);
	TestEqual(TEXT("...third pair's left foot 4"), SpiderLegOf(TEXT("foot_2_l")), 4);
	TestEqual(TEXT("...back right 7"), SpiderLegOf(TEXT("femur_3_r")), 7);
	TestEqual(TEXT("...not a leg"), SpiderLegOf(TEXT("head")), INDEX_NONE);
	TestEqual(TEXT("...no fifth pair"), SpiderLegOf(TEXT("femur_4_l")), INDEX_NONE);

	const AUnpaidCreature& Unpaid = *GetDefault<AUnpaidCreature>();
	const FUnpaidRigBones& Rig = Unpaid.Rig;
	TestTrue(TEXT("Unpaid head"), ZoneOf(Unpaid, Rig.Head, false) == ECreatureHitZone::Head);
	TestTrue(TEXT("...neck, jaw and hat too"), ZoneOf(Unpaid, Rig.Neck, false) == ECreatureHitZone::Head
		&& ZoneOf(Unpaid, Rig.Jaw, false) == ECreatureHitZone::Head && ZoneOf(Unpaid, Rig.Hat, false) == ECreatureHitZone::Head);
	TestTrue(TEXT("...chest: body"), ZoneOf(Unpaid, Rig.Chest, false) == ECreatureHitZone::Body);
	TestTrue(TEXT("...a crit there: the coal (core)"), ZoneOf(Unpaid, Rig.Chest, true) == ECreatureHitZone::Core
		&& ZoneOf(Unpaid, Rig.Coal, true) == ECreatureHitZone::Core);
	TestTrue(TEXT("...pelvis: body"), ZoneOf(Unpaid, Rig.Pelvis, false) == ECreatureHitZone::Body);
	TestTrue(TEXT("...an upper arm"), ZoneOf(Unpaid, Rig.LeftArm.UpperArm, false) == ECreatureHitZone::Arm);
	TestTrue(TEXT("...a finger is its arm"), Rig.RightArm.Fingers.Num() > 0 && ZoneOf(Unpaid, Rig.RightArm.Fingers[1], false) == ECreatureHitZone::Arm);
	TestTrue(TEXT("...the shroud"), Rig.Shroud.Num() > 2 && ZoneOf(Unpaid, Rig.Shroud[2], false) == ECreatureHitZone::Shroud);
	TestTrue(TEXT("...a strip"), Rig.LeftStrip.Num() > 0 && ZoneOf(Unpaid, Rig.LeftStrip[0], false) == ECreatureHitZone::Shroud);
	TestTrue(TEXT("...its mesh is its body"), ZoneOfHit(Unpaid, Unpaid.GetMesh(), Rig.Pelvis, false) == ECreatureHitZone::Body);
	// (Mutable only because the subobject lookup isn't const: nothing is changed.)
	ABellRingerCreature& Ringer = *GetMutableDefault<ABellRingerCreature>();
	const UPrimitiveComponent* Bell = Cast<UPrimitiveComponent>(Ringer.GetDefaultSubobjectByName(TEXT("BellHitZone")));
	if (TestNotNull(TEXT("The Bell-Ringer's bell zone"), Bell))
	{
		TestTrue(TEXT("...a weak spot"), ZoneOfHit(Ringer, Bell, NAME_None, true) == ECreatureHitZone::Weak);
	}

	const ASlimeCreature& Slime = *GetDefault<ASlimeCreature>();
	TestTrue(TEXT("Slime: its gel is body"), ZoneOf(Slime, TEXT("body"), false) == ECreatureHitZone::Body);
	TestTrue(TEXT("...a crit through the gel: its core"), ZoneOf(Slime, TEXT("body"), true) == ECreatureHitZone::Core);
	TestTrue(TEXT("...the core bone"), ZoneOf(Slime, TEXT("core"), false) == ECreatureHitZone::Core);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelSideTest, "Looter.Gunfeel.Hits.Side",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelSideTest::RunTest(const FString& Parameters)
{
	using namespace HitFeel;
	// Facing +X. Direction is the shot's way (shooter -> wound): a shooter in front fires along -X.
	const FVector Facing = FVector::ForwardVector;
	TestTrue(TEXT("From the front"), SideOf(Facing, FVector(-1.0, 0.0, 0.0)) == EHitSide::Front);
	TestTrue(TEXT("From behind"), SideOf(Facing, FVector(1.0, 0.0, 0.0)) == EHitSide::Back);
	TestTrue(TEXT("From its right (+Y)"), SideOf(Facing, FVector(0.0, -1.0, 0.0)) == EHitSide::Right);
	TestTrue(TEXT("From its left"), SideOf(Facing, FVector(0.0, 1.0, 0.0)) == EHitSide::Left);
	TestTrue(TEXT("45 degrees off the front: front"), SideOf(Facing, FVector(-1.0, -1.0, -0.3)) == EHitSide::Front);
	TestTrue(TEXT("130 degrees: back"), SideOf(Facing, FRotator(0.f, 130.f, 0.f).Vector() * -1.0) == EHitSide::Back);
	TestTrue(TEXT("A shot straight down: front"), SideOf(Facing, FVector(0.0, 0.0, -1.0)) == EHitSide::Front);

	// The offset: + on its right as it faces the shot; the shot's torque turns it + yaw for + offset, whichever side.
	TestEqual(TEXT("A front shot on its right shoulder: +20"), LateralOffsetOf(FVector::ZeroVector, FVector(-1.0, 0.0, 0.0), FVector(0.0, 20.0, 50.0)),
		20.f, 0.01f);
	TestEqual(TEXT("...on its left: -20"), LateralOffsetOf(FVector::ZeroVector, FVector(-1.0, 0.0, 0.0), FVector(0.0, -20.0, 50.0)), -20.f, 0.01f);
	const FVector Wound(0.0, 20.0, 0.0);
	const FVector Shot(1.0, 0.0, 0.0);
	const float Offset = LateralOffsetOf(FVector::ZeroVector, Shot, Wound);
	const double TorqueYaw = FVector::CrossProduct(Wound, Shot).Z;
	TestTrue(FString::Printf(TEXT("A back shot: its sign follows the torque (offset %.1f, torque %.1f)"), Offset, TorqueYaw),
		Offset < 0.f && TorqueYaw < 0.0);
	TestEqual(TEXT("A wound on the axis: no twist"), LateralOffsetOf(FVector::ZeroVector, Shot, FVector(-30.0, 0.0, 40.0)), 0.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelPoiseTest, "Looter.Gunfeel.Hits.Poise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelPoiseTest::RunTest(const FString& Parameters)
{
	const FGunfeelReactTuning Tuning;
	{
		// 6% every 0.1 s (no burst, never heavy): poise climbs 0.03 a hit past its drain and breaks at 0.25 on the eighth.
		FHitReaction Reaction;
		int32 BrokeAt = -1;
		for (int32 Index = 0; Index < 10 && BrokeAt < 0; ++Index)
		{
			const FHitReaction::FResponse Response = Reaction.OnHit(Index * 0.1, HitOf(6.f));
			BrokeAt = Response.Job == FHitReaction::EJob::Poise ? Index : BrokeAt;
		}
		TestEqual(TEXT("Poise breaks on the eighth light hit"), BrokeAt, 7);
		TestEqual(TEXT("...and is spent"), Reaction.GetPoise(0.71), 0.f, 0.001f);
	}
	{
		FHitReaction Slow;
		Slow.OnHit(0.0, HitOf(11.f));
		TestEqual(TEXT("Poise drains 0.3 a second"), Slow.GetPoise(0.2), 0.11f - 0.06f, 0.001f);
		TestEqual(TEXT("...a second later it's gone"), Slow.OnHit(1.0, HitOf(11.f)).StaggerSeconds, 0.f);
		FHitReaction Quick;
		Quick.OnHit(0.0, HitOf(11.f));
		Quick.OnHit(0.1, HitOf(11.f));
		const FHitReaction::FResponse Third = Quick.OnHit(0.2, HitOf(11.f));
		TestTrue(TEXT("A quarter fast staggers a Basic one"), Third.Job == FHitReaction::EJob::Poise && Third.StaggerSeconds > 0.f);
		FHitReaction Tough;
		Tough.OnHit(0.0, HitOf(11.f, EWeaponKind::None, false, 300.f, ECreatureRank::Epic));
		Tough.OnHit(0.1, HitOf(11.f, EWeaponKind::None, false, 300.f, ECreatureRank::Epic));
		TestEqual(TEXT("...not a Gravebound one (x1.35)"), Tough.OnHit(0.2, HitOf(11.f, EWeaponKind::None, false, 300.f, ECreatureRank::Epic)).StaggerSeconds, 0.f);
	}
	TestEqual(TEXT("Rank scale: Basic 1"), FHitReaction::PoiseScaleFor(ECreatureRank::Basic), 1.f);
	TestEqual(TEXT("...Restless 1.15"), FHitReaction::PoiseScaleFor(ECreatureRank::Rare), 1.15f);
	TestEqual(TEXT("...Gravebound 1.35"), FHitReaction::PoiseScaleFor(ECreatureRank::Epic), 1.35f);
	TestEqual(TEXT("...Hallowed 1.6"), FHitReaction::PoiseScaleFor(ECreatureRank::Hallowed), 1.6f);
	TestEqual(TEXT("...Ethereal 1.9"), FHitReaction::PoiseScaleFor(ECreatureRank::Ethereal), 1.9f);
	{
		// The elites' gap: a Gravebound one's next stagger no sooner than 1.6 s after the last began; a Basic one after the cooldown.
		FHitReaction Elite;
		const ECreatureRank Epic = ECreatureRank::Epic;
		TestTrue(TEXT("A Gravebound one staggers"), Elite.OnHit(0.0, HitOf(13.f, EWeaponKind::None, false, 300.f, Epic)).StaggerSeconds > 0.f);
		TestEqual(TEXT("...past its cooldown but inside the gap: no"), Elite.OnHit(1.2, HitOf(13.f, EWeaponKind::None, false, 300.f, Epic)).StaggerSeconds, 0.f);
		TestTrue(TEXT("...after the gap: yes"), Elite.OnHit(1.61, HitOf(13.f, EWeaponKind::None, false, 300.f, Epic)).StaggerSeconds > 0.f);
		FHitReaction Basic;
		Basic.OnHit(0.0, HitOf(13.f));
		TestTrue(TEXT("A Basic one again after stagger + cooldown (1.1 s)"), Basic.OnHit(1.11, HitOf(13.f)).StaggerSeconds > 0.f);
	}
	{
		FHitFeelTuning Off;
		Off.PoiseBreak = 0.f;
		FHitReaction Reaction;
		Reaction.SetFeel(Off);
		int32 Staggers = 0;
		for (int32 Index = 0; Index < 20; ++Index)
		{
			Staggers += Reaction.OnHit(Index * 0.1, HitOf(6.f)).StaggerSeconds > 0.f ? 1 : 0;
		}
		TestEqual(TEXT("Looter.Hits.PoiseBreak 0: off"), Staggers, 0);
	}
	// No stun-lock: 10 s of hard SMG fire (10 hits a second of 4.8% each, ~half its health a second) staggers a Basic one
	// at most 5 times, a Gravebound one at most 3; but it does stagger them (poise works).
	const int32 OnBasic = StaggersUnderFire(ECreatureRank::Basic, EWeaponKind::SMG, 0.048f, 10.f, 10.f);
	const int32 OnElite = StaggersUnderFire(ECreatureRank::Epic, EWeaponKind::SMG, 0.048f, 10.f, 10.f);
	TestTrue(FString::Printf(TEXT("SMG 10 s on a Basic one: %d staggers (1-5)"), OnBasic), OnBasic >= 1 && OnBasic <= 5);
	TestTrue(FString::Printf(TEXT("...on a Gravebound one: %d (1-3)"), OnElite), OnElite >= 1 && OnElite <= 3);
	TestEqual(TEXT("Soulfed: never"), StaggersUnderFire(ECreatureRank::Legendary, EWeaponKind::SMG, 0.048f, 10.f, 10.f), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelStaggerCurveTest, "Looter.Gunfeel.Hits.StaggerCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelStaggerCurveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("None as it begins"), FStaggerCurve::Alpha(0.f, 0.4f), 0.f);
	TestEqual(TEXT("Halfway up its rise: 0.75 (eased out)"), FStaggerCurve::Alpha(0.04f, 0.4f), 0.75f, 0.001f);
	TestEqual(TEXT("Whole at 0.08 s"), FStaggerCurve::Alpha(0.08f, 0.4f), 1.f, 0.001f);
	TestEqual(TEXT("...held to 55%"), FStaggerCurve::Alpha(0.21f, 0.4f), 1.f, 0.001f);
	const float Easing = FStaggerCurve::Alpha(0.31f, 0.4f);
	TestTrue(FString::Printf(TEXT("...easing back (%.2f)"), Easing), Easing > 0.f && Easing < 1.f);
	TestEqual(TEXT("None at its end"), FStaggerCurve::Alpha(0.4f, 0.4f), 0.f);
	TestEqual(TEXT("...or after"), FStaggerCurve::Alpha(0.6f, 0.4f), 0.f);
	TestEqual(TEXT("A short stagger (0.15 s) still reaches its pose"), FStaggerCurve::Alpha(0.081f, 0.15f), 1.f, 0.001f);

	// The rules give it over the stagger running.
	FHitReaction Reaction;
	Reaction.OnHit(1.0, HitOf(13.f));
	TestEqual(TEXT("Mid-stagger: the whole pose"), Reaction.GetStaggerAlpha(1.1), 1.f, 0.001f);
	TestEqual(TEXT("...none after it"), Reaction.GetStaggerAlpha(1.5), 0.f);
	FHitFeelTuning NoPose;
	NoPose.StaggerPose = false;
	FHitReaction Plain;
	Plain.SetFeel(NoPose);
	Plain.OnHit(1.0, HitOf(13.f));
	TestTrue(TEXT("Looter.Hits.StaggerPose 0: staggered, no pose"), Plain.IsStaggered(1.1) && Plain.GetStaggerAlpha(1.1) == 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelFarBoostTest, "Looter.Gunfeel.Hits.FarBoost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelFarBoostTest::RunTest(const FString& Parameters)
{
	using HitFeel::FarBoostAt;
	const FHitFeelTuning Feel;
	TestEqual(TEXT("No shooter known: x1"), FarBoostAt(-1.f, Feel), 1.f);
	TestEqual(TEXT("5 m: x1"), FarBoostAt(500.f, Feel), 1.f);
	TestEqual(TEXT("10 m: x1"), FarBoostAt(1000.f, Feel), 1.f);
	TestEqual(TEXT("17.5 m: x1.25"), FarBoostAt(1750.f, Feel), 1.25f, 0.001f);
	TestEqual(TEXT("25 m: x1.5"), FarBoostAt(2500.f, Feel), 1.5f, 0.001f);
	TestEqual(TEXT("40 m: x1.5"), FarBoostAt(4000.f, Feel), 1.5f, 0.001f);
	FHitFeelTuning Off;
	Off.FarBoost = 1.f;
	TestEqual(TEXT("Looter.Hits.FarBoost 1: off"), FarBoostAt(4000.f, Off), 1.f);

	// On the flinch: a rifle round on a 300-health spider at 10 m 0.97, at 25 m 1.45; never past the cap.
	const FGunfeelReactTuning Tuning;
	auto Strength = [&Tuning, &Feel](float Share, float Distance, bool bCritical = false)
	{
		FCreatureHit Hit;
		Hit.Share = Share;
		Hit.bCritical = bCritical;
		Hit.bFresh = true;
		Hit.FarBoost = FarBoostAt(Distance, Feel);
		return Hit.FlinchStrength(Tuning);
	};
	TestEqual(TEXT("Rifle 12 on 300 at 10 m: 0.97"), Strength(12.f / 300.f, 1000.f), 0.97f, 0.01f);
	TestEqual(TEXT("...at 25 m: 1.45"), Strength(12.f / 300.f, 2500.f), 1.455f, 0.01f);
	TestEqual(TEXT("A huge hit far off: capped at 3"), Strength(0.9f, 3000.f), 3.f, 0.001f);
	TestEqual(TEXT("A crit far off: x1.4 on top"), Strength(12.f / 300.f, 2500.f, true), 1.455f * 1.4f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelOverkillTest, "Looter.Gunfeel.Hits.Overkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelOverkillTest::RunTest(const FString& Parameters)
{
	using UComponent = UCreatureHitReactionComponent;
	TestEqual(TEXT("100 on 40 of 100: 0.6 past dead"), HitFeel::OverkillOf(100.f, 40.f, 100.f), 0.6f, 0.001f);
	TestEqual(TEXT("A hit that didn't kill: none"), HitFeel::OverkillOf(30.f, 40.f, 100.f), 0.f);
	TestEqual(TEXT("Just dead: none"), HitFeel::OverkillOf(50.f, 50.f, 100.f), 0.f);

	const FGunfeelReactTuning Tuning;
	const FHitFeelTuning Feel;
	// Its hit-stop: the overkill's 130 ms anticipation frame, longest of the kills.
	FHitReaction Over;
	TestEqual(TEXT("An overkill's hit-stop: 130 ms"), Over.OnHit(0.0, KillOf(90.f, 0.6f)).HitStopSeconds, Feel.HitStopOverkill, 0.0001f);
	FHitReaction Crit;
	TestEqual(TEXT("A plain crit kill: 110 ms"), Crit.OnHit(0.0, KillOf(10.f, 0.05f, EWeaponKind::Rifle, true)).HitStopSeconds, 0.11f, 0.0001f);
	FHitReaction Kill;
	TestEqual(TEXT("A plain kill: 85 ms"), Kill.OnHit(0.0, KillOf(10.f, 0.05f)).HitStopSeconds, 0.085f, 0.0001f);

	// Its corpse: knocked x1.5, a blast's lifted 60 cm; any other kill as before; KillStyles 0 as before.
	TestEqual(TEXT("A shotgun overkill's knock"), UComponent::CorpseKnockFor(EWeaponKind::Shotgun, false, EHitWeight::Overkill, Tuning, Feel),
		Tuning.KillShotgunKnock * Feel.OverkillKnock, 0.01f);
	TestEqual(TEXT("...a plain shotgun kill's"), UComponent::CorpseKnockFor(EWeaponKind::Shotgun, false, EHitWeight::Kill, Tuning, Feel),
		Tuning.KillShotgunKnock, 0.01f);
	TestEqual(TEXT("...lifted 60 cm"), UComponent::CorpseLiftFor(EWeaponKind::Shotgun, EHitWeight::Overkill, Tuning, Feel), 60.f, 0.01f);
	TestTrue(TEXT("A rocket's overkill: at least 60 cm"), UComponent::CorpseLiftFor(EWeaponKind::Launcher, EHitWeight::Overkill, Tuning, Feel) >= 60.f);
	TestEqual(TEXT("A sniper's overkill slides, unlifted"), UComponent::CorpseLiftFor(EWeaponKind::Sniper, EHitWeight::Overkill, Tuning, Feel), 0.f);
	FHitFeelTuning Plain;
	Plain.KillStyles = false;
	TestEqual(TEXT("Looter.Hits.KillStyles 0: today's knock"), UComponent::CorpseKnockFor(EWeaponKind::Shotgun, false, EHitWeight::Overkill, Tuning, Plain),
		Tuning.KillShotgunKnock, 0.01f);

	// The kill's layers: overkill adds up to 0.6 now (0.3 before), the cap 2.4.
	const float Rifle = VfxRules::KillWeight(EWeaponKind::Rifle, false, 0.3f);
	TestEqual(TEXT("Three times its health: +0.6"), VfxRules::KillWeight(EWeaponKind::Rifle, false, 3.f) - Rifle, 0.6f, 0.001f);
	TestTrue(TEXT("...no more"), VfxRules::KillWeight(EWeaponKind::Rifle, false, 100.f) <= Rifle + 0.6f + 0.001f);
	TestTrue(TEXT("...never past 2.4"), VfxRules::KillWeight(EWeaponKind::Launcher, true, 100.f) <= 2.4f + 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitFeelPoseTest, "Looter.Gunfeel.Hits.Pose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitFeelPoseTest::RunTest(const FString& Parameters)
{
	// The spring: a hit's kick held 0.06 s, followed critically damped (like the spider's jolt), at rest within a second.
	FHitSpring Spring;
	TestTrue(TEXT("At rest to start"), Spring.IsSettled());
	Spring.Add(FVector(10.f, 0.f, 0.f), 0.06f);
	float Peak = 0.f;
	float PeakAt = 0.f;
	constexpr float Frame = 1.f / 60.f;
	for (float Time = Frame; Time < 1.5f; Time += Frame)
	{
		Spring.Step(Frame);
		if (Spring.Offset.X > Peak)
		{
			Peak = static_cast<float>(Spring.Offset.X);
			PeakAt = Time;
		}
		if (FMath::IsNearlyEqual(Time, 0.1f, Frame * 0.5f))
		{
			Spring.Add(FVector(3.f, 0.f, 0.f), 0.06f);
		}
	}
	TestTrue(FString::Printf(TEXT("A hit peaks at %.2f of its kick (0.7-1.0)"), Peak / 10.f), Peak > 7.f && Peak <= 10.f);
	TestTrue(FString::Printf(TEXT("...%.0f ms on (50-150)"), PeakAt * 1000.f), PeakAt >= 0.05f && PeakAt <= 0.15f);
	TestTrue(TEXT("...at rest by 1.5 s (a weaker kick meanwhile ignored)"), Spring.IsSettled());

	TestEqual(TEXT("Envelope: none before"), HitEnvelope(0.f, 0.04f, 0.14f), 0.f);
	TestEqual(TEXT("...whole at its in"), HitEnvelope(0.04f, 0.04f, 0.14f), 1.f, 0.001f);
	const float Out = HitEnvelope(0.11f, 0.04f, 0.14f);
	TestTrue(FString::Printf(TEXT("...going (%.2f)"), Out), Out > 0.f && Out < 1.f);
	TestEqual(TEXT("...gone after"), HitEnvelope(0.2f, 0.04f, 0.14f), 0.f);

	TestEqual(TEXT("Wound stage: half or more 0"), WoundStage(0.5f), 0);
	TestEqual(TEXT("...under half 1"), WoundStage(0.49f), 1);
	TestEqual(TEXT("...a quarter 1"), WoundStage(0.25f), 1);
	TestEqual(TEXT("...under a quarter 2"), WoundStage(0.2f), 2);
	return true;
}

#endif
