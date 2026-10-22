#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/CreatureVoiceBarks.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Bestiary/BestiaryEntry.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/BellRingerGraves.h"
#include "Creatures/BellRingerRing.h"
#include "Creatures/BellRingerRules.h"
#include "Creatures/BellRingerSteady.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The Bell-Ringer (ABellRingerCreature): its rules alone, then the creature in test levels, its brain moved on a frame
// at a time (a test level never ticks anything itself). A test level's world clock stands still, so every death there
// happened "just now": the fallen's ages are checked on the rules.

namespace
{
	constexpr float Frame = 1.f / 30.f;

	/** A floor the steering's looks find ground on (its top at Z 0), 60 m across. */
	void SpawnFloor(UWorld* World)
	{
		LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	}

	/**
	 * A Bell-Ringer spawned in play, standing on the floor at Feet, dropping no loot, walking. A test level never gives a
	 * movement component its capsule (it doesn't initialize components), and without one the movement has no pawn to hand
	 * input to: every AddMovementInput, the brain's and the backing off's, would be dropped. Given here, as the Spitter's
	 * and the steering's play tests give theirs.
	 */
	ABellRingerCreature* SpawnRinger(UWorld* World, const FVector& Feet)
	{
		ABellRingerCreature* Ringer = Cast<ABellRingerCreature>(EncounterTestWorld::SpawnCreature(World, ABellRingerCreature::StaticClass(), Feet));
		if (Ringer)
		{
			UCharacterMovementComponent* Movement = Ringer->GetCharacterMovement();
			Movement->SetUpdatedComponent(Ringer->GetCapsuleComponent());
			Movement->SetMovementMode(MOVE_Falling);
			Movement->SetMovementMode(MOVE_Walking);
			Movement->Velocity = FVector::ZeroVector;
		}
		return Ringer;
	}

	AUnpaidCreature* SpawnUnpaid(UWorld* World, const FVector& Feet)
	{
		return Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), Feet));
	}

	/** A shot's damage as the engine hands it over: its point first (where it hit), then the damage itself. */
	void ShootAt(AActor* Victim, float Damage, const FVector& Where, UPrimitiveComponent* Part, bool bCritical)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		const UDamageType* Type = bCritical ? GetDefault<UWeaponCritDamageType>() : GetDefault<UWeaponDamageType>();
		Victim->OnTakePointDamage.Broadcast(Victim, Damage, nullptr, Where, Part, NAME_None, FVector::ForwardVector, Type, nullptr);
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, Type, nullptr, nullptr);
	}

	/** Moves the ringer's frame on for Seconds, its walking's input taken each frame as the game's movement takes it. */
	void Run(ABellRingerCreature& Ringer, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds - KINDA_SMALL_NUMBER; Time += Frame)
		{
			Ringer.ConsumeMovementInputVector();
			Ringer.Tick(Frame);
		}
	}

	/** Rings a toll from its tell to its landing (BeginToll, then the tell's length). */
	bool RingToll(ABellRingerCreature& Ringer)
	{
		if (!Ringer.BeginToll())
		{
			return false;
		}
		const int32 Before = Ringer.GetTollCount();
		for (int32 Step = 0; Step < 200 && Ringer.GetTollCount() == Before; ++Step)
		{
			Ringer.ConsumeMovementInputVector();
			Ringer.Tick(Frame);
		}
		return Ringer.GetTollCount() > Before;
	}

	/** The rings of a kind in the level; with bShowingOnly, not those let go and fading (a test level never ticks them away). */
	int32 CountRings(UWorld* World, EBellRingKind Kind, bool bShowingOnly = false)
	{
		int32 Count = 0;
		for (TActorIterator<ABellRingerRing> It(World); It; ++It)
		{
			Count += It->GetKind() == Kind && !It->IsActorBeingDestroyed() && !(bShowingOnly && It->IsFinishing()) ? 1 : 0;
		}
		return Count;
	}

	FBellRingerFallen MakeFallen(const FVector& Feet, double DiedAt)
	{
		FBellRingerFallen Fallen;
		Fallen.Class = AUnpaidCreature::StaticClass();
		Fallen.Feet = Feet;
		Fallen.DiedAt = DiedAt;
		return Fallen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerRulesTest, "Looter.Creatures.BellRinger.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerRulesTest::RunTest(const FString& Parameters)
{
	const FBellRingerTuning Tuning;
	// It hangs back: in from far off, holding behind its pack, backing off when they come near, a little at a time.
	TestTrue(TEXT("It holds 8-14 m off"), Tuning.HoldDistance >= 800.f && Tuning.HoldDistance <= 1400.f);
	TestTrue(TEXT("20 m off: it comes nearer"), BellRingerRules::ChooseStance(Tuning, 2000.f, false, 0.f, 100.f) == EBellRingerStance::Approach);
	TestTrue(TEXT("9 m off: it holds"), BellRingerRules::ChooseStance(Tuning, 900.f, false, 0.f, 100.f) == EBellRingerStance::Hold);
	TestTrue(TEXT("5 m off: it backs off"), BellRingerRules::ChooseStance(Tuning, 500.f, false, 0.f, 100.f) == EBellRingerStance::BackOff);
	TestTrue(TEXT("...not again right after it last did"),
		BellRingerRules::ChooseStance(Tuning, 500.f, false, 0.f, Tuning.BackOffRest * 0.5f) == EBellRingerStance::Hold);
	TestTrue(TEXT("Backing off at 9 m: it keeps on"), BellRingerRules::ChooseStance(Tuning, 900.f, true, 1.f, 0.f) == EBellRingerStance::BackOff);
	TestTrue(TEXT("...until it's far enough"), BellRingerRules::ChooseStance(Tuning, Tuning.BackOffUntil + 10.f, true, 1.f, 0.f) == EBellRingerStance::Hold);
	TestTrue(TEXT("...or has backed off long enough"),
		BellRingerRules::ChooseStance(Tuning, 600.f, true, Tuning.BackOffSeconds + 0.1f, 0.f) == EBellRingerStance::Hold);
	TestTrue(TEXT("Run down to 2 m it swipes"), BellRingerRules::CanSwipe(Tuning, 200.f, 1.f));
	TestFalse(TEXT("...not from 4 m"), BellRingerRules::CanSwipe(Tuning, 400.f, 1.f));

	// The toll: ranks ring sooner; the Gravebound's bell is cracked.
	TestTrue(TEXT("A Restless ringer tolls sooner"),
		BellRingerRules::TollInterval(Tuning, ECreatureRank::Rare) < BellRingerRules::TollInterval(Tuning, ECreatureRank::Basic));
	TestTrue(TEXT("A Gravebound's bell is cracked"), BellRingerRules::IsCrackedBell(ECreatureRank::Epic));
	TestFalse(TEXT("...a Basic one's isn't"), BellRingerRules::IsCrackedBell(ECreatureRank::Basic));
	TestTrue(TEXT("A fallen to raise: it raises"), BellRingerRules::ChooseToll(true, 0, 2, true) == EBellToll::Raise);
	TestTrue(TEXT("Two raised already: it steadies"), BellRingerRules::ChooseToll(true, 2, 2, true) == EBellToll::Steady);
	TestTrue(TEXT("None fallen: it steadies"), BellRingerRules::ChooseToll(false, 0, 2, true) == EBellToll::Steady);
	TestTrue(TEXT("No room under the caps: it steadies"), BellRingerRules::ChooseToll(true, 0, 2, false) == EBellToll::Steady);
	TestEqual(TEXT("Never more than two raised"), Tuning.MaxRaises, 2);
	TestTrue(TEXT("Fallen in the last 20 s"), FMath::IsNearlyEqual(Tuning.FallenMaxAge, 20.f));

	// Which fallen a toll raises: only the recent, near it, not at the player's feet, not raised before; the nearest.
	const FVector Ringer(0.0, 0.0, 100.0);
	const FVector Player(800.0, 0.0, 100.0);
	const double Now = 100.0;
	TArray<FBellRingerFallen> Fallen;
	Fallen.Add(MakeFallen(FVector(300.0, 0.0, 0.0), Now - 25.0));             // 0: too long ago
	Fallen.Add(MakeFallen(FVector(0.0, 300.0, 0.0), Now - 3.0));              // 1: raised already
	Fallen.Last().bRaised = true;
	Fallen.Add(MakeFallen(FVector(0.0, 2500.0, 0.0), Now - 3.0));             // 2: too far from the ringer
	Fallen.Add(MakeFallen(Player - FVector(50.0, 0.0, 100.0), Now - 3.0));     // 3: at the player's feet (7.5 m from the ringer)
	Fallen.Add(MakeFallen(FVector(0.0, -1000.0, 0.0), Now - 5.0));            // 4: fine, 10 m
	Fallen.Add(MakeFallen(FVector(-500.0, 0.0, 0.0), Now - 15.0));            // 5: fine, 5 m: the nearest
	Fallen.Add(MakeFallen(FVector(0.0, 0.0, 900.0), Now - 2.0));              // 6: up on another level
	TestFalse(TEXT("Not one fallen 25 s ago"), BellRingerRules::IsRaisable(Fallen[0], Ringer, Now, Player, Tuning));
	TestFalse(TEXT("Not one raised already"), BellRingerRules::IsRaisable(Fallen[1], Ringer, Now, Player, Tuning));
	TestFalse(TEXT("Not one 25 m off"), BellRingerRules::IsRaisable(Fallen[2], Ringer, Now, Player, Tuning));
	TestFalse(TEXT("Not one at the player's feet"), BellRingerRules::IsRaisable(Fallen[3], Ringer, Now, Player, Tuning));
	TestFalse(TEXT("Not one on another level of ground"), BellRingerRules::IsRaisable(Fallen[6], Ringer, Now, Player, Tuning));
	TestTrue(TEXT("One fallen 5 s ago 10 m off"), BellRingerRules::IsRaisable(Fallen[4], Ringer, Now, Player, Tuning));
	TestEqual(TEXT("The nearest is raised"), BellRingerRules::ChooseFallen(Fallen, Ringer, Now, Player, Tuning), 5);
	Fallen.Add(MakeFallen(FVector(-520.0, 40.0, 0.0), Now - 1.0));            // 7: as near as 5, and fell later
	TestEqual(TEXT("Of two as near, the one fallen last"), BellRingerRules::ChooseFallen(Fallen, Ringer, Now, Player, Tuning), 7);
	TestEqual(TEXT("None at all: none"), BellRingerRules::ChooseFallen(TArray<FBellRingerFallen>(), Ringer, Now, Player, Tuning), INDEX_NONE);

	// The bell's hit, and the way back.
	TestTrue(TEXT("A hit on the bell's ball is the bell's"), BellRingerRules::IsBellHit(FVector::ZeroVector, 14.f, FVector(14.f, 0.f, 0.f)));
	TestFalse(TEXT("...one 30 cm off isn't"), BellRingerRules::IsBellHit(FVector::ZeroVector, 14.f, FVector(30.f, 0.f, 0.f)));
	const TArray<FVector> Ways = BellRingerRules::BackOffDirections(FVector::ZeroVector, FVector(500.0, 0.0, 0.0));
	TestTrue(TEXT("It backs off straight away first"), Ways.Num() == 5 && Ways[0].Equals(FVector(-1.0, 0.0, 0.0), 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerStatsTest, "Looter.Creatures.BellRinger.Stats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerStatsTest::RunTest(const FString& Parameters)
{
	const ABellRingerCreature* Defaults = GetDefault<ABellRingerCreature>();
	const AUnpaidCreature* Unpaid = GetDefault<AUnpaidCreature>();
	const UHealthComponent* Health = Defaults->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* UnpaidHealth = Unpaid->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("It has health"), Health) && UnpaidHealth)
	{
		TestTrue(TEXT("Fragile: less health than an Unpaid"), Health->MaxHealth < UnpaidHealth->MaxHealth);
	}
	TestTrue(TEXT("Weak in a fight"), Defaults->AttackDamage < Unpaid->AttackDamage);
	TestTrue(TEXT("Worth killing first"), Defaults->XPReward > Unpaid->XPReward);
	TestTrue(TEXT("It trails the pack"), Defaults->ChaseSpeed < Unpaid->ChaseSpeed);
	TestEqual(TEXT("The Unpaid's pack"), Defaults->PackTag, Unpaid->PackTag);
	TestEqual(TEXT("It counts as one of the 16 Unpaid"), UEncounterSettings::Get().FindClassCap(ABellRingerCreature::StaticClass()), 16);  // (fights enlarged 2026-10-21; 12)
	TestTrue(TEXT("Its phase-steps never bring it nearer than it holds"), Defaults->PhaseStep.NearestToTarget >= 800.f);
	TestFalse(TEXT("Bare-headed"), Defaults->GetHat()->GetStaticMesh() != nullptr && Defaults->GetHat()->IsVisible());

	// Its own lines: it says them where it has them, and no other Unpaid ever does.
	const UCreatureVoiceComponent* Voice = Defaults->FindComponentByClass<UCreatureVoiceComponent>();
	if (TestNotNull(TEXT("It has a voice"), Voice))
	{
		TestEqual(TEXT("Its voice says its own lines"), Voice->BarkKind, ABellRingerCreature::BarkKind);
	}
	const TConstArrayView<FCreatureBarkLine> Lines = CreatureBarks::AllLines();
	int32 Own = 0;
	for (const FCreatureBarkLine& Line : Lines)
	{
		Own += Line.Kind && ABellRingerCreature::BarkKind == FName(Line.Kind) ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("About eight lines of its own (%d)"), Own), Own >= 8 && Own <= 12);
	FRandomStream Random(20261012);
	bool bOwnSpot = true;
	bool bOthersNever = true;
	for (int32 Roll = 0; Roll < 300; ++Roll)
	{
		const int32 Its = CreatureBarks::PickLine(ECreatureBark::Spot, ECreatureRank::Basic, Random, {}, ABellRingerCreature::BarkKind);
		bOwnSpot &= Lines.IsValidIndex(Its) && Lines[Its].Kind && FName(Lines[Its].Kind) == ABellRingerCreature::BarkKind;
		const int32 Theirs = CreatureBarks::PickLine(ECreatureBark::Spot, ECreatureRank::Rare, Random);
		bOthersNever &= Lines.IsValidIndex(Theirs) && !Lines[Theirs].Kind;
	}
	TestTrue(TEXT("It spots the player in its own words"), bOwnSpot);
	TestTrue(TEXT("No other Unpaid says its lines"), bOthersNever);

	// Its bell is a weak spot: a ball bullets meet, always critical.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ABellRingerCreature* Ringer = SpawnRinger(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Ringer spawned"), Ringer))
	{
		return false;
	}
	USphereComponent* Zone = Ringer->GetBellHitZone();
	TestTrue(TEXT("Bullets meet the bell"), Zone->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	TestTrue(TEXT("...while it lives"), Zone->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	FHitResult OnBell;
	OnBell.Component = Zone;
	OnBell.ImpactPoint = Ringer->GetBellCenter();
	TestTrue(TEXT("A shot on the bell is critical"), Ringer->IsCriticalSpot(OnBell));
	TestTrue(TEXT("...and the bell's"), Ringer->IsBellHit(Ringer->GetBellCenter() + FVector(Zone->GetScaledSphereRadius(), 0.0, 0.0)));
	TestTrue(TEXT("The bell hangs at about hand height"), Ringer->GetBellCenter().Z > Ringer->GetActorLocation().Z - 90.0
		&& Ringer->GetBellCenter().Z < Ringer->GetActorLocation().Z + 60.0);
	BossTestWorld::Kill(Ringer);
	TestFalse(TEXT("Dead, its bell takes no shots"), Zone->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerKeepsDistanceTest, "Looter.Creatures.BellRinger.KeepsDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerKeepsDistanceTest::RunTest(const FString& Parameters)
{
	// Too near, it backs away with its eyes on the player; at its distance it stands; far off it comes nearer; run down,
	// it swipes (its only attack: never from where it hangs back).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(500.0, 0.0, 100.0));
	ABellRingerCreature* Ringer = SpawnRinger(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Ringer spawned"), Ringer))
	{
		return false;
	}
	Ringer->AlertTo(Player);
	TestTrue(TEXT("It hunts the player"), Ringer->GetTarget() == Player);

	// 5 m off: it backs away.
	Ringer->ConsumeMovementInputVector();
	Ringer->Tick(Frame);
	TestTrue(TEXT("5 m off: it backs off"), Ringer->IsBackingOff());
	const FVector Step = Ringer->GetPendingMovementInputVector().GetSafeNormal2D();
	TestTrue(FString::Printf(TEXT("...away from the player (%s)"), *Step.ToCompactString()), FVector::DotProduct(Step, FVector(-1.0, 0.0, 0.0)) > 0.9);
	TestFalse(TEXT("...and doesn't swing at them from there"), Ringer->GetCreatureState() == ECreatureState::Attack);

	// Backing off at 9 m it keeps on; by 10.6 m it holds its ground (the brain only faces them; it takes no step).
	Player->SetActorLocation(FVector(900.0, 0.0, 100.0));
	Ringer->ConsumeMovementInputVector();
	Ringer->Tick(Frame);
	TestTrue(TEXT("9 m off, it's still backing off"), Ringer->IsBackingOff());
	Player->SetActorLocation(FVector(1060.0, 0.0, 100.0));
	Ringer->ConsumeMovementInputVector();
	Ringer->Tick(Frame);
	TestFalse(TEXT("10.6 m off: it stops backing off"), Ringer->IsBackingOff());
	TestTrue(TEXT("...and stands"), Ringer->GetPendingMovementInputVector().IsNearlyZero(0.01));
	TestTrue(TEXT("...holding"), Ringer->GetStance() == EBellRingerStance::Hold);

	// 18 m off (short of where it would phase-step): it drifts nearer.
	Player->SetActorLocation(FVector(1800.0, 0.0, 100.0));
	Ringer->ConsumeMovementInputVector();
	Ringer->Tick(Frame);
	const FVector Toward = Ringer->GetPendingMovementInputVector().GetSafeNormal2D();
	TestTrue(FString::Printf(TEXT("18 m off: it comes nearer (%s)"), *Toward.ToCompactString()), FVector::DotProduct(Toward, FVector(1.0, 0.0, 0.0)) > 0.5);
	TestTrue(TEXT("...approaching"), Ringer->GetStance() == EBellRingerStance::Approach);

	// Run down to 2 m: it swipes with the bell.
	Player->SetActorLocation(FVector(200.0, 0.0, 100.0));
	Run(*Ringer, 0.1f);
	TestTrue(TEXT("Run down, it swipes"), Ringer->GetCreatureState() == ECreatureState::Attack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerTellBeforeTollTest, "Looter.Creatures.BellRinger.TellBeforeToll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerTellBeforeTollTest::RunTest(const FString& Parameters)
{
	// Turned on a player, it waits a moment, then tells: the bell up, a halo gathering, standing still, for the tell's length;
	// only then the toll, a ring running out over the ground. With nobody fallen it steadies its pack (itself too).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(900.0, 0.0, 100.0));
	ABellRingerCreature* Ringer = SpawnRinger(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Ringer spawned"), Ringer))
	{
		return false;
	}
	const FBellRingerTuning& Tuning = Ringer->Tuning;
	const FVector CarriedBell = Ringer->GetBellCenter();
	Ringer->AlertTo(Player);
	float Time = 0.f;
	float TellAt = -1.f;
	float TollAt = -1.f;
	float HighestBell = -1.0e9f;
	bool bStoodInTell = true;
	bool bHaloInTell = true;
	for (int32 Step = 0; Step < 300 && TollAt < 0.f; ++Step)
	{
		Ringer->ConsumeMovementInputVector();
		Ringer->Tick(Frame);
		Time += Frame;
		if (Ringer->IsTelling())
		{
			if (TellAt < 0.f)
			{
				TellAt = Time;
			}
			bStoodInTell &= Ringer->GetPendingMovementInputVector().IsNearlyZero(0.01);
			bHaloInTell &= CountRings(World, EBellRingKind::Halo, true) == 1;
			HighestBell = FMath::Max(HighestBell, static_cast<float>(Ringer->GetBellCenter().Z));
		}
		if (Ringer->GetTollCount() > 0)
		{
			TollAt = Time;
		}
	}
	if (!TestTrue(TEXT("It tells"), TellAt > 0.f) || !TestTrue(TEXT("...and tolls"), TollAt > 0.f))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("Not at once: the pack's rush first (%.2f s)"), TellAt), TellAt >= Tuning.FirstTollDelay - Frame);
	TestTrue(FString::Printf(TEXT("The toll comes the tell's length after the tell (%.2f s)"), TollAt - TellAt),
		FMath::Abs((TollAt - TellAt) - Tuning.TellSeconds) <= 2.f * Frame);
	TestTrue(TEXT("It stands through the tell"), bStoodInTell);
	TestTrue(TEXT("A halo gathers round the bell through the tell"), bHaloInTell);
	if (Ringer->GetMesh()->GetSkeletalMeshAsset())
	{
		TestTrue(FString::Printf(TEXT("The bell goes up high (%.0f cm over where it was carried)"), HighestBell - CarriedBell.Z),
			HighestBell - CarriedBell.Z > 40.f);
	}
	else
	{
		AddWarning(TEXT("SK_Unpaid isn't imported: the bell's rise waits for the model."));
	}
	TestEqual(TEXT("The halo is let go with the toll"), CountRings(World, EBellRingKind::Halo, true), 0);
	TestTrue(TEXT("A ring runs out over the ground"), CountRings(World, EBellRingKind::Wave) == 1);
	TestTrue(TEXT("...and one bursts round the bell"), CountRings(World, EBellRingKind::Burst) == 1);
	TestTrue(TEXT("Nobody fallen: it steadied its pack"), Ringer->GetLastToll() == EBellToll::Steady);
	TestNotNull(TEXT("...itself too"), UBellRingerSteady::Find(*Ringer));
	TestTrue(FString::Printf(TEXT("The next waits its interval (%.1f s)"), Ringer->GetTollWait()),
		FMath::IsNearlyEqual(Ringer->GetTollWait(), BellRingerRules::TollInterval(Tuning, Ringer->GetRank()), 0.01f));
	TestTrue(TEXT("It lowers the bell after"), Ringer->GetAct() == ABellRingerCreature::EAct::Lower);
	Run(*Ringer, Tuning.LowerSeconds + Frame);
	TestTrue(TEXT("...then carries it again"), Ringer->GetAct() == ABellRingerCreature::EAct::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerRaisesTest, "Looter.Creatures.BellRinger.RaisesRecentFallen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerRaisesTest::RunTest(const FString& Parameters)
{
	// Three of the Unpaid fall near it: its tolls raise one each where it fell, at half health and rising, after the player,
	// marked so it falls for good the next time; the third toll steadies instead (two at most).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 100.0));
	ABellRingerCreature* Ringer = SpawnRinger(World, FVector::ZeroVector);
	UBellRingerGraves* Graves = UBellRingerGraves::Get(World);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Ringer spawned"), Ringer) || !TestNotNull(TEXT("The graves"), Graves))
	{
		return false;
	}
	const FVector Spots[] = { FVector(0.0, 400.0, 0.0), FVector(0.0, -500.0, 0.0), FVector(-600.0, 300.0, 0.0) };
	float FullHealth = 0.f;
	for (const FVector& Spot : Spots)
	{
		AUnpaidCreature* Doomed = SpawnUnpaid(World, Spot);
		if (!TestNotNull(TEXT("An Unpaid spawned"), Doomed))
		{
			return false;
		}
		FullHealth = Doomed->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
		BossTestWorld::Kill(Doomed);
	}
	Graves->Watch(World->GetTimeSeconds(), true);
	TestEqual(TEXT("Each death is noted"), Graves->GetFallen().Num(), 3);
	TestFalse(TEXT("A ringer itself is never raised"), UBellRingerGraves::CanBeRaised(*Ringer));

	Ringer->AlertTo(Player);
	TestTrue(TEXT("Fallen near it: its toll would raise one"), Ringer->ChooseTollNow() == EBellToll::Raise);
	if (!TestTrue(TEXT("The first toll"), RingToll(*Ringer)))
	{
		return false;
	}
	TestTrue(TEXT("...raised one"), Ringer->GetLastToll() == EBellToll::Raise && Ringer->GetRaisedCount() == 1);
	TArray<ACreatureBase*> Risen = Ringer->GetRaised();
	if (TestEqual(TEXT("One risen"), Risen.Num(), 1))
	{
		ACreatureBase* First = Risen[0];
		TestTrue(TEXT("...one of the Unpaid"), First->IsA<AUnpaidCreature>());
		TestNearlyEqual(TEXT("...at half health"), First->FindComponentByClass<UHealthComponent>()->GetMaxHealth(), FullHealth * 0.5f, 0.5f);
		TestTrue(TEXT("...rising out of the ground"), Cast<AUnpaidCreature>(First)->IsPhasing());
		TestTrue(TEXT("...after the player"), First->GetTarget() == Player);
		TestTrue(TEXT("...where one fell (the nearest)"), FVector::Dist2D(First->GetActorLocation(), Spots[0]) < 120.0);
		TestTrue(TEXT("...marked raised"), First->ActorHasTag(UBellRingerGraves::RaisedTag));
		TestFalse(TEXT("...so it falls for good next time"), UBellRingerGraves::CanBeRaised(*First));
	}
	Run(*Ringer, Ringer->Tuning.LowerSeconds + Frame);
	TestTrue(TEXT("The second toll"), RingToll(*Ringer));
	TestTrue(TEXT("...raised another"), Ringer->GetLastToll() == EBellToll::Raise && Ringer->GetRaisedCount() == 2);
	Run(*Ringer, Ringer->Tuning.LowerSeconds + Frame);
	TestTrue(TEXT("One fallen is left near it"), Graves->FindRaisable(Ringer->GetActorLocation(), World->GetTimeSeconds(),
		Player->GetActorLocation(), Ringer->Tuning) != INDEX_NONE);
	TestTrue(TEXT("The third toll"), RingToll(*Ringer));
	TestTrue(TEXT("...steadies: never more than two raised"), Ringer->GetLastToll() == EBellToll::Steady && Ringer->GetRaisedCount() == 2);

	// A raised one falling again is not noted for another toll.
	Risen = Ringer->GetRaised();
	if (Risen.Num() > 0)
	{
		const int32 Before = Graves->GetFallen().Num();
		BossTestWorld::Kill(Risen[0]);
		Graves->Watch(World->GetTimeSeconds(), true);
		TestEqual(TEXT("A raised one falls for good"), Graves->GetFallen().Num(), Before);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerSteadiesTest, "Looter.Creatures.BellRinger.SteadiesWhenNoneFallen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerSteadiesTest::RunTest(const FString& Parameters)
{
	// None fallen: its toll quickens the Unpaid near it for a while, a faint aura at their feet; one far off isn't; a
	// second toll only makes it last; then their pace comes back as it was.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 100.0));
	ABellRingerCreature* Ringer = SpawnRinger(World, FVector::ZeroVector);
	AUnpaidCreature* Near = SpawnUnpaid(World, FVector(400.0, 300.0, 0.0));
	AUnpaidCreature* Far = SpawnUnpaid(World, FVector(-2500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Ringer spawned"), Ringer) || !TestNotNull(TEXT("Near"), Near)
		|| !TestNotNull(TEXT("Far"), Far))
	{
		return false;
	}
	const FBellRingerTuning& Tuning = Ringer->Tuning;
	const float NearPace = Near->ChaseSpeed;
	const float NearCooldown = Near->AttackCooldown;
	const float FarPace = Far->ChaseSpeed;
	Ringer->AlertTo(Player);
	TestTrue(TEXT("None fallen: its toll would steady"), Ringer->ChooseTollNow() == EBellToll::Steady);
	if (!TestTrue(TEXT("It tolls"), RingToll(*Ringer)))
	{
		return false;
	}
	UBellRingerSteady* Steady = UBellRingerSteady::Find(*Near);
	if (!TestNotNull(TEXT("The one near it is steadied"), Steady))
	{
		return false;
	}
	TestNearlyEqual(TEXT("...chasing faster"), Near->ChaseSpeed, NearPace * Tuning.SteadySpeedScale, 0.01f);
	TestNearlyEqual(TEXT("...attacking sooner"), Near->AttackCooldown, NearCooldown * Tuning.SteadyCooldownScale, 0.001f);
	TestTrue(TEXT("...an aura at its feet"), CountRings(World, EBellRingKind::Aura) >= 2);
	TestNull(TEXT("The one 25 m off isn't"), UBellRingerSteady::Find(*Far));
	TestNearlyEqual(TEXT("...its pace as it was"), Far->ChaseSpeed, FarPace, 0.01f);

	Steady->Advance(Tuning.SteadySeconds * 0.5f);
	Run(*Ringer, Tuning.LowerSeconds + Frame);
	TestTrue(TEXT("A second toll"), RingToll(*Ringer));
	TestNearlyEqual(TEXT("...doesn't quicken it twice"), Near->ChaseSpeed, NearPace * Tuning.SteadySpeedScale, 0.01f);
	TestNearlyEqual(TEXT("...it lasts again"), Steady->GetTimeLeft(), Tuning.SteadySeconds, 0.05f);
	Steady->Advance(Tuning.SteadySeconds + 0.1f);
	TestNull(TEXT("Its time out, it's gone"), UBellRingerSteady::Find(*Near));
	TestNearlyEqual(TEXT("...its pace back as it was"), Near->ChaseSpeed, NearPace, 0.01f);
	TestNearlyEqual(TEXT("...and its attacks' wait"), Near->AttackCooldown, NearCooldown, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerBellShotTest, "Looter.Creatures.BellRinger.BellShotCancelsToll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerBellShotTest::RunTest(const FString& Parameters)
{
	// Its bell shot in the tell: the toll is lost (nothing raised, nothing steadied), it reels, and its next toll waits a whole
	// interval. A shot elsewhere in the tell doesn't stop it; a shot on the bell outside a tell is critical and no more.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(900.0, 0.0, 100.0));
	ABellRingerCreature* Ringer = SpawnRinger(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Ringer spawned"), Ringer))
	{
		return false;
	}
	const FBellRingerTuning& Tuning = Ringer->Tuning;
	Ringer->AlertTo(Player);
	if (!TestTrue(TEXT("It tells"), Ringer->BeginToll()))
	{
		return false;
	}
	Run(*Ringer, Tuning.TellSeconds * 0.5f);
	TestTrue(TEXT("Mid-tell"), Ringer->IsTelling());
	const float Radius = Ringer->GetBellHitZone()->GetScaledSphereRadius();
	const FVector OnBell = Ringer->GetBellCenter() + FVector(Radius, 0.0, 0.0);
	FHitResult Hit;
	Hit.Component = Ringer->GetBellHitZone();
	Hit.ImpactPoint = OnBell;
	TestTrue(TEXT("A shot on the bell is critical"), Ringer->IsCriticalSpot(Hit));
	ShootAt(Ringer, 5.f, OnBell, Ringer->GetBellHitZone(), true);
	TestFalse(TEXT("Shot, the tell is broken"), Ringer->IsTelling());
	TestTrue(TEXT("...it reels"), Ringer->GetAct() == ABellRingerCreature::EAct::Reel);
	TestTrue(TEXT("...and its next toll waits a whole interval"),
		FMath::IsNearlyEqual(Ringer->GetTollWait(), BellRingerRules::TollInterval(Tuning, Ringer->GetRank()), 0.01f));
	TestEqual(TEXT("...its halo knocked out"), CountRings(World, EBellRingKind::Halo, true), 0);
	Run(*Ringer, Tuning.TellSeconds);
	TestEqual(TEXT("No toll landed"), Ringer->GetTollCount(), 0);
	TestNull(TEXT("...nothing steadied"), UBellRingerSteady::Find(*Ringer));
	Run(*Ringer, Tuning.ReelSeconds);
	TestTrue(TEXT("The reel passes"), Ringer->GetAct() == ABellRingerCreature::EAct::None);

	// A shot on its body in the tell: the toll goes on.
	if (TestTrue(TEXT("It tells again"), Ringer->BeginToll()))
	{
		Run(*Ringer, Tuning.TellSeconds * 0.3f);
		ShootAt(Ringer, 5.f, Ringer->GetCoalLocation(), Ringer->GetMesh(), false);
		TestTrue(TEXT("A body shot doesn't break the tell"), Ringer->IsTelling());
		Run(*Ringer, Tuning.TellSeconds);
		TestEqual(TEXT("...the toll lands"), Ringer->GetTollCount(), 1);
	}

	// Outside a tell, the bell is still the weak spot, and no more.
	Run(*Ringer, Tuning.LowerSeconds + Frame);
	TestTrue(TEXT("Carrying the bell"), Ringer->GetAct() == ABellRingerCreature::EAct::None);
	ShootAt(Ringer, 5.f, Ringer->GetBellCenter() + FVector(Radius, 0.0, 0.0), Ringer->GetBellHitZone(), true);
	TestTrue(TEXT("A shot on the carried bell doesn't make it reel"), Ringer->GetAct() == ABellRingerCreature::EAct::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBellRingerBestiaryTest, "Looter.Creatures.BellRinger.Bestiary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBellRingerBestiaryTest::RunTest(const FString& Parameters)
{
	// Its own page (Tools/Unreal/create_bestiary_pages.py BellRinger), about it and not the Unpaid's, in the Enemies; and its
	// stand wears the bell and the rope once they're imported (Art/Models/Creatures/UnpaidBell.py).
	const TCHAR* PagePath = TEXT("/Game/Data/Bestiary/DA_Bestiary_BellRinger.DA_Bestiary_BellRinger");
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(PagePath))))
	{
		AddError(TEXT("No DA_Bestiary_BellRinger: run Tools/Unreal/create_bestiary_pages.py BellRinger in the editor."));
		return false;
	}
	const UBestiaryEntry* Page = LoadObject<UBestiaryEntry>(nullptr, PagePath);
	if (!TestNotNull(TEXT("Its page loads"), Page))
	{
		return false;
	}
	TestTrue(TEXT("Its page is about it"), Page->ActorClass.LoadSynchronous() == ABellRingerCreature::StaticClass());
	TestTrue(TEXT("...in the Enemies"), Page->Category == EBestiaryCategory::Enemy);
	TestFalse(TEXT("...written up"), Page->Description.IsEmpty() || Page->DisplayName.IsEmpty());
	TestNotNull(TEXT("...with a model for the stand"), Page->LoadPreviewMesh());

	const ABellRingerCreature* Defaults = GetDefault<ABellRingerCreature>();
	const UStaticMesh* BellModel = Defaults->GetBell()->GetStaticMesh();
	if (!BellModel || !Defaults->GetMesh()->GetSkeletalMeshAsset())
	{
		AddWarning(TEXT("SM_UnpaidBell isn't in the class's defaults yet (import Art/Models/Creatures/UnpaidBell.py, then restart the editor)."));
		return true;
	}
	const TArray<FBestiaryStandPart> Parts = Page->GetPreviewParts(Defaults->GetMesh()->GetSkeletalMeshAsset());
	const FBestiaryStandPart* Bell = Parts.FindByPredicate([BellModel](const FBestiaryStandPart& Part) { return Part.Mesh == BellModel; });
	if (TestNotNull(TEXT("The stand wears the bell"), Bell))
	{
		TestEqual(TEXT("...in its right fist"), Bell->Bone, Defaults->Rig.RightArm.Hand);
	}
	TestFalse(TEXT("...and no hat"), Parts.ContainsByPredicate([Defaults](const FBestiaryStandPart& Part) { return Part.Bone == Defaults->Rig.Hat; }));
	const FBox BellBox = BellModel->GetBounds().GetBox();
	TestTrue(FString::Printf(TEXT("The bell is a hand bell's size, oversized to read (%.0f cm tall)"), BellBox.GetSize().Z),
		BellBox.GetSize().Z > 20.0 && BellBox.GetSize().Z < 50.0);
	TestTrue(TEXT("...hanging under its pivot (the fist)"), BellBox.Min.Z < -15.0 && BellBox.Max.Z < 15.0);
	return true;
}

#endif
