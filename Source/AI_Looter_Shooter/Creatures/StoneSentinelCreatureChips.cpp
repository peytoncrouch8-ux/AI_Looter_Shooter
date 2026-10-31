// AStoneSentinelCreature's stone pieces: the Gravemother's headstone shards (SM_GravemotherStoneShard_1-5), sized down to
// chips and chunks, thrown off it in code (a hit, its waking, its slam, its crumbling): they fly, turn, bounce once on the
// ground under the burst, lie a moment and shrink away. One instanced set a shard mesh, no collision, no shadows; capped,
// and only near a viewer. A granite block of the engine's cube stands in until the shards are imported. Its dust is
// UGravemotherFX's stone and floor dust. Its crumble (the body bursting into stone) is here too.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossCameraShake.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/StoneSentinelFX.h"
#include "World/WorldQueries.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The most pieces about at once: the oldest go first. */
	constexpr int32 MaxChips = 48;
	/** Pieces are thrown only this near a viewer (cm): nobody sees them farther. */
	constexpr float ChipsSeenWithin = 6000.f;
	constexpr float ChipGravity = 980.f;
	/** Air drag (a share of speed lost a second), the bounce's give and what's kept along the ground. */
	constexpr float ChipDrag = 0.6f;
	constexpr float ChipBounce = 0.3f;
	constexpr float ChipSlide = 0.5f;
	/** A landing slower than this (cm/s, down) doesn't bounce: it lies. */
	constexpr float BounceSpeed = 120.f;
	/** Its last moment it shrinks away (s). */
	constexpr float ShrinkSeconds = 0.4f;
	/** How far under a burst its floor is looked for (cm). */
	constexpr float FloorReach = 600.f;
	/** The stand-in's block: a granite chip of the engine's 100 cm cube. */
	constexpr float StandInChipSize = 100.f;
	/** Pebbles off a joint: at most this many at once, how fast they fall away (cm/s), how big (cm), the share that bring a
	 *  breath of dust, and their sound now and then, never closer together than PebbleSoundGap (s). */
	constexpr int32 MostPebbles = 3;
	constexpr float PebbleSpeed = 70.f;
	constexpr float PebbleMin = 1.5f;
	constexpr float PebbleMax = 3.5f;
	constexpr float PebbleDustShare = 0.5f;
	constexpr float PebbleSoundShare = 0.45f;
	constexpr double PebbleSoundGap = 0.35;
}

void AStoneSentinelCreature::SetupChips()
{
	if (bChipsReady)
	{
		return;
	}
	bChipsReady = true;
	auto AddSet = [this](UStaticMesh* ChipMesh, UMaterialInterface* Look, int32 Index)
	{
		UInstancedStaticMeshComponent* Set = NewObject<UInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Chips%d"), Index), RF_Transient);
		Set->SetupAttachment(GetRootComponent());
		// In the world's frame: the pieces fly free of the body (and of its size).
		Set->SetUsingAbsoluteLocation(true);
		Set->SetUsingAbsoluteRotation(true);
		Set->SetUsingAbsoluteScale(true);
		Set->SetMobility(EComponentMobility::Movable);
		Set->SetStaticMesh(ChipMesh);
		if (Look)
		{
			Set->SetMaterial(0, Look);
		}
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		Set->SetCanEverAffectNavigation(false);
		Set->RegisterComponent();
		Set->SetWorldTransform(FTransform::Identity);
		ChipSets.Add(Set);
		// The shards are modelled in place round the headstone's pivot: each turns round its own middle, not that.
		const FBoxSphereBounds ChipBounds = ChipMesh->GetBounds();
		ChipCenters.Add(Look ? FVector::ZeroVector : FVector(ChipBounds.Origin));
		ChipSizes.Add(Look ? StandInChipSize : FMath::Max(static_cast<float>(ChipBounds.BoxExtent.GetMax()) * 2.f, 1.f));
	};
	for (int32 Index = 1; Index <= 5; ++Index)
	{
		if (UStaticMesh* Shard = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::ShardPath(Index)))
		{
			AddSet(Shard, nullptr, Index);
		}
	}
	if (ChipSets.Num() == 0)
	{
		if (UStaticMesh* Cube = GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Cube))
		{
			AddSet(Cube, GravemotherAssets::StandInLook(this, GravemotherAssets::GraniteColor() * 0.8f), 0);
		}
	}
}

void AStoneSentinelCreature::ThrowChips(const FVector& At, const FVector& Up, int32 Count, float Speed, float SizeMin, float SizeMax, float LifeMin,
	float LifeMax)
{
	UWorld* World = GetWorld();
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	if (Count <= 0 || ChipSets.Num() == 0 || !World || !Reacts || !Reacts->IsNearAViewer(At, ChipsSeenWithin))
	{
		return;
	}
	// One look for the floor under the burst: the pieces land on it.
	float FloorZ = static_cast<float>(At.Z) - 300.f;
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SentinelChips"), this);
	if (World->LineTraceSingleByObjectType(Hit, At + FVector(0.0, 0.0, 20.0), At - FVector(0.0, 0.0, FloorReach),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		FloorZ = static_cast<float>(Hit.ImpactPoint.Z);
	}
	const FVector Lift = Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Chips.Num() >= MaxChips)
		{
			Chips.RemoveAt(0, 1, EAllowShrinking::No);
		}
		FChip& Chip = Chips.AddDefaulted_GetRef();
		Chip.Set = FMath::RandRange(0, ChipSets.Num() - 1);
		Chip.Scale = FMath::FRandRange(SizeMin, SizeMax) / ChipSizes[Chip.Set];
		Chip.Location = At + FMath::VRand() * 5.f;
		const FVector Out = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
		const FVector Way = (Lift * 0.7f + Out * 0.6f + FMath::VRand() * 0.3f).GetSafeNormal();
		Chip.Velocity = Way * (Speed * FMath::FRandRange(0.6f, 1.2f));
		Chip.Rotation = FQuat(FMath::VRand(), FMath::FRandRange(0.f, 2.f * PI));
		Chip.Spin = FMath::VRand() * FMath::FRandRange(4.f, 10.f);
		Chip.Life = FMath::FRandRange(LifeMin, FMath::Max(LifeMin, LifeMax));
		Chip.FloorZ = FloorZ;
	}
}

void AStoneSentinelCreature::TickChips(float DeltaSeconds)
{
	if (Chips.IsEmpty())
	{
		if (bChipsDrawn)
		{
			DrawChips();
		}
		return;
	}
	for (int32 Index = Chips.Num() - 1; Index >= 0; --Index)
	{
		FChip& Chip = Chips[Index];
		Chip.Age += DeltaSeconds;
		if (Chip.Age >= Chip.Life)
		{
			Chips.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Chip.bResting)
		{
			continue;
		}
		Chip.Velocity.Z -= ChipGravity * DeltaSeconds;
		Chip.Velocity *= FMath::Max(0.f, 1.f - ChipDrag * DeltaSeconds);
		Chip.Location += Chip.Velocity * DeltaSeconds;
		const float Turn = static_cast<float>(Chip.Spin.Size()) * DeltaSeconds;
		if (Turn > 0.f)
		{
			Chip.Rotation = (FQuat(Chip.Spin.GetSafeNormal(), Turn) * Chip.Rotation).GetNormalized();
		}
		if (Chip.Location.Z <= Chip.FloorZ)
		{
			Chip.Location.Z = Chip.FloorZ;
			// One bounce if it came down hard, then it lies where it stopped.
			if (Chip.Bounces == 0 && -Chip.Velocity.Z > BounceSpeed)
			{
				Chip.Velocity = FVector(Chip.Velocity.X * ChipSlide, Chip.Velocity.Y * ChipSlide, -Chip.Velocity.Z * ChipBounce);
				Chip.Spin *= 0.5f;
				Chip.Bounces = 1;
			}
			else
			{
				Chip.bResting = true;
			}
		}
	}
	DrawChips();
}

void AStoneSentinelCreature::DrawChips()
{
	TArray<TArray<FTransform>> Sets;
	Sets.SetNum(ChipSets.Num());
	for (const FChip& Chip : Chips)
	{
		if (!Sets.IsValidIndex(Chip.Set))
		{
			continue;
		}
		const float Shrink = FMath::Clamp((Chip.Life - Chip.Age) / ShrinkSeconds, 0.f, 1.f);
		const float Scale = FMath::Max(Chip.Scale * Shrink, 0.001f);
		// Its own middle at its place (the mesh's middle sits off its pivot).
		const FVector Middle = ChipCenters.IsValidIndex(Chip.Set) ? ChipCenters[Chip.Set] : FVector::ZeroVector;
		Sets[Chip.Set].Add(FTransform(Chip.Rotation, Chip.Location - Chip.Rotation.RotateVector(Middle * Scale), FVector(Scale)));
	}
	bool bAnything = false;
	for (int32 Index = 0; Index < ChipSets.Num(); ++Index)
	{
		UInstancedStaticMeshComponent* Set = ChipSets[Index];
		if (!Set)
		{
			continue;
		}
		TArray<FTransform>& Transforms = Sets[Index];
		bAnything |= !Transforms.IsEmpty();
		// A pool that only grows: new instances added once, spare ones hidden (shrunk to nothing under its feet), so a frame
		// is one transform update, never the instances torn down and built again as pieces come and go.
		const int32 Have = Set->GetInstanceCount();
		const int32 Want = Transforms.Num();
		if (Want > Have)
		{
			TArray<FTransform> Extra;
			Extra.Append(Transforms.GetData() + Have, Want - Have);
			Set->AddInstances(Extra, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			Transforms.SetNum(Have);
		}
		else
		{
			const FTransform Hidden(FQuat::Identity, GetActorLocation() - FVector(0.0, 0.0, 500.0), FVector(0.001f));
			while (Transforms.Num() < Have)
			{
				Transforms.Add(Hidden);
			}
		}
		if (Transforms.Num() > 0)
		{
			Set->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		}
	}
	bChipsDrawn = bAnything;
}

void AStoneSentinelCreature::StoneDust(const FVector& At, int32 Count, float Size, float Spread, bool bFloor) const
{
	if (Count <= 0)
	{
		return;
	}
	// Short-lived, never at the lens and within the sentinels' dust budget (StoneSentinelFX: big translucent cards at the
	// camera cost a frame its life, 2026-10-31).
	FGraveDustBurst Burst;
	Burst.Kind = bFloor ? EGraveDust::Floor : EGraveDust::Stone;
	Burst.Count = Count;
	Burst.Spread = Spread;
	Burst.SpreadUp = Spread * 0.4f;
	Burst.StartSize = Size * 0.6f;
	Burst.EndSize = Size * 1.4f;
	Burst.Life = 1.f;
	Burst.Opacity = 0.32f;
	Burst.Drift = FVector(0.0, 0.0, 25.0);
	Burst.Outward = 90.f;
	StoneSentinelFX::Dust(this, At, Burst);
}

void AStoneSentinelCreature::ThrowStones(const FVector& At, const FVector& Up, int32 Count, float Speed, float SizeMin, float SizeMax)
{
	// Its own stone pieces (one instanced set a mesh, no shadows), short-lived: a rock's burst reuses them, nothing new spawned.
	ThrowChips(At, Up, Count, Speed, SizeMin, SizeMax, 0.8f, 1.4f);
}

void AStoneSentinelCreature::Crumble()
{
	bCrumbled = true;
	// Where its body was, before it goes.
	const float Scale = GetSizeScale();
	const FVector Middle = bStandIn ? FVector(StandInBody->Bounds.Origin) : FVector(GetMesh()->Bounds.Origin);
	const FVector Feet = GetFeet();
	GetMesh()->SetVisibility(false);
	StandInBody->SetVisibility(false);
	StandInHead->SetVisibility(false);
	// It bursts into stone: chunks off the body, heavy pieces that barely leave the ground, a cloud of stone dust and the
	// ground's own kicked up, the rubble's thud, a jolt for whoever stood near.
	ThrowChips(Middle, FVector::UpVector, Tuning.CrumbleChips, 300.f, 12.f * Scale, 34.f * Scale);
	ThrowChips(Feet + FVector(0.0, 0.0, 30.0 * Scale), FVector::UpVector, 6, 160.f, 18.f * Scale, 40.f * Scale);
	StoneDust(Middle, 4, 120.f * Scale, 70.f * Scale);
	StoneDust(Feet, 3, 130.f * Scale, 110.f * Scale, /*bFloor*/ true);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::StoneChip, Middle, FVector::UpVector, 4, 380.f, 0.8f);
	}
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Step), Feet, 1.f, LooterSoundRules::PitchForSize(Scale) * 0.85f);
	BossCameraShake::Kick(this, Middle, 0.15f, 0.4f, 1100.f * Scale);
}

// ---------------------------------------------------------------------------
// Grit off its joints
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::TickGrit(float DeltaSeconds)
{
	if (Phase == EStoneSentinelPhase::Dormant || Phase == EStoneSentinelPhase::Crumbling || IsPoseFrozen())
	{
		GritOwed = 0.f;
		return;
	}
	// Stone moving sheds itself: a little standing, more walking, a stream as it heaves, the most as it cracks loose.
	float Rate = Tuning.GritIdle;
	if (Phase == EStoneSentinelPhase::Waking)
	{
		Rate = Tuning.GritWaking;
	}
	else
	{
		Rate += Tuning.GritWalking * Walking;
		if (GetCreatureState() == ECreatureState::Attack && GetStateTime() < AttackWindup)
		{
			Rate += Tuning.GritWindup;
		}
		if (IsExposed())
		{
			Rate += Tuning.GritExposed;
		}
	}
	GritOwed += Rate * DeltaSeconds;
	if (GritOwed >= 1.f)
	{
		const int32 Count = FMath::FloorToInt32(GritOwed);
		GritOwed -= static_cast<float>(Count);
		ShedGrit(FMath::Min(Count, MostPebbles));
	}
}

void AStoneSentinelCreature::ShedGrit(int32 Count, int32 Joint)
{
	if (Count <= 0)
	{
		return;
	}
	// A joint: the one asked for, or a shoulder, an elbow, its neck, a knee or its back by chance.
	int32 Bone = Joint;
	if (Bone == INDEX_NONE && Rig.bReady)
	{
		const int32 Joints[] = { Rig.Upper[0], Rig.Upper[1], Rig.Lower[0], Rig.Lower[1], Rig.Head, Rig.Calf[0], Rig.Calf[1], Rig.Spine2 };
		Bone = Joints[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Joints)) - 1)];
	}
	const float Scale = GetSizeScale();
	const FVector Anywhere = GetActorLocation() + FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(0.f, 90.f)) * Scale;
	const FVector At = BoneSpot(Bone, Anywhere);
	// Down and a little off it, as grit falls out of a crack.
	const FVector Down(FMath::FRandRange(-0.3f, 0.3f), FMath::FRandRange(-0.3f, 0.3f), -1.f);
	ThrowChips(At, Down, Count, PebbleSpeed, PebbleMin, PebbleMax);
	if (FMath::FRand() < PebbleDustShare)
	{
		StoneDust(At, 1, 14.f * Scale, 4.f * Scale);
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastPebbleSound >= PebbleSoundGap && FMath::FRand() < PebbleSoundShare)
	{
		LastPebbleSound = Now;
		LooterSound::PlayAt(this, FName(StoneSentinelCue::Pebbles), At, 0.7f, LooterSoundRules::PitchForSize(Scale));
	}
}
