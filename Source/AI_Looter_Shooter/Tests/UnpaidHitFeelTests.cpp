#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Unpaid's hit reactions you can feel (HITS lane C, 2026-10-28): an arm hit flings that arm, the head whips after the
// hips, a hard head hit knocks its hat off (flies, lands, lies, sinks, back at respawn, four off at most), a coal crit's kill
// snuffs it, an overkill scatters it within a breath. The plain parts (FHurtChannels, FUnpaidHatFlight) are driven without a
// world; the creature's are checked in a test world, the model's only once SK_Unpaid and SM_UnpaidHat are imported.

#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidHat.h"
#include "Tests/BossTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "UObject/Script.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float HitFrame = 1.f / 60.f;
	constexpr float SpringStep = 1.f / 120.f;

	AUnpaidCreature* SpawnHitUnpaid(UWorld* World, const FVector& Where)
	{
		AUnpaidCreature* Unpaid = World->SpawnActor<AUnpaidCreature>(Where, FRotator::ZeroRotator);
		if (Unpaid && !Unpaid->HasActorBegunPlay())
		{
			Unpaid->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Unpaid);
		return Unpaid;
	}

	/** A shot's damage as the game's bullets deal it (the bone through the point damage, then the damage), from in front of it. */
	void ShootAt(AUnpaidCreature* Unpaid, float Damage, FName Bone, bool bCritical)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		USkeletalMeshComponent* Body = Unpaid->GetMesh();
		const int32 Index = Body->GetBoneIndex(Bone);
		const FVector At = Index != INDEX_NONE ? Body->GetBoneLocation(Bone) : Unpaid->GetActorLocation();
		const UDamageType* Type = bCritical ? static_cast<const UDamageType*>(GetDefault<UWeaponCritDamageType>()) : GetDefault<UDamageType>();
		Unpaid->OnTakePointDamage.Broadcast(Unpaid, Damage, nullptr, At, Body, Bone, FVector::BackwardVector, Type, nullptr);
		Unpaid->OnTakeAnyDamage.Broadcast(Unpaid, Damage, Type, nullptr, nullptr);
	}

	UHealthComponent* HealthOf(AUnpaidCreature* Unpaid)
	{
		return Unpaid ? Unpaid->FindComponentByClass<UHealthComponent>() : nullptr;
	}

	void Run(AUnpaidCreature* Unpaid, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += HitFrame)
		{
			Unpaid->Tick(HitFrame);
		}
	}

	/** Its model and hat are imported (else the creature's checks are skipped with a warning). */
	bool HasModel(const AUnpaidCreature* Unpaid)
	{
		return Unpaid && Unpaid->GetMesh()->GetSkeletalMeshAsset() && Unpaid->GetHat() && Unpaid->GetHat()->GetStaticMesh()
			&& Unpaid->GetBonePose().Num() >= 20;
	}

	FCreatureHit FreshHit(ECreatureHitZone Zone, EHitWeight Weight = EHitWeight::Body)
	{
		FCreatureHit Hit;
		Hit.bFresh = true;
		Hit.Zone = Zone;
		Hit.Weight = Weight;
		Hit.Direction = FVector::BackwardVector;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidHitArmFlingTest, "Looter.Creatures.Hits.Unpaid.ArmFling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidHitArmFlingTest::RunTest(const FString& Parameters)
{
	// A shot on its left arm flings that arm at least twice as hard as the other, however hard (the flings' cap too).
	UnpaidBody::FHurtChannels Hurt;
	Hurt.OnHit(FreshHit(ECreatureHitZone::Arm), 1.f, /*Arm*/ 0, -1, FVector::BackwardVector, FVector::ForwardVector, false, false);
	TestTrue(FString::Printf(TEXT("The hit arm's share (%.2f) is at least twice the other's (%.2f)"), Hurt.ArmShare[0], Hurt.ArmShare[1]),
		Hurt.ArmShare[0] >= 2.f * Hurt.ArmShare[1]);
	TestTrue(TEXT("Its fingers fan out for a moment"), Hurt.SplayTime[0] >= 0.f && Hurt.SplayTime[1] < 0.f);
	for (const float Fling : { 100.f, 220.f, 440.f, 900.f })
	{
		float Flings[2];
		UnpaidBody::FHurtChannels::ArmFlings(Fling, Hurt.ArmShare, 320.f, Flings);
		TestTrue(FString::Printf(TEXT("Flung at %.0f, the hit arm (%.0f) still twice the other (%.0f) under the cap"), Fling, Flings[0], Flings[1]),
			Flings[0] >= 2.f * Flings[1] - 0.01f && Flings[0] <= 320.f);
	}
	Hurt.OnHit(FreshHit(ECreatureHitZone::Body), 1.f, -1, -1, FVector::BackwardVector, FVector::ForwardVector, false, false);
	TestTrue(TEXT("A body hit flings both alike"), FMath::IsNearlyEqual(Hurt.ArmShare[0], Hurt.ArmShare[1]));
	Hurt.OnHit(FreshHit(ECreatureHitZone::Arm), 1.f, 0, -1, FVector::BackwardVector, FVector::ForwardVector, false, /*bBoss*/ true);
	TestTrue(TEXT("A boss's arms are its own"), Hurt.ArmShare[0] == 1.f && Hurt.ArmShare[1] == 1.f);
	Hurt.OnHit(FreshHit(ECreatureHitZone::Shroud), 1.f, -1, /*Chain*/ 1, FVector::BackwardVector, FVector::ForwardVector, false, false);
	TestTrue(TEXT("Through the shroud: its strip jerks hard, the body flinches less"), Hurt.ChainJerk[1] > 1.5f && Hurt.BodyShare < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidHitWhiplashTest, "Looter.Creatures.Hits.Unpaid.Whiplash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidHitWhiplashTest::RunTest(const FString& Parameters)
{
	// A body hit from in front: the hips jolt back on the flinch's spring (UnpaidCreatureArms.cpp's StepFlinch, as it is), the
	// head holds back a moment and whips on after them, so it peaks later.
	UnpaidBody::FHurtChannels Hurt;
	const float Strength = 1.f;
	Hurt.OnHit(FreshHit(ECreatureHitZone::Body), Strength, -1, -1, FVector::BackwardVector, FVector::ForwardVector, false, false);
	float Kick = 0.6f * 1.54f * Strength;
	float Flinch = 0.f;
	float Speed = 0.f;
	float PelvisPeak = 0.f;
	float PelvisPeakAt = 0.f;
	float HeadPeak = 0.f;
	float HeadPeakAt = 0.f;
	float EarlyHead = 0.f;
	float EarlyPelvis = 0.f;
	for (float Time = SpringStep; Time <= 0.6f; Time += SpringStep)
	{
		Kick -= Kick * FMath::Min(6.f * SpringStep, 1.f);
		Speed += (40.f * 40.f * (Kick - Flinch) - 2.f * 40.f * Speed) * SpringStep;
		Flinch += Speed * SpringStep;
		Hurt.Step(SpringStep);
		const float Pelvis = -Hurt.PelvisGain * Flinch;
		// The head's pitch in the world, about: the pelvis's turn and all the turns between, each on its parent's.
		const float Head = Pelvis + Hurt.ChestPitch(Pelvis) + Hurt.NeckPitch() + Hurt.HeadPitch(Pelvis);
		if (FMath::Abs(Pelvis) > PelvisPeak)
		{
			PelvisPeak = FMath::Abs(Pelvis);
			PelvisPeakAt = Time;
		}
		if (FMath::Abs(Head) > HeadPeak)
		{
			HeadPeak = FMath::Abs(Head);
			HeadPeakAt = Time;
		}
		if (Time <= 0.03f)
		{
			EarlyHead = FMath::Max(EarlyHead, FMath::Abs(Head));
			EarlyPelvis = FMath::Max(EarlyPelvis, FMath::Abs(Pelvis));
		}
	}
	TestTrue(FString::Printf(TEXT("The hips jolt (%.1f degrees at the peak, 11 per unit)"), PelvisPeak), PelvisPeak > 4.f);
	TestTrue(FString::Printf(TEXT("The head peaks after the hips (%.0f ms vs %.0f ms)"), HeadPeakAt * 1000.f, PelvisPeakAt * 1000.f),
		HeadPeakAt > PelvisPeakAt + 0.01f);
	TestTrue(FString::Printf(TEXT("At first the head lags (%.1f vs the hips' %.1f degrees)"), EarlyHead, EarlyPelvis), EarlyHead < 0.6f * EarlyPelvis + 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidHitHatTest, "Looter.Creatures.Hits.Unpaid.Hat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidHitHatTest::RunTest(const FString& Parameters)
{
	// The flight over a flat floor: off along the shot, one bounce, down, lying 6 s, sunk and gone by 7.
	auto Floor = [](const FVector& From, const FVector& To, FVector& OutPoint, FVector& OutNormal)
	{
		if (From.Z >= 0.0 && To.Z < 0.0)
		{
			const double Along = From.Z / (From.Z - To.Z);
			OutPoint = FMath::Lerp(From, To, Along);
			OutNormal = FVector::UpVector;
			return true;
		}
		return false;
	};
	FRandomStream Dice(7);
	FUnpaidHatFlight Flight;
	Flight.Launch(FVector(0.0, 0.0, 170.0), FQuat::Identity, FVector::ForwardVector, 5.f, Dice);
	float Flown = 0.f;
	while (Flight.Stage == FUnpaidHatFlight::EStage::Flying && Flown < 3.f)
	{
		Flight.Step(HitFrame, Floor);
		Flown += HitFrame;
	}
	TestTrue(TEXT("It comes down and lies"), Flight.Stage == FUnpaidHatFlight::EStage::Lying);
	TestEqual(TEXT("After one bounce"), Flight.Bounces, 1);
	TestTrue(FString::Printf(TEXT("Thrown along the shot (%.0f cm)"), Flight.Location.X), Flight.Location.X > 150.0);
	TestTrue(FString::Printf(TEXT("Its brim on the floor (pivot %.1f cm up)"), Flight.Location.Z), FMath::IsNearlyEqual(static_cast<float>(Flight.Location.Z), 5.f, 0.5f));
	float Down = 0.f;
	while (Down < 6.5f)
	{
		Flight.Step(HitFrame, Floor);
		Down += HitFrame;
	}
	TestTrue(TEXT("Still to be seen 6.5 s after it came down"), Flight.IsShown());
	while (Down < 7.1f)
	{
		Flight.Step(HitFrame, Floor);
		Down += HitFrame;
	}
	TestTrue(TEXT("Sunk and gone by 7"), !Flight.IsShown() && Flight.IsOff());

	// A wall in the way stops it there.
	auto Wall = [&Floor](const FVector& From, const FVector& To, FVector& OutPoint, FVector& OutNormal)
	{
		if (From.X < 80.0 && To.X >= 80.0)
		{
			OutPoint = FMath::Lerp(From, To, (80.0 - From.X) / (To.X - From.X));
			OutNormal = FVector::BackwardVector;
			return true;
		}
		return Floor(From, To, OutPoint, OutNormal);
	};
	Flight.Launch(FVector(0.0, 0.0, 170.0), FQuat::Identity, FVector::ForwardVector, 5.f, Dice);
	for (int32 Frame = 0; Frame < 180; ++Frame)
	{
		Flight.Step(HitFrame, Wall);
	}
	TestTrue(FString::Printf(TEXT("A wall stops it (%.0f cm, the wall at 80)"), Flight.Location.X), Flight.Location.X < 80.0);

	// On a real Unpaid: a hard head hit knocks it off, it's back on at its respawn, and no more than four are off at once.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AUnpaidCreature* Unpaid = SpawnHitUnpaid(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	if (!HasModel(Unpaid))
	{
		AddWarning(TEXT("SK_Unpaid or SM_UnpaidHat isn't imported: the hat on a real Unpaid isn't checked."));
		return true;
	}
	UStaticMeshComponent* Hat = Unpaid->GetHat();
	const FVector OnHead = Hat->GetComponentLocation();
	ShootAt(Unpaid, 0.3f * HealthOf(Unpaid)->GetMaxHealth(), Unpaid->Rig.Head, false);
	TestTrue(TEXT("A hard head hit knocks its hat off"), Hat->GetAttachParent() == nullptr && Hat->IsVisible());
	Run(Unpaid, 0.5f);
	TestTrue(FString::Printf(TEXT("It flies off (%.0f cm from the head)"), FVector::Dist(Hat->GetComponentLocation(), OnHead)),
		FVector::Dist(Hat->GetComponentLocation(), OnHead) > 50.0);
	Unpaid->CorpseTime = 0.f;
	Unpaid->RespawnDelay = 0.f;
	BossTestWorld::Kill(Unpaid);
	Run(Unpaid, 0.2f);
	World->GetTimerManager().Tick(2.6f);
	TestFalse(TEXT("It came back"), Unpaid->IsDead());
	TestTrue(TEXT("With its hat on its head"), Hat->GetAttachParent() == Unpaid->GetMesh() && Hat->IsVisible());
	TestEqual(TEXT("No hat off in the world now"), UnpaidHat::NumOff(World), 0);

	int32 Knocked = 0;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		AUnpaidCreature* Other = SpawnHitUnpaid(World, FVector(0.0, 400.0 * (Index + 1), 0.0));
		if (Other)
		{
			ShootAt(Other, 0.3f * HealthOf(Other)->GetMaxHealth(), Other->Rig.Head, false);
			Knocked += Other->GetHat()->GetAttachParent() == nullptr ? 1 : 0;
		}
	}
	TestEqual(TEXT("Six hats knocked off"), Knocked, 6);
	TestEqual(TEXT("Four of them fly, the rest go in a puff of dust"), UnpaidHat::NumOff(World), UnpaidHat::MaxOff);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidHitSnuffedTest, "Looter.Creatures.Hits.Unpaid.Snuffed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidHitSnuffedTest::RunTest(const FString& Parameters)
{
	using UnpaidBody::EKillStyle;
	// Which death: an overkill scatters it, a coal crit's kill snuffs it, a blast tears it, anything else is today's.
	UnpaidBody::FHurtChannels Hurt;
	FCreatureHit Hit = FreshHit(ECreatureHitZone::Core, EHitWeight::Kill);
	Hit.bCritical = true;
	Hit.Kind = EWeaponKind::Rifle;
	TestTrue(TEXT("A coal crit's kill snuffs it"), Hurt.OnKilled(Hit, FVector::BackwardVector, false) == EKillStyle::Snuffed);
	TestTrue(FString::Printf(TEXT("Its coal swells (%.2f) and is out within 0.12 s (%.2f)"), Hurt.CoalDeathScale(0.f), Hurt.CoalDeathScale(0.12f)),
		Hurt.CoalDeathScale(0.f) >= 1.4f && Hurt.CoalDeathScale(0.12f) < 0.05f);
	TestTrue(TEXT("Stiff a moment, then down twice as quick"), Hurt.IsStiff(0.05f) && !Hurt.IsStiff(0.15f) && Hurt.CrumpleRate() == 2.f);
	Hit.Weight = EHitWeight::Overkill;
	TestTrue(TEXT("An overkill scatters it"), Hurt.OnKilled(Hit, FVector::BackwardVector, false) == EKillStyle::Scattered);
	Hit = FreshHit(ECreatureHitZone::Body, EHitWeight::Kill);
	Hit.Kind = EWeaponKind::Shotgun;
	TestTrue(TEXT("A shotgun's kill tears it"), Hurt.OnKilled(Hit, FVector::BackwardVector, false) == EKillStyle::Torn);
	TestTrue(TEXT("Thrown back along the shot"), Hurt.KillTilt(0.2f).X < -30.f && Hurt.KillShift(0.2f).X < -35.0);
	Hit.Kind = EWeaponKind::Rifle;
	TestTrue(TEXT("A rifle's body kill is today's"), Hurt.OnKilled(Hit, FVector::BackwardVector, false) == EKillStyle::Body);
	Hit.Weight = EHitWeight::Overkill;
	TestTrue(TEXT("A boss's death is its own"), Hurt.OnKilled(Hit, FVector::BackwardVector, true) == EKillStyle::Body);

	// On a real Unpaid: a crit on its coal that kills it puts the coal out at once.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AUnpaidCreature* Unpaid = SpawnHitUnpaid(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	if (!HasModel(Unpaid) || Unpaid->Rig.Coal.IsNone())
	{
		AddWarning(TEXT("SK_Unpaid isn't imported: a real snuffed coal isn't checked."));
		return true;
	}
	// Worn down in small hits first (a kill from full health would be an overkill: a Big kill).
	UHealthComponent* Health = HealthOf(Unpaid);
	while (Health->GetHealth() > 0.25f * Health->GetMaxHealth())
	{
		BossTestWorld::Hurt(Unpaid, 0.2f * Health->GetMaxHealth());
	}
	ShootAt(Unpaid, Health->GetHealth() + 1.f, Unpaid->Rig.Coal, true);
	if (!TestTrue(TEXT("It's dead"), Unpaid->IsDead()))
	{
		return false;
	}
	Run(Unpaid, 0.15f);
	float CoalScale = 1.f;
	for (const FCreatureBonePose& Bone : Unpaid->GetBonePose())
	{
		if (Bone.Bone == Unpaid->Rig.Coal)
		{
			CoalScale = static_cast<float>(Bone.Transform.GetScale3D().X);
		}
	}
	TestTrue(FString::Printf(TEXT("Its coal is out (scale %.2f)"), CoalScale), CoalScale < 0.2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidHitScatteredTest, "Looter.Creatures.Hits.Unpaid.Scattered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidHitScatteredTest::RunTest(const FString& Parameters)
{
	// Overkilled, it's gone through its phase fade within 0.35 s; killed plainly, it's still there then (its fade starts at 0.9).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AUnpaidCreature* Overkilled = SpawnHitUnpaid(World, FVector::ZeroVector);
	AUnpaidCreature* Plain = SpawnHitUnpaid(World, FVector(0.0, 600.0, 0.0));
	if (!TestNotNull(TEXT("Unpaid spawned"), Overkilled) || !TestNotNull(TEXT("Another"), Plain))
	{
		return false;
	}
	BossTestWorld::Kill(Overkilled);
	UHealthComponent* Health = HealthOf(Plain);
	while (Health->GetHealth() > 0.25f * Health->GetMaxHealth())
	{
		BossTestWorld::Hurt(Plain, 0.2f * Health->GetMaxHealth());
	}
	BossTestWorld::Hurt(Plain, Health->GetHealth());
	if (!TestTrue(TEXT("Both dead"), Overkilled->IsDead() && Plain->IsDead()))
	{
		return false;
	}
	Run(Overkilled, 0.35f);
	Run(Plain, 0.35f);
	TestTrue(FString::Printf(TEXT("Overkilled, it's faded out (phase %.2f)"), Overkilled->GetPhase()), Overkilled->GetPhase() >= 0.99f);
	TestTrue(FString::Printf(TEXT("Killed plainly, it's still there (phase %.2f)"), Plain->GetPhase()), Plain->GetPhase() <= 0.01f);
	return true;
}

#endif
