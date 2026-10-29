#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The creatures that react (the 2026-10-14 polish): the notice beat as a creature first spots its prey (once a hunt, never
// for one already hurt or a boss, ended by a hit), corpses lying their time without blocking anyone, the hit flash fading,
// the level's cap on footfall dust, and Mister Sexton's breath. The rules are checked on their own (CreatureReactRules.h),
// then on creatures in a test level, ticked by hand (a test level never ticks).

#include "Bosses/BossComponent.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Story/MisterSexton.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float ReactFrame = 1.f / 60.f;

	/** A creature of Class spawned as play spawns it, standing at Feet, dropping no loot. */
	template <typename TCreature>
	TCreature* ReactSpawn(UWorld* World, const FVector& Feet)
	{
		return Cast<TCreature>(EncounterTestWorld::SpawnCreature(World, TCreature::StaticClass(), Feet));
	}

	/** Its walking movement given its capsule (a test level's movement takes none), so its steering sees it walking. */
	void ReactGiveMovement(ACreatureBase& Creature)
	{
		UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
		Movement->SetUpdatedComponent(Creature.GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Falling);
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Velocity = FVector::ZeroVector;
	}

	/** One frame of its brain, its pending steer looked at and dropped (nothing in a test level moves it). */
	bool ReactStepWantsToMove(ACreatureBase& Creature)
	{
		Creature.ConsumeMovementInputVector();
		Creature.Tick(ReactFrame);
		return !Creature.GetPendingMovementInputVector().IsNearlyZero(0.01);
	}

	/** Moves a creature on by Seconds, a frame at a time. */
	void ReactRun(ACreatureBase& Creature, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += ReactFrame)
		{
			Creature.ConsumeMovementInputVector();
			Creature.Tick(ReactFrame);
		}
	}

	/** Its parts that would stop a walking body (the player's capsule): collision on and blocking pawns. */
	FString ReactPawnBlockers(const ACreatureBase& Creature)
	{
		TArray<FString> Blocking;
		TInlineComponentArray<UPrimitiveComponent*> Parts(&Creature);
		for (const UPrimitiveComponent* Part : Parts)
		{
			if (Part && Part->GetCollisionEnabled() != ECollisionEnabled::NoCollision
				&& Part->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
			{
				Blocking.Add(Part->GetName());
			}
		}
		return FString::Join(Blocking, TEXT(", "));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureReactNoticeTest, "Looter.Creatures.React.Notice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureReactNoticeTest::RunTest(const FString& Parameters)
{
	// The rules: a first sight from calm, home-going or a search that never chased has its beat; nothing else does.
	TestTrue(TEXT("From idle: a beat"), FCreatureNotice::MayNotice(ECreatureState::Idle, false, false, false));
	TestTrue(TEXT("From a stroll: a beat"), FCreatureNotice::MayNotice(ECreatureState::Wander, false, false, false));
	TestTrue(TEXT("Walking home: a beat"), FCreatureNotice::MayNotice(ECreatureState::Return, false, false, false));
	TestTrue(TEXT("A search that never chased: a beat"), FCreatureNotice::MayNotice(ECreatureState::Search, false, false, false));
	TestFalse(TEXT("A search back to the chase it had: none"), FCreatureNotice::MayNotice(ECreatureState::Search, false, false, true));
	TestFalse(TEXT("Hurt: none"), FCreatureNotice::MayNotice(ECreatureState::Idle, true, false, false));
	TestFalse(TEXT("A boss: none"), FCreatureNotice::MayNotice(ECreatureState::Idle, false, true, false));
	TestFalse(TEXT("Mid-attack: none"), FCreatureNotice::MayNotice(ECreatureState::Attack, false, false, false));
	TestNearlyEqual(TEXT("Its length runs from the shortest..."), FCreatureNotice::LengthFor(0.f, false), FCreatureNotice::ShortestSeconds, 0.001f);
	TestNearlyEqual(TEXT("...to the longest by its seed"), FCreatureNotice::LengthFor(1.f, false), FCreatureNotice::LongestSeconds, 0.001f);
	TestNearlyEqual(TEXT("Point blank: short"), FCreatureNotice::LengthFor(0.5f, true), FCreatureNotice::PointBlankSeconds, 0.001f);
	TestTrue(TEXT("The whole delay stays small"), FCreatureNotice::LongestSeconds <= 0.7f);
	FCreatureNotice Beat;
	Beat.Begin(0.5f);
	TestNearlyEqual(TEXT("Its pose starts from nothing"), Beat.GetStrength(), 0.f, 0.001f);
	Beat.Advance(0.25f);
	TestTrue(FString::Printf(TEXT("...is full halfway (%.2f)"), Beat.GetStrength()), Beat.GetStrength() > 0.95f);
	Beat.Advance(0.26f);
	TestTrue(TEXT("...and over at its length"), !Beat.IsHolding() && Beat.GetStrength() == 0.f);

	// On a spider in a test level, 15 m from a player stand-in, on a floor its steering can walk.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const AActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1500.0, 0.0, 100.0));
	ASpiderCreature* Spider = ReactSpawn<ASpiderCreature>(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A player stand-in"), Player) || !TestNotNull(TEXT("A spider"), Spider)
		|| !TestNotNull(TEXT("...with its reactions"), Spider->GetReact()))
	{
		return false;
	}
	ReactGiveMovement(*Spider);
	const FCreatureNotice& Notice = Spider->GetReact()->GetNotice();

	Spider->DevSight(Player);
	TestTrue(TEXT("Seen: it hunts the player"), Spider->GetCreatureState() == ECreatureState::Chase && Spider->GetTarget() == Player);
	TestTrue(TEXT("...after its notice beat"), Spider->IsNoticing());
	float Held = 0.f;
	bool bStood = true;
	while (Spider->IsNoticing() && Held < 2.f)
	{
		// The frame its beat runs out it rightly sets off in the same tick (the beat ends before its brain runs): only frames
		// still in the beat after their tick count.
		const bool bWantsToMove = ReactStepWantsToMove(*Spider);
		bStood &= !bWantsToMove || !Spider->IsNoticing();
		Held += ReactFrame;
	}
	TestTrue(FString::Printf(TEXT("The beat lasts its length (%.2f s)"), Held),
		Held >= FCreatureNotice::ShortestSeconds - 2.f * ReactFrame && Held <= FCreatureNotice::LongestSeconds + 2.f * ReactFrame);
	TestTrue(TEXT("...standing its ground through it"), bStood);
	bool bCame = false;
	for (float Waited = 0.f; Waited < 1.f && !bCame; Waited += ReactFrame)
	{
		bCame = ReactStepWantsToMove(*Spider);
	}
	TestTrue(TEXT("Then it comes for them"), bCame);

	// Once a chase: lost and seen again in the same hunt, no second beat; a new hunt has its own.
	Spider->DevPutInState(ECreatureState::Search, Player);
	Spider->DevSight(Player);
	TestTrue(TEXT("Seen again in the same hunt: chasing"), Spider->GetCreatureState() == ECreatureState::Chase);
	TestFalse(TEXT("...with no second beat"), Spider->IsNoticing());
	TestEqual(TEXT("One beat so far"), Notice.GetCount(), 1);
	Spider->DevPutInState(ECreatureState::Idle);
	Spider->DevSight(Player);
	TestTrue(TEXT("A new hunt has its beat"), Spider->IsNoticing() && Notice.GetCount() == 2);

	// A hit ends it, and a creature already hurt has none.
	BossTestWorld::Hurt(Spider, 5.f);
	TestFalse(TEXT("A hit in the beat ends it"), Spider->IsNoticing());
	Spider->DevPutInState(ECreatureState::Idle);
	Spider->DevSight(Player);
	TestTrue(TEXT("Hurt and seeing them: chasing"), Spider->GetCreatureState() == ECreatureState::Chase);
	TestFalse(TEXT("...at once, no beat"), Spider->IsNoticing());
	TestEqual(TEXT("Still two beats"), Notice.GetCount(), 2);

	// A boss keeps its fight's rules: no beat.
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector(0.0, 3000.0, 0.0), 0.f);
	ACreatureBase* BossBody = Boss ? Cast<ACreatureBase>(Boss->GetOwner()) : nullptr;
	if (TestNotNull(TEXT("The test boss"), BossBody))
	{
		BossTestWorld::NoLoot(BossBody);
		BossBody->DevSight(Player);
		TestFalse(TEXT("A boss has no beat"), BossBody->IsNoticing());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureReactCorpsesTest, "Looter.Creatures.React.Corpses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureReactCorpsesTest::RunTest(const FString& Parameters)
{
	// Each kind's body lies its time (spiders about ten seconds, a slime's puddle eight, the Unpaid crumble to ash in under
	// three), and no corpse stops anyone walking: nothing of it blocks a pawn.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	struct FReactCorpseKind
	{
		const TCHAR* Name;
		UClass* Class;
		float AtLeast;
	};
	const FReactCorpseKind Kinds[] = {
		{ TEXT("Spider"), ASpiderCreature::StaticClass(), 9.f },
		{ TEXT("Spitter"), ASpitterSpider::StaticClass(), 9.f },
		{ TEXT("Slime"), ASlimeCreature::StaticClass(), 7.f },
		{ TEXT("Unpaid"), AUnpaidCreature::StaticClass(), 2.f },
		{ TEXT("Dead miner"), ADeadMinerCreature::StaticClass(), 2.f },
		{ TEXT("Bell-Ringer"), ABellRingerCreature::StaticClass(), 2.f } };
	TMap<FString, ACreatureBase*> Dead;
	double Along = 0.0;
	for (const FReactCorpseKind& Kind : Kinds)
	{
		ACreatureBase* Creature = EncounterTestWorld::SpawnCreature(World, Kind.Class, FVector(Along, 0.0, 0.0));
		Along += 800.0;
		if (!TestNotNull(FString::Printf(TEXT("A %s"), Kind.Name), Creature))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: alive, it stands in the way"), Kind.Name), !ReactPawnBlockers(*Creature).IsEmpty());
		BossTestWorld::Kill(Creature);
		TestTrue(FString::Printf(TEXT("%s: dead"), Kind.Name), Creature->IsDead());
		TestTrue(FString::Printf(TEXT("%s: its body lies %.1f s (at least %.0f)"), Kind.Name, Creature->CorpseTime, Kind.AtLeast),
			Creature->CorpseTime >= Kind.AtLeast);
		const FString Blocking = ReactPawnBlockers(*Creature);
		TestTrue(FString::Printf(TEXT("%s: its corpse blocks nobody (%s)"), Kind.Name, *Blocking), Blocking.IsEmpty());
		Dead.Add(Kind.Name, Creature);
	}

	// The spider's throe and curl are over by 1.5 s (Looter.Creatures.Anim.SpiderDeathCurl has them): it lies still on its
	// folded legs through its time, shudders once, then sinks away.
	if (ACreatureBase* const* Found = Dead.Find(TEXT("Spider")))
	{
		ACreatureBase* Spider = *Found;
		if (Spider->GetBonePose().IsEmpty())
		{
			AddWarning(TEXT("SK_Spider isn't imported: its corpse's time isn't checked."));
		}
		else
		{
			ReactRun(*Spider, 1.5f);
			const double Curled = Spider->GetBonePose()[0].Transform.GetLocation().Z;
			// The pose's space has its origin on the ground under the thorax: the body's height over the ground.
			const double Ride = ASpiderCreature::DeadRide * Spider->GetSizeScale();
			TestTrue(FString::Printf(TEXT("Spider: by 1.5 s it lies on its folded legs (%.1f cm up, its dead ride %.0f)"), Curled, Ride),
				FMath::Abs(Curled - Ride) < 3.0);
			ReactRun(*Spider, Spider->CorpseTime - 1.8f);
			const double Late = Spider->GetBonePose()[0].Transform.GetLocation().Z;
			TestTrue(FString::Printf(TEXT("Spider: still lying there near its time's end (%.1f cm from curled)"), Late - Curled),
				FMath::Abs(Late - Curled) < 3.0);
			ReactRun(*Spider, 2.3f);
			const double Sunk = Spider->GetBonePose()[0].Transform.GetLocation().Z;
			TestTrue(FString::Printf(TEXT("Spider: then it sinks away (%.0f cm)"), Curled - Sunk), Curled - Sunk > 40.0);
		}
	}
	// The Unpaid crumples first, then breaks up into ash, gone by its time's end.
	if (ACreatureBase* const* Found = Dead.Find(TEXT("Unpaid")))
	{
		AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(*Found);
		ReactRun(*Unpaid, 0.5f);
		TestTrue(FString::Printf(TEXT("Unpaid: crumpling, not ash yet (%.2f)"), Unpaid->GetPhase()), Unpaid->GetPhase() <= 0.001f);
		ReactRun(*Unpaid, 1.2f);
		TestTrue(FString::Printf(TEXT("Unpaid: breaking up (%.2f)"), Unpaid->GetPhase()), Unpaid->GetPhase() > 0.05f && Unpaid->GetPhase() < 0.95f);
		ReactRun(*Unpaid, Unpaid->CorpseTime - 1.7f + 0.05f);
		TestTrue(FString::Printf(TEXT("Unpaid: ash by its time's end (%.2f)"), Unpaid->GetPhase()), Unpaid->GetPhase() >= 0.999f);
	}
	// The slime's puddle lies its time, then dries up.
	if (ACreatureBase* const* Found = Dead.Find(TEXT("Slime")))
	{
		ACreatureBase* Slime = *Found;
		if (Slime->GetBonePose().IsEmpty())
		{
			AddWarning(TEXT("SK_Slime isn't imported: its puddle's time isn't checked."));
		}
		else
		{
			ReactRun(*Slime, Slime->CorpseTime - 2.f);
			const double Puddle = Slime->GetBonePose()[0].Transform.GetScale3D().GetMax();
			TestTrue(FString::Printf(TEXT("Slime: its puddle lies there (spread %.2f)"), Puddle), Puddle > 1.2);
			ReactRun(*Slime, 3.4f);
			const double Dried = Slime->GetBonePose()[0].Transform.GetScale3D().GetMax();
			TestTrue(FString::Printf(TEXT("Slime: then it dries up (%.2f)"), Dried), Dried < 0.1);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureReactHitFlashTest, "Looter.Creatures.React.HitFlash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureReactHitFlashTest::RunTest(const FString& Parameters)
{
	// The rules: full at once, fading over under a tenth of a second; a crit's stronger; worn a moment after, then off.
	FCreatureHitFlash Flash;
	TestFalse(TEXT("No flash before a hit"), Flash.IsOn() || Flash.IsWorn());
	Flash.Start(false);
	const float Full = Flash.GetIntensity();
	TestNearlyEqual(TEXT("A hit: full at once"), Full, FCreatureHitFlash::Glow * FCreatureHitFlash::NormalStrength, 0.001f);
	Flash.Advance(FCreatureHitFlash::Seconds * 0.5f);
	TestTrue(TEXT("...fading"), Flash.GetIntensity() < Full * 0.5f && Flash.GetIntensity() > 0.f);
	Flash.Advance(FCreatureHitFlash::Seconds * 0.5f + 0.001f);
	TestTrue(TEXT("...gone within its time"), !Flash.IsOn() && Flash.GetIntensity() == 0.f && Flash.IsWorn());
	Flash.Advance(FCreatureHitFlash::LingerSeconds);
	TestFalse(TEXT("...its overlay off a moment after"), Flash.IsWorn());
	Flash.Start(true);
	TestTrue(TEXT("A crit flashes harder"), Flash.GetIntensity() > Full && Flash.IsCritical());
	TestTrue(TEXT("Under a tenth of a second"), FCreatureHitFlash::Seconds <= 0.1f);

	// HITS (2026-10-28): the flash's tint by what the body spills (strength unchanged): a spider's warm bone, a slime's pale gel, an
	// Unpaid's ember, the old warm white for anything else; a crit's gold is the same for all.
	{
		const FLinearColor Bone = FCreatureHitFlash::TintFor(ECreatureBlood::Ichor, false);
		const FLinearColor Pale = FCreatureHitFlash::TintFor(ECreatureBlood::Gel, false);
		const FLinearColor Coal = FCreatureHitFlash::TintFor(ECreatureBlood::Embers, false);
		TestTrue(TEXT("Spider flash: warm bone (1, .9, .7)"), Bone.Equals(FLinearColor(1.f, 0.9f, 0.7f), 0.001f));
		TestTrue(TEXT("Slime flash: pale gel (.85, 1, .8)"), Pale.Equals(FLinearColor(0.85f, 1.f, 0.8f), 0.001f));
		TestTrue(TEXT("Unpaid flash: ember (1, .62, .32)"), Coal.Equals(FLinearColor(1.f, 0.62f, 0.32f), 0.001f));
		TestTrue(TEXT("...a miner's too, the dark ichor is a spider's"), FCreatureHitFlash::TintFor(ECreatureBlood::MinerEmbers, false).Equals(Coal, 0.001f)
			&& FCreatureHitFlash::TintFor(ECreatureBlood::DarkIchor, false).Equals(Bone, 0.001f));
		TestTrue(TEXT("The three differ"), !Bone.Equals(Pale, 0.05f) && !Pale.Equals(Coal, 0.05f) && !Bone.Equals(Coal, 0.05f));
		TestTrue(TEXT("Anything else: the old warm white"), FCreatureHitFlash::TintFor(ECreatureBlood::None, false).Equals(FLinearColor(1.f, 0.94f, 0.86f), 0.001f));
		bool bCritsAlike = true;
		for (const ECreatureBlood Blood : { ECreatureBlood::Ichor, ECreatureBlood::Gel, ECreatureBlood::Embers, ECreatureBlood::None })
		{
			bCritsAlike &= FCreatureHitFlash::TintFor(Blood, true).Equals(FLinearColor(1.f, 0.78f, 0.48f), 0.001f);
		}
		TestTrue(TEXT("A crit keeps its gold (1, .78, .48) on every body"), bCritsAlike);
	}

	// Steady fire only nudges it (the see-through ghost under full auto, 2026-10-22); a pause and it flashes in full again.
	{
		FCreatureHitFlash Auto;
		Auto.Advance(FCreatureHitFlash::LingerSeconds + 1.f);
		Auto.Start(false);
		const float First = Auto.GetIntensity();
		float Loudest = 0.f;
		for (int32 Shot = 0; Shot < 20; ++Shot)
		{
			Auto.Advance(0.1f);
			Auto.Start(false);
			Loudest = FMath::Max(Loudest, Auto.GetIntensity());
		}
		TestNearlyEqual(TEXT("Full auto: the first hit flashes in full"), First, FCreatureHitFlash::Glow * FCreatureHitFlash::NormalStrength, 0.001f);
		TestTrue(FString::Printf(TEXT("...the rest only nudge it (%.2f of %.2f)"), Loudest, First),
			Loudest <= First * FCreatureHitFlash::RepeatShare + 0.001f);
		TestTrue(TEXT("...and the overlay stays on through it"), Auto.IsWorn());
		Auto.Advance(0.1f);
		Auto.Start(true);
		TestTrue(TEXT("A crit in steady fire still stands out"), Auto.GetIntensity() > Loudest * 1.5f && Auto.IsCritical());
		Auto.Advance(FCreatureHitFlash::QuietSeconds + 0.01f);
		Auto.Start(false);
		TestNearlyEqual(TEXT("A pause, then a hit: full again"), Auto.GetIntensity(), First, 0.001f);

		FCreatureHitFlash Pellets;
		Pellets.Start(false);
		for (int32 Pellet = 0; Pellet < 7; ++Pellet)
		{
			Pellets.Start(false);
		}
		TestNearlyEqual(TEXT("A shotgun's pellets: the first's flash stands, nothing adds up"), Pellets.GetIntensity(), First, 0.001f);
		// In the scene's light (a 7-lux sun) an overlay of 0.1 doubles a sunlit body: a crit's whole flash stays under that,
		// and its rim no bigger than its lift (a rim of 1.3 made a white dome of the abdomen and cards of the bristles).
		TestTrue(TEXT("The flash: a small lift, its rim no bigger"),
			FCreatureHitFlash::Glow * FCreatureHitFlash::CriticalStrength * (FCreatureHitFlash::Even + FCreatureHitFlash::Rim) <= 0.15f
			&& FCreatureHitFlash::Rim <= FCreatureHitFlash::Even + 0.001f && FCreatureHitFlash::RimPower >= 2.f);
	}

	// On a spider: the flash worn on its body, fading, then taken off.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	ASpiderCreature* Spider = ReactSpawn<ASpiderCreature>(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("...with its reactions"), Spider->GetReact()))
	{
		return false;
	}
	const UCreatureReactComponent* React = Spider->GetReact();
	BossTestWorld::Hurt(Spider, 5.f);
	TestTrue(TEXT("Shot: its body flashes"), React->GetFlash().IsOn() && React->GetFlash().GetIntensity() > 0.f);
	const UMaterialInstanceDynamic* Look = React->GetFlashLook();
	if (Look)
	{
		TestTrue(TEXT("...worn on its body"), Spider->GetMesh()->OverlayMaterial.Get() == Look);
	}
	else
	{
		AddWarning(TEXT("M_FX_HitFlash isn't in this checkout: the flash isn't worn, only timed."));
	}
	// ...in the tint of what it is (HITS): the material's Color as the hit set it, for a spider, a slime and an Unpaid.
	if (Look)
	{
		auto ColorOf = [](const UMaterialInstanceDynamic* Flash, FLinearColor& Out)
		{
			return Flash && Flash->GetVectorParameterValue(FHashedMaterialParameterInfo(FName(TEXT("Color"))), Out);
		};
		FLinearColor Worn;
		TestTrue(TEXT("Spider: its flash is warm bone"), ColorOf(Look, Worn) && Worn.Equals(FLinearColor(1.f, 0.9f, 0.7f), 0.01f));
		ASlimeCreature* Slime = ReactSpawn<ASlimeCreature>(WorldWrapper.GetTestWorld(), FVector(400.0, 0.0, 0.0));
		AUnpaidCreature* Unpaid = ReactSpawn<AUnpaidCreature>(WorldWrapper.GetTestWorld(), FVector(800.0, 0.0, 0.0));
		if (Slime && Slime->GetReact() && Unpaid && Unpaid->GetReact())
		{
			BossTestWorld::Hurt(Slime, 5.f);
			BossTestWorld::Hurt(Unpaid, 5.f);
			TestTrue(TEXT("Slime: its flash is pale gel"), ColorOf(Slime->GetReact()->GetFlashLook(), Worn) && Worn.Equals(FLinearColor(0.85f, 1.f, 0.8f), 0.01f));
			TestTrue(TEXT("Unpaid: its flash is ember"), ColorOf(Unpaid->GetReact()->GetFlashLook(), Worn) && Worn.Equals(FLinearColor(1.f, 0.62f, 0.32f), 0.01f));
		}
	}
	float Last = React->GetFlash().GetIntensity();
	bool bFading = true;
	for (float Elapsed = 0.f; Elapsed < FCreatureHitFlash::Seconds + 0.02f; Elapsed += ReactFrame)
	{
		Spider->Tick(ReactFrame);
		bFading &= React->GetFlash().GetIntensity() <= Last + 0.0001f;
		Last = React->GetFlash().GetIntensity();
	}
	TestTrue(TEXT("It fades, never brightening"), bFading);
	TestTrue(TEXT("...and is gone in under a tenth of a second"), !React->GetFlash().IsOn() && Last == 0.f);
	ReactRun(*Spider, FCreatureHitFlash::LingerSeconds + 0.1f);
	TestFalse(TEXT("Its overlay comes off"), React->IsFlashWorn());
	if (Look)
	{
		TestTrue(TEXT("...off its body"), Spider->GetMesh()->OverlayMaterial.Get() == nullptr);
	}

	// A power's burn tick flashes nothing (its tint shows the burn).
	{
		UCreatureReactComponent::FQuietHits Quiet;
		BossTestWorld::Hurt(Spider, 1.f);
	}
	TestFalse(TEXT("A quiet burn tick: no flash"), React->GetFlash().IsOn() || React->IsFlashWorn());

	// The killing shot: the body shows its death, never a white flash held through the kill's hit-stop (the white ghost).
	BossTestWorld::Kill(Spider);
	TestTrue(TEXT("Killed: no flash on the corpse"), !React->GetFlash().IsOn() && !React->IsFlashWorn());
	if (Look)
	{
		TestTrue(TEXT("...its overlay off at once"), Spider->GetMesh()->OverlayMaterial.Get() == nullptr);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureReactFootfallCapTest, "Looter.Creatures.React.FootfallCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureReactFootfallCapTest::RunTest(const FString& Parameters)
{
	// However many feet land, the level throws at most its allowance: its rate on average, its burst at once.
	FCreatureDustBudget Budget(40.f, 12.f);
	int32 Granted = 0;
	for (int32 Step = 0; Step <= 600; ++Step)
	{
		Granted += Budget.Take(Step / 60.0, 5);
	}
	TestTrue(FString::Printf(TEXT("10 s of 300 puffs a second asked: at most its allowance (%d)"), Granted), Granted <= 40 * 10 + 12);
	TestTrue(FString::Printf(TEXT("...and it gives what it has (%d)"), Granted), Granted >= 380);
	FCreatureDustBudget Burst(40.f, 12.f);
	TestEqual(TEXT("A thousand at once: its burst"), Burst.Take(5.0, 1000), 12);
	TestEqual(TEXT("...and nothing more that moment"), Burst.Take(5.0, 1), 0);
	TestEqual(TEXT("A tenth of a second on: four more"), Burst.Take(5.1, 1000), 4);

	// The level's: the same cap for every creature together; and nobody near to see it, a creature throws none.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(World);
	if (!TestNotNull(TEXT("The level's reactions"), Reacts))
	{
		return false;
	}
	const int32 First = Reacts->TakeFootfalls(1000);
	TestTrue(FString::Printf(TEXT("A thousand footfalls at once: its burst at most (%d)"), First),
		First <= static_cast<int32>(UCreatureReactSubsystem::FootfallBurst));
	TestEqual(TEXT("...none more that moment"), Reacts->TakeFootfalls(50), 0);
	TestTrue(TEXT("Ash has its own"), Reacts->TakeAsh(1) == 1);
	ASpiderCreature* Spider = ReactSpawn<ASpiderCreature>(World, FVector::ZeroVector);
	if (TestNotNull(TEXT("A spider"), Spider) && TestNotNull(TEXT("...with its reactions"), Spider->GetReact()))
	{
		TestEqual(TEXT("Nobody near to see it: no dust thrown"), Spider->GetReact()->KickFootfall(FVector::ZeroVector, FVector(500.0, 0.0, 0.0), 1.f, 3), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureReactSextonTest, "Looter.Creatures.React.SextonBreathes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureReactSextonTest::RunTest(const FString& Parameters)
{
	// Mister Sexton (one posed mesh, no rig) breathes: his body swells a little about his seat and settles, over and over.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	AMisterSexton* Sexton = WorldWrapper.GetTestWorld()->SpawnActor<AMisterSexton>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Mister Sexton"), Sexton))
	{
		return false;
	}
	Sexton->DispatchBeginPlay();
	float Most = 0.f;
	float Least = 1.f;
	for (float Elapsed = 0.f; Elapsed < AMisterSexton::BreathSecondsSeated * 1.5f; Elapsed += 0.05f)
	{
		Sexton->UpdatePose(0.05f);
		const float Swell = static_cast<float>(Sexton->GetBreathScale().Z) - 1.f;
		Most = FMath::Max(Most, Swell);
		Least = FMath::Min(Least, Swell);
	}
	TestTrue(FString::Printf(TEXT("He breathes in (%.4f)"), Most), Most > AMisterSexton::BreathSwell * 0.9f && Most <= AMisterSexton::BreathSwell + 0.0001f);
	TestTrue(FString::Printf(TEXT("...and out again (%.4f)"), Least), Least < AMisterSexton::BreathSwell * 0.1f && Least >= -0.0001f);
	TestTrue(TEXT("...wider by a third as much"), Sexton->GetBreathScale().X <= 1.0 + AMisterSexton::BreathSwell / 3.f + 0.0001f);
	return true;
}

#endif
