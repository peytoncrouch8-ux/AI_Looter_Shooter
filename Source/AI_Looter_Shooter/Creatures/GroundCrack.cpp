#include "Creatures/GroundCrack.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The engine cube the pieces are made of is this big (cm). */
	constexpr float CubeSize = 100.f;

	/** A piece of the line is this many times the crack's width long, and the line wanders this many widths off it. */
	constexpr float PieceLength = 2.2f;
	constexpr float Wander = 0.45f;
	constexpr int32 MaxLinePieces = 48;

	/** A twig splits off every few pieces of the line, this many widths long in two pieces. */
	constexpr int32 TwigEvery = 3;
	constexpr float TwigLength = 2.5f;

	/** How much of the line a piece takes to open fully, as a share of the line (a soft front rather than a pop). */
	constexpr float OpenFade = 0.06f;

	/** The lips: dark, flush with the ground, sunk a little into it; the seam: narrower, glowing, standing a hair above them (cm). */
	constexpr float LipHeight = 5.f;
	constexpr float LipLift = 0.5f;
	constexpr float SeamHeight = 5.f;
	constexpr float SeamLift = 1.5f;
	constexpr float SeamShare = 0.4f;

	/** The lips' color (near-black earth) and how brightly the seam glows (M_StylizedSurface's emissive multiplier of its color). */
	const FLinearColor LipsColor(0.022f, 0.017f, 0.013f);
	constexpr float SeamGlow = 3.f;

	/** It looks this far above and below a point for the ground under it (cm). */
	constexpr float GroundReach = 200.f;

	/** Writes a layer's pieces: in place while the count holds, else laid out afresh. */
	void WritePieces(UInstancedStaticMeshComponent& Layer, const TArray<FTransform>& Shapes)
	{
		if (Layer.GetInstanceCount() != Shapes.Num())
		{
			Layer.ClearInstances();
			if (Shapes.Num() > 0)
			{
				Layer.AddInstances(Shapes, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
		}
		else if (Shapes.Num() > 0)
		{
			Layer.BatchUpdateInstancesTransforms(0, Shapes, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		Layer.MarkRenderStateDirty();
	}
}

AGroundCrack::AGroundCrack()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(
		TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	auto MakeLayer = [this, Root](const TCHAR* Name, UStaticMesh* Mesh)
	{
		UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Layer->SetupAttachment(Root);
		Layer->SetMobility(EComponentMobility::Movable);
		Layer->SetStaticMesh(Mesh);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetCastShadow(false);
		Layer->SetCanEverAffectNavigation(false);
		Layer->bReceivesDecals = false;
		return Layer;
	};
	Lips = MakeLayer(TEXT("Lips"), Cube.Object);
	Seam = MakeLayer(TEXT("Seam"), Cube.Object);
	SurfaceBase = Surface.Object;
}

AGroundCrack* AGroundCrack::Spawn(UWorld* World, const FVector& Start, const FVector& End, float Width, const FLinearColor& InGlowColor,
	int32 Seed)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGroundCrack* Crack = World->SpawnActor<AGroundCrack>(AGroundCrack::StaticClass(), FTransform(Start), Params);
	if (!Crack)
	{
		return nullptr;
	}
	// Its looks are made here rather than as play begins, so a crack laid in a level that isn't playing (a test's) shows too.
	if (Crack->SurfaceBase)
	{
		Crack->LipsLook = UMaterialInstanceDynamic::Create(Crack->SurfaceBase, Crack);
		Crack->LipsLook->SetVectorParameterValue(TEXT("Color"), LipsColor);
		Crack->LipsLook->SetScalarParameterValue(TEXT("Glow"), 0.f);
		Crack->LipsLook->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Crack->Lips->SetMaterial(0, Crack->LipsLook);
		Crack->SeamLook = UMaterialInstanceDynamic::Create(Crack->SurfaceBase, Crack);
		Crack->SeamLook->SetVectorParameterValue(TEXT("Color"), InGlowColor);
		Crack->SeamLook->SetScalarParameterValue(TEXT("Glow"), SeamGlow);
		Crack->SeamLook->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Crack->Seam->SetMaterial(0, Crack->SeamLook);
	}
	Crack->Lay(Start, End, FMath::Max(Width, 4.f), Seed);
	Crack->Draw();
	return Crack;
}

// ---------------------------------------------------------------------------
// Its shape
// ---------------------------------------------------------------------------

FVector AGroundCrack::OnGround(const FVector& Point) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Point;
	}
	// The terrain and solid props, never volumes or what a scatter put there (it runs under a rock, not over it).
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("GroundCrack"), this);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, GroundReach), Point - FVector(0.0, 0.0, GroundReach),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return Hit.ImpactPoint;
	}
	return Point;
}

void AGroundCrack::Lay(const FVector& Start, const FVector& End, float Width, int32 Seed)
{
	Random.Initialize(Seed);
	Points.Reset();
	Pieces.Reset();
	const FVector Line = End - Start;
	const double Length = Line.Size2D();
	const FVector Ahead = Length > 1.0 ? FVector(Line.X, Line.Y, 0.0) / Length : FVector::ForwardVector;
	const FVector Side(-Ahead.Y, Ahead.X, 0.0);
	const int32 Count = FMath::Clamp(FMath::CeilToInt32(static_cast<float>(Length) / (Width * PieceLength)), 2, MaxLinePieces);

	// The line wanders either side of its course, by turns and by chance, but starts and ends on it.
	for (int32 Index = 0; Index <= Count; ++Index)
	{
		const float T = static_cast<float>(Index) / static_cast<float>(Count);
		const bool bEnd = Index == 0 || Index == Count;
		const float Off = bEnd ? 0.f : ((Index % 2 == 0 ? 0.3f : -0.3f) + Random.FRandRange(-0.35f, 0.35f)) * Width * Wander * 2.f;
		Points.Add(OnGround(Start + Line * T + Side * Off));
	}
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Middle = (static_cast<float>(Index) + 0.5f) / static_cast<float>(Count);
		FPiece& Piece = Pieces.AddDefaulted_GetRef();
		Piece.A = Points[Index];
		Piece.B = Points[Index + 1];
		// Widest about its middle, narrowing toward hairlines at its ends.
		Piece.Width = Width * (0.35f + 0.65f * FMath::Sqrt(FMath::Sin(UE_PI * Middle)));
		Piece.Along = static_cast<float>(Index) / static_cast<float>(Count);

		// Now and then a twig splits off it at an angle, as cracks do, and opens a little after the line passes.
		if (Index % TwigEvery != 1 || Index + 1 >= Count)
		{
			continue;
		}
		const float Turn = Random.FRandRange(35.f, 70.f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
		FVector Heading = (Piece.B - Piece.A).GetSafeNormal2D().RotateAngleAxis(Turn, FVector::UpVector);
		FVector From = Piece.B;
		const float TwigWidth = Piece.Width;
		const float TwigAlong = Piece.Along;
		for (int32 Step = 0; Step < 2; ++Step)
		{
			Heading = Heading.RotateAngleAxis(Random.FRandRange(-20.f, 20.f), FVector::UpVector);
			const FVector To = OnGround(From + Heading * (Width * TwigLength * 0.5f));
			FPiece& Twig = Pieces.AddDefaulted_GetRef();
			Twig.A = From;
			Twig.B = To;
			Twig.Width = TwigWidth * (0.5f - 0.15f * static_cast<float>(Step));
			Twig.Along = FMath::Min(TwigAlong + static_cast<float>(Step + 1) * 0.5f / static_cast<float>(Count), 1.f);
			From = To;
		}
	}
	bDirty = true;
}

void AGroundCrack::Burst(const FVector& At, float Radius)
{
	// Six or seven cracks out from where it slammed down, each in two or three crooked pieces, narrowing as they go.
	const FVector Center = OnGround(At);
	float Widest = 0.f;
	for (const FPiece& Piece : Pieces)
	{
		Widest = FMath::Max(Widest, Piece.bBurst ? 0.f : Piece.Width);
	}
	const float BaseWidth = Widest > 0.f ? Widest : FMath::Max(Radius * 0.12f, 4.f);
	const int32 Count = Random.RandRange(6, 7);
	const float FirstAngle = Random.FRandRange(0.f, 360.f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector Heading = FVector::ForwardVector.RotateAngleAxis(FirstAngle + 360.f * static_cast<float>(Index) / static_cast<float>(Count)
			+ Random.FRandRange(-18.f, 18.f), FVector::UpVector);
		const float Reach = FMath::Max(Radius, 10.f) * Random.FRandRange(0.55f, 1.f);
		const int32 Steps = Random.RandRange(2, 3);
		FVector From = Center;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Heading = Heading.RotateAngleAxis(Random.FRandRange(-25.f, 25.f), FVector::UpVector);
			const FVector To = OnGround(From + Heading * (Reach / static_cast<float>(Steps)));
			FPiece& Piece = Pieces.AddDefaulted_GetRef();
			Piece.A = From;
			Piece.B = To;
			Piece.Width = BaseWidth * (1.f - 0.3f * static_cast<float>(Step)) * Random.FRandRange(0.7f, 1.f);
			// The burst spreads outward from the middle over BurstSeconds.
			Piece.Along = static_cast<float>(Step) / static_cast<float>(Steps);
			Piece.bBurst = true;
			From = To;
		}
	}
	BurstCracks += Count;
	BurstAge = 0.f;
	bDirty = true;
	Draw();
}

// ---------------------------------------------------------------------------
// Opening, closing and drawing
// ---------------------------------------------------------------------------

void AGroundCrack::SetOpen(float Amount)
{
	const float Wanted = FMath::Clamp(Amount, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(Wanted, Open, 0.001f))
	{
		Open = Wanted;
		bDirty = true;
		Draw();
	}
}

void AGroundCrack::Close(float AfterSeconds)
{
	if (!bCloseAsked)
	{
		bCloseAsked = true;
		CloseIn = FMath::Max(AfterSeconds, 0.f);
	}
}

void AGroundCrack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AGroundCrack::Advance(float DeltaSeconds)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	Age += DeltaSeconds;
	if (BurstCracks > 0 && BurstAge < BurstSeconds)
	{
		BurstAge += DeltaSeconds;
		bDirty = true;
	}
	// Left open by whatever made it (it went away mid-charge): it closes by itself after a while.
	if (!bCloseAsked && Age >= MaxLife)
	{
		Close(0.f);
	}
	if (bCloseAsked)
	{
		CloseIn -= DeltaSeconds;
		if (CloseIn <= 0.f)
		{
			Closing = FMath::Max(0.f, Closing - DeltaSeconds / CloseSeconds);
			bDirty = true;
			if (Closing <= 0.f)
			{
				Destroy();
				return;
			}
		}
	}
	if (bDirty)
	{
		Draw();
	}
}

float AGroundCrack::ShownShare(const FPiece& Piece) const
{
	const float Reached = Piece.bBurst ? FMath::Clamp(BurstAge / BurstSeconds, 0.f, 1.f) : Open;
	// A soft front: each piece widens as the opening passes it, and all of it is open once the opening reaches the end.
	const float Share = Reached >= 1.f ? 1.f : FMath::Clamp((Reached - Piece.Along) / OpenFade, 0.f, 1.f);
	return Share * Closing;
}

void AGroundCrack::Draw()
{
	TArray<FTransform> LipShapes;
	TArray<FTransform> SeamShapes;
	LipShapes.Reserve(Pieces.Num());
	SeamShapes.Reserve(Pieces.Num());
	// Closing, it narrows and sinks back into the ground.
	const float Sink = (1.f - Closing) * LipHeight;
	for (const FPiece& Piece : Pieces)
	{
		const float Share = ShownShare(Piece);
		const FVector Span = Piece.B - Piece.A;
		const double Length = Span.Size();
		if (Share <= 0.01f || Length < 1.0)
		{
			continue;
		}
		// Along the ground's slope, flat across; pieces overlap a little at their joints so the zigzag reads as one crack.
		const FRotator Along = FRotationMatrix::MakeFromX(Span).Rotator();
		const FVector Middle = (Piece.A + Piece.B) * 0.5;
		const double Long = (Length + Piece.Width * 0.5) / CubeSize;
		const double Wide = Piece.Width * Share / CubeSize;
		LipShapes.Emplace(Along, Middle + FVector(0.0, 0.0, LipLift - Sink), FVector(Long, Wide, LipHeight / CubeSize));
		SeamShapes.Emplace(Along, Middle + FVector(0.0, 0.0, SeamLift - Sink), FVector(Long, Wide * SeamShare, SeamHeight / CubeSize));
	}
	PiecesShown = LipShapes.Num();
	WritePieces(*Lips, LipShapes);
	WritePieces(*Seam, SeamShapes);
	bDirty = false;
}
