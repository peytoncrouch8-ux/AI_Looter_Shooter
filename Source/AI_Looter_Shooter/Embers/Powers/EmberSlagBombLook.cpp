// UEmberSlagBomb: how the pool looks (its models set down, cooling, crusting over and sinking away; or, without them, its
// glowing edge and hot blobs), the burst's ring of fire racing out, sparks, smoke and haze rising, its light. The lob, the
// burst and the burns are EmberSlagBomb.cpp.

#include "Embers/Powers/EmberSlagBomb.h"
#include "Embers/Powers/EmberPropActor.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The pool's models (Art/Models/Props/SlagPool.py; Saved/Polish/SLAGPOOL.md): pivots on the ground at their middles. */
	const TCHAR* const PoolPath = TEXT("/Game/Art/Props/SM_SlagPool.SM_SlagPool");
	const TCHAR* const CrustPath = TEXT("/Game/Art/Props/SM_SlagPoolCrust.SM_SlagPoolCrust");
	const TCHAR* const SplatPaths[] = { TEXT("/Game/Art/Props/SM_SlagSplatA.SM_SlagSplatA"), TEXT("/Game/Art/Props/SM_SlagSplatB.SM_SlagSplatB") };
	/** SM_SlagPool's mean radius at scale 1 (cm), and its height scale: a flat pour (at the full Radius / 50 = 9 its crust plates
	 *  stood 13 cm tall). */
	constexpr float PoolModelRadius = 50.f;
	constexpr float PoolHeightScale = 5.f;
	/** Its glowing slots (M_StylizedSurface: emissive = Color x Glow), cooling toward a dark red at CooledGlow of their glow. */
	const FName GlowSlots[] = { FName(TEXT("SlagMolten")), FName(TEXT("SlagWhiteHot")), FName(TEXT("SlagGlowEdge")) };
	const FName ColorName(TEXT("Color"));
	const FName GlowName(TEXT("Glow"));
	const FLinearColor CooledRed(FColor(0x5A, 0x14, 0x06));
	constexpr float CooledGlow = 0.15f;
	/** The crust overlay lies this far under the melt while it's hot (model cm); it rises to 0 as it cools. */
	constexpr float CrustSunk = 0.9f;
	/** Over its last FadeSeconds the pool sinks this far (model cm) as its glow goes. */
	constexpr float PoolSink = 2.f;
	constexpr float FadeSeconds = 0.6f;
	/** EmberFX::GroundRing lifts the edge's points this far over the ground (EmberSlagBomb.cpp lays it so). */
	constexpr float EdgeLift = 4.f;

	/** The slag's colours: white-hot, molten, cooling to a dark crust. */
	const FLinearColor WhiteHot(1.f, 0.85f, 0.55f);
	const FLinearColor Molten(1.f, 0.42f, 0.05f);
	const FLinearColor Cooling(0.75f, 0.12f, 0.02f);
	/** The burst's ring of fire runs out over this long. */
	constexpr float RingSeconds = 0.3f;
	/** Sparks and smoke rising off the pool, a second (VFX-2: smoke 4 -> 2.5, haze 9 -> 5 and smaller: the player stands in it, and
	 * its puffs round the eye were most of the pool's cost on Medium). */
	constexpr float PoolSparksPerSecond = 22.f;
	constexpr float PoolSmokePerSecond = 2.5f;
	/**
	 * A low haze of molten light hanging over it (puffs in the slag's own colour, unlit, so they glow), a second: without it
	 * the pool read in daylight as a thin ring with a few dots in it (2026-10-20 play check), not a pool of slag.
	 */
	constexpr float PoolHazePerSecond = 5.f;
}

void UEmberSlagBomb::DrawPool(float DeltaSeconds)
{
	FWeaponFX* FX = Effects();
	if (!FX)
	{
		return;
	}
	const float Cool = FMath::Clamp(PoolAge / PoolSeconds, 0.f, 1.f);
	const FLinearColor Hot = FMath::Lerp(Molten, Cooling, Cool);
	const float Fade = FMath::Clamp(PoolLeft / FadeSeconds, 0.f, 1.f);
	if (bPoolModel)
	{
		// The slag itself: cooling, crusting over, sinking away at the end.
		DrivePool(Cool, Fade);
	}
	else
	{
		// Without its models: its glowing edge, pulsing a little.
		EmberFX::DrawLine(*FX, Edge, true, 7.f, Hot, (18.f - 8.f * Cool) * (0.85f + 0.15f * FMath::Sin(PoolAge * 7.f)) * Fade);
	}
	// Hot blobs lying on it, flickering; more of them crusted over (dark) as it cools (the models' own cracks do it).
	for (int32 Index = 0; Index < Blobs.Num() && !bPoolModel; ++Index)
	{
		const float Seed = FMath::Frac(Index * 0.618034f);
		if (Seed < Cool * 0.65f)
		{
			continue;
		}
		const float Angle = Seed * UE_TWO_PI;
		const FVector Way(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const float Size = 16.f + 24.f * Seed;
		const float Flicker = 0.7f + 0.3f * FMath::Sin(PoolAge * (5.f + 6.f * Seed) + Index);
		FX->AddStreak(Blobs[Index] - Way * Size, Blobs[Index] + Way * Size, Size * 0.9f, Seed > 0.8f ? WhiteHot : Hot, (28.f - 12.f * Cool) * Flicker * Fade);
	}
	// The molten haze, thinning as it crusts over.
	if (!Blobs.IsEmpty() && Random.FRand() < PoolHazePerSecond * DeltaSeconds * (1.f - 0.6f * Cool))
	{
		const FVector& Spot = Blobs[Random.RandRange(0, Blobs.Num() - 1)];
		FX->SpawnDustPuff(Spot + FVector(0.0, 0.0, 12.0), FVector(0.0, 0.0, Random.FRandRange(15.f, 35.f)) + Random.GetUnitVector() * 15.f, Hot * 0.9f,
			0.35f * Fade, Random.FRandRange(55.f, 85.f), Random.FRandRange(115.f, 160.f), Random.FRandRange(1.f, 1.5f));
	}
	// The first moments: the ring of fire racing out from the burst.
	if (RingAge >= 0.f)
	{
		RingAge += DeltaSeconds;
		const float U = FMath::Clamp(RingAge / RingSeconds, 0.f, 1.f);
		TArray<FVector> Racing;
		Racing.Reserve(Edge.Num());
		for (const FVector& Point : Edge)
		{
			Racing.Add(FMath::Lerp(BurstAt, Point, 1.f - FMath::Square(1.f - U)) + FVector(0.0, 0.0, 10.0));
		}
		EmberFX::DrawLine(*FX, Racing, true, 9.f, WhiteHot, 30.f * (1.f - U));
		if (U >= 1.f)
		{
			RingAge = -1.f;
		}
	}
	// Sparks and pale smoke rising off it.
	SmokeClock += DeltaSeconds;
	const int32 SparksNow = FMath::FloorToInt(PoolSparksPerSecond * DeltaSeconds + Random.FRand());
	for (int32 Index = 0; Index < SparksNow && !Blobs.IsEmpty(); ++Index)
	{
		const FVector& Spot = Blobs[Random.RandRange(0, Blobs.Num() - 1)];
		Sparks.Spawn(Spot, FVector(0.0, 0.0, Random.FRandRange(140.f, 300.f)) + Random.GetUnitVector() * 40.f, Random.FRandRange(0.5f, 0.9f), 1.1f, Hot,
			18.f * Fade, 250.f, 1.f);
	}
	if (SmokeClock >= 1.f / PoolSmokePerSecond && !Blobs.IsEmpty())
	{
		SmokeClock = 0.f;
		const FVector& Spot = Blobs[Random.RandRange(0, Blobs.Num() - 1)];
		FX->SpawnWisp(Spot + FVector(0.0, 0.0, 15.0), FVector(0.0, 0.0, Random.FRandRange(50.f, 90.f)), 0.2f, 35.f, 150.f, Random.FRandRange(1.6f, 2.4f));
	}
	if (AEmberPropActor* Here = Anchor.Get())
	{
		Here->SetLightIntensity(30.f * (1.f - 0.6f * Cool) * (0.85f + 0.15f * FMath::Sin(PoolAge * 11.f)) * Fade);
	}
}

bool UEmberSlagBomb::PlacePool(UWorld& World, const FVector& At)
{
	AEmberPropActor* Here = Anchor.Get();
	UStaticMesh* PoolMesh = AEmberPropActor::FindMesh(PoolPath);
	UStaticMeshComponent* Pool = Here ? Here->GetModel() : nullptr;
	PoolGlows.Reset();
	PoolGlowColors.Reset();
	PoolGlowStrengths.Reset();
	PoolPieces.Reset();
	PoolPieceRest.Reset();
	PoolCrust.Reset();
	if (!Pool || !PoolMesh)
	{
		return false;
	}
	// Lying on the plane through the ground round it (the edge's points; Newell's normal), its pivot on that plane over the
	// burst (held near the burst's own ground), turned any way.
	FVector Normal = FVector::UpVector;
	FVector Middle = At;
	if (Edge.Num() >= 3)
	{
		FVector Sum = FVector::ZeroVector;
		FVector Newell = FVector::ZeroVector;
		for (int32 Index = 0; Index < Edge.Num(); ++Index)
		{
			const FVector& A = Edge[Index];
			const FVector& B = Edge[(Index + 1) % Edge.Num()];
			Newell.X += (A.Y - B.Y) * (A.Z + B.Z);
			Newell.Y += (A.Z - B.Z) * (A.X + B.X);
			Newell.Z += (A.X - B.X) * (A.Y + B.Y);
			Sum += A;
		}
		Middle = Sum / static_cast<double>(Edge.Num()) - FVector(0.0, 0.0, EdgeLift);
		Normal = (Newell.Z < 0.0 ? -Newell : Newell).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		// A ring gone wild (half of it over a drop): it lies level.
		Normal = Normal.Z < 0.5 ? FVector::UpVector : Normal;
	}
	const double PlaneZ = Middle.Z - (Normal.X * (At.X - Middle.X) + Normal.Y * (At.Y - Middle.Y)) / Normal.Z;
	const FVector Pivot(At.X, At.Y, FMath::Clamp(PlaneZ, At.Z - 5.0, At.Z + 15.0));
	const FVector Facing = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
	Here->SetActorLocationAndRotation(Pivot, FRotationMatrix::MakeFromZX(Normal, Facing).Rotator());

	PoolScaleZ = PoolHeightScale;
	Pool->SetStaticMesh(PoolMesh);
	Pool->SetRelativeScale3D(FVector(Radius / PoolModelRadius, Radius / PoolModelRadius, PoolScaleZ));
	Pool->SetRelativeLocation(FVector::ZeroVector);
	Pool->SetCastShadow(false);
	// One instance per glowing slot, shared by the pool and its splashes, so one set of numbers cools them all.
	for (const FName& Slot : GlowSlots)
	{
		const int32 Index = Pool->GetMaterialIndex(Slot);
		UMaterialInterface* Base = Index != INDEX_NONE ? Pool->GetMaterial(Index) : nullptr;
		UMaterialInstanceDynamic* Look = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
		PoolGlows.Add(Look);
		PoolGlowColors.Add(Look ? Look->K2_GetVectorParameterValue(ColorName) : FLinearColor::White);
		PoolGlowStrengths.Add(Look ? Look->K2_GetScalarParameterValue(GlowName) : 0.f);
		if (Look)
		{
			Pool->SetMaterial(Index, Look);
		}
	}
	PoolPieces.Add(Pool);
	PoolPieceRest.Add(FVector::ZeroVector);

	// The crust overlay on the pool (its pivot, its scale), under the melt while it's hot.
	PoolCrust = Here->AddPiece(AEmberPropActor::FindMesh(CrustPath), Pool);
	if (UStaticMeshComponent* Crust = PoolCrust.Get())
	{
		Crust->SetRelativeLocation(FVector(0.0, 0.0, -CrustSunk));
	}

	// Two or three splashes at its rim, each on its own ground, breaking its round outline.
	const int32 Splats = Random.RandRange(2, 3);
	const float First = Random.FRandRange(0.f, UE_TWO_PI);
	for (int32 Index = 0; Index < Splats; ++Index)
	{
		UStaticMesh* SplatMesh = AEmberPropActor::FindMesh(SplatPaths[Index % 2]);
		const float Angle = First + UE_TWO_PI * static_cast<float>(Index) / static_cast<float>(Splats) + Random.FRandRange(-0.4f, 0.4f);
		const FVector Out = At + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Radius * Random.FRandRange(0.9f, 1.05f));
		FVector Ground;
		FVector GroundNormal = FVector::UpVector;
		if (!SplatMesh || !FindGround(World, Out, 150.f, 200.f, Ground, &GroundNormal, Body.Get()))
		{
			continue;
		}
		UStaticMeshComponent* Splat = Here->AddPiece(SplatMesh);
		if (!Splat)
		{
			continue;
		}
		const FVector Turn = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
		Splat->SetWorldLocationAndRotation(Ground, FRotationMatrix::MakeFromZX(GroundNormal, Turn).Rotator());
		Splat->SetWorldScale3D(FVector(Random.FRandRange(0.8f, 1.5f)));
		for (int32 Glow = 0; Glow < PoolGlows.Num(); ++Glow)
		{
			const int32 Slot = Splat->GetMaterialIndex(GlowSlots[Glow]);
			if (Slot != INDEX_NONE && PoolGlows[Glow])
			{
				Splat->SetMaterial(Slot, PoolGlows[Glow]);
			}
		}
		PoolPieces.Add(Splat);
		PoolPieceRest.Add(Splat->GetRelativeLocation());
	}
	DrivePool(0.f, 1.f);
	return true;
}

void UEmberSlagBomb::DrivePool(float Cool, float Fade)
{
	// Hot to a dark red crack as it cools (Heat 1 to 0, slow at first), a small flicker in the melt; out with Fade at the end.
	const float Heat = 1.f - FMath::Pow(FMath::Clamp(Cool, 0.f, 1.f), 1.5f);
	const float Flicker = 0.93f + 0.07f * FMath::Sin(PoolAge * 9.f);
	for (int32 Index = 0; Index < PoolGlows.Num(); ++Index)
	{
		if (UMaterialInstanceDynamic* Look = PoolGlows[Index])
		{
			// The glowing edge of the plates holds steady; the melt and the white-hot lake flicker.
			const float Shimmer = GlowSlots[Index] == FName(TEXT("SlagGlowEdge")) ? 1.f : Flicker;
			Look->SetScalarParameterValue(GlowName, PoolGlowStrengths[Index] * FMath::Lerp(CooledGlow, 1.f, Heat) * Fade * Shimmer);
			Look->SetVectorParameterValue(ColorName, FMath::Lerp(CooledRed, PoolGlowColors[Index], Heat));
		}
	}
	// The crust rises out of the melt as it cools: plates growing out into the cracks.
	if (UStaticMeshComponent* Crust = PoolCrust.Get())
	{
		Crust->SetRelativeLocation(FVector(0.0, 0.0, -CrustSunk * (1.f - FMath::SmoothStep(0.2f, 0.9f, Cool))));
	}
	// At the end it sinks away into the ground (model cm by each piece's own height scale).
	const float Sink = 1.f - FMath::Clamp(Fade, 0.f, 1.f);
	for (int32 Index = 0; Index < PoolPieces.Num(); ++Index)
	{
		if (UStaticMeshComponent* Piece = PoolPieces[Index].Get())
		{
			Piece->SetRelativeLocation(PoolPieceRest[Index] - FVector(0.0, 0.0, PoolSink * Piece->GetRelativeScale3D().Z * Sink));
		}
	}
}
