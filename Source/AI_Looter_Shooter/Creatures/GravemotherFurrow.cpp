// AGravemotherFurrow (GravemotherFurrow.h): its line laid on the ground, the heave's crack crawling out with its dust jets,
// the ploughed strip laid as she passes, its standing and fading. The berms, the earth thrown, the crest and the crumble are
// GravemotherFurrowBerms.cpp's.

#include "Creatures/GravemotherFurrow.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GravemotherRubble.h"
#include "Creatures/GroundCrack.h"
#include "World/WorldQueries.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** It looks this far above and below a point for the ground under it (cm). */
	constexpr float GroundReach = 250.f;
	/** The crack's end cap looks only this far above the last tile's ground (cm): a step, not a ledge. */
	constexpr float CapRise = 60.f;
	/** A tile fades in over this long once the split (or her plough) reaches it (s). */
	constexpr float TileFadeIn = 0.12f;
	/** Dust jets from the split's newest stretch every this often (s): a tall and a short one. */
	constexpr float JetEvery = 0.1f;
	/** A cancelled crack fades over this long (s). */
	constexpr float CancelFade = 0.5f;
	/** The crack's stand-in: AGroundCrack this wide (cm), its seam a dark earth (no soul-light). */
	constexpr float StandInCrackWidth = 34.f;
	const FLinearColor StandInSeam(0.035f, 0.028f, 0.02f);
	/** The slam's scar (cm across, MI_Decal_ImpactScar). */
	constexpr float SlamScarSize = 340.f;
	/**
	 * The crack crawls out through each tile with its Reveal (M_GraveDecal: U up to it shows), its front soft over this share
	 * of a tile (34 cm), rather than popping in a tile (2.8 m) at a time; checked by eye 2026-10-20.
	 */
	constexpr float CrackRevealSoft = 0.12f;
	/**
	 * The strips' ends at her end and the far end fade over these shares of a tile (M_GraveDecal's FadeIn/FadeOut): cut
	 * square, the ploughed strip's start stood out as a hard-edged rectangle once she'd left it (2026-10-20 by eye).
	 */
	constexpr float CrackFadeIn = 0.3f;
	constexpr float StripFadeIn = 0.35f;
	constexpr float StripFadeOut = 0.25f;
}

AGravemotherFurrow::AGravemotherFurrow()
{
	PrimaryActorTick.bCanEverTick = true;
	// Nothing spawns on its berms (spawn probes, phase steps), as on rocks.
	Tags.Add(GravemotherAssets::ObstacleTag);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	FrontPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FrontPoint"));
	FrontPoint->SetupAttachment(Root);
	FrontPoint->SetMobility(EComponentMobility::Movable);
	FrontPoint->SetUsingAbsoluteLocation(true);

	// The berms: cover to every query as rocks are (bullets, sight, walking, lobs, her own lane), never the camera.
	BermSet = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Berms"));
	BermSet->SetupAttachment(Root);
	BermSet->SetMobility(EComponentMobility::Movable);
	BermSet->SetUsingAbsoluteLocation(true);
	BermSet->SetUsingAbsoluteRotation(true);
	BermSet->SetUsingAbsoluteScale(true);
	BermSet->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BermSet->SetCollisionObjectType(ECC_WorldStatic);
	BermSet->SetCollisionResponseToAllChannels(ECR_Block);
	BermSet->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BermSet->SetGenerateOverlapEvents(false);
	BermSet->SetCanEverAffectNavigation(false);
	BermSet->ComponentTags.Add(GravemotherAssets::SurfaceDirtTag);
	// A ground decal (a later furrow's crack or strip, a scar) lies on the ground, not over a berm standing on it.
	BermSet->bReceivesDecals = false;

	Crest = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Crest"));
	Crest->SetupAttachment(Root);
	Crest->SetMobility(EComponentMobility::Movable);
	Crest->SetUsingAbsoluteLocation(true);
	Crest->SetUsingAbsoluteRotation(true);
	Crest->SetUsingAbsoluteScale(true);
	Crest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Crest->SetCastShadow(false);
	Crest->SetCanEverAffectNavigation(false);
	Crest->bReceivesDecals = false;
}

AGravemotherFurrow* AGravemotherFurrow::Spawn(UWorld* World, const FVector& InStart, const FVector& InDirection, float InLength,
	const FGravemotherFurrowSettings& InSettings)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGravemotherFurrow* Furrow = World->SpawnActor<AGravemotherFurrow>(AGravemotherFurrow::StaticClass(), FTransform(InStart), Params);
	if (!Furrow)
	{
		return nullptr;
	}
	Furrow->Settings = InSettings;
	Furrow->Start = InStart;
	Furrow->Direction = InDirection.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : InDirection.GetSafeNormal2D();
	Furrow->Across = FVector(-Furrow->Direction.Y, Furrow->Direction.X, 0.0);
	Furrow->Length = FMath::Max(InLength, 1.f);
	Furrow->MakeBermLooks();
	Furrow->Lay();
	if (!Furrow->HasActorBegunPlay())
	{
		Furrow->DispatchBeginPlay();
	}
	Furrow->SetHeave(0.f);
	return Furrow;
}

FVector AGravemotherFurrow::OnGround(const FVector& Point, float Rise) const
{
	const float Over = Rise > 0.f ? Rise : GroundReach;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Point;
	}
	// The terrain and solid props (never volumes, nor any furrow's berms or her rubble: a tile laid over an older furrow's
	// berm sat on its top, 70 cm over the ground, and its decal never reached the ground under it).
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("GravemotherFurrow"), this);
	for (TActorIterator<AGravemotherFurrow> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	for (TActorIterator<AGravemotherRubble> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, Over), Point - FVector(0.0, 0.0, GroundReach),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return Hit.ImpactPoint;
	}
	return Point;
}

void AGravemotherFurrow::Lay()
{
	const float Yaw = static_cast<float>(Direction.Rotation().Yaw);
	// Each furrow starts its crack's and its strip's pictures somewhere new (both tile along U, so every tile of one furrow
	// with the same start still joins up).
	const float CrackShift = FMath::FRand();
	const float StripShift = FMath::FRand();
	// The crack: tiles end to end along the line, the last one its end cap.
	const int32 CrackCount = FMath::Max(1, FMath::CeilToInt32(Length / CrackTileLength));
	const float CrackStep = Length / CrackCount;
	for (int32 Index = 0; Index < CrackCount; ++Index)
	{
		FTile& Tile = CrackTiles.AddDefaulted_GetRef();
		Tile.Along = CrackStep * (static_cast<float>(Index) + 0.5f);
		Tile.Center = OnGround(Start + Direction * Tile.Along);
		Tile.Decal = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalHeaveCrack, Tile.Center, Yaw, CrackStep, CrackWidth, 1);
		GravemotherAssets::SetDecalShift(Tile.Decal, CrackShift);
		GravemotherAssets::SetDecalReveal(Tile.Decal, 0.f, CrackRevealSoft);
	}
	if (CrackTiles.Num() > 0)
	{
		// Out from under her forelegs it fades in; the far end has its cap.
		GravemotherAssets::SetDecalEnds(CrackTiles[0].Decal, CrackFadeIn, 0.f);
	}
	// The cap a little past the line's end, on the ground at the last tile's level: the line stops short of a rise, so the
	// floor a ledge over it would give is not its ground (it once lay on a rock 1.8 m up, its decal reaching nothing).
	FVector CapAt = Start + Direction * (Length + CrackWidth * 0.3f);
	CapAt.Z = CrackTiles.Num() > 0 ? CrackTiles.Last().Center.Z : CapAt.Z;
	EndCap = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalHeaveCrackEnd, OnGround(CapAt, CapRise), Yaw, CrackWidth, CrackWidth, 1);
	if (CrackTiles.Num() > 0 && !CrackTiles[0].Decal && GetWorld())
	{
		// The stand-in: today's crack, its seam dark earth rather than soul-light.
		StandInCrack = AGroundCrack::Spawn(GetWorld(), Start, Start + Direction * Length, StandInCrackWidth, StandInSeam, FMath::Rand());
	}

	// The ploughed strip: tiles laid as she passes.
	const int32 StripCount = FMath::Max(1, FMath::CeilToInt32(Length / StripTileLength));
	const float StripStep = Length / StripCount;
	for (int32 Index = 0; Index < StripCount; ++Index)
	{
		FTile& Tile = StripTiles.AddDefaulted_GetRef();
		Tile.Along = StripStep * (static_cast<float>(Index) + 0.5f);
		Tile.Center = OnGround(Start + Direction * Tile.Along);
		Tile.Decal = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalFurrow, Tile.Center, Yaw, StripStep, StripWidth, 0);
		GravemotherAssets::SetDecalShift(Tile.Decal, StripShift);
	}
	if (StripTiles.Num() > 0)
	{
		// Both its ends fade (one tile: in and out).
		GravemotherAssets::SetDecalEnds(StripTiles[0].Decal, StripFadeIn, StripTiles.Num() == 1 ? StripFadeOut : 0.f);
		if (StripTiles.Num() > 1)
		{
			GravemotherAssets::SetDecalEnds(StripTiles.Last().Decal, 0.f, StripFadeOut);
		}
	}

	// The berms' spots, both sides, every BermEvery from her forelegs to the line's end: turned a little and sized apart.
	const float Every = FMath::Max(Settings.BermEvery, 20.f);
	for (float Along = 0.f; Along <= Length + 1.f; Along += Every)
	{
		for (const float Side : { -1.f, 1.f })
		{
			FBerm& Berm = Berms.AddDefaulted_GetRef();
			Berm.Along = Along;
			Berm.Side = Side;
			Berm.Ground = OnGround(Start + Direction * Along + Across * (Side * Settings.BermOffset));
			// SM_GraveBerm's steeper face is its -Y: turned to the line on both sides (the cut she ploughed), the gentle
			// slope outward.
			Berm.Yaw = Yaw + (Side < 0.f ? 180.f : 0.f) + FMath::FRandRange(-5.f, 5.f);
			Berm.Height = FMath::FRandRange(0.88f, 1.12f);
		}
	}
}

void AGravemotherFurrow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AGravemotherFurrow::Advance(float DeltaSeconds)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	PhaseTime += Dt;
	switch (Phase)
	{
	case EPhase::Heave:
		TickHeave(Dt);
		break;
	case EPhase::Plough:
		TickPlough(Dt);
		break;
	case EPhase::Standing:
		if (PhaseTime >= Settings.BermSeconds)
		{
			BeginCrumble();
		}
		break;
	case EPhase::Crumbling:
		TickCrumble(Dt);
		return;
	case EPhase::Cancelled:
		FadeDecals(1.f - PhaseTime / CancelFade);
		if (PhaseTime >= CancelFade)
		{
			Destroy();
		}
		return;
	}
	TickBerms(Dt);
	// Tiles reached fade in.
	for (TArray<FTile>* Tiles : { &CrackTiles, &StripTiles })
	{
		for (FTile& Tile : *Tiles)
		{
			if (Tile.bLaid && Tile.Shown < 1.f)
			{
				Tile.Shown = FMath::Min(1.f, Tile.Shown + Dt / TileFadeIn);
				GravemotherAssets::SetDecalOpacity(Tile.Decal, Tile.Shown);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// The heave
// ---------------------------------------------------------------------------

void AGravemotherFurrow::SetHeave(float InFront)
{
	Front = FMath::Clamp(InFront, 0.f, Length);
	FrontPoint->SetWorldLocation(OnGround(Start + Direction * Front));
	const float Half = Length / FMath::Max(CrackTiles.Num(), 1) * 0.5f;
	for (FTile& Tile : CrackTiles)
	{
		Tile.bLaid |= Front > 0.f && Tile.Along - Half <= Front;
		// The split crawls through the tile with its front (U runs the line's way: from her end toward the far end), the
		// soft front kept just behind it; a tile it's past shows whole.
		const float Through = (Front - (Tile.Along - Half)) / FMath::Max(Half * 2.f, 1.f);
		GravemotherAssets::SetDecalReveal(Tile.Decal, Through >= 1.f ? 1.f : Through - CrackRevealSoft * 0.5f);
	}
	if (Front >= Length - 1.f && EndCap)
	{
		GravemotherAssets::SetDecalOpacity(EndCap, 1.f);
	}
	if (AGroundCrack* Crack = StandInCrack.Get())
	{
		Crack->SetOpen(Front / Length);
	}
	PlaceCrest();
}

void AGravemotherFurrow::TickHeave(float DeltaSeconds)
{
	if (Front <= 0.f)
	{
		return;
	}
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	JetIn -= DeltaSeconds;
	if (!Effects || JetIn > 0.f)
	{
		return;
	}
	JetIn += JetEvery;
	// Dust jetting from the split's newest stretch (the sheet's 34 x 100 to 70 x 210 cm), a chunk tossed, the floor's dust.
	const FVector Newest = OnGround(Start + Direction * FMath::Max(Front - FMath::FRandRange(0.f, 120.f), 0.f));
	Effects->Jet(Newest + Across * FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(55.f, 70.f), FMath::FRandRange(170.f, 210.f), 0.7f);
	Effects->Jet(Newest + Direction * 40.f + Across * FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(30.f, 40.f), FMath::FRandRange(90.f, 110.f), 0.6f);
	Effects->Chunks(EGraveChunk::Debris, FrontPoint->GetComponentLocation() + FVector(0.0, 0.0, 10.0), FVector::UpVector, 1, 380.f, 0.8f);
	FGraveDustBurst Edge;
	Edge.Kind = EGraveDust::Floor;
	Edge.Count = 2;
	Edge.Spread = 40.f;
	Edge.SpreadUp = 5.f;
	Edge.StartSize = 45.f;
	Edge.EndSize = 120.f;
	Edge.Life = 1.1f;
	Edge.Opacity = 0.25f;
	Edge.Drift = FVector(0.0, 0.0, 20.0);
	Edge.Outward = 70.f;
	Effects->Puffs(FrontPoint->GetComponentLocation(), Edge);
}

// ---------------------------------------------------------------------------
// The plough
// ---------------------------------------------------------------------------

void AGravemotherFurrow::BeginPlough()
{
	if (Phase != EPhase::Heave)
	{
		return;
	}
	SetHeave(Length);
	Phase = EPhase::Plough;
	PhaseTime = 0.f;
	bPloughed = true;
	Crest->ClearInstances();
}

void AGravemotherFurrow::SetPlough(float InCovered)
{
	if (Phase != EPhase::Plough)
	{
		return;
	}
	Covered = FMath::Clamp(InCovered, 0.f, Length + Settings.RearLag);
	// The strip is laid as she passes; the berms rise once her body is past them.
	for (FTile& Tile : StripTiles)
	{
		Tile.bLaid |= Tile.Along <= Covered;
	}
	RaiseBerms();
}

void AGravemotherFurrow::TickPlough(float DeltaSeconds)
{
	// Earth thrown out both sides of her as she goes (GravemotherFurrowBerms.cpp).
	SprayIn -= DeltaSeconds;
	if (SprayIn <= 0.f)
	{
		SprayIn += 0.06f;
		ThrowEarthAt(FMath::Min(Covered, Length));
	}
}

void AGravemotherFurrow::EndPlough(const FVector& SlamAt, float SlamRadius)
{
	if (Phase == EPhase::Heave)
	{
		// Slammed down before she ran (no room): the line stands as it is, with no berms.
		bPloughed = true;
	}
	else if (Phase != EPhase::Plough)
	{
		return;
	}
	// Every berm her body got past rises; the rest of the line has none. Then they stand their time.
	SetPlough(Covered + Settings.RearLag);
	Phase = EPhase::Standing;
	PhaseTime = 0.f;
	Scar = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalImpactScar, OnGround(SlamAt), FMath::FRandRange(0.f, 360.f),
		FMath::Max(SlamScarSize, SlamRadius * 2.f), FMath::Max(SlamScarSize, SlamRadius * 2.f), 2, 200.f);
	GravemotherAssets::SetDecalOpacity(Scar, 1.f);
	if (AGroundCrack* Crack = StandInCrack.Get())
	{
		Crack->Burst(SlamAt, SlamRadius);
		Crack->Close(Settings.BermSeconds);
	}
}

void AGravemotherFurrow::Cancel()
{
	if (Phase != EPhase::Heave)
	{
		return;
	}
	Phase = EPhase::Cancelled;
	PhaseTime = 0.f;
	Crest->ClearInstances();
	if (AGroundCrack* Crack = StandInCrack.Get())
	{
		Crack->Close(0.f);
	}
}

void AGravemotherFurrow::FadeDecals(float Opacity)
{
	const float Shown = FMath::Clamp(Opacity, 0.f, 1.f);
	for (TArray<FTile>* Tiles : { &CrackTiles, &StripTiles })
	{
		for (FTile& Tile : *Tiles)
		{
			if (Tile.bLaid)
			{
				GravemotherAssets::SetDecalOpacity(Tile.Decal, FMath::Min(Tile.Shown, Shown));
			}
		}
	}
	if (EndCap && Front >= Length - 1.f)
	{
		GravemotherAssets::SetDecalOpacity(EndCap, Shown);
	}
	GravemotherAssets::SetDecalOpacity(Scar, Shown);
}

int32 AGravemotherFurrow::NumDecals() const
{
	int32 Count = (EndCap ? 1 : 0) + (Scar ? 1 : 0);
	for (const TArray<FTile>* Tiles : { &CrackTiles, &StripTiles })
	{
		for (const FTile& Tile : *Tiles)
		{
			Count += Tile.Decal ? 1 : 0;
		}
	}
	return Count;
}
