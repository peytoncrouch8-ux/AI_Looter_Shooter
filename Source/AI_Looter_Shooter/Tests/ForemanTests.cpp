#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Foreman (AForemanCreature, ForemanFight) and his stope's things (fuse sticks, rock falls, timber props): his rules
// alone, his numbers, then his fight in test levels, moved on a frame at a time (a test level never ticks anything itself).

#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/ForemanFight.h"
#include "Creatures/ForemanFuseStick.h"
#include "Creatures/ForemanRockFall.h"
#include "Creatures/ForemanTimberProp.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "AnimationRuntime.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "ReferenceSkeleton.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float FightStep = 0.05f;

	/** The Foreman as his lair spawns him (Legendary), begun, dropping no loot. */
	AForemanCreature* SpawnForeman(UWorld* World, const FVector& Feet)
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

	/** A timber prop standing at Foot, begun. */
	AForemanTimberProp* SpawnProp(UWorld* World, const FVector& Foot)
	{
		AForemanTimberProp* Prop = World->SpawnActor<AForemanTimberProp>(Foot, FRotator::ZeroRotator);
		if (Prop && !Prop->HasActorBegunPlay())
		{
			Prop->DispatchBeginPlay();
		}
		return Prop;
	}

	/** A stope lantern: an actor tagged StopeLantern with a light. */
	UPointLightComponent* SpawnLantern(UWorld* World, const FVector& Where)
	{
		AActor* Lantern = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Where));
		if (!Lantern)
		{
			return nullptr;
		}
		Lantern->Tags.Add(Foreman::LanternTag);
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Lantern, TEXT("LanternLight"));
		Lantern->SetRootComponent(Light);
		Light->RegisterComponent();
		Light->SetWorldLocation(Where);
		Light->SetVisibility(true);
		return Light;
	}

	/** Moves him (and his boss) on a frame at a time until Done holds (at most Limit seconds); the seconds it took, or negative. */
	float TickUntil(AForemanCreature* Teague, float Limit, TFunctionRef<bool()> Done)
	{
		for (float Time = 0.f; Teague && Time <= Limit; Time += FightStep)
		{
			if (Done())
			{
				return Time;
			}
			Teague->ConsumeMovementInputVector();
			Teague->Tick(FightStep);
			Teague->GetBoss()->TickFight(FightStep);
		}
		return -1.f;
	}

	void TickFor(AForemanCreature* Teague, float Seconds)
	{
		TickUntil(Teague, Seconds, []() { return false; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForemanRulesTest, "Looter.Creatures.Foreman.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FForemanRulesTest::RunTest(const FString& Parameters)
{
	// Four phases: the shift at 66%, the dark at 50%, the shift again at 33%.
	const TArray<FBossPhase> Phases = ForemanFight::MakePhases();
	if (!TestEqual(TEXT("Four phases"), Phases.Num(), 4))
	{
		return false;
	}
	TestTrue(TEXT("...at full, 66%, 50% and 33%"), Phases[0].HealthShare == 1.f && FMath::IsNearlyEqual(Phases[1].HealthShare, 0.66f)
		&& FMath::IsNearlyEqual(Phases[2].HealthShare, 0.5f) && FMath::IsNearlyEqual(Phases[3].HealthShare, 0.33f));
	TestEqual(TEXT("...named on his bar"), Phases[2].Name.ToString(), FString(TEXT("Lights Out")));

	// His pace: the roof from the shift on, quicker in everything in the last shift, his swing's tell still readable.
	const ForemanFight::FPace First = ForemanFight::PaceFor(0);
	const ForemanFight::FPace Shift = ForemanFight::PaceFor(Foreman::ShiftPhase);
	const ForemanFight::FPace Last = ForemanFight::PaceFor(Foreman::LastShiftPhase);
	TestTrue(TEXT("No roof in the first phase; from the shift on, every 18 s or so"), !First.bRoof && Shift.bRoof && FMath::IsNearlyEqual(Shift.RoofEvery, 18.f));
	TestTrue(TEXT("The last shift: quicker fuses, roof and chase"), Last.FuseCooldown < First.FuseCooldown && Last.RoofEvery < Shift.RoofEvery
		&& Last.ChaseScale > 1.f);
	TestTrue(TEXT("...his swing's wind-up still over half a second"), GetDefault<AForemanCreature>()->AttackWindup * Last.SwingWindupScale >= 0.5f);

	// His coal: open lighting a fuse, for 3 s after a roof strike, staggered; covered otherwise.
	TestFalse(TEXT("Covered at rest"), ForemanFight::IsCoalOpen(false, -1.f, 3.f, false));
	TestTrue(TEXT("Open lighting a fuse"), ForemanFight::IsCoalOpen(true, -1.f, 3.f, false));
	TestTrue(TEXT("Open 2 s after a roof strike"), ForemanFight::IsCoalOpen(false, 2.f, 3.f, false));
	TestFalse(TEXT("...covered again 3.5 s after"), ForemanFight::IsCoalOpen(false, 3.5f, 3.f, false));
	TestTrue(TEXT("Open staggered"), ForemanFight::IsCoalOpen(false, -1.f, 3.f, true));

	// A fuse stick's burst: full in its heart, a fifth at its 3 m edge, nothing past it.
	TestNearlyEqual(TEXT("Full at its heart"), ForemanFight::BurstFalloff(50.f, 300.f, 100.f, 0.2f), 1.f, 0.001f);
	TestNearlyEqual(TEXT("A fifth at its edge"), ForemanFight::BurstFalloff(300.f, 300.f, 100.f, 0.2f), 0.2f, 0.001f);
	TestNearlyEqual(TEXT("Nothing past it"), ForemanFight::BurstFalloff(320.f, 300.f, 100.f, 0.2f), 0.f, 0.001f);

	// The roof's spots: on the prop he struck, the player's spot, their cover, and round them; apart, inside the stope.
	FRandomStream Random(20261013);
	const FVector Player(600.0, 0.0, 0.0);
	const FVector Struck(100.0, 0.0, 0.0);
	const FVector Cover(600.0, 300.0, 0.0);
	const TArray<FVector> Spots = ForemanFight::RoofSpots(Player, Struck, Cover, FVector::ZeroVector, 1500.f, 4, 260.f, 340.f, Random);
	TestEqual(TEXT("Four spots"), Spots.Num(), 4);
	TestTrue(TEXT("...the struck prop's, the player's and the cover's among them"), Spots.Num() >= 3
		&& FVector::Dist2D(Spots[0], Struck) < 1.0 && FVector::Dist2D(Spots[1], Player) < 1.0 && FVector::Dist2D(Spots[2], Cover) < 1.0);
	bool bApart = true;
	bool bInside = true;
	for (int32 First1 = 0; First1 < Spots.Num(); ++First1)
	{
		bInside &= FVector::Dist2D(Spots[First1], FVector::ZeroVector) <= 1500.0;
		for (int32 Second = First1 + 1; Second < Spots.Num(); ++Second)
		{
			bApart &= FVector::Dist2D(Spots[First1], Spots[Second]) >= 260.0;
		}
	}
	TestTrue(TEXT("...at least 2.6 m apart"), bApart);
	TestTrue(TEXT("...all inside the stope"), bInside);
	const TArray<FVector> Alone = ForemanFight::RoofSpots(Player, TOptional<FVector>(), TOptional<FVector>(), FVector::ZeroVector, 1500.f, 4, 260.f, 340.f, Random);
	TestTrue(TEXT("No props: the player's spot first, and three round them"), Alone.Num() == 4 && FVector::Dist2D(Alone[0], Player) < 1.0);

	// His lines.
	TestFalse(TEXT("His last word"), ForemanFight::LastWord().Text.IsEmpty());
	TestEqual(TEXT("...his"), ForemanFight::LastWord().Speaker.ToString(), FString(TEXT("Cyrus Teague")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForemanStatsTest, "Looter.Creatures.Foreman.Stats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FForemanStatsTest::RunTest(const FString& Parameters)
{
	const AForemanCreature* Defaults = GetDefault<AForemanCreature>();
	TestTrue(TEXT("A Legendary monster, his name his rank's word"), Defaults->StartingRank == ECreatureRank::Legendary && Defaults->bNameIsRankWord);
	TestNearlyEqual(TEXT("1.5 times an Unpaid as seen"), Defaults->BodyScale * UCreatureRankSettings::Get(ECreatureRank::Legendary).Size, 1.5f, 0.01f);
	const UHealthComponent* Health = Defaults->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("He has health"), Health))
	{
		// At his set level 7 (5 and Soulfed's 2): about 7,600 (380 health since 2026-10-30, was 190 and about 3,800; growth
		// compounds +9% a level: x1.68 at 7).
		const float AtSeven = Health->MaxHealth * UPlayerProgressionSubsystem::GetLevelRules().EnemyScale(7)
			* UCreatureRankSettings::Get(ECreatureRank::Legendary).HealthMultiplier;
		TestTrue(FString::Printf(TEXT("About 7,600 health at level 7 (%.0f)"), AtSeven), AtSeven >= 7200.f && AtSeven <= 8000.f);
	}
	const UBossComponent* Boss = Defaults->GetBoss();
	if (!TestNotNull(TEXT("His boss"), Boss))
	{
		return false;
	}
	TestTrue(TEXT("He waits digging at the fall until a player comes into his stope (on his level) or shoots him"), Boss->bWaitsPassive
		&& Boss->bStartWhenHurt && Boss->EngageRadius == 0.f && Defaults->EngageRadius > 0.f && Defaults->EngageRise > 0.f);
	TestTrue(TEXT("...stands down when the player leaves, no fog ring through the rock"), Boss->StandDownSeconds > 0.f && Boss->SealRadius == 0.f
		&& Boss->LeashRadius == 0.f);
	TestTrue(TEXT("...his shift crumbles with him; the lair, not his boss, records his death"), Boss->bAddsDieWithBoss && Boss->BossId.IsNone());
	TestTrue(TEXT("...his coal staggers him; his loot showers"), Boss->Stagger.CritShare > 0.f && Boss->LootShower.bEnabled);
	TestEqual(TEXT("...four phases"), Boss->Phases.Num(), 4);
	TestEqual(TEXT("...titled"), Boss->Show.Title.ToString(), FString(TEXT("Foreman of the Providence")));

	// His things where his model has them at rest (Foreman_parts.json): the pick handle on the pick bone or in his right
	// fist, the satchel modelled in place at the chest bone's head.
	const USkeletalMesh* Model = Defaults->GetMesh()->GetSkeletalMeshAsset();
	if (!TestNotNull(TEXT("His body (SK_Foreman, else the dead miners' or the Unpaid's)"), Model))
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	const auto AtRest = [&Skeleton](const USceneComponent* Part)
	{
		const int32 Bone = Skeleton.FindBoneIndex(Part->GetAttachSocketName());
		return Bone == INDEX_NONE ? FTransform::Identity
			: Part->GetRelativeTransform() * FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Bone);
	};
	const auto BoneHead = [&Skeleton](FName Bone)
	{
		const int32 Index = Skeleton.FindBoneIndex(Bone);
		return Index == INDEX_NONE ? FVector::ZeroVector : FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index).GetLocation();
	};
	const FQuat PickTurn = FQuat(0.04972, 0.49584, -0.08651, 0.86266).GetNormalized();
	const bool bPickBone = Skeleton.FindBoneIndex(ADeadMinerCreature::PickBone) != INDEX_NONE;
	const FVector PickAt = bPickBone ? BoneHead(ADeadMinerCreature::PickBone) : BoneHead(Defaults->Rig.RightArm.Hand) + FVector(4.16, -3.11, -8.06);
	const FTransform Handle = AtRest(Defaults->GetPick());
	TestTrue(*FString::Printf(TEXT("His pick handle in his fist as modelled (%s)"), *Handle.GetLocation().ToCompactString()),
		Defaults->GetPick()->GetAttachSocketName() == (bPickBone ? ADeadMinerCreature::PickBone : Defaults->Rig.RightArm.Hand)
		&& Handle.GetLocation().Equals(PickAt, 0.1) && Handle.GetRotation().AngularDistance(PickTurn) < 0.01);
	if (bPickBone)
	{
		TestTrue(TEXT("...his model's pick bone where the parts table has the handle off his right hand"),
			BoneHead(ADeadMinerCreature::PickBone).Equals(BoneHead(Defaults->Rig.RightArm.Hand) + FVector(4.16, -3.11, -8.06), 0.5));
	}
	const FTransform Bag = AtRest(Defaults->Satchel.Get());
	TestTrue(TEXT("His satchel modelled in place: its pivot at the chest bone's head, unturned"),
		Bag.GetLocation().Equals(BoneHead(Defaults->Rig.Chest), 0.1) && Bag.GetRotation().AngularDistance(FQuat::Identity) < 0.01);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForemanFightTest, "Looter.Creatures.Foreman.Fight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FForemanFightTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(800.0, 0.0, 90.0));
	AForemanCreature* Teague = SpawnForeman(World, FVector::ZeroVector);
	UPointLightComponent* Lantern = SpawnLantern(World, FVector(300.0, 300.0, 400.0));
	UBossComponent* Boss = Teague ? Teague->GetBoss() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Foreman and his boss"), Boss) || !TestNotNull(TEXT("A lantern"), Lantern))
	{
		return false;
	}
	TestTrue(TEXT("Before his fight he waits, hunting nobody"), Teague->IsPassive() && !Boss->IsFighting());
	TestFalse(TEXT("...his coal covered"), Teague->IsCoalOpen());

	// His entrance: he turns, the pick handle raised and slammed down.
	Boss->StartFight(Player);
	TestTrue(TEXT("His fight starts; his entrance is next"), Boss->IsFighting() && Teague->IsIntroWanted());
	TickFor(Teague, FightStep);
	TestTrue(TEXT("...the pick handle up"), Teague->GetCreatureState() == ECreatureState::Attack && Teague->GetMove() == EForemanMove::Intro);
	TickFor(Teague, Teague->IntroWindup + Teague->IntroRecovery + 0.1f);
	TestFalse(TEXT("...then he comes on"), Teague->GetMove() == EForemanMove::Intro && Teague->GetCreatureState() == ECreatureState::Attack);

	// A fuse at a player 8 m off: lit in his fist (his coal open), then lobbed; it lands, burns and bursts.
	Teague->ReadyFuse();
	const float LitAt = TickUntil(Teague, 3.f, [Teague]() { return Teague->IsLightingFuse(); });
	TestTrue(FString::Printf(TEXT("8 m off, he lights a fuse (%.2f s)"), LitAt), LitAt >= 0.f && Teague->GetHeldStick() != nullptr);
	TestTrue(TEXT("...his coal open while it hisses"), Teague->IsCoalOpen());
	AForemanFuseStick* Stick = Teague->GetHeldStick();
	if (Stick && Stick->GetRootComponent())
	{
		// In his left fist's grip as SK_Foreman holds it (Foreman_parts.json): off hand_l's head, the fuse up the fist's tunnel.
		const FTransform Grip = ADeadMinerCreature::PartOnBone(Teague->GetMesh()->GetSkeletalMeshAsset(), Teague->Rig.LeftArm.Hand,
			FVector(5.16, 3.16, -7.43), FQuat(-0.07278, 0.42818, 0.15094, 0.88802).GetNormalized());
		const USceneComponent* Held = Stick->GetRootComponent();
		TestTrue(TEXT("...in his left fist's grip, as his model holds it"), Held->GetAttachSocketName() == Teague->Rig.LeftArm.Hand
			&& Held->GetRelativeLocation().Equals(Grip.GetLocation(), 0.1) && Held->GetRelativeTransform().GetRotation().AngularDistance(Grip.GetRotation()) < 0.01);
	}
	TickFor(Teague, Teague->FuseLightSeconds + 0.1f);
	TestEqual(TEXT("...then he lobs it"), Teague->GetFusesThrown(), 1);
	TestFalse(TEXT("...his coal covered again"), Teague->IsCoalOpen());
	if (TestNotNull(TEXT("The stick"), Stick))
	{
		for (float Time = 0.f; Time < 4.f && !Stick->HasBurst(); Time += FightStep)
		{
			Stick->Advance(FightStep);
		}
		TestTrue(TEXT("It came down and burst"), Stick->HasBurst());
		TestTrue(FString::Printf(TEXT("...on the player's spot (%.0f cm off)"), FVector::Dist2D(Stick->GetLanding(), Player->GetActorLocation())),
			FVector::Dist2D(Stick->GetLanding(), Player->GetActorLocation()) < 150.0);
		TestTrue(TEXT("...catching the player"), Stick->HitPlayer());
	}

	// At 66%: the shift rises out of the rubble, his adds.
	BossTestWorld::HurtTo(*Boss, Foreman::ShiftShare);
	TestEqual(TEXT("At 66%: the shift"), Boss->GetPhase(), Foreman::ShiftPhase);
	TestEqual(TEXT("...three dead miners, his boss's adds"), Boss->NumAliveAdds(), Foreman::ShiftSize);
	TestEqual(TEXT("...his own"), Teague->GetShift().Num(), Foreman::ShiftSize);
	TestTrue(TEXT("...and the roof from now on"), Teague->GetPace().bRoof);

	// The roof: a prop by him struck, a prop beside the player their cover; rock falls on four marked spots and splinters them.
	// Off his line to the player, so it hides nobody from him.
	AForemanTimberProp* ByHim = SpawnProp(World, FVector(150.0, 150.0, 0.0));
	AForemanTimberProp* ByPlayer = SpawnProp(World, Player->GetActorLocation() - FVector(0.0, 0.0, 90.0) + FVector(0.0, 300.0, 0.0));
	if (!TestNotNull(TEXT("Two props"), ByHim) || !ByPlayer)
	{
		return false;
	}
	BossTestWorld::KillAdds(*Boss);
	Teague->ReadyRoof();
	const float StruckAt = TickUntil(Teague, 6.f, [Teague]() { return Teague->GetRoofStrikes() > 0; });
	TestTrue(FString::Printf(TEXT("He strikes the roof (%.2f s)"), StruckAt), StruckAt >= 0.f);
	TestTrue(TEXT("...at the prop by him"), Teague->GetStruckProp() == ByHim);
	TestTrue(TEXT("...his coal open after"), Teague->IsCoalOpen());
	const TArray<AForemanRockFall*> Falls = Teague->GetRockFalls();
	TestEqual(TEXT("...four marked spots"), Falls.Num(), Teague->RoofSpots);
	TestTrue(TEXT("...nothing falls during the dust"), Falls.Num() > 0 && !Falls[0]->HasFallen() && ByHim->IsStanding() && ByPlayer->IsStanding());
	for (float Time = 0.f; Time < Teague->RoofDustSeconds + 0.5f; Time += FightStep)
	{
		for (AForemanRockFall* Fall : Falls)
		{
			Fall->Advance(FightStep);
		}
	}
	bool bAllFallen = true;
	bool bHitPlayer = false;
	for (AForemanRockFall* Fall : Falls)
	{
		bAllFallen &= Fall->HasFallen();
		bHitPlayer |= Fall->HitPlayer();
	}
	TestTrue(TEXT("Then the rock falls on every spot"), bAllFallen);
	TestTrue(TEXT("...on a player who stood still"), bHitPlayer);
	TestTrue(TEXT("...both props splintered: the cover is gone"), !ByHim->IsStanding() && !ByPlayer->IsStanding());

	// At 50%: the dark. The lantern goes out for 12 s, then catches again.
	BossTestWorld::HurtTo(*Boss, Foreman::DarkShare);
	TestEqual(TEXT("At 50%: lights out"), Boss->GetPhase(), Foreman::DarkPhase);
	TestTrue(TEXT("...the stope dark"), Teague->IsDark() && Teague->NumDarkenedLights() == 1 && !Lantern->IsVisible());
	TickFor(Teague, Teague->DarkSeconds + 1.f);
	TestFalse(TEXT("After 12 s it's over"), Teague->IsDark());
	TestTrue(TEXT("...the lantern lit again"), Lantern->IsVisible());

	// At 33%: the shift again.
	BossTestWorld::KillAdds(*Boss);
	BossTestWorld::HurtTo(*Boss, Foreman::LastShiftShare);
	TestEqual(TEXT("At 33%: the last shift"), Boss->GetPhase(), Foreman::LastShiftPhase);
	TestEqual(TEXT("...the shift called twice"), Teague->GetShiftsCalled(), Foreman::NumShifts);
	TestTrue(TEXT("...quicker now"), Teague->GetPace().ChaseScale > 1.f);

	// His death wins it: his shift crumbles with him, he kneels and fades.
	BossTestWorld::Kill(Teague);
	TestTrue(TEXT("Killed: his fight is won"), Boss->IsWon() && !Boss->IsFighting());
	TestEqual(TEXT("...his shift gone with him"), Boss->NumAliveAdds(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForemanResetTest, "Looter.Creatures.Foreman.Reset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FForemanResetTest::RunTest(const FString& Parameters)
{
	// The player falls mid-fight: he's home and healed, the stope's lanterns lit, his props whole, no fuse left burning.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(800.0, 0.0, 90.0));
	AForemanCreature* Teague = SpawnForeman(World, FVector::ZeroVector);
	UPointLightComponent* Lantern = SpawnLantern(World, FVector(300.0, 300.0, 400.0));
	AForemanTimberProp* Prop = SpawnProp(World, FVector(300.0, -200.0, 0.0));
	UBossComponent* Boss = Teague ? Teague->GetBoss() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Foreman"), Boss) || !Lantern || !Prop)
	{
		return false;
	}
	Boss->StartFight(Player);
	BossTestWorld::HurtTo(*Boss, Foreman::DarkShare);
	Prop->Splinter(FVector::ZeroVector);
	TestTrue(TEXT("Mid-fight: dark, a prop down"), Teague->IsDark() && !Lantern->IsVisible() && !Prop->IsStanding());
	Boss->ResetFight();
	TestFalse(TEXT("Started over: not fighting"), Boss->IsFighting());
	TestTrue(TEXT("...healed"), FMath::IsNearlyEqual(Boss->GetBossHealth()->GetHealth(), Boss->GetBossHealth()->GetMaxHealth()));
	TestTrue(TEXT("...the lantern lit at once"), !Teague->IsDark() && Lantern->IsVisible());
	TestTrue(TEXT("...his props whole"), Prop->IsStanding());
	TestTrue(TEXT("...waiting at the fall again"), Teague->IsPassive());
	TestTrue(TEXT("...no fuse in his fist"), Teague->GetHeldStick() == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForemanEngageTest, "Looter.Creatures.Foreman.Engage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FForemanEngageTest::RunTest(const FString& Parameters)
{
	// A player on the bench over the mine doesn't start his fight; one come into his stope does.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(800.0, 0.0, 1500.0));
	AForemanCreature* Teague = SpawnForeman(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Foreman"), Teague))
	{
		return false;
	}
	TickFor(Teague, 1.f);
	TestFalse(TEXT("8 m off but 15 m over him: no fight"), Teague->GetBoss()->IsFighting());
	Player->SetActorLocation(FVector(800.0, 0.0, 90.0));
	TickFor(Teague, 0.5f);
	TestTrue(TEXT("Come into his stope: his fight starts"), Teague->GetBoss()->IsFighting());
	return true;
}

#endif
