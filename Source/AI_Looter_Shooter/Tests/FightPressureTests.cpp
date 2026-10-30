#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/CreatureTrait.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Creatures/FightPressureRules.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/PackLeaderRules.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"

// The fight director's pressure (FIGHTS.md section 2, 2026-10-28): the player read (still, hidden, hurt, camping) and the
// close-call ease, the leaps' gate, a kind family's melee gap (the slime's drumbeat; spiders and the Unpaid have none, the
// user's "leave them"), and the packs' leaders (Rallying worn, led packs, the falter, break-off and lull at a leader's fall).

namespace
{
	/** A test level with a fight director in it (one is made only in played worlds unless a test lets it). */
	struct FPressureLevel
	{
		FTestWorldWrapper Wrapper;
		bool Create()
		{
			UFightDirectorSubsystem::bAllowInTestWorlds = true;
			const bool bMade = Wrapper.CreateTestWorld(EWorldType::EditorPreview);
			UFightDirectorSubsystem::bAllowInTestWorlds = false;
			return bMade;
		}
	};

	/** A seed whose first roll is under (bUnder) or at least Chance: a pack component seeded with it rolls that way first. */
	int32 SeedRolling(float Chance, bool bUnder)
	{
		for (int32 Seed = 1; Seed < 10000; ++Seed)
		{
			FRandomStream Probe(Seed);
			if ((Probe.FRand() < Chance) == bUnder)
			{
				return Seed;
			}
		}
		return 1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightPressureTest, "Looter.Fight.Pressure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightPressureTest::RunTest(const FString& Parameters)
{
	// The plan's numbers, as the cvars start.
	const FFightTuning Tuning;
	TestEqual(TEXT("Melee gaps as each kind has them"), Tuning.MeleeGap, 1.f);
	TestEqual(TEXT("Still: 2.5 s"), Tuning.StillSeconds, 2.5f);
	TestEqual(TEXT("...within 1.5 m"), Tuning.StillRadius, 150.f);
	TestEqual(TEXT("Hidden: 2 s"), Tuning.HiddenSeconds, 2.f);
	TestEqual(TEXT("Camping with 3 or more fighting"), Tuning.CampMinFighters, 3);
	TestEqual(TEXT("The ease is on"), Tuning.Ease, 1);
	TestEqual(TEXT("...under 25%"), Tuning.EaseBelow, 0.25f);
	TestEqual(TEXT("...for 4 s"), Tuning.EaseSeconds, 4.f);
	TestEqual(TEXT("...once in 20 s"), Tuning.EaseGap, 20.f);

	// Standing still: within the circle the clock runs on; out of it, it starts again.
	FFightStillWatch Still;
	TestEqual(TEXT("Not watched yet: still for 0"), Still.StillFor(5.0), 0.f);
	Still.Note(FVector::ZeroVector, 0.0, 150.f);
	Still.Note(FVector(100.0, 0.0, 0.0), 1.0, 150.f);
	TestEqual(TEXT("A step inside the circle: still 1 s"), Still.StillFor(1.0), 1.f, 0.001f);
	Still.Note(FVector(120.0, 80.0, 300.0), 3.0, 150.f);
	TestEqual(TEXT("...a jump up, same spot seen from above: still 3 s"), Still.StillFor(3.0), 3.f, 0.001f);
	Still.Note(FVector(400.0, 0.0, 0.0), 3.5, 150.f);
	TestEqual(TEXT("Out of it: the clock starts again"), Still.StillFor(3.5), 0.f, 0.001f);

	// Hidden: seconds since a fighter last saw them.
	FFightSightWatch Sight;
	Sight.Note(false, 0.0);
	TestEqual(TEXT("A fight starts with sight"), Sight.HiddenFor(0.0), 0.f);
	Sight.Note(false, 1.0);
	Sight.Note(false, 2.5);
	TestEqual(TEXT("Unseen since: hidden 2.5 s"), Sight.HiddenFor(2.5), 2.5f, 0.001f);
	Sight.Note(true, 3.0);
	TestEqual(TEXT("Seen again: 0"), Sight.HiddenFor(3.0), 0.f, 0.001f);

	// Camping: still or hidden long enough, with enough fighting them.
	TestTrue(TEXT("Still 2.6 s, 3 fighting: camping"), FightPressureRules::IsCamping(2.6f, 0.f, 3, 2.5f, 2.f, 3));
	TestFalse(TEXT("...2 fighting: not"), FightPressureRules::IsCamping(2.6f, 0.f, 2, 2.5f, 2.f, 3));
	TestTrue(TEXT("Hidden 2.1 s, 4 fighting: camping"), FightPressureRules::IsCamping(0.f, 2.1f, 4, 2.5f, 2.f, 3));
	TestFalse(TEXT("Moving and seen: not"), FightPressureRules::IsCamping(1.f, 1.f, 6, 2.5f, 2.f, 3));

	// Health and ward together.
	TestEqual(TEXT("20 + 5 of 100 + 50"), FightPressureRules::HealthShare(20.f, 100.f, 5.f, 50.f), 25.f / 150.f, 0.001f);
	TestEqual(TEXT("Nothing to measure: whole"), FightPressureRules::HealthShare(0.f, 0.f, 0.f, 0.f), 1.f);

	// The close-call ease: once in its gap, for its seconds, one turn fewer (never under 1).
	FFightEaseWatch Ease;
	TestFalse(TEXT("Ease off: never"), Ease.TryOpen(0.1f, 0.0, false, 0.25f, 4.f, 20.f));
	TestFalse(TEXT("At 30%: not yet"), Ease.TryOpen(0.3f, 0.0, true, 0.25f, 4.f, 20.f));
	TestTrue(TEXT("At 20%: it opens"), Ease.TryOpen(0.2f, 0.0, true, 0.25f, 4.f, 20.f));
	TestTrue(TEXT("...on at 3.9 s"), Ease.IsEasing(3.9));
	TestFalse(TEXT("...over at 4.1 s"), Ease.IsEasing(4.1));
	TestFalse(TEXT("...not again at 10 s"), Ease.TryOpen(0.2f, 10.0, true, 0.25f, 4.f, 20.f));
	TestTrue(TEXT("...again at 21 s"), Ease.TryOpen(0.2f, 21.0, true, 0.25f, 4.f, 20.f));
	TestEqual(TEXT("3 turns eased: 2"), FightPressureRules::EasedTokens(3, true), 2);
	TestEqual(TEXT("1 turn eased: still 1"), FightPressureRules::EasedTokens(1, true), 1);
	TestEqual(TEXT("Not easing: 3"), FightPressureRules::EasedTokens(3, false), 3);

	// One leap at a target in a gap.
	FFightLeapGate Leaps;
	TestTrue(TEXT("First leap: yes"), Leaps.MayLeap(0.0, 1.5f));
	Leaps.Note(0.0);
	TestFalse(TEXT("...another 1 s later: no"), Leaps.MayLeap(1.0, 1.5f));
	TestTrue(TEXT("...1.5 s later: yes"), Leaps.MayLeap(1.5, 1.5f));

	// The director in a test level: its read of a player three spiders fight, and the ease when the player is nearly dead.
	FPressureLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	TArray<ACreatureBase*> Spiders;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Radians = FMath::DegreesToRadians(120.f * static_cast<float>(Index));
		Spiders.Add(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(),
			FVector(FMath::Cos(Radians) * 700.f, FMath::Sin(Radians) * 700.f, 0.f)));
	}
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player)
		|| !TestFalse(TEXT("Three spiders"), Spiders.Contains(nullptr)))
	{
		return false;
	}
	for (ACreatureBase* Spider : Spiders)
	{
		Spider->DevPutInState(ECreatureState::Chase, Player);
	}
	Director->RefreshNow();
	FFightPlayerRead Read = Director->GetPlayerRead(*Player);
	TestEqual(TEXT("Three fight the player"), Read.Fighters, 3);
	TestEqual(TEXT("...in the open: not hidden"), Read.HiddenFor, 0.f);
	TestEqual(TEXT("...whole"), Read.HealthShare, 1.f, 0.001f);
	TestFalse(TEXT("...not easing"), Read.bEasing);

	const UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("The stand-in's health"), Life))
	{
		BossTestWorld::Hurt(Player, Life->GetMaxHealth() * 0.85f);
		Director->RefreshNow();
		Read = Director->GetPlayerRead(*Player);
		TestTrue(TEXT("Hurt to under a quarter"), Read.HealthShare < 0.25f && Read.HealthShare > 0.f);
		TestTrue(TEXT("...the fight eases off"), Read.bEasing);
		TestEqual(TEXT("...noted for the log"), Director->GetEventCount(EFightEvent::Ease), 1);
		const int32 Turns = FightPressureRules::EasedTokens(FFightTuning::Live().MeleeTokens, true);
		int32 Given = 0;
		for (ACreatureBase* Spider : Spiders)
		{
			Given += Director->TryTakeToken(*Spider, *Player, EFightRole::Melee) ? 1 : 0;
		}
		TestEqual(TEXT("...one melee turn fewer at once"), Given, FMath::Min(Turns, Spiders.Num()));
		TestTrue(TEXT("...each turn noted"), Director->GetEventCount(EFightEvent::TurnTaken) >= Given);
	}

	// A leap's gate at the director: one at a time.
	TestTrue(TEXT("A spider may leap"), Director->MayLeap(*Spiders[0], *Player, 1.5f));
	Director->NoteLeap(*Spiders[0], *Player);
	TestFalse(TEXT("...the next waits the gap"), Director->MayLeap(*Spiders[1], *Player, 1.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightGapTest, "Looter.Fight.Gap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightGapTest::RunTest(const FString& Parameters)
{
	// The kinds' gaps: the slime's drumbeat; spiders and the Unpaid none (the user's "leave them", 2026-10-28).
	TestEqual(TEXT("The slime: 0.45 s"), GetDefault<ASlimeCreature>()->GetMeleeGap(), 0.45f, 0.001f);
	TestEqual(TEXT("Spiders: none"), GetDefault<ASpiderCreature>()->GetMeleeGap(), 0.f);
	TestEqual(TEXT("The Unpaid: none"), GetDefault<AUnpaidCreature>()->GetMeleeGap(), 0.f);
	TestTrue(TEXT("A kind's family is its class"), FightRules::FamilyOf(ASpiderCreature::StaticClass()) == ASpiderCreature::StaticClass()->GetFName());
	TestTrue(TEXT("...two kinds, two families"), FightRules::FamilyOf(ASlimeCreature::StaticClass()) != FightRules::FamilyOf(ASpiderCreature::StaticClass()));
	TestTrue(TEXT("...none for nothing"), FightRules::FamilyOf(nullptr).IsNone());

	// The tokens: a family's turns begin its gap apart; another family isn't held up behind it; a kept turn isn't a new one.
	const FName Slime(TEXT("Slime"));
	const FName Spider(TEXT("Spider"));
	FFightTokens Tokens;
	TestTrue(TEXT("A (slime) at 0: a turn"), Tokens.TryTake(1, 0.0, 3, 0.5f, 0.f, Slime, 0.45f));
	TestFalse(TEXT("B (slime) at 0.2: waits the gap"), Tokens.TryTake(2, 0.2, 3, 0.5f, 0.f, Slime, 0.45f));
	TestTrue(TEXT("C (spider) at 0.2: not held up behind B"), Tokens.TryTake(3, 0.2, 3, 0.5f, 0.f, Spider, 0.f));
	TestTrue(TEXT("B at 0.46: its turn"), Tokens.TryTake(2, 0.46, 3, 0.5f, 0.f, Slime, 0.45f));
	Tokens.Release(1, 0.5, 0.f);
	Tokens.Release(2, 0.5, 0.f);
	Tokens.Release(3, 0.5, 0.f);
	TestTrue(TEXT("D (slime) at 1.0: a turn"), Tokens.TryTake(4, 1.0, 3, 0.5f, 0.f, Slime, 0.45f));
	TestTrue(TEXT("...staggered at 1.1"), Tokens.ReleaseKeepTurn(4, 1.1));
	TestTrue(TEXT("...its turn back at 1.2: no gap for a kept turn"), Tokens.TryTake(4, 1.2, 3, 0.5f, 0.f, Slime, 0.45f));
	TestFalse(TEXT("E (slime) at 1.2: the gap from D's turn"), Tokens.TryTake(5, 1.2, 3, 0.5f, 0.f, Slime, 0.45f));
	TestTrue(TEXT("E at 1.5: its turn"), Tokens.TryTake(5, 1.5, 3, 0.5f, 0.f, Slime, 0.45f));

	// The director: two slimes asking at once, one waits; two spiders, both go.
	FPressureLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	ACreatureBase* SlimeA = EncounterTestWorld::SpawnCreature(World, ASlimeCreature::StaticClass(), FVector(600.0, 0.0, 0.0));
	ACreatureBase* SlimeB = EncounterTestWorld::SpawnCreature(World, ASlimeCreature::StaticClass(), FVector(-600.0, 0.0, 0.0));
	ACreatureBase* SpiderA = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(0.0, 600.0, 0.0));
	ACreatureBase* SpiderB = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(0.0, -600.0, 0.0));
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Slime A"), SlimeA)
		|| !TestNotNull(TEXT("Slime B"), SlimeB) || !TestNotNull(TEXT("Spider A"), SpiderA) || !TestNotNull(TEXT("Spider B"), SpiderB))
	{
		return false;
	}
	for (ACreatureBase* Each : { SlimeA, SlimeB, SpiderA, SpiderB })
	{
		Each->DevPutInState(ECreatureState::Chase, Player);
	}
	const float SlimeGap = Director->GetMeleeGapFor(*SlimeA);
	TestEqual(TEXT("The slime's gap at the director: its own, times Looter.Fight.MeleeGap"), SlimeGap,
		SlimeA->GetMeleeGap() * FFightTuning::Live().MeleeGap, 0.001f);
	TestEqual(TEXT("...a spider's: none"), Director->GetMeleeGapFor(*SpiderA), 0.f);
	TestTrue(TEXT("Slime A: a turn"), Director->TryTakeToken(*SlimeA, *Player, EFightRole::Melee));
	if (SlimeGap > 0.f)
	{
		TestFalse(TEXT("...slime B in the same moment: waits its pack's beat"), Director->TryTakeToken(*SlimeB, *Player, EFightRole::Melee));
	}
	TestTrue(TEXT("Spider A: a turn (not held up)"), Director->TryTakeToken(*SpiderA, *Player, EFightRole::Melee));
	TestTrue(TEXT("Spider B in the same moment: a turn too"), Director->TryTakeToken(*SpiderB, *Player, EFightRole::Melee));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightLeaderTest, "Looter.Fight.Leader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightLeaderTest::RunTest(const FString& Parameters)
{
	const FFightTuning Tuning;
	TestEqual(TEXT("A leader's reach: 20 m"), Tuning.LeaderReach, 2000.f);
	TestEqual(TEXT("...its pack falters 1 s"), Tuning.LeaderFalter, 1.f);
	TestEqual(TEXT("...breaks at 0.35"), Tuning.LeaderBreakChance, 0.35f);
	TestEqual(TEXT("...and lulls 16 s"), Tuning.LeaderLull, 16.f);

	// The rules.
	PackLeaderRules::FBreakOffAsk Ask;
	TestTrue(TEXT("A Basic spider hunting: rolls"), PackLeaderRules::WantsBreakOffRoll(Ask));
	TestTrue(TEXT("...under the chance: breaks"), PackLeaderRules::BreaksOff(Ask, 0.3f, 0.35f));
	TestFalse(TEXT("...over it: holds"), PackLeaderRules::BreaksOff(Ask, 0.4f, 0.35f));
	PackLeaderRules::FBreakOffAsk Ranked = Ask;
	Ranked.Rank = ECreatureRank::Rare;
	TestFalse(TEXT("A Restless one never breaks"), PackLeaderRules::WantsBreakOffRoll(Ranked));
	PackLeaderRules::FBreakOffAsk Dead = Ask;
	Dead.bAllowedKind = false;
	TestFalse(TEXT("A kind that never breaks off (the dead)"), PackLeaderRules::WantsBreakOffRoll(Dead));
	PackLeaderRules::FBreakOffAsk Running = Ask;
	Running.bRetreating = true;
	TestFalse(TEXT("One already running"), PackLeaderRules::WantsBreakOffRoll(Running));
	PackLeaderRules::FBreakOffAsk Idle = Ask;
	Idle.bHunting = false;
	TestFalse(TEXT("One not hunting"), PackLeaderRules::WantsBreakOffRoll(Idle));

	PackLeaderRules::FPackmateLook Look;
	Look.Distance = 1500.f;
	TestTrue(TEXT("A packmate 15 m off falters"), PackLeaderRules::Falters(Look, 2000.f));
	PackLeaderRules::FPackmateLook Far = Look;
	Far.Distance = 2100.f;
	TestFalse(TEXT("...21 m off: no"), PackLeaderRules::Falters(Far, 2000.f));
	PackLeaderRules::FPackmateLook Self = Look;
	Self.bSelf = true;
	TestFalse(TEXT("...the leader itself: no"), PackLeaderRules::Falters(Self, 2000.f));
	PackLeaderRules::FPackmateLook Other = Look;
	Other.bSharesPack = false;
	TestFalse(TEXT("...another pack: no"), PackLeaderRules::Falters(Other, 2000.f));
	PackLeaderRules::FPackmateLook Boss = Look;
	Boss.bOwnFight = true;
	TestFalse(TEXT("...a boss: no"), PackLeaderRules::Falters(Boss, 2000.f));

	const UClass* SpiderClass = ASpiderCreature::StaticClass();
	const UClass* SlimeClass = ASlimeCreature::StaticClass();
	TestTrue(TEXT("Same tag: one pack"), PackLeaderRules::SamePack(TEXT("Yard"), SpiderClass, TEXT("Yard"), SlimeClass));
	TestFalse(TEXT("Two tags: two packs"), PackLeaderRules::SamePack(TEXT("Yard"), SpiderClass, TEXT("Gate"), SpiderClass));
	TestTrue(TEXT("Untagged, same class: one pack"), PackLeaderRules::SamePack(NAME_None, SpiderClass, TEXT("Gate"), SpiderClass));
	TestFalse(TEXT("Untagged, two classes: two packs"), PackLeaderRules::SamePack(NAME_None, SpiderClass, NAME_None, SlimeClass));

	// In a test level: a Restless leader and four Basic spiders near it, one more far off.
	FPressureLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(12000.0, 12000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	ACreatureBase* Leader = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(800.0, 0.0, 0.0));
	TArray<ACreatureBase*> Pack;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Radians = FMath::DegreesToRadians(90.f * static_cast<float>(Index + 1));
		Pack.Add(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(),
			FVector(FMath::Cos(Radians) * 700.f, FMath::Sin(Radians) * 700.f, 0.f)));
	}
	ACreatureBase* Distant = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(-3500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The leader"), Leader)
		|| !TestFalse(TEXT("Its pack"), Pack.Contains(nullptr)) || !TestNotNull(TEXT("A spider far off"), Distant))
	{
		return false;
	}
	Leader->SetRank(ECreatureRank::Rare);
	Leader->GetPack()->SetLeader(true);
	TestTrue(TEXT("It leads"), Leader->GetPack()->IsLeader());
	const UCreatureTraitComponent* Traits = Leader->FindComponentByClass<UCreatureTraitComponent>();
	TestTrue(TEXT("...wearing Rallying"), Traits && Traits->HasTrait(ECreatureTrait::Rallying));
	TestTrue(TEXT("A packmate near it is led"), Director->IsPackLed(*Pack[0]));
	TestFalse(TEXT("...one 43 m off is not"), Director->IsPackLed(*Distant));
	Leader->DevPutInState(ECreatureState::Chase, Player);
	for (ACreatureBase* Each : Pack)
	{
		Each->DevPutInState(ECreatureState::Chase, Player);
	}
	Distant->DevPutInState(ECreatureState::Chase, Player);

	// Two packmates hold turns, the far one too; one rolls low, one high at the fall.
	TestTrue(TEXT("Packmate 0: a turn"), Director->TryTakeToken(*Pack[0], *Player, EFightRole::Melee));
	TestTrue(TEXT("Packmate 1: a turn"), Director->TryTakeToken(*Pack[1], *Player, EFightRole::Melee));
	TestTrue(TEXT("The far one: a turn"), Director->TryTakeToken(*Distant, *Player, EFightRole::Melee));
	const float Chance = FFightTuning::Live().LeaderBreakChance;
	Pack[0]->GetPack()->SetSeed(SeedRolling(Chance, true));
	Pack[1]->GetPack()->SetSeed(SeedRolling(Chance, false));

	BossTestWorld::Kill(Leader);
	TestTrue(TEXT("The leader is dead"), Leader->IsDead());
	TestEqual(TEXT("...noted for the log"), Director->GetEventCount(EFightEvent::LeaderDown), 1);
	TestFalse(TEXT("Packmate 0 falters: its turn back"), Director->HoldsToken(*Pack[0]));
	TestFalse(TEXT("...packmate 1 too"), Director->HoldsToken(*Pack[1]));
	TestTrue(TEXT("...the far one is unshaken"), Director->HoldsToken(*Distant));
	TestTrue(TEXT("Its pack is in the lull"), Director->IsInLeaderLull(*Pack[2]));
	TestTrue(TEXT("...just now"), Director->GetSinceLeaderDown(*Pack[2]) < 0.5f);
	TestFalse(TEXT("...the far one is not"), Director->IsInLeaderLull(*Distant));
	TestFalse(TEXT("No living leader: not led"), Director->IsPackLed(*Pack[2]));
	if (UEncounterSettings::Get().MayRetreat(ASpiderCreature::StaticClass()) && UEncounterSettings::Get().RetreatSeconds > 0.f && Chance > 0.f
		&& Chance < 1.f)
	{
		TestTrue(TEXT("The low roll breaks off"), Pack[0]->GetPack()->IsRetreating());
		TestFalse(TEXT("...the high roll holds"), Pack[1]->GetPack()->IsRetreating());
	}
	else
	{
		AddInfo(TEXT("Spiders don't break off in this project's settings (or the chance is 0 or 1): the break-off rolls aren't checked."));
	}

	// A group's LeaderTraits on top of Rallying (the mountain fights' Restless leaders wear Warded: the RR tuning the user took,
	// 2026-10-29 night), last, so the checks above meet no second leader.
	ACreatureBase* WardedLeader = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(4000.0, 4000.0, 0.0));
	if (TestNotNull(TEXT("Another leader"), WardedLeader))
	{
		WardedLeader->SetRank(ECreatureRank::Rare);
		const TArray<ECreatureTrait> MountainLeader = { ECreatureTrait::Warded };
		WardedLeader->GetPack()->SetLeader(true, MountainLeader);
		const UCreatureTraitComponent* Worn = WardedLeader->FindComponentByClass<UCreatureTraitComponent>();
		TestTrue(TEXT("A leader given Warded wears it and Rallying"), Worn && Worn->HasTrait(ECreatureTrait::Warded)
			&& Worn->HasTrait(ECreatureTrait::Rallying));
	}
	return true;
}

#endif
