#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The bosses' shared attack rules (round 5; Bosses/BossAttackRules.h): the rules on plain numbers, a boss's attack log and
// its hurt zones' shapes; then each boss under them in a test level (a test level never ticks: the tests move them on): the
// Gravemother never bites three times in a row (her stomp, or her quake, breaks the run) and turns to a player behind her
// before she bites; the Foreman never swings three times in a row (his stamp); Abel flares and fires after two lunges; and
// their hurt zones grey in a wind-up, red as it lands, gathered for Looter.Boss.HurtZones.

#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossAttackRules.h"
#include "Bosses/BossComponent.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFight.h"
#include "Dev/BossHurtZoneDebug.h"
#include "Tests/AbelTestWorld.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float RulesStep = 0.05f;

	/** The rules on at their starting numbers for a test, whatever the console had; put back after. */
	struct FRulesAsShipped
	{
		struct FSaved
		{
			IConsoleVariable* Var = nullptr;
			FString Old;
		};
		TArray<FSaved> Saved;

		FRulesAsShipped()
		{
			Set(TEXT("Looter.Boss.Attack.Rules"), TEXT("1"));
			Set(TEXT("Looter.Boss.Attack.MinGap"), FString::SanitizeFloat(BossAttackRules::DefaultMinGap));
			Set(TEXT("Looter.Boss.Attack.MaxIdle"), FString::SanitizeFloat(BossAttackRules::DefaultMaxIdle));
			Set(TEXT("Looter.Boss.Attack.MaxRepeats"), FString::FromInt(BossAttackRules::DefaultMaxRepeats));
			Set(TEXT("Looter.Boss.Attack.FrontDegrees"), FString::SanitizeFloat(BossAttackRules::DefaultFrontDegrees));
			Set(TEXT("Looter.Boss.Attack.AimedDegrees"), FString::SanitizeFloat(BossAttackRules::DefaultAimedDegrees));
		}

		~FRulesAsShipped()
		{
			for (const FSaved& Each : Saved)
			{
				Each.Var->Set(*Each.Old, ECVF_SetByConsole);
			}
		}

		void Set(const TCHAR* Name, const FString& Value)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
			{
				Saved.Add({ Var, Var->GetString() });
				Var->Set(*Value, ECVF_SetByConsole);
			}
		}
	};

	/** The longest run of one attack in Attacks. */
	int32 LongestRun(const TArray<FName>& Attacks)
	{
		int32 Longest = 0;
		int32 Run = 0;
		for (int32 Index = 0; Index < Attacks.Num(); ++Index)
		{
			Run = Index > 0 && Attacks[Index] == Attacks[Index - 1] ? Run + 1 : 1;
			Longest = FMath::Max(Longest, Run);
		}
		return Longest;
	}

	FString Describe(const TArray<FName>& Attacks)
	{
		FString Text;
		for (const FName& Each : Attacks)
		{
			Text += (Text.IsEmpty() ? TEXT("") : TEXT(", ")) + Each.ToString();
		}
		return Text;
	}

	/** Moves a boss on (Step: one frame of it) for Seconds, noting each attack it starts, in order. */
	TArray<FName> RecordAttacks(UBossComponent& Boss, float Seconds, TFunctionRef<void()> Step)
	{
		TArray<FName> Attacks;
		int32 Seen = Boss.AttackLog.GetStarted();
		for (float Time = 0.f; Time < Seconds; Time += RulesStep)
		{
			Step();
			if (Boss.AttackLog.GetStarted() != Seen)
			{
				Seen = Boss.AttackLog.GetStarted();
				Attacks.Add(Boss.AttackLog.GetLast());
			}
		}
		return Attacks;
	}

	const FBossHurtZone* FindZone(const TArray<FBossHurtZone>& Zones, FName Attack, EBossAttackWindow Window)
	{
		return Zones.FindByPredicate([Attack, Window](const FBossHurtZone& Each) { return Each.Attack == Attack && Each.Window == Window; });
	}

	/** The Foreman as his lair spawns him (Legendary), begun, dropping no loot. */
	AForemanCreature* SpawnTeague(UWorld* World, const FVector& Feet)
	{
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = ECreatureRank::Legendary;
		Spawn.Level = 5;
		AForemanCreature* Spawned = Cast<AForemanCreature>(ACreatureBase::SpawnAtRuntime(World, AForemanCreature::StaticClass(), Feet, 0.f, Spawn));
		if (Spawned && !Spawned->HasActorBegunPlay())
		{
			Spawned->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Spawned);
		return Spawned;
	}

	void StepTeague(AForemanCreature* Teague)
	{
		Teague->ConsumeMovementInputVector();
		Teague->Tick(RulesStep);
		Teague->GetBoss()->TickFight(RulesStep);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossAttackRulesRulesTest, "Looter.Bosses.AttackRules.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossAttackRulesRulesTest::RunTest(const FString& Parameters)
{
	FRulesAsShipped Shipped;
	TestTrue(TEXT("The rules are on, at their starting numbers"), BossAttackRules::IsOn()
		&& FMath::IsNearlyEqual(BossAttackRules::MinGap(), BossAttackRules::DefaultMinGap)
		&& FMath::IsNearlyEqual(BossAttackRules::MaxIdle(), BossAttackRules::DefaultMaxIdle)
		&& BossAttackRules::MaxRepeats() == 2 && FMath::IsNearlyEqual(BossAttackRules::FrontDegrees(), BossAttackRules::DefaultFrontDegrees));
	TestNotNull(TEXT("The hurt-zone view's switch"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Boss.HurtZones")));

	// Reach and facing.
	const FBossAttackSpec Clod(TEXT("Clod"), 700.f, 2200.f, 120.f);
	TestTrue(TEXT("In reach between its distances"), BossAttackRules::IsInReach(Clod, 1000.f) && !BossAttackRules::IsInReach(Clod, 600.f)
		&& !BossAttackRules::IsInReach(Clod, 2300.f));
	TestTrue(TEXT("No most: any distance past its least"), BossAttackRules::IsInReach(FBossAttackSpec(TEXT("Roar"), 0.f, 0.f, 180.f), 1.0e5f));
	const FVector Front = FVector::ForwardVector;
	TestTrue(TEXT("45 degrees off: in front for a frontal attack"), BossAttackRules::IsFacing(70.f, Front, FVector(1.0, 1.0, 0.0)));
	TestTrue(TEXT("70 degrees off: on its edge, still in front"), BossAttackRules::IsFacing(70.f, Front, FVector(1.0, 0.0, 0.0).RotateAngleAxis(70.0, FVector::UpVector)));
	TestFalse(TEXT("90 off: not"), BossAttackRules::IsFacing(70.f, Front, FVector(0.0, 1.0, 0.0)));
	TestFalse(TEXT("Behind: not"), BossAttackRules::IsFacing(70.f, Front, FVector(-1.0, 0.0, 0.0)));
	TestTrue(TEXT("Behind is fine for an attack all round (180)"), BossAttackRules::IsFacing(180.f, Front, FVector(-1.0, 0.0, 0.0)));
	TestTrue(TEXT("Right on top of it: fine"), BossAttackRules::IsFacing(70.f, Front, FVector::ZeroVector));
	TestTrue(TEXT("Up and down don't count"), BossAttackRules::IsFacing(70.f, Front, FVector(100.0, 0.0, 900.0)));
	TestNearlyEqual(TEXT("A bite's 0.4 front is 66 degrees each side"), BossAttackRules::DegreesForDot(0.4f), 66.42f, 0.05f);

	// Never three of one in a row; the gap; the idle cap.
	const FName Bite(TEXT("Bite"));
	const FName Stomp(TEXT("Stomp"));
	TestFalse(TEXT("A second bite is fine"), BossAttackRules::WouldRepeat(Bite, Bite, 1, 2));
	TestTrue(TEXT("A third isn't"), BossAttackRules::WouldRepeat(Bite, Bite, 2, 2));
	TestFalse(TEXT("...another move is"), BossAttackRules::WouldRepeat(Stomp, Bite, 2, 2));
	TestFalse(TEXT("...no limit at 0"), BossAttackRules::WouldRepeat(Bite, Bite, 9, 0));
	TestFalse(TEXT("The gap: not 0.5 s after"), BossAttackRules::IsGapOpen(0.5f, false, 0.6f));
	TestTrue(TEXT("...open at 0.6 s"), BossAttackRules::IsGapOpen(0.6f, false, 0.6f));
	TestFalse(TEXT("...never while one is under way"), BossAttackRules::IsGapOpen(10.f, true, 0.6f));
	TestTrue(TEXT("Idle 5 s: past the cap"), BossAttackRules::IsOverdue(5.f, 5.f) && !BossAttackRules::IsOverdue(4.9f, 5.f) && !BossAttackRules::IsOverdue(99.f, 0.f));

	// An attack's windows: its wind-up, a live moment, its recovery.
	TestTrue(TEXT("Windows: wind-up, live, recovery, over"), BossAttackRules::WindowAt(0.1f, 0.5f, 0.15f, 0.4f) == EBossAttackWindow::Windup
		&& BossAttackRules::WindowAt(0.55f, 0.5f, 0.15f, 0.4f) == EBossAttackWindow::Active
		&& BossAttackRules::WindowAt(0.7f, 0.5f, 0.15f, 0.4f) == EBossAttackWindow::Recovery
		&& BossAttackRules::WindowAt(0.95f, 0.5f, 0.15f, 0.4f) == EBossAttackWindow::None);
	TestTrue(TEXT("...its live moment never past its recovery"), BossAttackRules::WindowAt(0.54f, 0.5f, 0.15f, 0.03f) == EBossAttackWindow::None);

	// A boss's log.
	FBossAttackLog Log;
	TestTrue(TEXT("A new log holds nothing back"), BossAttackRules::IsGapOpen(Log) && !BossAttackRules::IsBlocked(Bite, Log) && Log.GetStarted() == 0);
	Log.NoteStarted(Bite);
	TestTrue(TEXT("A bite under way: the gap shut, idle nothing"), Log.IsAttacking() && !BossAttackRules::IsGapOpen(Log) && Log.GetIdle() == 0.f);
	Log.Advance(1.f, true);
	TestEqual(TEXT("...idle doesn't count while it attacks"), Log.GetIdle(), 0.f);
	Log.NoteEnded();
	Log.Advance(0.3f, true);
	TestFalse(TEXT("0.3 s after: the gap still shut"), BossAttackRules::IsGapOpen(Log));
	Log.Advance(0.4f, true);
	TestTrue(TEXT("0.7 s after: open; idle 0.7 s"), BossAttackRules::IsGapOpen(Log) && FMath::IsNearlyEqual(Log.GetIdle(), 0.7f, 0.001f));
	Log.NoteStarted(Bite);
	Log.NoteEnded();
	TestTrue(TEXT("Two bites: a third is held back"), Log.GetStreak() == 2 && BossAttackRules::IsBlocked(Bite, Log) && !BossAttackRules::IsBlocked(Stomp, Log));
	Log.NoteStarted(Stomp);
	TestTrue(TEXT("A stomp between: the bite's free again"), Log.GetStreak() == 1 && !BossAttackRules::IsBlocked(Bite, Log));
	Log.NoteEnded();
	Log.Advance(BossAttackRules::DefaultMaxIdle + 0.1f, true);
	TestTrue(TEXT("Free and idle past the cap: overdue"), BossAttackRules::IsOverdue(Log));
	Log.NoteNudged();
	TestFalse(TEXT("...nudged: its clock starts again"), BossAttackRules::IsOverdue(Log));
	Log.Advance(1.f, false);
	TestEqual(TEXT("Not free (staggered, a moment of its own): idle starts again"), Log.GetIdle(), 0.f);
	Log.Reset();
	TestTrue(TEXT("Reset: nothing remembered"), Log.GetStarted() == 0 && Log.GetLast().IsNone() && BossAttackRules::IsGapOpen(Log));

	// Hurt zones' shapes.
	const FBossHurtZone Disc = FBossHurtZone::Disc(TEXT("Quake"), EBossAttackWindow::Active, FVector::ZeroVector, 300.f, 250.f);
	TestTrue(TEXT("A disc: inside its ring and height, not past them"), Disc.Contains(FVector(250.0, 0.0, 100.0)) && !Disc.Contains(FVector(350.0, 0.0, 100.0))
		&& !Disc.Contains(FVector(0.0, 0.0, 400.0)));
	const FBossHurtZone Wedge = FBossHurtZone::Wedge(Bite, EBossAttackWindow::Windup, FVector::ZeroVector, FVector::ForwardVector, 400.f, 60.f);
	TestTrue(TEXT("A wedge: in front within reach, not behind"), Wedge.Contains(FVector(300.0, 100.0, 50.0)) && !Wedge.Contains(FVector(-300.0, 0.0, 50.0))
		&& !Wedge.Contains(FVector(500.0, 0.0, 50.0)));
	const FBossHurtZone Lane = FBossHurtZone::Lane(TEXT("Furrow"), EBossAttackWindow::Active, FVector::ZeroVector, FVector(1000.0, 0.0, 0.0), 150.f);
	TestTrue(TEXT("A lane: beside its line, not off its side or past its end"), Lane.Contains(FVector(600.0, 120.0, 50.0)) && !Lane.Contains(FVector(600.0, 200.0, 50.0))
		&& !Lane.Contains(FVector(1300.0, 0.0, 50.0)));
	FColor Color;
	TestTrue(TEXT("The view: red live, grey in a wind-up, nothing in a recovery"), UBossHurtZoneDebug::ColorFor(EBossAttackWindow::Active, Color) && Color.R > 200 && Color.G < 80
		&& UBossHurtZoneDebug::ColorFor(EBossAttackWindow::Windup, Color) && Color.R == Color.G && !UBossHurtZoneDebug::ColorFor(EBossAttackWindow::Recovery, Color));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossAttackRulesGravemotherTest, "Looter.Bosses.AttackRules.Gravemother",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossAttackRulesGravemotherTest::RunTest(const FString& Parameters)
{
	FRulesAsShipped Shipped;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// A player in her bite's reach in front of her, in her first phase (no quake): her roar, then bites; never a third bite
	// in a row, her stomp between.
	GravemotherTestWorld::SpawnFloor(World, FVector::ZeroVector, 3000.0);
	AGravemotherCreature* Mother = GravemotherTestWorld::SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	const double Near = FMath::Max(Mother->GetAttackRange() * 0.6, Mother->GetCapsuleComponent()->GetScaledCapsuleRadius() + 80.0);
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, Mother->GetActorLocation() + FVector(Near, 0.0, 0.0));
	UBossComponent* Boss = Mother->GetBoss();
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Her boss"), Boss))
	{
		return false;
	}
	Boss->StartFight(Player);
	const TArray<FName> Attacks = RecordAttacks(*Boss, 18.f, [Mother]() { Mother->Tick(RulesStep); });
	const FString Seen = Describe(Attacks);
	TestTrue(FString::Printf(TEXT("She keeps attacking (%s)"), *Seen), Attacks.Num() >= 5);
	TestTrue(FString::Printf(TEXT("...never the same three times in a row (%s)"), *Seen), LongestRun(Attacks) <= 2);
	TestTrue(FString::Printf(TEXT("...her stomp breaks a run of bites (%s)"), *Seen), Attacks.Contains(GravemotherFight::AttackId(EGravemotherMove::Stomp))
		&& Attacks.Contains(GravemotherFight::AttackId(EGravemotherMove::Bite)));

	// Her hurt zones through a stomp: grey as she rears, red as her forelegs come down, nothing in her recovery.
	TArray<FBossHurtZone> Zones;
	const FName StompId = GravemotherFight::AttackId(EGravemotherMove::Stomp);
	const float TellAt = GravemotherTestWorld::TickUntil(Mother, 8.f, [Mother, &Zones, StompId]()
	{
		Zones.Reset();
		Mother->GatherHurtZones(Zones);
		return FindZone(Zones, StompId, EBossAttackWindow::Windup) != nullptr;
	});
	TestTrue(TEXT("A stomp's wind-up: its disc grey"), TellAt >= 0.f);
	const FBossHurtZone* Tell = FindZone(Zones, StompId, EBossAttackWindow::Windup);
	TestTrue(TEXT("...the player inside it"), Tell && Tell->Contains(Player->GetActorLocation()));
	const float LiveAt = GravemotherTestWorld::TickUntil(Mother, 2.f, [Mother, &Zones, StompId]()
	{
		Zones.Reset();
		Mother->GatherHurtZones(Zones);
		return FindZone(Zones, StompId, EBossAttackWindow::Active) != nullptr;
	});
	TestTrue(FString::Printf(TEXT("...red as it lands (%.2f s on, her wind-up %.2f s)"), LiveAt, Mother->StompWindup), LiveAt >= 0.f);
	GravemotherTestWorld::TickFor(Mother, BossAttackRules::ActiveSeconds() + RulesStep * 2.f);
	Zones.Reset();
	Mother->GatherHurtZones(Zones);
	TestNull(TEXT("...nothing live in her recovery"), FindZone(Zones, StompId, EBossAttackWindow::Active));
	TArray<FBossHurtZone> Gathered;
	TestTrue(TEXT("The view gathers her boss's zones"), UBossHurtZoneDebug::Gather(World, Gathered) >= 1);

	// A player behind her: she turns to them before she bites (never a bite started at someone behind her).
	GravemotherTestWorld::SpawnFloor(World, FVector(0.0, 8000.0, 0.0), 3000.0);
	AGravemotherCreature* Turner = GravemotherTestWorld::SpawnMother(World, FVector(0.0, 8000.0, 0.0));
	if (!TestNotNull(TEXT("Second Gravemother"), Turner))
	{
		return false;
	}
	ACharacter* Behind = BossTestWorld::SpawnPlayer(World, Turner->GetActorLocation() - FVector(Near, 0.0, 0.0));
	if (!TestNotNull(TEXT("Second stand-in"), Behind))
	{
		return false;
	}
	Turner->AlertTo(Behind);
	bool bFacedWhenStarted = true;
	const float BitAt = GravemotherTestWorld::TickUntil(Turner, 4.f, [Turner, Behind, &bFacedWhenStarted]()
	{
		if (Turner->GetCreatureState() != ECreatureState::Attack)
		{
			return false;
		}
		// Its first frame: the rules let it start only with them in front (a frame's turn of slack).
		bFacedWhenStarted = BossAttackRules::IsFacing(BossAttackRules::FrontDegrees() + 30.f, Turner->GetActorForwardVector(),
			Behind->GetActorLocation() - Turner->GetActorLocation());
		return true;
	});
	TestTrue(FString::Printf(TEXT("Behind her: she turns and bites (%.2f s)"), BitAt), BitAt > 0.f);
	TestTrue(TEXT("...facing them as she starts"), bFacedWhenStarted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossAttackRulesForemanTest, "Looter.Bosses.AttackRules.Foreman",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossAttackRulesForemanTest::RunTest(const FString& Parameters)
{
	FRulesAsShipped Shipped;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	EncounterTestWorld::SpawnBlocks(World, { FBox(FVector(-3000.0, -3000.0, -100.0), FVector(3000.0, 3000.0, 0.0)) });
	AForemanCreature* Teague = SpawnTeague(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("The Foreman"), Teague))
	{
		return false;
	}
	// A player crowding him (inside his fuse's least distance) in his first phase (no roof): his entrance, then the handle;
	// never a third swing in a row, his stamp between.
	const double Near = FMath::Max(Teague->GetAttackRange() * 0.6, Teague->GetCapsuleComponent()->GetScaledCapsuleRadius() + 80.0);
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, Teague->GetActorLocation() + FVector(Near, 0.0, 0.0));
	UBossComponent* Boss = Teague->GetBoss();
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("His boss"), Boss))
	{
		return false;
	}
	Boss->StartFight(Player);
	const TArray<FName> Attacks = RecordAttacks(*Boss, 18.f, [Teague]() { StepTeague(Teague); });
	const FString Seen = Describe(Attacks);
	TestTrue(FString::Printf(TEXT("He keeps attacking (%s)"), *Seen), Attacks.Num() >= 5);
	TestTrue(FString::Printf(TEXT("...his entrance first (%s)"), *Seen), Attacks.Num() > 0 && Attacks[0] == AForemanCreature::AttackId(EForemanMove::Intro));
	TestTrue(FString::Printf(TEXT("...never the same three times in a row (%s)"), *Seen), LongestRun(Attacks) <= 2);
	TestTrue(FString::Printf(TEXT("...his stamp breaks a run of swings (%s)"), *Seen), Attacks.Contains(AForemanCreature::AttackId(EForemanMove::Stamp))
		&& Attacks.Contains(AForemanCreature::AttackId(EForemanMove::Swing)));

	// His swing's wedge in front, grey through the heave.
	TArray<FBossHurtZone> Zones;
	const FName SwingId = AForemanCreature::AttackId(EForemanMove::Swing);
	float Waited = 0.f;
	for (; Waited < 8.f; Waited += RulesStep)
	{
		Zones.Reset();
		Teague->GatherHurtZones(Zones);
		if (FindZone(Zones, SwingId, EBossAttackWindow::Windup))
		{
			break;
		}
		StepTeague(Teague);
	}
	const FBossHurtZone* Heave = FindZone(Zones, SwingId, EBossAttackWindow::Windup);
	TestTrue(TEXT("A swing's heave: its wedge grey, the player in it"), Heave && Heave->Contains(Player->GetActorLocation()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossAttackRulesAbelTest, "Looter.Bosses.AttackRules.Abel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossAttackRulesAbelTest::RunTest(const FString& Parameters)
{
	FRulesAsShipped Shipped;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = AbelTestWorld::SpawnAbel(World);
	if (!TestNotNull(TEXT("Abel"), Abel))
	{
		return false;
	}
	// A player 3 m in front of him on the deck, in his lunge's reach.
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, Abel->GetActorLocation() + FVector(300.0, 0.0, 0.0));
	UBossComponent* Boss = Abel->GetBoss();
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("His boss"), Boss))
	{
		return false;
	}
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Boss->StartFight(Player);
	}
	// His entrance over, nothing under way.
	const float FreeAt = AbelTestWorld::RunUntil(Abel, 6.f, [Abel]()
	{
		return Abel->GetMove() == EAbelMove::None && Abel->GetCreatureState() != ECreatureState::Attack && !Abel->IsLunging();
	});
	if (!TestTrue(TEXT("His entrance over, he's free"), FreeAt >= 0.f))
	{
		return false;
	}
	// Two lunges in a row (as his log has them): a third is held back; his lantern flares instead once the gap is open.
	FBossAttackLog& Log = Boss->AttackLog;
	Log.NoteStarted(AbelRules::LungeAttack());
	Log.NoteEnded();
	Log.NoteStarted(AbelRules::LungeAttack());
	Log.NoteEnded();
	TestTrue(TEXT("Two lunges: a third is held back"), BossAttackRules::IsBlocked(AbelRules::LungeAttack(), Log));
	const float FlareAt = AbelTestWorld::RunUntil(Abel, 2.f, [Abel]() { return Abel->GetMove() == EAbelMove::Flare; });
	TestTrue(FString::Printf(TEXT("...his flare instead (%.2f s on)"), FlareAt), FlareAt >= 0.f);
	TestTrue(FString::Printf(TEXT("...after the gap (%.2f s)"), BossAttackRules::MinGap()), FlareAt >= BossAttackRules::MinGap() - AbelTestWorld::Frame);
	TestTrue(TEXT("...noted as his buckshot"), Log.GetLast() == AbelRules::BuckshotAttack() && Log.GetStreak() == 1);

	// The buckshot's cone grey through the flare, at the player; red as it fires.
	TArray<FBossHurtZone> Zones;
	Abel->GatherHurtZones(Zones);
	const FBossHurtZone* Cone = FindZone(Zones, AbelRules::BuckshotAttack(), EBossAttackWindow::Windup);
	TestTrue(TEXT("The flare: his cone grey, the player in it"), Cone && Cone->Contains(Player->GetActorLocation()));
	AbelTestWorld::RunUntil(Abel, 2.f, [Abel]() { return Abel->GetMove() == EAbelMove::Fire; });
	Zones.Reset();
	Abel->GatherHurtZones(Zones);
	TestNotNull(TEXT("...red as it fires"), FindZone(Zones, AbelRules::BuckshotAttack(), EBossAttackWindow::Active));

	// Over a stretch of his fight: never three of one in a row.
	const TArray<FName> Attacks = RecordAttacks(*Boss, 16.f, [Abel]() { AbelTestWorld::Run(Abel, RulesStep); });
	TestTrue(FString::Printf(TEXT("Never the same three times in a row (%s)"), *Describe(Attacks)), LongestRun(Attacks) <= 2);
	return true;
}

#endif
