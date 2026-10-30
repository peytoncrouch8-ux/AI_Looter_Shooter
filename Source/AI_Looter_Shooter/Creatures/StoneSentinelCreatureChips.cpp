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

void AStoneSentinelCreature::ThrowChips(const FVector& At, const FVector& Up, int32 Count, float Speed, float SizeMin, float SizeMax)
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
		Chip.Life = FMath::FRandRange(1.8f, 3.f);
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
		const TArray<FTransform>& Transforms = Sets[Index];
		bAnything |= !Transforms.IsEmpty();
		if (Set->GetInstanceCount() != Transforms.Num())
		{
			Set->ClearInstances();
			if (Transforms.Num() > 0)
			{
				Set->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
			Set->MarkRenderStateDirty();
		}
		else if (Transforms.Num() > 0)
		{
			Set->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		}
	}
	bChipsDrawn = bAnything;
}

void AStoneSentinelCreature::StoneDust(const FVector& At, int32 Count, float Size, float Spread, bool bFloor) const
{
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	if (!Effects || Count <= 0)
	{
		return;
	}
	FGraveDustBurst Burst;
	Burst.Kind = bFloor ? EGraveDust::Floor : EGraveDust::Stone;
	Burst.Count = Count;
	Burst.Spread = Spread;
	Burst.SpreadUp = Spread * 0.4f;
	Burst.StartSize = Size * 0.6f;
	Burst.EndSize = Size * 1.6f;
	Burst.Life = 1.4f;
	Burst.Opacity = 0.32f;
	Burst.Drift = FVector(0.0, 0.0, 25.0);
	Burst.Outward = 90.f;
	Effects->Puffs(At, Burst);
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
	StoneDust(Middle, 10, 140.f * Scale, 70.f * Scale);
	StoneDust(Feet, 8, 160.f * Scale, 110.f * Scale, /*bFloor*/ true);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::StoneChip, Middle, FVector::UpVector, 10, 380.f, 1.f);
	}
	LooterSound::PlayAt(this, FName(LooterSoundCue::Sentinel::Step), Feet, 1.f, LooterSoundRules::PitchForSize(Scale) * 0.85f);
	BossCameraShake::Kick(this, Middle, 0.15f, 0.4f, 1100.f * Scale);
}
