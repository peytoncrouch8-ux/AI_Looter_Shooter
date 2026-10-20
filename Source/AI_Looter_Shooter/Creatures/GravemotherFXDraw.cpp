// UGravemotherFX's moving and drawing: cards drifting, growing and thinning (dust), rising (jets), flying and cooling
// (sparks), flashing; chunks falling, bouncing on their floor, lying and shrinking away; all redrawn as instances turned to
// the camera. Below, FWeaponFX's stand-ins for kinds whose art isn't imported yet.

#include "Creatures/GravemotherFX.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/GravemotherAssets.h"
#include "Weapons/WeaponFX.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr float FXGravity = 980.f;
	/** Under this speed (cm/s), or after this many bounces, a landed chunk lies still. */
	constexpr float RestSpeed = 60.f;
	constexpr uint8 MostBounces = 3;
	/** A chunk shrinks away over the last this long of its life (s). */
	constexpr float ShrinkSeconds = 0.4f;
	/** Dust thins out as the camera comes within this much of its edge (cm), so a puff by the eye doesn't fill the view. */
	constexpr float NearFade = 60.f;

	FVector Perpendicular(const FVector& Direction)
	{
		const FVector Other = FMath::Abs(Direction.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Direction, Other).GetSafeNormal();
	}

	/** A quad from Tail to Head turned about that line to the camera (the engine plane: 100 x 100 in XY, facing +Z, its U
	 *  along +X: the spark sheet's head, at U = 1, leads). */
	FTransform StreakTransform(const FVector& Tail, const FVector& Head, float Width, const FVector& Camera)
	{
		const FVector Axis = Head - Tail;
		const float Length = Axis.Size();
		const FVector Direction = Length > UE_KINDA_SMALL_NUMBER ? Axis / Length : FVector::ForwardVector;
		const FVector Middle = (Tail + Head) * 0.5f;
		const FVector ToCamera = Camera - Middle;
		FVector Facing = ToCamera - Direction * FVector::DotProduct(ToCamera, Direction);
		if (!Facing.Normalize())
		{
			Facing = Perpendicular(Direction);
		}
		return FTransform(FRotationMatrix::MakeFromXZ(Direction, Facing).ToQuat(), Middle, FVector(FMath::Max(Length, 0.1f) / 100.f, Width / 100.f, 1.f));
	}

	/** A square quad facing the camera, turned Roll about its facing. */
	FTransform SpriteTransform(const FVector& Location, float Size, float Roll, const FVector& Camera)
	{
		FVector Facing = Camera - Location;
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		return FTransform(FRotationMatrix::MakeFromZ(Facing).ToQuat() * FQuat(FVector::UpVector, Roll), Location, FVector(Size / 100.f, Size / 100.f, 1.f));
	}

	/** Replaces a set's instances with these (moved in place when the count is unchanged), as FWeaponFX draws its own. */
	void DrawSet(UInstancedStaticMeshComponent* Set, const TArray<FTransform>& Transforms, const TArray<float>& Data)
	{
		if (!Set)
		{
			return;
		}
		bool bCountChanged = false;
		if (Set->GetInstanceCount() != Transforms.Num())
		{
			bCountChanged = true;
			Set->ClearInstances();
			if (Transforms.Num() > 0)
			{
				Set->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
		}
		else if (Transforms.Num() > 0)
		{
			Set->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		if (Transforms.Num() > 0 && Data.Num() == Transforms.Num() * Set->NumCustomDataFloats)
		{
			Set->SetCustomData(0, Transforms.Num() - 1, Data, false);
		}
		if (bCountChanged)
		{
			Set->MarkRenderStateDirty();
		}
	}

	/** The weapons' effects in this world, ready to spawn into (null where nothing draws). */
	FWeaponFX* WeaponEffects(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}
}

void UGravemotherFX::Advance(float DeltaSeconds)
{
	Simulate(FMath::Clamp(DeltaSeconds, 0.f, 0.1f));
	Redraw();
}

void UGravemotherFX::Simulate(float Dt)
{
	for (int32 Index = Cards.Num() - 1; Index >= 0; --Index)
	{
		FCard& Card = Cards[Index];
		Card.Age += Dt;
		if (Card.Age >= Card.Life)
		{
			Cards.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		Card.Velocity.Z -= FXGravity * Card.Fall * Dt;
		Card.Velocity *= FMath::Exp(-Card.Drag * Dt);
		Card.Location += Card.Velocity * Dt;
	}
	for (int32 Index = Chunks_.Num() - 1; Index >= 0; --Index)
	{
		FChunk& Chunk = Chunks_[Index];
		Chunk.Age += Dt;
		if (Chunk.Age >= Chunk.Life)
		{
			Chunks_.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Chunk.bResting)
		{
			continue;
		}
		Chunk.Velocity.Z -= FXGravity * Dt;
		Chunk.Location += Chunk.Velocity * Dt;
		const float Turn = Chunk.Spin.Size() * Dt;
		if (Turn > 0.f)
		{
			Chunk.Rotation = FQuat(Chunk.Spin.GetSafeNormal(), Turn) * Chunk.Rotation;
		}
		if (Chunk.Location.Z <= Chunk.FloorZ && Chunk.Velocity.Z < 0.f)
		{
			// A bounce: up a little, slowed along the ground, its spin damped; still after a few or once slow.
			Chunk.Location.Z = Chunk.FloorZ;
			Chunk.Velocity = FVector(Chunk.Velocity.X * 0.55f, Chunk.Velocity.Y * 0.55f, -Chunk.Velocity.Z * 0.32f);
			Chunk.Spin *= 0.5f;
			++Chunk.Bounces;
			if (Chunk.Bounces >= MostBounces || Chunk.Velocity.Size() < RestSpeed)
			{
				Chunk.bResting = true;
				Chunk.Velocity = FVector::ZeroVector;
			}
		}
	}
}

FVector UGravemotherFX::GetCameraLocation() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	return Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
}

void UGravemotherFX::Redraw()
{
	if (!Owner.IsValid())
	{
		return;
	}
	const FVector Camera = GetCameraLocation();
	constexpr int32 SetCount = static_cast<int32>(ECardSet::Count);
	TArray<FTransform> Transforms[SetCount];
	TArray<float> Data[SetCount];

	for (const FCard& Card : Cards)
	{
		const int32 Set = static_cast<int32>(Card.Set);
		const float T = FMath::Clamp(Card.Age / Card.Life, 0.f, 1.f);
		float Strength = Card.Opacity;
		switch (Card.Set)
		{
		case ECardSet::Dust:
		case ECardSet::Floor:
		{
			// Grows fast then slow, eases in, thins out; thinned by the camera's nearness.
			const float Size = FMath::Lerp(Card.StartSize, Card.EndSize, 1.f - FMath::Square(1.f - T));
			Transforms[Set].Add(SpriteTransform(Card.Location, Size, Card.Roll + T * 0.6f, Camera));
			const float Near = Camera.IsZero() ? 1.f
				: FMath::Clamp((static_cast<float>(FVector::Dist(Card.Location, Camera)) - Size * 0.3f) / NearFade, 0.f, 1.f);
			Strength *= FMath::Clamp(T * 8.f, 0.f, 1.f) * FMath::Pow(1.f - T, 1.3f) * Near;
			break;
		}
		case ECardSet::Jet:
		{
			// Upright, turned about its own upright to face the camera; it rises taller as it thins.
			const float Height = Card.Length * (0.6f + 0.4f * T);
			const float Width = FMath::Lerp(Card.StartSize, Card.EndSize, T);
			FVector ToCamera = Camera - Card.Location;
			ToCamera.Z = 0.0;
			if (!ToCamera.Normalize())
			{
				ToCamera = FVector::ForwardVector;
			}
			const FVector Middle = Card.Location + FVector(0.0, 0.0, Height * 0.5f);
			// The jet sheet's foot is at V = 1, and the engine plane's V runs along its +Y: +Y down, U across.
			Transforms[Set].Add(FTransform(FRotationMatrix::MakeFromYZ(-FVector::UpVector, ToCamera).ToQuat(), Middle,
				FVector(Width / 100.f, Height / 100.f, 1.f)));
			Strength *= FMath::Clamp(T * 10.f, 0.f, 1.f) * FMath::Pow(1.f - T, 1.2f);
			break;
		}
		case ECardSet::Spark:
		{
			const FVector Back = Card.Velocity.GetSafeNormal() * (Card.Length * (1.f - 0.5f * T));
			Transforms[Set].Add(StreakTransform(Card.Location - Back, Card.Location, FMath::Lerp(Card.StartSize, Card.EndSize, T), Camera));
			Strength *= FMath::Pow(1.f - T, 1.5f);
			break;
		}
		case ECardSet::Flash:
		default:
			Transforms[Set].Add(SpriteTransform(Card.Location, FMath::Lerp(Card.StartSize, Card.EndSize, FMath::Sqrt(T)), Card.Roll, Camera));
			Strength *= FMath::Square(1.f - T);
			break;
		}
		Data[Set].Append({ Card.Tint.R, Card.Tint.G, Card.Tint.B, Strength, Card.Cell });
	}

	TArray<TArray<FTransform>> ChunkTransforms;
	ChunkTransforms.SetNum(ChunkSets.Num());
	for (const FChunk& Chunk : Chunks_)
	{
		if (!ChunkTransforms.IsValidIndex(Chunk.Mesh))
		{
			continue;
		}
		const float Shrink = FMath::Clamp((Chunk.Life - Chunk.Age) / ShrinkSeconds, 0.f, 1.f);
		ChunkTransforms[Chunk.Mesh].Add(FTransform(Chunk.Rotation, Chunk.Location, FVector(FMath::Max(Chunk.Size * Shrink, 0.01f))));
	}

	bool bAnything = false;
	for (int32 Set = 0; Set < SetCount; ++Set)
	{
		bAnything |= !Transforms[Set].IsEmpty();
	}
	for (const TArray<FTransform>& Set : ChunkTransforms)
	{
		bAnything |= !Set.IsEmpty();
	}
	if (!bAnything && !bDrawnAnything)
	{
		return;
	}
	for (int32 Set = 0; Set < SetCount; ++Set)
	{
		DrawSet(CardSets[Set].Get(), Transforms[Set], Data[Set]);
	}
	for (int32 Index = 0; Index < ChunkSets.Num(); ++Index)
	{
		DrawSet(ChunkSets[Index].Get(), ChunkTransforms[Index], {});
	}
	bDrawnAnything = bAnything;
}

// ---------------------------------------------------------------------------
// Stand-ins until the art is imported: the weapons' own effects
// ---------------------------------------------------------------------------

void UGravemotherFX::FallbackPuffs(const FVector& Center, const FGraveDustBurst& Burst)
{
	FWeaponFX* Effects = WeaponEffects(GetWorld());
	if (!Effects)
	{
		return;
	}
	// The smoke material is unlit: the dust's colour as the light where it is shows it.
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	const FLinearColor Light = Reacts ? Reacts->GetDustLight() : FLinearColor::White;
	const FLinearColor Dust = Burst.Kind == EGraveDust::Floor ? GravemotherAssets::FloorDustColor()
		: (Burst.Kind == EGraveDust::Stone ? GravemotherAssets::StoneDustColor() : GravemotherAssets::EarthDustColor());
	const FLinearColor Color = Dust * Light;
	// The weapons' budget is shared: a few puffs stand for the burst.
	const int32 Count = FMath::Min(Burst.Count, 8);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Out = FVector(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
		const FVector Where = Center + Out * Random.FRandRange(0.f, Burst.Spread) + FVector(0.0, 0.0, Random.FRandRange(0.f, Burst.SpreadUp));
		const FVector Velocity = Burst.Drift + (Out + FVector(0.0, 0.0, 0.25)) * (Burst.Outward * Random.FRandRange(0.5f, 1.f));
		Effects->SpawnDustPuff(Where, Velocity, Color, Burst.Opacity, Burst.StartSize * 0.8f, Burst.EndSize * 0.8f,
			Burst.Life * Random.FRandRange(0.8f, 1.2f));
	}
}

void UGravemotherFX::FallbackChunks(EGraveChunk Kind, const FVector& At, const FVector& Up, int32 Count, float Speed, float Size)
{
	FWeaponFX* Effects = WeaponEffects(GetWorld());
	if (!Effects || Count <= 0)
	{
		return;
	}
	const FVector Lift = Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal();
	if (Kind == EGraveChunk::StoneChip)
	{
		// Grit and stone chips, as a shot on rock throws them.
		Effects->SpawnSurfaceImpact(At, Lift, -Lift, EImpactMaterial::Stone, FMath::Clamp(Size, 0.5f, 2.f), false);
		return;
	}
	// Grave dirt: brown puffs and clods tossed out (the grave wake-up's), stronger for more.
	Effects->SpawnDirt(At, Lift, FMath::Clamp(static_cast<float>(Count) / 10.f + Speed / 2000.f, 0.2f, 1.f));
}

void UGravemotherFX::FallbackSparks(const FVector& At, const FVector& Normal, const FVector& ShotDirection)
{
	if (FWeaponFX* Effects = WeaponEffects(GetWorld()))
	{
		Effects->SpawnSurfaceImpact(At, Normal, ShotDirection, EImpactMaterial::Stone, 1.f, true);
	}
}
