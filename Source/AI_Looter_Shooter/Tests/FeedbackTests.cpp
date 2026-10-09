#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "UI/HUD/HudDamageIndicatorWidget.h"
#include "UI/World/DamageNumberActor.h"
#include "World/LightBeam.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The feedback pass's rules (Docs/Polish/BorderlandsComparison.md, items 2 and 10): hit-stop, stagger, the view's kicks
// and their setting, the damage indicator's angle, the loot fanfare and the damage numbers' motion.

namespace
{
	constexpr float Millisecond = 0.001f;

	/** A creature in a test world, started as play starts it, dropping nothing when it dies. */
	template <typename TCreature>
	TCreature* SpawnStarted(UWorld* World, const FVector& Location, ECreatureRank Rank = ECreatureRank::Basic)
	{
		TCreature* Creature = World->SpawnActor<TCreature>(Location, FRotator::ZeroRotator);
		if (Creature)
		{
			Creature->StartingRank = Rank;
			if (ULootDropComponent* Loot = Creature->template FindComponentByClass<ULootDropComponent>())
			{
				Loot->bDropOnDeath = false;
			}
			Creature->DispatchBeginPlay();
		}
		return Creature;
	}

	/**
	 * A shot's damage as AActor::TakeDamage hands it to its listeners: a test level has no game mode, so a pawn would
	 * ignore ApplyDamage, and the engine drops actors' own events until play begins (BossTestWorld::Hurt).
	 */
	void Shoot(AActor* Victim, float Damage, bool bCritical)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		const UDamageType* Type = bCritical ? GetDefault<UWeaponCritDamageType>() : GetDefault<UWeaponDamageType>();
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, Type, nullptr, nullptr);
	}

	/** The highest value of Read while the stack plays out over Seconds, in millisecond steps. */
	template <typename TRead>
	float PeakOver(FViewKickStack& Stack, float Seconds, TRead Read)
	{
		float Peak = 0.f;
		for (float Time = 0.f; Time < Seconds; Time += Millisecond)
		{
			Stack.Tick(Millisecond);
			const float Value = Read(Stack);
			Peak = FMath::Abs(Value) > FMath::Abs(Peak) ? Value : Peak;
		}
		return Peak;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitStopRulesTest, "Looter.Feedback.HitStopRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitStopRulesTest::RunTest(const FString& Parameters)
{
	using FResponse = FHitReaction::FResponse;
	// Only a crit or the kill stops a creature's clock, for 40-80 ms; a plain hit never does.
	{
		FHitReaction Reaction;
		TestEqual(TEXT("A plain hit: no hit-stop"), Reaction.OnHit(0.0, 5.f, 100.f, false, false, ECreatureRank::Basic).HitStopSeconds, 0.f);
		const FResponse Crit = Reaction.OnHit(1.0, 5.f, 100.f, true, false, ECreatureRank::Basic);
		TestEqual(TEXT("A crit: 45 ms"), Crit.HitStopSeconds, FHitReaction::CritHitStop);
		TestTrue(TEXT("...and it's stopped meanwhile"), Reaction.IsHitStopped(1.02) && !Reaction.IsHitStopped(1.05));
		// The crit cooldown: a rifle on its head doesn't freeze it half the time...
		TestEqual(TEXT("Another crit 0.1 s later: none"), Reaction.OnHit(1.1, 5.f, 100.f, true, false, ECreatureRank::Basic).HitStopSeconds, 0.f);
		TestEqual(TEXT("...one after the cooldown: again"), Reaction.OnHit(1.3, 5.f, 100.f, true, false, ECreatureRank::Basic).HitStopSeconds,
			FHitReaction::CritHitStop);
		// ...but the kill always lands with weight.
		TestEqual(TEXT("A kill in the cooldown still stops it"), Reaction.OnHit(1.35, 5.f, 100.f, false, true, ECreatureRank::Basic).HitStopSeconds,
			FHitReaction::KillHitStop);
	}
	{
		FHitReaction Reaction;
		TestEqual(TEXT("A critical kill: 80 ms"), Reaction.OnHit(0.0, 50.f, 100.f, true, true, ECreatureRank::Rare).HitStopSeconds,
			FHitReaction::CritKillHitStop);
	}
	for (const float Seconds : { FHitReaction::CritHitStop, FHitReaction::KillHitStop, FHitReaction::CritKillHitStop })
	{
		TestTrue(FString::Printf(TEXT("%.0f ms is within 40-80 ms"), Seconds * 1000.f), Seconds >= 0.04f - KINDA_SMALL_NUMBER && Seconds <= 0.08f + KINDA_SMALL_NUMBER);
	}
	TestTrue(TEXT("Its clock all but stops (never quite 0)"), FHitReaction::HitStopDilation > 0.f && FHitReaction::HitStopDilation < 0.1f);
	// A boss's own code runs its big moments: no hit-stop on one.
	{
		FHitReaction Reaction;
		TestEqual(TEXT("A boss's crit: none"), Reaction.OnHit(0.0, 5.f, 100.f, true, false, ECreatureRank::Boss).HitStopSeconds, 0.f);
		TestEqual(TEXT("A boss's death: none"), Reaction.OnHit(0.0, 5.f, 100.f, true, true, ECreatureRank::Boss).HitStopSeconds, 0.f);
		FHitReaction Legendary;
		TestEqual(TEXT("A Soulfed monster's crit: yes"), Legendary.OnHit(0.0, 5.f, 100.f, true, false, ECreatureRank::Legendary).HitStopSeconds,
			FHitReaction::CritHitStop);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaggerCooldownTest, "Looter.Feedback.StaggerCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStaggerCooldownTest::RunTest(const FString& Parameters)
{
	// A heavy hit staggers (a crit, or a fifth of its health in one blast), then nothing staggers it again until its
	// stagger is over and the cooldown has passed: no stun-lock.
	FHitReaction Reaction;
	TestEqual(TEXT("A light body shot: no stagger"), Reaction.OnHit(0.0, 10.f, 100.f, false, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
	const float Basic = FHitReaction::StaggerSecondsFor(ECreatureRank::Basic);
	TestEqual(TEXT("A crit staggers a Basic one"), Reaction.OnHit(1.0, 10.f, 100.f, true, false, ECreatureRank::Basic).StaggerSeconds, Basic);
	TestTrue(TEXT("...staggered meanwhile"), Reaction.IsStaggered(1.0 + Basic * 0.5) && !Reaction.IsStaggered(1.0 + Basic + 0.01));
	TestEqual(TEXT("Another crit during it: no more"), Reaction.OnHit(1.1, 10.f, 100.f, true, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
	const double Ready = 1.0 + Basic + FHitReaction::StaggerCooldown;
	TestEqual(TEXT("Another just before the cooldown ends: none"), Reaction.OnHit(Ready - 0.05, 10.f, 100.f, true, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
	TestEqual(TEXT("...once it has: staggered again"), Reaction.OnHit(Ready + 0.01, 10.f, 100.f, true, false, ECreatureRank::Basic).StaggerSeconds, Basic);
	TestTrue(TEXT("The cooldown is longer than a stagger"), FHitReaction::StaggerCooldown > Basic);

	// A shotgun blast up close: its pellets land together and add up.
	{
		FHitReaction Blast;
		TestEqual(TEXT("Pellet 1 (8%)"), Blast.OnHit(5.0, 8.f, 100.f, false, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
		TestEqual(TEXT("Pellet 2 (16%)"), Blast.OnHit(5.0, 8.f, 100.f, false, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
		const FHitReaction::FResponse Third = Blast.OnHit(5.01, 8.f, 100.f, false, false, ECreatureRank::Basic);
		TestEqual(TEXT("Pellet 3 (24%) staggers"), Third.StaggerSeconds, Basic);
		TestEqual(TEXT("...its burst's share"), Third.BurstShare, 0.24f, 0.001f);
		FHitReaction Far;
		Far.OnHit(5.0, 8.f, 100.f, false, false, ECreatureRank::Basic);
		Far.OnHit(5.1, 8.f, 100.f, false, false, ECreatureRank::Basic);
		TestEqual(TEXT("Pellets spread over time don't add up"), Far.OnHit(5.2, 8.f, 100.f, false, false, ECreatureRank::Basic).StaggerSeconds, 0.f);
	}

	// Tougher ranks stagger less; a Soulfed monster or a boss never; nothing staggers on the killing blow.
	TestTrue(TEXT("Restless shorter than Basic"), FHitReaction::StaggerSecondsFor(ECreatureRank::Rare) < Basic);
	TestTrue(TEXT("Gravebound shorter still"), FHitReaction::StaggerSecondsFor(ECreatureRank::Epic) < FHitReaction::StaggerSecondsFor(ECreatureRank::Rare));
	for (const ECreatureRank Rank : { ECreatureRank::Legendary, ECreatureRank::Boss })
	{
		FHitReaction Big;
		TestEqual(FString::Printf(TEXT("%s: never staggered"), *UEnum::GetValueAsString(Rank)), Big.OnHit(0.0, 90.f, 100.f, true, false, Rank).StaggerSeconds, 0.f);
	}
	FHitReaction Killed;
	TestEqual(TEXT("The kill doesn't stagger"), Killed.OnHit(0.0, 100.f, 100.f, true, true, ECreatureRank::Basic).StaggerSeconds, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitStopVictimOnlyTest, "Looter.Feedback.HitStopVictimOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHitStopVictimOnlyTest::RunTest(const FString& Parameters)
{
	// In a level: a crit stops only the creature it hit (its own clock), never another one or the world; a plain hit
	// stops nothing; the kill stops the one killed.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Hit = SpawnStarted<ASpiderCreature>(World, FVector::ZeroVector);
	ASpiderCreature* Beside = SpawnStarted<ASpiderCreature>(World, FVector(400.0, 0.0, 0.0));
	ASlimeCreature* Plain = SpawnStarted<ASlimeCreature>(World, FVector(0.0, 3000.0, 0.0));
	if (!TestNotNull(TEXT("Spiders and slime spawned"), Hit) || !TestNotNull(TEXT("..."), Beside) || !TestNotNull(TEXT("..."), Plain))
	{
		return false;
	}
	const UCreatureHitReactionComponent* Reaction = Hit->FindComponentByClass<UCreatureHitReactionComponent>();
	if (!TestNotNull(TEXT("Every creature has its hit reactions"), Reaction))
	{
		return false;
	}
	const float WorldDilation = World->GetWorldSettings() ? World->GetWorldSettings()->TimeDilation : 1.f;

	Shoot(Hit, 1.f, /*bCritical*/ true);
	TestEqual(TEXT("A crit stops the creature it hit"), Hit->CustomTimeDilation, FHitReaction::HitStopDilation);
	TestTrue(TEXT("...which knows it's stopped"), Reaction->IsHitStopped());
	TestTrue(TEXT("...and staggered (a crit is a heavy hit)"), Reaction->IsStaggered());
	TestEqual(TEXT("The spider beside it runs on"), Beside->CustomTimeDilation, 1.f);
	TestEqual(TEXT("The slime across the field runs on"), Plain->CustomTimeDilation, 1.f);
	TestEqual(TEXT("The world's time runs on"), World->GetWorldSettings() ? World->GetWorldSettings()->TimeDilation : 1.f, WorldDilation);

	Shoot(Plain, 1.f, /*bCritical*/ false);
	TestEqual(TEXT("A plain hit stops nothing"), Plain->CustomTimeDilation, 1.f);

	const UHealthComponent* Health = Beside->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("Spider's health"), Health))
	{
		Shoot(Beside, Health->GetHealth() + 10.f, /*bCritical*/ false);
		TestTrue(TEXT("The kill stops the one killed"), Beside->IsDead() && Beside->CustomTimeDilation == FHitReaction::HitStopDilation);
	}
	TestEqual(TEXT("...and still nothing else"), Plain->CustomTimeDilation, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FViewKicksTest, "Looter.Feedback.ViewKicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FViewKicksTest::RunTest(const FString& Parameters)
{
	// A kick's spring peaks at its numbers in a frame or two and settles; the rifle's is a quick tick, the shotgun's a
	// heavy shove; aiming halves the turn; and however many pile up, the view stays within its limits.
	TestEqual(TEXT("A spring starts at rest"), FViewKickStack::Response(0.f, 14.f, 0.6f), 0.f);
	FRandomStream Random(7);
	const FViewKick Rifle = ViewKicks::ForShot(EWeaponKind::Rifle, 1.f, 0.f, Random);
	const FViewKick Shotgun = ViewKicks::ForShot(EWeaponKind::Shotgun, 1.f, 0.f, Random);
	TestTrue(TEXT("The shotgun shoves harder than the rifle"), Shotgun.Pitch > Rifle.Pitch * 2.f && Shotgun.FieldOfView > Rifle.FieldOfView * 2.f);
	TestTrue(TEXT("...and slower"), Shotgun.Frequency < Rifle.Frequency);
	TestTrue(TEXT("Small: under 1.5 degrees a shot"), Shotgun.Pitch <= 1.5f && Rifle.Pitch <= 0.5f);

	for (const FViewKick& Kick : { Rifle, Shotgun })
	{
		FViewKickStack Stack;
		Stack.Add(Kick);
		const float Peak = PeakOver(Stack, 0.08f, [](const FViewKickStack& Each) { return Each.GetRotation().Pitch; });
		TestEqual(FString::Printf(TEXT("A %.0f Hz kick peaks at its pitch"), Kick.Frequency), Peak, Kick.Pitch, Kick.Pitch * 0.02f);
		PeakOver(Stack, 0.5f, [](const FViewKickStack&) { return 0.f; });
		TestTrue(FString::Printf(TEXT("A %.0f Hz kick has settled within 0.58 s"), Kick.Frequency), Stack.IsSettled());
	}
	{
		FViewKickStack Stack;
		Stack.Add(Rifle);
		PeakOver(Stack, 0.1f, [](const FViewKickStack&) { return 0.f; });
		TestTrue(TEXT("A rifle's tick is all but gone by the next round (0.1 s)"), FMath::Abs(Stack.GetRotation().Pitch) < Rifle.Pitch * 0.15f);
	}

	FRandomStream Same(3);
	const FViewKick Hip = ViewKicks::ForShot(EWeaponKind::Rifle, 1.f, 0.f, Same);
	const FViewKick Sight = ViewKicks::ForShot(EWeaponKind::Rifle, 1.f, 1.f, Same);
	TestEqual(TEXT("Through the sight: half the turn"), Sight.Pitch, Hip.Pitch * 0.5f, 0.0001f);
	TestTrue(TEXT("...and less of the widening"), Sight.FieldOfView < Hip.FieldOfView);
	TestEqual(TEXT("Heavy parts kick harder"), ViewKicks::ForShot(EWeaponKind::Rifle, 1.4f, 0.f, Same).Pitch, Hip.Pitch * 1.4f, 0.0001f);
	TestEqual(TEXT("...held to 1.5x"), ViewKicks::ForShot(EWeaponKind::Rifle, 9.f, 0.f, Same).Pitch, Hip.Pitch * 1.5f, 0.0001f);

	// Hurt: knocked away from the hit, harder for a bigger share of the bar.
	const FViewKick FromRight = ViewKicks::ForHurt(0.25f, 1.f, 0.f);
	TestTrue(TEXT("A hit from the right rolls the view left"), FromRight.Roll < 0.f);
	TestTrue(TEXT("A hit from ahead tips it back"), ViewKicks::ForHurt(0.25f, 0.f, 1.f).Pitch > 0.f);
	TestTrue(TEXT("A hit from behind tips it forward"), ViewKicks::ForHurt(0.25f, 0.f, -1.f).Pitch < 0.f);
	TestEqual(TEXT("A quarter of the bar is the full jolt"), ViewKicks::ForHurt(0.9f, 1.f, 0.f).Roll, FromRight.Roll);
	TestEqual(TEXT("A scratch is a fifth of it"), ViewKicks::ForHurt(0.001f, 1.f, 0.f).Roll, FromRight.Roll * 0.2f, 0.0001f);
	TestTrue(TEXT("Never more than 3 degrees"), FMath::Abs(FromRight.Roll) <= 3.f);

	// A kill punches in; a heavy one more.
	const FViewKick Kill = ViewKicks::ForKill(false);
	TestTrue(TEXT("A kill narrows the view"), Kill.FieldOfView < 0.f);
	TestEqual(TEXT("A heavy kill is 40% more"), ViewKicks::ForKill(true).FieldOfView, Kill.FieldOfView * 1.4f, 0.0001f);

	// Thirty shotgun blasts at once still stay within the limits.
	FViewKickStack Pile;
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Pile.Add(Shotgun);
	}
	TestEqual(TEXT("No more than MaxKicks at once"), Pile.Num(), FViewKickStack::MaxKicks);
	const float PilePitch = PeakOver(Pile, 0.1f, [](const FViewKickStack& Each) { return Each.GetRotation().Pitch; });
	const float PileFov = PeakOver(Pile, 0.01f, [](const FViewKickStack& Each) { return Each.GetFieldOfView(); });
	TestTrue(TEXT("The pile's turn is held to the limit"), FMath::Abs(PilePitch) <= FViewKickStack::MaxTurn + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("...and its widening"), FMath::Abs(PileFov) <= FViewKickStack::MaxFieldOfView + KINDA_SMALL_NUMBER);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraShakeSettingTest, "Looter.Feedback.CameraShakeSetting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraShakeSettingTest::RunTest(const FString& Parameters)
{
	using Controls = UControlSettingsSubsystem;
	// A fresh install, and a save from before the setting, shake as designed (100%).
	TestEqual(TEXT("Defaults to 100%"), Controls::DefaultCameraShake, 1.f);
	TestEqual(TEXT("A fresh save starts at the default"), GetDefault<ULooterControlsSave>()->CameraShake, Controls::DefaultCameraShake);
	TestEqual(TEXT("No save: the default"), Controls::CameraShakeOf(nullptr), Controls::DefaultCameraShake);

	// 0% (still) to 100%, in the slider's 5% steps; anything outside (a hand-edited save) is pulled back in.
	TestEqual(TEXT("0% is allowed: no shake"), Controls::ClampCameraShake(0.f), 0.f);
	TestEqual(TEXT("Below 0 clamps to 0"), Controls::ClampCameraShake(-1.f), 0.f);
	TestEqual(TEXT("Past 100% clamps to it"), Controls::ClampCameraShake(4.f), 1.f);
	TestEqual(TEXT("Rounded to the slider's steps"), Controls::ClampCameraShake(0.63f), 0.65f);
	TestEqual(TEXT("Not a number: the default"), Controls::ClampCameraShake(std::numeric_limits<float>::quiet_NaN()), Controls::DefaultCameraShake);

	// Saved and read back through a slot of the test's own, so the player's setting is never touched.
	const FString Slot = TEXT("ControlSettings_FeedbackTest");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	ULooterControlsSave* Written = NewObject<ULooterControlsSave>();
	Written->CameraShake = 0.35f;
	if (TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToSlot(Written, Slot, 0)))
	{
		TestEqual(TEXT("...read back"), Controls::CameraShakeOf(Controls::LoadControls(Slot, nullptr)), 0.35f);
		TestEqual(TEXT("...the look sensitivity beside it untouched"), Controls::LookSensitivityOf(Controls::LoadControls(Slot, nullptr)),
			Controls::DefaultLookSensitivity);
	}
	UGameplayStatics::DeleteGameInSlot(Slot, 0);

	// What the view shows is the kicks times the setting (UCameraShakeModifier): at 0 the view never moves.
	FViewKickStack Stack;
	FRandomStream Random(11);
	const FViewKick Blast = ViewKicks::ForShot(EWeaponKind::Shotgun, 1.f, 0.f, Random);
	Stack.Add(Blast);
	Stack.Tick(0.02f);
	const FRotator Full = Stack.GetRotation();
	TestTrue(TEXT("A kick moves the view at 100%"), !Full.IsNearlyZero(0.01f));
	TestTrue(TEXT("...half as far at 50%"), (Full * 0.5f).Equals(Full * Controls::ClampCameraShake(0.5f), 0.0001f));
	TestTrue(TEXT("...not at all at 0%"), (Full * Controls::ClampCameraShake(0.f)).IsNearlyZero());

	// The one camera modifier carries the kicks as well as the boss fights' shakes (with no player, at full strength).
	UCameraShakeModifier* Modifier = NewObject<UCameraShakeModifier>(GetTransientPackage());
	Modifier->AddKick(Blast);
	FMinimalViewInfo View;
	View.Rotation = FRotator::ZeroRotator;
	View.FOV = 90.f;
	TestFalse(TEXT("It never stops the modifiers after it"), Modifier->ModifyCamera(0.02f, View));
	TestTrue(TEXT("The kick tips the view up"), View.Rotation.Pitch > 0.1f);
	TestTrue(TEXT("...and widens it"), View.FOV > 90.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageDirectionTest, "Looter.Feedback.DamageDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageDirectionTest::RunTest(const FString& Parameters)
{
	// The arc's angle on the ring: ahead at the top (0), right 90, left -90, behind 180; turning the view turns it.
	using Indicator = UHudDamageIndicatorWidget;
	const FVector Eye(100.0, 200.0, 150.0);
	TestEqual(TEXT("Ahead"), Indicator::ScreenAngle(Eye, 0.f, Eye + FVector(500.0, 0.0, 0.0)), 0.f, 0.01f);
	TestEqual(TEXT("Right"), Indicator::ScreenAngle(Eye, 0.f, Eye + FVector(0.0, 500.0, 0.0)), 90.f, 0.01f);
	TestEqual(TEXT("Left"), Indicator::ScreenAngle(Eye, 0.f, Eye + FVector(0.0, -500.0, 0.0)), -90.f, 0.01f);
	TestEqual(TEXT("Behind"), FMath::Abs(Indicator::ScreenAngle(Eye, 0.f, Eye + FVector(-500.0, 0.0, 0.0))), 180.f, 0.01f);
	TestEqual(TEXT("Ahead and to the right"), Indicator::ScreenAngle(Eye, 0.f, Eye + FVector(300.0, 300.0, 0.0)), 45.f, 0.01f);
	TestEqual(TEXT("Facing it (yaw 90): ahead"), Indicator::ScreenAngle(Eye, 90.f, Eye + FVector(0.0, 500.0, 0.0)), 0.f, 0.01f);
	TestEqual(TEXT("Facing away (yaw 90): the thing ahead is now on the left"), Indicator::ScreenAngle(Eye, 90.f, Eye + FVector(500.0, 0.0, 0.0)), -90.f, 0.01f);
	TestEqual(TEXT("Height doesn't count"), Indicator::ScreenAngle(Eye, 0.f, Eye + FVector(0.0, 500.0, 900.0)), 90.f, 0.01f);
	TestEqual(TEXT("Right on top of the eye: ahead"), Indicator::ScreenAngle(Eye, 30.f, Eye), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootFanfareTest, "Looter.Feedback.LootFanfare",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootFanfareTest::RunTest(const FString& Parameters)
{
	using Fanfare = ULootFanfareSubsystem;
	// The drop sound by rarity: a Common only knocks, every rarer gun has its own, the Epic and Legendary heard flat.
	TestTrue(TEXT("Common: only its landing's knock"), Fanfare::DropCue(EWeaponRarity::Common).IsNone());
	TestEqual(TEXT("Uncommon"), Fanfare::DropCue(EWeaponRarity::Uncommon), FName(LooterSoundCue::DropUncommon));
	TestEqual(TEXT("Rare"), Fanfare::DropCue(EWeaponRarity::Rare), FName(LooterSoundCue::DropRare));
	TestEqual(TEXT("Epic"), Fanfare::DropCue(EWeaponRarity::Epic), FName(LooterSoundCue::DropEpic));
	TestEqual(TEXT("Legendary: the sting"), Fanfare::DropCue(EWeaponRarity::Legendary), FName(LooterSoundCue::DropLegendary));
	TArray<FName> Cues;
	for (const EWeaponRarity Rarity : { EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary })
	{
		Cues.AddUnique(Fanfare::DropCue(Rarity));
	}
	TestEqual(TEXT("Each rarer gun sounds its own"), Cues.Num(), 4);
	TestFalse(TEXT("A Rare is heard from where it fell"), Fanfare::IsDropCueFlat(EWeaponRarity::Rare));
	TestTrue(TEXT("An Epic is heard flat"), Fanfare::IsDropCueFlat(EWeaponRarity::Epic) && Fanfare::IsDropCueFlat(EWeaponRarity::Legendary));
	TestFalse(TEXT("An Uncommon's beam doesn't flare"), Fanfare::FlaresBeam(EWeaponRarity::Uncommon));
	TestTrue(TEXT("A Rare's and up do"), Fanfare::FlaresBeam(EWeaponRarity::Rare) && Fanfare::FlaresBeam(EWeaponRarity::Legendary));
	TestFalse(TEXT("A Rare isn't announced"), Fanfare::IsAnnounced(EWeaponRarity::Rare));
	TestTrue(TEXT("An Epic and a Legendary are"), Fanfare::IsAnnounced(EWeaponRarity::Epic) && Fanfare::IsAnnounced(EWeaponRarity::Legendary));

	// The beam's flare: up from a stub past its height, flashing, and left exactly as it was at the end.
	const LightBeams::FBeamFlare Start = LightBeams::FlareAt(0.f, false);
	TestTrue(TEXT("It starts low"), Start.Lift < 0.5f);
	TestTrue(TEXT("...flashing"), Start.Flash > 3.f);
	TestTrue(TEXT("It shoots past its height"), LightBeams::FlareAt(0.2f, false).Lift > 1.2f);
	TestTrue(TEXT("A Legendary's goes higher and brighter"), LightBeams::FlareAt(0.2f, true).Lift > LightBeams::FlareAt(0.2f, false).Lift
		&& LightBeams::FlareAt(0.f, true).Flash > Start.Flash);
	for (const bool bGrand : { false, true })
	{
		const LightBeams::FBeamFlare End = LightBeams::FlareAt(LightBeams::FlareSeconds, bGrand);
		TestTrue(TEXT("Over: the beam as it was"), End.Lift == 1.f && End.Flash == 1.f && End.Width == 1.f);
		const LightBeams::FBeamFlare Almost = LightBeams::FlareAt(LightBeams::FlareSeconds - 0.01f, bGrand);
		TestTrue(TEXT("...with no jump at the end"), FMath::IsNearlyEqual(Almost.Lift, 1.f, 0.01f) && FMath::IsNearlyEqual(Almost.Flash, 1.f, 0.01f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageNumberMotionTest, "Looter.Feedback.DamageNumbers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageNumberMotionTest::RunTest(const FString& Parameters)
{
	// A crit's number slams in big and white-hot, settles to its size, and lobs off to its side; a plain one only ticks in.
	const DamageNumberMotion::FFrame Slam = DamageNumberMotion::At(0.f, true, 1.f);
	TestEqual(TEXT("A crit slams in at twice its size and more"), Slam.Scale, DamageNumberMotion::CritSlamScale);
	TestEqual(TEXT("...white-hot"), Slam.Heat, 1.f);
	const DamageNumberMotion::FFrame Settled = DamageNumberMotion::At(0.3f, true, 1.f);
	TestEqual(TEXT("...settled to its size by 0.3 s"), Settled.Scale, 1.f, 0.001f);
	TestEqual(TEXT("...cooled to its colour"), Settled.Heat, 0.f);
	TestTrue(TEXT("...risen on its lob"), Settled.Offset.Y < -30.f);
	TestTrue(TEXT("...off to its side"), Settled.Offset.X > 15.f && DamageNumberMotion::At(0.3f, true, -1.f).Offset.X < -15.f);
	TestTrue(TEXT("...tilted its way"), Settled.Angle > 0.f && DamageNumberMotion::At(0.3f, true, -1.f).Angle < 0.f);
	TestTrue(TEXT("The lob comes back down a little by the end"), DamageNumberMotion::At(1.f, true, 1.f).Offset.Y > Settled.Offset.Y);
	const DamageNumberMotion::FFrame Plain = DamageNumberMotion::At(0.f, false, 1.f);
	TestTrue(TEXT("A plain number pops a little"), Plain.Scale > 1.f && Plain.Scale < DamageNumberMotion::CritSlamScale);
	TestTrue(TEXT("...and doesn't lob or burn"), Plain.Offset.IsZero() && Plain.Heat == 0.f && DamageNumberMotion::At(0.5f, false, 1.f).Scale == 1.f);
	return true;
}

#endif
