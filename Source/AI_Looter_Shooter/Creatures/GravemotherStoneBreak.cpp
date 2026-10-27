// AGravemotherStone's crack-off (the sheet's "Phase 2" strip: 0.0 s the cracks race across it, 0.3 s it splits and the
// roots snap, 0.6 s the halves slide off her back, 1.0 s they hit the floor): the slab goes, and its five pieces, laid
// where they sat in it, drift apart, then fall away off her back with a turn each, bounce once on the floor, lie a while
// and sink away. Plain moving meshes (no physics, no collision): looks only.

#include "Creatures/GravemotherStone.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFX.h"
#include "World/WorldQueries.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
	constexpr int32 ShardCount = 5;
	/** The split (s): the pieces part slowly before they fall. */
	constexpr float SplitSeconds = 0.3f;
	constexpr float SplitSpeed = 30.f;
	/** Then off her back: outward this fast (cm/s), up a little, turning. */
	constexpr float SlideSpeedMin = 150.f;
	constexpr float SlideSpeedMax = 250.f;
	constexpr float FXGravity = 980.f;
	/** How far below a piece its floor is looked for (cm), and how deep it sinks at the end. */
	constexpr float FloorReach = 800.f;
	constexpr float SinkDepth = 60.f;

	/**
	 * The stand-in's pieces in the slab's own frame (its unit cube, -50..50): two at its foot, two over them, the top. ART's
	 * pieces are modelled in place round the stone's pivot, so they sit at the slab's own transform.
	 */
	const FTransform StandInPieces[ShardCount] = {
		FTransform(FQuat::Identity, FVector(0.0, -25.0, -25.0), FVector(1.0, 0.48, 0.48)),
		FTransform(FQuat::Identity, FVector(0.0, 25.0, -25.0), FVector(1.0, 0.48, 0.48)),
		FTransform(FQuat::Identity, FVector(0.0, -25.0, 15.0), FVector(1.0, 0.48, 0.3)),
		FTransform(FQuat::Identity, FVector(0.0, 25.0, 15.0), FVector(1.0, 0.48, 0.3)),
		FTransform(FQuat::Identity, FVector(0.0, 0.0, 40.0), FVector(1.0, 0.96, 0.2)),
	};
}

void AGravemotherStone::MakeShards()
{
	Shards.Reset();
	ShardStates.Reset();
	for (int32 Index = 0; Index < ShardCount; ++Index)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Shard%d"), Index + 1), RF_Transient);
		Piece->SetupAttachment(Pivot);
		// Once it falls it's in the world's frame, not her abdomen's.
		Piece->SetUsingAbsoluteLocation(true);
		Piece->SetUsingAbsoluteRotation(true);
		Piece->SetUsingAbsoluteScale(true);
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(true);
		Piece->SetCanEverAffectNavigation(false);
		Piece->SetHiddenInGame(true);
		FShard& Shard = ShardStates.AddDefaulted_GetRef();
		if (UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::ShardPath(Index + 1)))
		{
			Piece->SetStaticMesh(Mesh);
			Shard.InSlab = FTransform::Identity;
			Shard.Center = Mesh->GetBounds().Origin;
		}
		else
		{
			Piece->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Cube));
			Piece->SetMaterial(0, GravemotherAssets::StandInLook(this, GravemotherAssets::GraniteColor() * 0.8f));
			Shard.InSlab = StandInPieces[Index];
			Shard.Center = FVector::ZeroVector;
		}
		Piece->RegisterComponent();
		Shards.Add(Piece);
	}
}

void AGravemotherStone::Break(bool bByHerThird)
{
	if (!IsOn())
	{
		return;
	}
	// The cracks race across it and it splits: chips and dust burst off it, the roots snap, the slab is gone.
	const FVector Middle = Slab->Bounds.Origin;
	// Its target's glow goes with it.
	ClearMark();
	Shower(10, 18, 520.f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherStoneBreak, Middle);
	BossCameraShake::Kick(this, Middle, 0.45f, 0.6f);
	StartShards();
	State = EGravemotherStone::Broken;
	ShownMesh = -1;
	Slab->SetHiddenInGame(true);
	Slab->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UE_LOG(LogLooter, Log, TEXT("%s: the headstone cracks off%s."), *GetNameSafe(Mother.Get()), bByHerThird ? TEXT(" at her third") : TEXT(""));
	if (AGravemotherCreature* Her = Mother.Get())
	{
		Her->OnStoneBroken(bByHerThird);
	}
}

void AGravemotherStone::StartShards()
{
	if (Shards.Num() != ShardStates.Num())
	{
		return;
	}
	UWorld* World = GetWorld();
	const FTransform SlabWorld = Slab->GetComponentTransform();
	const FVector Middle = Slab->Bounds.Origin;
	const FVector Side = Mother.IsValid() ? Mother->GetActorRightVector() : FVector::RightVector;
	const FCollisionQueryParams Ground = LooterWorld::StaticGeometryParams(World, TEXT("GravemotherShards"), Mother.Get());
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		FShard& Shard = ShardStates[Index];
		const FTransform Start = Shard.InSlab * SlabWorld;
		Shard.Rotation = Start.GetRotation();
		Shard.Location = Start.TransformPosition(Shard.Center);
		FVector Out = Shard.Location - Middle;
		Out.Z = 0.0;
		if (!Out.Normalize())
		{
			Out = Side * (Index % 2 == 0 ? 1.0 : -1.0);
		}
		Shard.Velocity = Out * SplitSpeed;
		Shard.Spin = FMath::VRand() * FMath::FRandRange(1.5f, 4.f);
		Shard.bLanded = false;
		Shard.bBounced = false;
		Shard.FloorZ = static_cast<float>(Shard.Location.Z) - 300.f;
		FHitResult Hit;
		if (World && World->LineTraceSingleByObjectType(Hit, Shard.Location, Shard.Location - FVector(0.0, 0.0, FloorReach),
			FCollisionObjectQueryParams(ECC_WorldStatic), Ground))
		{
			Shard.FloorZ = static_cast<float>(Hit.ImpactPoint.Z);
		}
		UStaticMeshComponent* Piece = Shards[Index];
		Piece->SetWorldScale3D(Start.GetScale3D());
		Piece->SetHiddenInGame(false);
	}
	ShardTime = 0.f;
	TickShards(0.f);
	SetActorTickEnabled(true);
}

void AGravemotherStone::HideShards()
{
	for (UStaticMeshComponent* Piece : Shards)
	{
		if (Piece)
		{
			Piece->SetHiddenInGame(true);
		}
	}
	ShardTime = -1.f;
	UpdateTickNeed();
}

void AGravemotherStone::Advance(float DeltaSeconds)
{
	if (ShardTime >= 0.f)
	{
		TickShards(DeltaSeconds);
	}
	TickMark(DeltaSeconds);
}

void AGravemotherStone::TickShards(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	const float Before = ShardTime;
	ShardTime += Dt;
	const float LieEnds = 1.f + Settings.ShardLieSeconds;
	const float Gone = LieEnds + Settings.ShardSinkSeconds;
	if (ShardTime >= Gone)
	{
		HideShards();
		return;
	}
	const bool bFalls = ShardTime >= SplitSeconds;
	const bool bJustFell = Before < SplitSeconds && bFalls;
	const float Sink = ShardTime > LieEnds ? SinkDepth * FMath::SmoothStep(LieEnds, Gone, ShardTime) : 0.f;
	for (int32 Index = 0; Index < Shards.Num() && Index < ShardStates.Num(); ++Index)
	{
		FShard& Shard = ShardStates[Index];
		UStaticMeshComponent* Piece = Shards[Index];
		if (!Piece)
		{
			continue;
		}
		if (bJustFell)
		{
			// Off her back: outward, up a little, turning.
			FVector Out = Shard.Velocity.GetSafeNormal2D();
			Shard.Velocity = Out * FMath::FRandRange(SlideSpeedMin, SlideSpeedMax) + FVector(0.0, 0.0, FMath::FRandRange(60.f, 140.f));
		}
		if (bFalls && !Shard.bLanded)
		{
			Shard.Velocity.Z -= FXGravity * Dt;
			const float Turn = Shard.Spin.Size() * Dt;
			if (Turn > 0.f)
			{
				Shard.Rotation = FQuat(Shard.Spin.GetSafeNormal(), Turn) * Shard.Rotation;
			}
		}
		if (!Shard.bLanded)
		{
			Shard.Location += Shard.Velocity * Dt;
		}
		// Its foot about half its size under its middle: it meets the floor there.
		const float Half = static_cast<float>(Piece->Bounds.BoxExtent.Z) * 0.6f;
		if (bFalls && !Shard.bLanded && Shard.Location.Z - Half <= Shard.FloorZ && Shard.Velocity.Z < 0.f)
		{
			Shard.Location.Z = Shard.FloorZ + Half;
			if (!Shard.bBounced)
			{
				Shard.bBounced = true;
				Shard.Velocity = FVector(Shard.Velocity.X * 0.4, Shard.Velocity.Y * 0.4, -Shard.Velocity.Z * 0.28);
				Shard.Spin *= 0.35f;
				if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
				{
					FGraveDustBurst Thud;
					Thud.Kind = EGraveDust::Floor;
					Thud.Count = 4;
					Thud.Spread = 30.f;
					Thud.StartSize = 40.f;
					Thud.EndSize = 120.f;
					Thud.Life = 1.f;
					Thud.Opacity = 0.3f;
					Thud.Outward = 90.f;
					Effects->Puffs(FVector(Shard.Location.X, Shard.Location.Y, Shard.FloorZ), Thud);
				}
			}
			else
			{
				Shard.bLanded = true;
				Shard.Velocity = FVector::ZeroVector;
			}
		}
		const FVector Scale = Piece->GetComponentScale();
		const FVector Placed = Shard.Location - Shard.Rotation.RotateVector(Shard.Center * Scale) - FVector(0.0, 0.0, Sink);
		Piece->SetWorldLocationAndRotation(Placed, Shard.Rotation);
	}
}

int32 AGravemotherStone::NumShardsShown() const
{
	int32 Shown = 0;
	for (const UStaticMeshComponent* Piece : Shards)
	{
		Shown += Piece && !Piece->bHiddenInGame ? 1 : 0;
	}
	return Shown;
}

int32 AGravemotherStone::NumShardsFalling() const
{
	if (ShardTime < 0.f)
	{
		return 0;
	}
	int32 Falling = 0;
	for (const FShard& Shard : ShardStates)
	{
		Falling += Shard.bLanded ? 0 : 1;
	}
	return Falling;
}
