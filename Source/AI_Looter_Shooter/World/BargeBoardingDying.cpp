// ABargeBoarding: her end. The cage's burst takes her pilot light: every light out, a list to her berth side, a drift
// west on the dusk wind, her back half burning and the third stack down across the port lane (step 7). Then BargeFall
// carries her over the Rest raining soul coal and down into Gravewind Canyon, and she's gone (Pilot.BargeFell).

#include "World/BargeBoarding.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "Player/ViewKick.h"
#include "World/BargeFurnace.h"
#include "World/BargeHoseDrum.h"
#include "World/BargeRules.h"
#include "World/LamplighterEffects.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Her back half's fires (the barge's frame): where the furnace's breach spilled onto the after deck. */
	const FVector SternFires[] = { FVector(-3100.0, -500.0, 0.0), FVector(-2900.0, 600.0, 0.0), FVector(-2300.0, -850.0, 0.0),
		FVector(-2000.0, 300.0, 0.0), FVector(-3300.0, 100.0, 0.0), FVector(-1600.0, 900.0, 0.0) };
	/** Smoke and the odd burst off them this often while she dies; her timbers' groan this often. */
	constexpr float FireEvery = 0.2f;
	constexpr float GroanEvery = 5.f;
	/** The falling stack hurts what it lands on: within this of its line, this share of their health. */
	constexpr float StackHitReach = 230.f;
	constexpr float StackHitShare = 0.35f;
	/** Where the soul coal falls from under her (the barge's frame: a box under her keel). */
	const FVector CoalFrom(0.0, 0.0, -50.0);
	const FVector CoalSpread(3000.0, 900.0, 0.0);
	/** The coal's fall (cm/s/s): heavy lumps, no drift of their own but hers. */
	constexpr double CoalGravity = 980.0;
}

void ABargeBoarding::BeginDying()
{
	if (State == EBargeState::Dying || State == EBargeState::Falling || State == EBargeState::Fallen || State == EBargeState::Away)
	{
		return;
	}
	SetState(EBargeState::Dying);
	DyingClock = 0.f;
	FireWait = 0.f;
	bStackDown = false;
	bStackHurtDone = false;
	// Her pilot light is gone: every light out, the furnace's breath with it, the drum loose.
	if (Furnace)
	{
		Furnace->SetCageBurst(true);
		Furnace->PutOut();
	}
	for (UPointLightComponent* Lamp : DeckLamps)
	{
		if (Lamp)
		{
			Lamp->SetVisibility(false);
		}
	}
	if (HoseDrum)
	{
		HoseDrum->SetTurning(false);
	}
	LooterSound::Stop(FurnaceDrone.Get(), 3.f);
	FurnaceDrone.Reset();
	// Her back half alight where the breach spilled the furnace's coal.
	TArray<FTransform> Fires;
	for (const FVector& Spot : SternFires)
	{
		Fires.Add(FTransform(FQuat::Identity, Spot + FVector(0.0, 0.0, BargeRules::DeckTop + 160.0), FVector(2.2, 2.2, 3.2)));
	}
	SternFire->ClearInstances();
	SternFire->AddInstances(Fires, false);
	// The death of a ship: her iron groaning, a lurch under the feet.
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::RoofGroan, GetDeckMiddle(), 1.4f, 0.6f);
	LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, GetDeckMiddle(), 1.3f, 0.7f);
	LamplighterEffects::Shake(this, 0.6f, 1.2f);
	FViewKick Lurch;
	Lurch.Roll = BargeRules::BerthSide * 3.f;
	Lurch.Pitch = -1.f;
	Lurch.Frequency = 3.f;
	Lurch.Damping = 0.35f;
	LamplighterEffects::Kick(this, Lurch);
	UE_LOG(LogLooter, Log, TEXT("%s: her pilot light gone; she lists and drifts west."), *GetActorNameOrLabel());
}

void ABargeBoarding::AdvanceDying(float DeltaSeconds)
{
	DyingClock += DeltaSeconds;
	PoseDying();
	const float StackAlpha = (DyingClock - BargeRules::StackFallsAt) / BargeRules::StackFallSeconds;
	if (StackAlpha > 0.f && !bStackDown)
	{
		DropStack(FMath::Min(StackAlpha, 1.f));
	}
	FireWait -= DeltaSeconds;
	if (FireWait <= 0.f)
	{
		FireWait = FireEvery;
		const FVector Spot = SternFires[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(SternFires)) - 1)];
		const FVector At = ToWorld(Spot + FVector(0.0, 0.0, BargeRules::DeckTop + 300.0));
		LamplighterEffects::SmokePuff(this, At, FVector(0.0, -BargeRules::DriftSpeed, 260.0));
		if (FMath::FRand() < 0.12f)
		{
			LamplighterEffects::Fireball(this, At, FVector(0.0, 0.0, 120.0));
		}
	}
	if (FMath::Fmod(DyingClock, GroanEvery) < DeltaSeconds)
	{
		LooterSound::PlayAt(this, LooterSoundCue::Foreman::RoofGroan, GetDeckMiddle(), 1.1f, 0.65f);
	}
}

void ABargeBoarding::PoseDying()
{
	// From where she hung: drifting west (the level's -Y), sinking a little, listing to her berth side (a turn about her +X
	// lifts her +Y: her -Y side goes down with a positive turn).
	const FTransform Rest = RestTransform();
	const float List = BargeRules::ListAt(DyingClock);
	const FQuat Turn = Rest.GetRotation() * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-BargeRules::BerthSide * List));
	const FVector Where = Rest.GetLocation() + FVector(0.0, -BargeRules::DriftAt(DyingClock), -12.0 * DyingClock);
	SetActorTransform(FTransform(Turn, Where, Rest.GetScale3D()), /*bSweep*/ false, nullptr, ETeleportType::None);
}

void ABargeBoarding::DropStack(float Alpha)
{
	if (!FallingStackPivot)
	{
		return;
	}
	// Down across the port lane, a little aft: a turn about the line square to that, through its foot.
	const float Aft = FMath::DegreesToRadians(BargeRules::StackFallAft);
	const FVector Down(-FMath::Sin(Aft), BargeRules::BerthSide * FMath::Cos(Aft), 0.0);
	const FVector Axis(-Down.Y, Down.X, 0.0);
	// Falling under its weight (a square ease in), a small bounce once it's down.
	const float Fall = Alpha < 1.f ? Alpha * Alpha : 1.f;
	FallingStackPivot->SetRelativeRotation(FQuat(Axis, FMath::DegreesToRadians(88.f * Fall)));
	if (Alpha < 1.f || bStackDown)
	{
		return;
	}
	bStackDown = true;
	const FVector Foot = ToWorld(BargeRules::StackSpots[2]);
	const FVector Lying = GetActorTransform().TransformVectorNoScale(Down);
	const FVector Middle = Foot + Lying * (BargeRules::StackHeight * 0.5);
	LooterSound::PlayAt(this, LooterSoundCue::Gunfeel::ImpactHeavy, Middle, 1.5f, 0.6f);
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::RockFall, Middle, 1.2f, 0.8f);
	LamplighterEffects::Sparks(this, Middle, FVector::UpVector, 24, 1.4f);
	LamplighterEffects::DustSkirt(this, Middle, 400.f, 10);
	LamplighterEffects::Shake(this, 0.8f, 1.f);
	if (bStackHurtDone)
	{
		return;
	}
	bStackHurtDone = true;
	// What it lands on is hurt (the player who didn't run): within its line's reach, along its whole length.
	if (APawn* Player = FindPlayer())
	{
		const FVector Closest = FMath::ClosestPointOnSegment(Player->GetActorLocation(), Foot, Foot + Lying * BargeRules::StackHeight);
		if (FVector::Dist(Closest, Player->GetActorLocation()) < StackHitReach)
		{
			const UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
			UGameplayStatics::ApplyDamage(Player, (Life ? Life->GetMaxHealth() : 100.f) * StackHitShare, nullptr, this, UDamageType::StaticClass());
		}
	}
}

void ABargeBoarding::ApplyFallAt(float Seconds)
{
	if (State == EBargeState::Fallen || State == EBargeState::Away)
	{
		return;
	}
	if (State != EBargeState::Falling)
	{
		// Over the brink: from wherever her drift has taken her.
		if (State == EBargeState::Hanging || State == EBargeState::Burst)
		{
			BeginDying();
		}
		FallFrom = GetActorTransform();
		bImpacted = false;
		SetState(EBargeState::Falling);
		HoseShare = 0.f;
		HoseSections->ClearInstances();
	}
	const FVector Target = FallTarget.IsNearlyZero() ? FallFrom.GetLocation() + BargeRules::FallFallback : FallTarget;
	SetActorTransform(BargeRules::FallPose(FallFrom, Target, Seconds), /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	LayCoal(Seconds);
	// Her trail: black smoke off her burning back half, thickening as she goes.
	FireWait -= 1.f / 30.f;
	if (FireWait <= 0.f)
	{
		FireWait = FireEvery;
		LamplighterEffects::SmokePuff(this, ToWorld(FVector(-2600.0, 0.0, BargeRules::DeckTop + 400.0)), FVector(0.0, 0.0, 300.0));
	}
	if (!bImpacted && Seconds >= BargeRules::ImpactAt)
	{
		// Into the canyon: the fireball off its floor, heard across the Rest.
		bImpacted = true;
		LamplighterEffects::Fireball(this, Target + FVector(0.0, 0.0, 400.0), FVector(0.0, 0.0, 300.0));
		LamplighterEffects::Fireball(this, Target + FVector(600.0, -300.0, 200.0), FVector(0.0, 0.0, 200.0));
		for (int32 Puff = 0; Puff < 6; ++Puff)
		{
			LamplighterEffects::SmokePuff(this, Target + FVector(FMath::FRandRange(-800.f, 800.f), FMath::FRandRange(-800.f, 800.f), 600.0 + Puff * 300.0),
				FVector(0.0, 0.0, 380.0));
		}
		LooterSound::PlayAt(this, LooterSoundCue::Launcher::ExplosionDistant, Target, 2.f, 0.6f);
		LamplighterEffects::Shake(this, 0.35f, 1.5f);
	}
}

void ABargeBoarding::LayCoal(float Seconds)
{
	if (!CoalRain)
	{
		return;
	}
	const FVector Target = FallTarget.IsNearlyZero() ? FallFrom.GetLocation() + BargeRules::FallFallback : FallTarget;
	TArray<FTransform> Lumps;
	Lumps.Reserve(BargeRules::CoalLumps);
	for (int32 Index = 0; Index < BargeRules::CoalLumps; ++Index)
	{
		const float Released = Index * BargeRules::CoalEvery;
		const float Age = Seconds - Released;
		if (Age < 0.f || Age > BargeRules::CoalLife || Released > BargeRules::PlungeAt)
		{
			// Not yet shaken loose, or long gone below: kept as an empty instance so the count never changes.
			Lumps.Add(FTransform(FQuat::Identity, FallFrom.GetLocation(), FVector::ZeroVector));
			continue;
		}
		// Each lump from its own spot under her keel (the same every replay), with her speed as it left her, then falling.
		FRandomStream Spot(4000 + Index);
		const FVector Under = CoalFrom + FVector(Spot.FRandRange(-1.f, 1.f) * CoalSpread.X, Spot.FRandRange(-1.f, 1.f) * CoalSpread.Y, 0.0);
		const FTransform At = BargeRules::FallPose(FallFrom, Target, Released);
		const FTransform After = BargeRules::FallPose(FallFrom, Target, Released + 0.1f);
		const FVector Speed = (After.GetLocation() - At.GetLocation()) / 0.1;
		const FVector Where = At.TransformPosition(Under) + Speed * Age - FVector(0.0, 0.0, 0.5 * CoalGravity * Age * Age);
		Lumps.Add(FTransform(FRotator(Age * 140.f, Index * 37.f, Age * 90.f), Where, FVector(Spot.FRandRange(0.25f, 0.45f))));
	}
	if (CoalRain->GetInstanceCount() != Lumps.Num())
	{
		CoalRain->ClearInstances();
		CoalRain->AddInstances(Lumps, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
	}
	else
	{
		CoalRain->BatchUpdateInstancesTransforms(0, Lumps, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
}

void ABargeBoarding::FinishFall()
{
	if (State == EBargeState::Fallen)
	{
		return;
	}
	CoalRain->ClearInstances();
	SternFire->ClearInstances();
	HoseSections->ClearInstances();
	SetState(EBargeState::Fallen);
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		// The wreck in the canyon shows from this (the build script's AStoryProp), and she never hangs over the Rest again.
		Runner->SetFlag(BargeRules::BargeFellFlag());
	}
	UE_LOG(LogLooter, Log, TEXT("%s: down in Gravewind Canyon (%s)."), *GetActorNameOrLabel(), *BargeRules::BargeFellFlag().ToString());
}
