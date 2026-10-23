#include "Weapons/WeaponFXImpactLooks.h"
#include "AI_Looter_Shooter.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Templates/UnrealTemplate.h"

// The gun feel pass (Docs/Work/2026-10-22-Gunfeel/PLAN.md, GF-IMPACT): what a gun's kind adds to its round's world hit, on
// top of the surface's own look (WeaponFXImpacts.cpp, already sized by the kind's weight). The user found every gun's hit
// alike; each kind now lands its own way:
//  - a sniper's round punches (weight from Looter.Vfx.Impact.PunchFrom): a faint ring of air and a ring of dust snapping out
//    over the surface, a tall column thrown up, big chunks you watch fall (sparks off metal) - it reads at 40 m;
//  - a heavy round (a revolver's slug) on a hard surface, and an LMG's every third round on anything, throw an extra burst
//    of what they struck;
//  - a shotgun's pellets gather (Sheet) and their blast lays one dust sheet across its pattern's middle once its last
//    pellet is down: soft ground a fan of it thrown up, hard surfaces a scatter of sparks, water a fan of spray.
// All code-drawn quads and chunks on FWeaponFX's components (the engine Plane, never Nanite), no lights; thinned far off
// like the rest (Detail).

using namespace ImpactLooks;

namespace
{
	// A punch (numbers at a sniper round's size; the ring and column grow with Size).
	/** The ring of air over the surface: from, to (cm), thickness, strength (faint: air, not fire), life. */
	constexpr float PunchRingFrom = 6.f;
	constexpr float PunchRingTo = 70.f;
	constexpr float PunchRingWidth = 6.f;
	constexpr float PunchRingGlow = 5.f;
	constexpr float PunchRingLife = 0.16f;
	/** The dust ring rolling out over the surface: puffs and their speed (cm/s). */
	constexpr int32 PunchSkirt = 8;
	constexpr float PunchSkirtSpeed = 420.f;
	/** The tall column: billows thrown up the normal (cm/s; a rifle's dense geyser is 520). */
	constexpr int32 PunchColumn = 3;
	constexpr float PunchColumnSpeed = 760.f;
	/** Big chunks thrown high, and their size (cm). */
	constexpr int32 PunchChunks = 4;
	constexpr float PunchChunkSize[2] = { 8.f, 12.f };

	/** A chunk burst's pieces (at Amount 1), thrown high. */
	constexpr int32 ChunkBurstPieces = 6;

	// A blast's sheet.
	/** The fan's billows (soft ground) and the scatter's sparks (hard), at a full sheet. */
	constexpr int32 SheetFan = 5;
	constexpr int32 SheetSparks = 14;
	constexpr int32 SheetClods = 6;
	constexpr int32 SheetDrops = 14;
}

double FWeaponFX::EffectsNow() const
{
	const UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	return World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
}

void FWeaponFX::SpawnImpactKindLayers(const FImpactSpot& Spot, const FVfxImpactLook& Look)
{
	// What landed how (log LogLooter Verbose): the play-check's way to tell a missing look from one grabbed too late.
	UE_LOG(LogLooter, Verbose, TEXT("Impact: kind %d on %s, %.0f m off, pieces x%.2f, size x%.2f, detail %.2f%s%s%s"),
		static_cast<int32>(LandingRound.Kind), ImpactSurface::Name(Spot.Material),
		bHasCamera ? static_cast<float>(FVector::Dist(Spot.Location, LastCamera)) / 100.f : 0.f, Look.Count, Look.Size, Spot.Detail,
		Look.bPunch ? TEXT(", punch") : TEXT(""), Look.bChunks ? TEXT(", chunks") : TEXT(""), Look.bPellet ? TEXT(", pellet") : TEXT(""));
	if (Look.bPellet)
	{
		// One blast at a time: a new blast's first pellet lays the last one's sheet (its last pellet is long down).
		if (Sheet.Hits > 0 && Sheet.Volley != LandingRound.Volley)
		{
			SpawnBlastSheet(FVfxTuning::Live());
			Sheet.Reset();
		}
		if (Sheet.Hits == 0)
		{
			Sheet.Volley = LandingRound.Volley;
			Sheet.Started = EffectsNow();
			Sheet.bFloor = Spot.bFloor;
			Sheet.Floor = Spot.Floor;
		}
		Sheet.Add(Spot.Location, Spot.Up, Spot.Shot, Spot.Material);
		return;
	}
	const float Extra = FVfxTuning::Live().ImpactScale * Spot.Detail;
	if (Look.bChunks && Extra > 0.f)
	{
		SpawnImpactChunks(Spot, Extra);
	}
	if (Look.bPunch && Extra > 0.f)
	{
		SpawnImpactPunch(Spot);
	}
}

void FWeaponFX::SpawnImpactChunks(const FImpactSpot& Spot, float Amount)
{
	const int32 Count = VfxRules::Count(ChunkBurstPieces, Amount);
	if (Count <= 0)
	{
		return;
	}
	// Thrown high off the face, a little along the bounce: the bits you see fly from a heavy round's hit.
	auto Way = [this, &Spot]() { return (Spot.Up * 1.1f + Spot.Bounce * 0.35f + Random.GetUnitVector() * 0.55f).GetSafeNormal(); };
	auto Bits = [this, &Spot, &Way, Count](uint8 Set, float MinSize, float MaxSize, float MinSpeed, float MaxSpeed, float Drag)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FParticle& Bit = AddBit(Set, Spot.Point, Way() * Random.FRandRange(MinSpeed, MaxSpeed) * Spot.Reach,
				Random.FRandRange(MinSize, MaxSize) * Spot.Seen, Random.FRandRange(0.9f, 1.3f));
			Bit.Drag = Drag;
		}
	};
	switch (Spot.Material)
	{
	case EImpactMaterial::Gravel:
	case EImpactMaterial::Stone:
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FParticle& Chip = AddParticle(EParticle::Chip, Spot.Point, Way() * Random.FRandRange(320.f, 720.f) * Spot.Reach, Random.FRandRange(0.8f, 1.2f));
			Chip.StartSize = Random.FRandRange(4.f, 7.f) * Spot.Seen;
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
		}
		break;
	case EImpactMaterial::Wood:
		Bits(BitSetFor(EBitShape::Splinter, ImpactSplinterColor, 0.f), 5.f, 8.5f, 300.f, 680.f, 0.3f);
		break;
	case EImpactMaterial::Metal:
	case EImpactMaterial::SheetMetal:
		// Steel doesn't chip: more hot sparks, a few that bounce on.
		for (int32 Index = 0; Index < Count * 2; ++Index)
		{
			FParticle& Spark = AddParticle(EParticle::Spark, Spot.Point, Way() * Random.FRandRange(500.f, 1500.f), Random.FRandRange(0.15f, 0.5f));
			Spark.Color = ImpactHotSparkColor;
			Spark.Intensity = Random.FRandRange(24.f, 36.f);
			Spark.StartSize = Random.FRandRange(0.9f, 1.4f);
			Spark.Drag = 1.5f;
		}
		break;
	case EImpactMaterial::Glass:
		Bits(BitSetFor(EBitShape::Plate, ImpactShardColor, 0.35f), 1.6f, 3.6f, 260.f, 640.f, 0.2f);
		break;
	case EImpactMaterial::Bone:
		Bits(BitSetFor(EBitShape::Plate, ImpactBoneColor, 0.f), 1.6f, 3.2f, 260.f, 600.f, 0.3f);
		break;
	case EImpactMaterial::Hay:
		Bits(BitSetFor(EBitShape::Splinter, ImpactStrawColor, 0.f), 3.f, 5.f, 200.f, 480.f, 1.2f);
		break;
	case EImpactMaterial::Cloth:
		Bits(BitSetFor(EBitShape::Plate, ImpactLintColor, 0.f), 1.f, 2.f, 100.f, 240.f, 2.5f);
		break;
	case EImpactMaterial::Water:
		Bits(BitSetFor(EBitShape::Round, ImpactDropColor, 0.15f), 1.8f, 3.2f, 450.f, 900.f, 0.2f);
		break;
	case EImpactMaterial::Grass:
		Bits(BitSetFor(EBitShape::Splinter, ImpactTurfColor, 0.f), 3.f, 5.f, 260.f, 560.f, 1.f);
		break;
	case EImpactMaterial::Dirt:
	case EImpactMaterial::Sand:
	default:
		Bits(BitSetFor(EBitShape::Plate, ImpactClodColor, 0.f), 4.f, 7.f, 300.f, 620.f, 0.25f);
		break;
	}
}

void FWeaponFX::SpawnImpactPunch(const FImpactSpot& Spot)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	const float Amount = Vfx.ImpactScale * Spot.Detail;
	// A ring of air snapping out over the surface (the round's blow, square to it; never thinned: it reads the hit far off).
	// Its width as the impact pieces are drawn: wider far off (5 cm was a pixel at 35 m; the play-check, 2026-10-23).
	SpawnShockRing(Spot.Point, Spot.Up, PunchRingFrom * Spot.Size, PunchRingTo * Spot.Size, PunchRingWidth * Spot.Seen / FMath::Max(Spot.Size, 0.1f),
		ImpactPunchColor,
		PunchRingGlow * Vfx.Glow * Vfx.ImpactScale, PunchRingLife);
	// ...and a ring of its dust rolling out after it.
	const FLinearColor Dust = DustOf(Spot.Material) * Spot.Shade;
	const int32 Skirt = VfxRules::Count(PunchSkirt, Amount);
	for (int32 Index = 0; Index < Skirt; ++Index)
	{
		const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.3f, 0.3f)) / FMath::Max(Skirt, 1);
		const FVector Out = Spot.SideA * FMath::Cos(Angle) + Spot.SideB * FMath::Sin(Angle);
		FParticle& Puff = AddParticle(EParticle::Smoke, Spot.Point + Out * 8.f * Spot.Seen, (Out * PunchSkirtSpeed + Spot.Up * 40.f) * Spot.Reach,
			Random.FRandRange(1.f, 1.4f));
		Puff.Color = Dust;
		Puff.Intensity = FMath::Min(0.45f * Vfx.Smoke, 1.f);
		Puff.StartSize = 18.f * Spot.Seen;
		Puff.EndSize = 85.f * Spot.Seen * Random.FRandRange(0.85f, 1.15f);
		Puff.Gravity = -0.02f;
		Puff.Drag = 4.f;
	}
	// The column: off metal a fountain of sparks, off water a spout, off anything else billows of it thrown tall.
	const int32 Column = VfxRules::Count(PunchColumn, Amount);
	if (Spot.Material == EImpactMaterial::Metal || Spot.Material == EImpactMaterial::SheetMetal)
	{
		for (int32 Index = 0; Index < Column * 6; ++Index)
		{
			const FVector Way = (Spot.Up + Random.GetUnitVector() * 0.35f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, Spot.Point, Way * Random.FRandRange(700.f, 1600.f), Random.FRandRange(0.25f, 0.6f));
			Spark.Color = ImpactHotSparkColor;
			Spark.Intensity = Random.FRandRange(26.f, 38.f);
			Spark.StartSize = Random.FRandRange(1.f, 1.5f);
			Spark.Drag = 1.2f;
		}
	}
	else if (Spot.Material == EImpactMaterial::Water)
	{
		const uint8 Drops = BitSetFor(EBitShape::Round, ImpactDropColor, 0.15f);
		for (int32 Index = 0; Index < Column * 4; ++Index)
		{
			const FVector Way = (Spot.Up + Random.GetUnitVector() * 0.12f).GetSafeNormal();
			AddBit(Drops, Spot.Point, Way * Random.FRandRange(800.f, 1300.f) * Spot.Reach, Random.FRandRange(2.f, 3.4f) * Spot.Size,
				Random.FRandRange(0.8f, 1.1f));
		}
	}
	else
	{
		for (int32 Index = 0; Index < Column; ++Index)
		{
			const float Slower = 1.f - 0.4f * static_cast<float>(Index) / FMath::Max(Column, 1);
			FVfxFlame Billow;
			Billow.Location = Spot.Point;
			Billow.Velocity = (Spot.Up + Random.GetUnitVector() * 0.1f).GetSafeNormal() * PunchColumnSpeed * Slower * Spot.Reach;
			Billow.StartSize = 26.f * Spot.Seen;
			Billow.EndSize = 120.f * Spot.Seen * Random.FRandRange(0.85f, 1.15f);
			Billow.Smoke = ThrownOf(Spot.Material) * Spot.Shade;
			Billow.Opacity = FMath::Min(0.95f * Vfx.Smoke, 1.f);
			Billow.Life = 1.4f * Random.FRandRange(0.9f, 1.1f);
			Billow.Burn = 0.f;
			Billow.Gravity = 0.35f;
			Billow.Drag = 3.f;
			SpawnFlame(Billow);
		}
	}
	// Big chunks thrown high, a little along the bounce (stone and gravel as chips; the rest as the chunk burst's pieces, bigger).
	const int32 Chunks = VfxRules::Count(PunchChunks, Amount);
	for (int32 Index = 0; Index < Chunks && Spot.Material != EImpactMaterial::Water && Spot.Material != EImpactMaterial::Metal
		&& Spot.Material != EImpactMaterial::SheetMetal; ++Index)
	{
		const FVector Way = (Spot.Up * 1.3f + Spot.Bounce * 0.3f + Random.GetUnitVector() * 0.45f).GetSafeNormal();
		const FVector Velocity = Way * Random.FRandRange(420.f, 700.f) * Spot.Reach;
		// Their size as given (already a sniper round's), drawn bigger far off as every impact piece is.
		const float ChunkSize = Random.FRandRange(PunchChunkSize[0], PunchChunkSize[1]) * Spot.Seen / FMath::Max(Spot.Size, 0.1f);
		if (Spot.Material == EImpactMaterial::Stone || Spot.Material == EImpactMaterial::Gravel)
		{
			FParticle& Chip = AddParticle(EParticle::Chip, Spot.Point, Velocity, Random.FRandRange(1.f, 1.4f));
			Chip.StartSize = ChunkSize;
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(6.f, 14.f);
			continue;
		}
		const bool bSplinters = Spot.Material == EImpactMaterial::Wood || Spot.Material == EImpactMaterial::Hay;
		const FLinearColor Color = Spot.Material == EImpactMaterial::Wood ? ImpactSplinterColor
			: Spot.Material == EImpactMaterial::Hay ? ImpactStrawColor
			: Spot.Material == EImpactMaterial::Grass ? ImpactTurfColor
			: Spot.Material == EImpactMaterial::Bone ? ImpactBoneColor
			: Spot.Material == EImpactMaterial::Glass ? ImpactShardColor
			: ImpactClodColor;
		FParticle& Chunk = AddBit(BitSetFor(bSplinters ? EBitShape::Splinter : EBitShape::Plate, Color, 0.f), Spot.Point, Velocity, ChunkSize,
			Random.FRandRange(1.f, 1.4f));
		Chunk.Drag = 0.2f;
	}
}

bool FWeaponFX::SettleBlastSheet(bool bStillFlying)
{
	if (Sheet.Hits <= 0)
	{
		return false;
	}
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (bStillFlying && EffectsNow() - Sheet.Started < Vfx.ImpactSheetWait)
	{
		return false;
	}
	SpawnBlastSheet(Vfx);
	Sheet.Reset();
	return true;
}

void FWeaponFX::SpawnBlastSheet(const FVfxTuning& Vfx)
{
	const EImpactMaterial Material = Sheet.Material();
	const EVfxSheet Kind = VfxRules::SheetOf(Material);
	if (Sheet.Hits < Vfx.ImpactSheetMin || Kind == EVfxSheet::None || Vfx.ImpactSheet <= 0.f || Vfx.ImpactScale <= 0.f)
	{
		return;
	}
	// Its pieces land on the ground under the blast's first pellet, as that pellet's did.
	TGuardValue<TOptional<FPlane>> FloorDuringSpawn(SpawnFloor, Sheet.bFloor ? TOptional<FPlane>(Sheet.Floor) : TOptional<FPlane>());
	const FVector Up = Sheet.Normal();
	const FVector Center = Sheet.Centroid() + Up * 3.f;
	const FVector Bounce = Sheet.Shot().MirrorByVector(Up);
	const float Radius = VfxRules::SheetRadius(Sheet.Spread());
	const float Amount = Vfx.ImpactSheet * Vfx.ImpactScale * DetailAt(Center);
	const float View = VfxRules::ImpactViewScale(bHasCamera ? static_cast<float>(FVector::Dist(Center, LastCamera)) : 0.f);
	const FLinearColor Shade = ImpactShade(Center);
	FVector SideA, SideB;
	Up.FindBestAxisVectors(SideA, SideB);
	// Across the bounce, on the surface: the fan opens that way.
	FVector Across = FVector::CrossProduct(Up, Bounce).GetSafeNormal();
	if (Across.IsNearlyZero())
	{
		Across = SideA;
	}
	auto OnSheet = [this, &Center, &SideA, &SideB, Radius](float Share)
	{
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Out = Radius * Share * FMath::Sqrt(Random.FRand());
		return Center + (SideA * FMath::Cos(Angle) + SideB * FMath::Sin(Angle)) * Out;
	};

	// The sheet: a few low, wide puffs of its dust across the pattern, drifting out and hanging.
	const FLinearColor Dust = DustOf(Material) * Shade;
	const int32 Puffs = VfxRules::Count(VfxRules::SheetPuffs(Radius), Amount);
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		const FVector Where = OnSheet(0.6f);
		const FVector Out = (Where - Center).GetSafeNormal();
		FParticle& Puff = AddParticle(EParticle::Smoke, Where, Out * 70.f + Up * 30.f + Bounce * 40.f, Random.FRandRange(1.3f, 1.7f));
		Puff.Color = Dust;
		Puff.Intensity = FMath::Min(0.42f * Vfx.Smoke, 1.f);
		Puff.StartSize = Radius * 0.45f * View;
		Puff.EndSize = Radius * 1.1f * View * Random.FRandRange(0.85f, 1.15f);
		Puff.Gravity = -0.02f;
		Puff.Drag = 2.5f;
	}

	switch (Kind)
	{
	case EVfxSheet::Soft:
	{
		// A fan of it thrown up and along the bounce, opening across the pattern; clods of it tossed in the fan.
		const int32 Fan = VfxRules::Count(SheetFan, Amount);
		for (int32 Index = 0; Index < Fan; ++Index)
		{
			const float Along = Fan > 1 ? (static_cast<float>(Index) / (Fan - 1) - 0.5f) * 1.2f : 0.f;
			FVfxFlame Billow;
			Billow.Location = Center + Across * (Along * Radius);
			Billow.Velocity = (Up * 0.9f + Bounce * 0.5f + Across * Along * 0.4f + Random.GetUnitVector() * 0.12f).GetSafeNormal() * 430.f;
			Billow.StartSize = 22.f * View;
			Billow.EndSize = (60.f + 0.35f * Radius) * View * Random.FRandRange(0.85f, 1.15f);
			Billow.Smoke = ThrownOf(Material) * Shade;
			Billow.Opacity = FMath::Min(0.85f * Vfx.Smoke, 1.f);
			Billow.Life = 1.1f * Random.FRandRange(0.9f, 1.1f);
			Billow.Burn = 0.f;
			Billow.Gravity = 0.35f;
			Billow.Drag = 3.2f;
			SpawnFlame(Billow);
		}
		const bool bWoody = Material == EImpactMaterial::Wood || Material == EImpactMaterial::Hay;
		const FLinearColor Color = Material == EImpactMaterial::Wood ? ImpactSplinterColor : Material == EImpactMaterial::Hay ? ImpactStrawColor
			: Material == EImpactMaterial::Grass ? ImpactTurfColor : ImpactClodColor;
		const uint8 Set = BitSetFor(bWoody ? EBitShape::Splinter : EBitShape::Plate, Color, 0.f);
		for (int32 Index = 0; Index < VfxRules::Count(SheetClods, Amount); ++Index)
		{
			const FVector Way = (Up + Bounce * 0.5f + Random.GetUnitVector() * 0.5f).GetSafeNormal();
			FParticle& Clod = AddBit(Set, OnSheet(0.8f), Way * Random.FRandRange(260.f, 560.f), Random.FRandRange(3.5f, 6.f) * View,
				Random.FRandRange(0.9f, 1.2f));
			Clod.Drag = 0.3f;
		}
		break;
	}
	case EVfxSheet::Hard:
	{
		// A scatter of sparks across the pattern, each off its own spot (metal hotter and more).
		const bool bMetal = Material == EImpactMaterial::Metal || Material == EImpactMaterial::SheetMetal;
		const int32 Count = VfxRules::Count(bMetal ? SheetSparks + 6 : SheetSparks, Amount);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Way = (Up * 0.8f + Bounce * 0.5f + Random.GetUnitVector() * 0.6f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, OnSheet(1.f), Way * Random.FRandRange(350.f, 1000.f), Random.FRandRange(0.1f, 0.3f));
			Spark.Color = bMetal ? ImpactHotSparkColor : ImpactSparkColor;
			Spark.Intensity = Random.FRandRange(20.f, 30.f);
			Spark.StartSize = Random.FRandRange(0.8f, 1.3f);
			Spark.Drag = 1.5f;
		}
		break;
	}
	case EVfxSheet::Splash:
	{
		// A fan of spray thrown up off the water across the pattern.
		const uint8 Drops = BitSetFor(EBitShape::Round, ImpactDropColor, 0.15f);
		for (int32 Index = 0; Index < VfxRules::Count(SheetDrops, Amount); ++Index)
		{
			const FVector Way = (Up + Bounce * 0.4f + Random.GetUnitVector() * 0.3f).GetSafeNormal();
			AddBit(Drops, OnSheet(0.8f), Way * Random.FRandRange(380.f, 800.f), Random.FRandRange(1.4f, 2.8f), Random.FRandRange(0.6f, 0.9f));
		}
		break;
	}
	default:
		break;
	}
}
