#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Abel Ransom, Corrupted (Main 7 "Given to the Wind", MAINS_2_7.md section 11; the user's calls of 2026-10-31 night): what the
// furnace did to him (UAbelCorruptionComponent, AbelCorruptionRules): the call-back lock at 66% and 33% answered at the keeper's
// post, the pips under his bar, the smog tendril, the drink lines to the six dead, the fog surge, and his freeing (and the
// smog's echo that has none of it).

#include "Bosses/AbelCorruption.h"
#include "Bosses/AbelCorruptionRules.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Tests/AbelTestWorld.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "World/KeeperLanternPost.h"
#include "Blueprint/UserWidget.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace AbelTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelCallBackTest, "Looter.Bosses.Abel.CallBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelCallBackTest::RunTest(const FString& Parameters)
{
	// The rules: a third of the smog off him with each pip; the call-backs open the second and third phases only.
	TestEqual(TEXT("No pips: all the smog on him"), AbelCorruptionRules::CorruptionFor(0), 1.f);
	TestNearlyEqual(TEXT("One: two thirds"), AbelCorruptionRules::CorruptionFor(1), 2.f / 3.f, 0.001f);
	TestEqual(TEXT("Three: freed"), AbelCorruptionRules::CorruptionFor(3), 0.f);
	TestTrue(TEXT("The call-backs open \"The Hose\" and \"Keeper\""), AbelCorruptionRules::CallBackFor(0) == INDEX_NONE
		&& AbelCorruptionRules::CallBackFor(1) == 0 && AbelCorruptionRules::CallBackFor(2) == 1);

	FTendrilsOff NoTendrils;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}
	UAbelCorruptionComponent* Corruption = Abel->GetCorruption();
	AKeeperLanternPost* Post = Abel->GetKeepersPost();
	UHealthComponent* Health = Boss->GetBossHealth();

	// At 66% the bar locks: hits only spark, he kneels facing his post, and the post opens a 1.5 s hold.
	HurtTo(Abel, AbelRules::HoseShare);
	TestTrue(TEXT("Locked and knelt"), Corruption->IsLocked() && Corruption->IsKneeling() && Abel->GetMove() == EAbelMove::CallKneel);
	TestTrue(TEXT("...his boss can't be hurt, but he hasn't left the fight"), Boss->IsUntargetable() && !Abel->IsPassive());
	TestTrue(TEXT("...the post's hold is open"), Post && Post->IsCallBackOpen());
	const float Locked = Health->GetHealth();
	BossTestWorld::Hurt(Abel, 2000.f);
	TestEqual(TEXT("...a shot takes nothing"), Health->GetHealth(), Locked);

	// Missed: after 6 s he rises, still locked; 12 s later down again. It can't be skipped, it can't be failed.
	Run(Abel, AbelCorruptionRules::KneelSeconds + 0.2f);
	TestTrue(TEXT("Missed: he rises, the bar still locked, the post shut"), !Corruption->IsKneeling() && Corruption->IsLocked() && !Post->IsCallBackOpen()
		&& Abel->GetMove() != EAbelMove::CallKneel);
	BossTestWorld::Hurt(Abel, 2000.f);
	TestEqual(TEXT("...still nothing hurts him"), Health->GetHealth(), Locked);
	TestFalse(TEXT("...and the post's hold does nothing now"), CallBack(Abel, Player));
	const float Again = RunUntil(Abel, AbelCorruptionRules::RetryAfter + 2.f, [Corruption]() { return Corruption->IsKneeling(); });
	TestTrue(FString::Printf(TEXT("...down again %.1f s later"), Again), Again >= AbelCorruptionRules::RetryAfter - 0.5f);

	// Called back: a pip, a third of the smog off him for good, the bar unlocked, and the phase goes on (the drift).
	TestTrue(TEXT("Called back at the post"), CallBack(Abel, Player));
	TestTrue(TEXT("...a pip"), Corruption->GetPips() == 1 && !Corruption->IsLocked() && !Corruption->IsKneeling());
	TestTrue(TEXT("...his post's lantern warms a third"), FMath::IsNearlyEqual(Post->GetCallGlow(), 1.f / 3.f, 0.01f));
	TestTrue(TEXT("...and he drifts out into the fog"), Abel->IsOutInFog());
	Run(Abel, 2.f);
	TestTrue(TEXT("His look eases to two thirds smog"), FMath::IsNearlyEqual(Corruption->GetCorruption(), 2.f / 3.f, 0.02f));

	// One hit past both lines: two call-backs owed, one at a time; the drift passed over for the second.
	FTestWorldWrapper SecondWrapper;
	if (!TestTrue(TEXT("Second test world created"), SecondWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Second = SecondWrapper.GetTestWorld();
	AAbelKeeper* Twice = SpawnAbel(Second);
	ACharacter* SecondPlayer = nullptr;
	UBossComponent* SecondBoss = Twice ? StartFight(*this, Second, Twice, SecondPlayer) : nullptr;
	if (!SecondBoss)
	{
		return false;
	}
	HurtTo(Twice, 0.3f);
	UAbelCorruptionComponent* Both = Twice->GetCorruption();
	TestTrue(TEXT("Past both lines in one hit: \"Keeper\", locked"), SecondBoss->GetPhase() == 2 && Both->IsLocked());
	TestTrue(TEXT("...the first call: a pip, still locked for the second"), CallBack(Twice, SecondPlayer) && Both->GetPips() == 1 && Both->IsLocked());
	TestTrue(TEXT("...the second: two pips, the surge and the wind"), CallBack(Twice, SecondPlayer) && Both->GetPips() == 2 && !Both->IsLocked()
		&& Both->IsSurging() && Twice->IsWindBlowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelPipsTest, "Looter.Bosses.Abel.Pips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelPipsTest::RunTest(const FString& Parameters)
{
	// His bar: SMOG over its far end, three gold pips ABEL under it, lit as he's called back; a plain boss's bar has none.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHudBossBarWidget* Bar = CreateWidget<UHudBossBarWidget>(World, UHudBossBarWidget::StaticClass());
	if (!TestNotNull(TEXT("The boss bar"), Bar))
	{
		return false;
	}
	Bar->SetBoss(FText::FromString(TEXT("Abel Ransom, Corrupted")), 7, UCreatureRankSettings::Get(ECreatureRank::Boss).Color, { 1.f, 0.66f, 0.33f });
	Bar->TakeWidget();
	TestEqual(TEXT("No pips until a boss asks"), Bar->GetPipsTotal(), 0);
	Bar->SetPips(3, 0, FText::FromString(TEXT("Abel")), FText::FromString(TEXT("Smog")));
	TestTrue(TEXT("Three pips, none lit, the bar reading SMOG"), Bar->GetPipsTotal() == 3 && Bar->GetPipsFilled() == 0
		&& Bar->GetBarLabelText().ToString() == TEXT("Smog"));
	Bar->SetPips(3, 1, FText::FromString(TEXT("Abel")), FText::FromString(TEXT("Smog")));
	TestEqual(TEXT("A call-back: one lit"), Bar->GetPipsFilled(), 1);
	Bar->SetPips(3, 3, FText::FromString(TEXT("Abel")), FText::FromString(TEXT("Smog")));
	TestEqual(TEXT("Freed: all three"), Bar->GetPipsFilled(), 3);
	Bar->SetPips(0, 0, FText::GetEmpty(), FText::GetEmpty());
	TestEqual(TEXT("Asked for none: the row goes (its count kept for a later ask)"), Bar->GetPipsTotal(), 3);

	// His look follows the pips: a third of the smog off him each (the rules his component plays).
	const FLinearColor Own(0.8f, 0.75f, 0.7f, 1.f);
	const FLinearColor Black = AbelCorruptionRules::SmogTint(Own, 1.f);
	const FLinearColor Third = AbelCorruptionRules::SmogTint(Own, AbelCorruptionRules::CorruptionFor(2));
	TestTrue(TEXT("Smog-black at full, his own colour coming back a third at a time"), Black.GetLuminance() < 0.06f
		&& Third.GetLuminance() > Black.GetLuminance() && AbelCorruptionRules::SmogTint(Own, 0.f).Equals(Own));
	TestTrue(TEXT("His coal furnace-orange under smog, gold-white freed"), AbelCorruptionRules::CoalColor(1.f).G < 0.5f
		&& AbelCorruptionRules::CoalColor(0.f).G > 0.7f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelTendrilTest, "Looter.Bosses.Abel.Tendril",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelTendrilTest::RunTest(const FString& Parameters)
{
	// The rules: it breaks at 14 m, on a stagger, on a shot at the black lantern as it flares, after a while; the last call's
	// holds through all of that.
	AbelCorruptionRules::FTendrilFacts Facts;
	Facts.Distance = 800.f;
	TestTrue(TEXT("Latched within 14 m: it holds"), AbelCorruptionRules::TendrilBreaks(Facts) == AbelCorruptionRules::EBreak::None);
	Facts.Distance = 1500.f;
	TestTrue(TEXT("Past 14 m: it breaks"), AbelCorruptionRules::TendrilBreaks(Facts) == AbelCorruptionRules::EBreak::Distance);
	Facts.Distance = 800.f;
	Facts.bStaggered = true;
	TestTrue(TEXT("A staggering hit: it breaks"), AbelCorruptionRules::TendrilBreaks(Facts) == AbelCorruptionRules::EBreak::Staggered);
	Facts.bStaggered = false;
	Facts.bLanternShot = true;
	TestTrue(TEXT("The black lantern shot: it breaks"), AbelCorruptionRules::TendrilBreaks(Facts) == AbelCorruptionRules::EBreak::LanternShot);
	Facts.bLanternShot = false;
	Facts.LatchedFor = AbelCorruptionRules::TendrilLongest;
	TestTrue(TEXT("Held its while: it lets go"), AbelCorruptionRules::TendrilBreaks(Facts) == AbelCorruptionRules::EBreak::Time);
	Facts.bFinal = true;
	Facts.Distance = 5000.f;
	TestTrue(TEXT("The last call's: it holds through the hold"), AbelCorruptionRules::TendrilBreaks(Facts) == AbelCorruptionRules::EBreak::None);
	TestTrue(TEXT("The black lantern flares a second after the bite, then every 3 s"), !AbelCorruptionRules::IsLanternFlaring(0.5f)
		&& AbelCorruptionRules::IsLanternFlaring(1.5f) && !AbelCorruptionRules::IsLanternFlaring(2.5f) && AbelCorruptionRules::IsLanternFlaring(4.5f));

	// In a test level: thrown at the player 7 m off after the lantern's flare, latched, draining; a shot on the flaring lantern
	// breaks it.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}
	Run(Abel, Abel->ShowRules.IntroSeconds + 0.1f);
	UAbelCorruptionComponent* Corruption = Abel->GetCorruption();
	UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
	Life->MaxHealth = 100000.f;
	Life->ResetHealth();
	TestTrue(TEXT("Thrown: the black lantern flares first"), Corruption->ThrowTendrils(false) && Corruption->IsLanternFlaring()
		&& Corruption->NumTendrilsLatched() == 0);
	TestFalse(TEXT("...one at a time"), Corruption->ThrowTendrils(false));
	const float LatchedAt = RunUntil(Abel, AbelCorruptionRules::TendrilTell + 1.5f, [Corruption]() { return Corruption->NumTendrilsLatched() > 0; });
	TestTrue(FString::Printf(TEXT("...then it flies and latches (%.2f s)"), LatchedAt), LatchedAt >= AbelCorruptionRules::TendrilTell - 0.1f);
	// Counted, not read off their health: a test level has no game mode, and without one a pawn ignores ApplyDamage
	// (APawn::ShouldTakeDamage), as the Dredgers' hose test counts its drain.
	const float Before = Corruption->GetTendrilDrainDealt();
	Run(Abel, 0.8f);
	TestTrue(TEXT("...and drains"), Corruption->GetTendrilDrainDealt() > Before);
	const float FlareAt = RunUntil(Abel, AbelCorruptionRules::TendrilFlareEvery + 1.f, [Corruption]() { return Corruption->IsLanternFlaring(); });
	TestTrue(TEXT("Latched, the black lantern flares now and then"), FlareAt >= 0.f);
	TestTrue(TEXT("...shot then: it lets go"), Corruption->ShootLantern() && Corruption->NumTendrilsLatched() == 0);
	TestFalse(TEXT("...a shot on it unflaring does nothing"), Corruption->ShootLantern());

	// Two at once in "Keeper"; walking past 14 m breaks them.
	Run(Abel, 1.f);
	TestTrue(TEXT("Two at once"), Corruption->ThrowTendrils(true) && Corruption->NumTendrilsOut() == 2);
	RunUntil(Abel, 3.f, [Corruption]() { return Corruption->NumTendrilsLatched() == 2; });
	TestEqual(TEXT("...both latched"), Corruption->NumTendrilsLatched(), 2);
	Player->SetActorLocation(Abel->GetActorLocation() + FVector(-1600.0, 0.0, 0.0));
	Run(Abel, 0.2f);
	TestEqual(TEXT("Past 14 m: both let go"), Corruption->NumTendrilsLatched(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelDrinkTest, "Looter.Bosses.Abel.Drink",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelDrinkTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The next to drink: in order round the boards, the drunk passed over"), AbelCorruptionRules::NextDrink({ false, true, false }, 0) == 2
		&& AbelCorruptionRules::NextDrink({ false, false }, INDEX_NONE) == 0 && AbelCorruptionRules::NextDrink({ true, true }, 0) == INDEX_NONE);
	TestTrue(TEXT("The six saved: bodies on the boards, none drunk"), AbelCorruptionRules::AreSixSaved(6, 0) && !AbelCorruptionRules::AreSixSaved(6, 1)
		&& !AbelCorruptionRules::AreSixSaved(0, 0));

	FTendrilsOff NoTendrils;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}
	UAbelCorruptionComponent* Corruption = Abel->GetCorruption();
	TestEqual(TEXT("Six dead on the boards"), Corruption->BoardsDead.Num(), 6);
	HurtIntoPhase(Abel, AbelRules::HoseShare);
	Run(Abel, Abel->DriftSeconds + 0.1f);
	TestTrue(TEXT("Out in the fog"), Abel->GetMove() == EAbelMove::InFog);
	const float LineAt = RunUntil(Abel, 3.f, [Corruption]() { return Corruption->GetDrinking() != INDEX_NONE; });
	TestTrue(FString::Printf(TEXT("He runs a smog line to one of the dead (%.1f s)"), LineAt), LineAt >= 0.f);
	// Three hits on its collar break it: the body saved, for now.
	const int32 First = Corruption->GetDrinking();
	TestTrue(TEXT("A hit on the collar"), Corruption->HitCollar() && Corruption->GetCollarHits() == 1);
	Corruption->HitCollar();
	Corruption->HitCollar();
	TestTrue(TEXT("...the third breaks it: the body not drunk"), Corruption->GetDrinking() == INDEX_NONE && !Corruption->IsDrunk(First));
	// Left alone 12 s, a body is drunk out: he heals, it rises Restless, the optional is lost.
	const float Next = RunUntil(Abel, AbelCorruptionRules::DrinkRest + 1.f, [Corruption]() { return Corruption->GetDrinking() != INDEX_NONE; });
	TestTrue(TEXT("A rest, then the next line"), Next >= AbelCorruptionRules::DrinkRest - 0.5f && Corruption->GetDrinking() != First);
	const int32 Drinking = Corruption->GetDrinking();
	const int32 AddsBefore = Boss->NumAliveAdds();
	const float HealthBefore = Boss->GetBossHealth()->GetHealth();
	Run(Abel, AbelCorruptionRules::DrinkSeconds + 0.1f);
	TestTrue(TEXT("Drunk out"), Corruption->IsDrunk(Drinking) && Corruption->NumDrunk() == 1);
	TestTrue(TEXT("...he heals on it"), Boss->GetBossHealth()->GetHealth() > HealthBefore);
	TestTrue(TEXT("...and it rises, one of his adds, Restless"), Boss->NumAliveAdds() > AddsBefore
		&& Boss->GetAliveAdds().ContainsByPredicate([](const ACreatureBase* Add) { return Add && Add->GetRank() == ECreatureRank::Rare; }));
	// Back on the deck, the lines stop; a reset lays every body back on its bier.
	for (AKeeperLanternPost* Post : Abel->GetLanternPosts())
	{
		Post->Relight(Player);
	}
	Run(Abel, Abel->DragBackSeconds + 0.2f);
	TestEqual(TEXT("Dragged back: no line"), Corruption->GetDrinking(), INDEX_NONE);
	Boss->ResetFight();
	TestEqual(TEXT("Started over: none drunk"), Corruption->NumDrunk(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelSurgeTest, "Looter.Bosses.Abel.Surge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelSurgeTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual(TEXT("At its height: the surge's density"), AbelCorruptionRules::SurgeDensityAt(0.004f, 1.f), AbelCorruptionRules::SurgeDensity, 0.0001f);
	TestNearlyEqual(TEXT("...brought in close"), AbelCorruptionRules::SurgeStartAt(6000.f, 1.f), AbelCorruptionRules::SurgeStart, 0.1f);
	TestNearlyEqual(TEXT("None of it: the level's own"), AbelCorruptionRules::SurgeDensityAt(0.004f, 0.f), 0.004f, 0.00001f);

	FTendrilsOff NoTendrils;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>();
	AAbelKeeper* Abel = SpawnAbel(World);
	if (!TestNotNull(TEXT("The level's fog"), Fog) || !TestNotNull(TEXT("Abel"), Abel))
	{
		return false;
	}
	UExponentialHeightFogComponent* Level = Fog->GetComponent();
	Level->SetFogDensity(0.004f);
	Level->SetStartDistance(6000.f);
	UAbelCorruptionComponent* Corruption = Abel->GetCorruption();
	Corruption->SetSurge(true);
	Run(Abel, AbelCorruptionRules::SurgeIn + 0.1f);
	TestTrue(FString::Printf(TEXT("The wreck's smog up over the deck: the fog thick and close (%.3f, %.0f cm)"), Level->FogDensity, Level->StartDistance),
		FMath::IsNearlyEqual(Level->FogDensity, AbelCorruptionRules::SurgeDensity, 0.001f) && Level->StartDistance <= AbelCorruptionRules::SurgeStart + 1.f);
	Corruption->SetSurge(false);
	Run(Abel, AbelCorruptionRules::SurgeOut + 0.1f);
	TestTrue(TEXT("Thinned: the level's own fog given back exactly"), FMath::IsNearlyEqual(Level->FogDensity, 0.004f, 0.00001f)
		&& FMath::IsNearlyEqual(Level->StartDistance, 6000.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelFreeTest, "Looter.Bosses.Abel.Free",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelFreeTest::RunTest(const FString& Parameters)
{
	// At zero: the last call (a 3 s hold) under a tendril's drain; held, the third pip, the black lantern falling; freed, he's
	// his own colour again. (The scene itself: Looter.Bosses.Abel.Kneel.)
	FScenesSetting ScenesOff(0);
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}
	UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
	Life->MaxHealth = 100000.f;
	Life->ResetHealth();
	UAbelCorruptionComponent* Corruption = Abel->GetCorruption();
	AKeeperLanternPost* Post = Abel->GetKeepersPost();
	BossTestWorld::Kill(Abel);
	Run(Abel, 2.2f);
	TestTrue(TEXT("Knelt at zero: the last call"), Corruption->IsFinalCallOpen() && Post->IsCallBackOpen());
	// The player at the post: the last of the smog fights back (a tendril they hold through).
	Player->SetActorLocation(Post->GetActorLocation() + FVector(150.0, 0.0, 90.0));
	RunUntil(Abel, 2.f, [Corruption]() { return Corruption->NumTendrilsLatched() > 0; });
	TestEqual(TEXT("...a tendril on them"), Corruption->NumTendrilsLatched(), 1);
	Player->SetActorLocation(Abel->GetActorLocation() + FVector(-3000.0, 0.0, 0.0));
	Run(Abel, 0.2f);
	TestEqual(TEXT("...that holds through the hold, whatever the distance"), Corruption->NumTendrilsLatched(), 1);
	TestTrue(TEXT("Held: called home"), CallBack(Abel, Player));
	TestTrue(TEXT("...three pips, freed, the tendril gone, the black lantern out of his hand"), Corruption->GetPips() == 3 && Corruption->IsFreed()
		&& Corruption->NumTendrilsLatched() == 0 && !Abel->Lantern->IsVisible());
	// Scenes off: his ending at once; he's his own colour (the scene boils the last third off).
	Run(Abel, AbelTestWorld::Frame);
	TestTrue(TEXT("In his lantern, the lantern lit and taken down, the smog all gone"), Abel->IsEndingDone() && Post->IsKeepersLanternTaken()
		&& Corruption->GetCorruption() == 0.f);

	// The smog's echo (a rematch): no lock at 66%, no pips, no last call.
	FTestWorldWrapper EchoWrapper;
	if (!TestTrue(TEXT("Echo's test world created"), EchoWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* EchoWorld = EchoWrapper.GetTestWorld();
	AAbelKeeper* Echo = SpawnAbel(EchoWorld);
	if (!TestNotNull(TEXT("The echo"), Echo))
	{
		return false;
	}
	Echo->GetBoss()->MarkRematch();
	ACharacter* EchoPlayer = nullptr;
	UBossComponent* EchoBoss = StartFight(*this, EchoWorld, Echo, EchoPlayer);
	if (!EchoBoss)
	{
		return false;
	}
	HurtTo(Echo, AbelRules::HoseShare);
	TestTrue(TEXT("The echo: no lock, no pips, straight out into the fog"), !Echo->GetCorruption()->IsLocked() && Echo->GetCorruption()->GetPips() == 0
		&& Echo->IsOutInFog());
	TestFalse(TEXT("...and no drinking: the six went west with the Gravewind"), Echo->GetCorruption()->StartDrink(0));
	return true;
}

#endif
