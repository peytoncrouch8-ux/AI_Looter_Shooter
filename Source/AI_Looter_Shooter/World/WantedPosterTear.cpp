// AWantedPoster: tearing it down (the remnant, Hob's remarks, the save), and the scrap that tumbles off and fades.

#include "World/WantedPoster.h"
#include "AI_Looter_Shooter.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"

namespace
{
	/** The scrap card's custom primitive data, as M_PosterScrap reads it (build_decal_materials.py): its cell, the back's, the fade. */
	constexpr int32 CellData = 0;
	constexpr int32 BackCellData = 4;
	constexpr int32 FadeData = 8;

	/** The engine's plane is 100 cm across, in XY, facing +Z. */
	constexpr double PlaneSize = 100.0;

	/**
	 * Where the scrap comes off: a little out from the surface, and up where the art tore it from (the "already" under
	 * DEAD OR ALIVE), as a share of the whole poster's height above its middle.
	 */
	constexpr double ScrapOut = 3.0;
	constexpr double ScrapRise = 0.17;

	/**
	 * Paper falls slowly: it reaches its drop speed (cm/s) within about a quarter second, is pushed a hand's width off the
	 * surface as it's ripped away, drifts to one side and flutters, tumbling over and over.
	 */
	constexpr double FallSpeed = 110.0;
	constexpr double FallLag = 0.25;
	constexpr double PushOut = 18.0;
	constexpr double PushTime = 0.35;
	constexpr double Flutter = 9.0;
	constexpr double FlutterRate = 1.2;

	/** It stays solid for the first fifth of its fall, then fades out by its end. */
	constexpr float FadeFrom = 0.2f;
}

bool AWantedPoster::Tear(AActor* ByWhom)
{
	if (!CanTear())
	{
		return false;
	}
	bTorn = true;
	RefreshLook();
	DropScrap();
	const int32 TornNow = CountTorn(GetWorld());
	UE_LOG(LogLooter, Log, TEXT("%s: wanted poster torn down%s; %d of the level's are down."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" by %s"), *ByWhom->GetName()) : TEXT(""), TornNow);
	SayRemark(TornNow);
	// Progress: the session keeps the torn posters with this map's world (FSavedMapWorld::TornPosters).
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
	return true;
}

void AWantedPoster::RestoreTorn()
{
	if (!CanTear())
	{
		return;
	}
	bTorn = true;
	RefreshLook();
}

void AWantedPoster::SayRemark(int32 TornNow)
{
	const FWantedPosterRemark* Remark = TearRemarks.FindByPredicate([TornNow](const FWantedPosterRemark& Each) { return Each.TornCount == TornNow; });
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Remark && Captions && !Remark->Lines.IsEmpty())
	{
		// A remark on something that happened: after whatever is being said.
		Captions->Play(Remark->Lines, ECaptionPlay::Queue);
	}
}

// ---------------------------------------------------------------------------
// The falling scrap
// ---------------------------------------------------------------------------

void AWantedPoster::DropScrap()
{
	EndScrap();
	UStaticMesh* Mesh = GetScrapMesh();
	if (!Mesh || !GetWorld())
	{
		return;
	}
	UStaticMeshComponent* Card = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	Card->SetMobility(EComponentMobility::Movable);
	Card->SetupAttachment(Root);
	Card->SetStaticMesh(Mesh);
	if (UMaterialInterface* Material = GetScrapMaterial())
	{
		Card->SetMaterial(0, Material);
	}
	// A wisp of paper: nothing meets it, it casts no shadow, and the poster's own decal (which it starts inside) skips it.
	Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Card->SetGenerateOverlapEvents(false);
	Card->SetCanEverAffectNavigation(false);
	Card->SetCastShadow(false);
	Card->SetReceivesDecals(false);
	// Square, at the cell's longer side: M_PosterScrap shows the cell in its middle at its own shape and nothing past it,
	// whichever way the plane's UVs run (the tumble hides which).
	const FVector2D Size = Atlas.SizeOf(Atlas.Scrap) * Scale;
	const double Side = FMath::Max(Size.X, Size.Y) / PlaneSize;
	Card->SetRelativeScale3D(FVector(Side, Side, 1.0));
	Card->SetCustomPrimitiveDataVector4(CellData, Atlas.Scrap);
	Card->SetCustomPrimitiveDataVector4(BackCellData, Atlas.ScrapBack);
	Card->SetCustomPrimitiveDataFloat(FadeData, 1.f);
	Card->RegisterComponent();
	Scrap = Card;

	// Thrown a little differently from each poster: off to one side or the other, tumbling about its own way.
	FRandomStream Random(static_cast<int32>(GetTypeHash(GetFName())));
	ScrapAge = 0.f;
	ScrapStart = FVector(ScrapOut, 0.0, Atlas.SizeOf(GetWholeCell()).Y * Scale * ScrapRise);
	ScrapDrift = Random.FRandRange(10.f, 25.f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
	ScrapFlutterPhase = Random.FRandRange(0.f, 2.f * PI);
	// Mostly end over end about the across-the-wall line, with a little twist.
	ScrapSpinAxis = FVector(Random.FRandRange(-0.3f, 0.3f), Random.FRand() < 0.5f ? -1.f : 1.f, Random.FRandRange(-0.4f, 0.4f)).GetSafeNormal();
	ScrapSpinRate = Random.FRandRange(220.f, 420.f);
	PoseScrap();
	SetActorTickEnabled(true);
}

float AWantedPoster::GetScrapOpacity() const
{
	if (!Scrap)
	{
		return 0.f;
	}
	const float Share = ScrapSeconds > 0.f ? ScrapAge / ScrapSeconds : 1.f;
	return 1.f - FMath::SmoothStep(FadeFrom, 1.f, Share);
}

void AWantedPoster::Advance(float DeltaSeconds)
{
	if (!Scrap)
	{
		SetActorTickEnabled(false);
		return;
	}
	ScrapAge += DeltaSeconds;
	if (ScrapAge >= ScrapSeconds)
	{
		EndScrap();
		return;
	}
	PoseScrap();
}

void AWantedPoster::PoseScrap()
{
	if (!Scrap)
	{
		return;
	}
	const double T = ScrapAge;
	const double Fall = FallSpeed * (T - FallLag * (1.0 - FMath::Exp(-T / FallLag)));
	const double Out = PushOut * (1.0 - FMath::Exp(-T / PushTime));
	const double Across = ScrapDrift * T + Flutter * FMath::Sin(2.0 * PI * FlutterRate * T + ScrapFlutterPhase);
	Scrap->SetRelativeLocation(ScrapStart + FVector(Out, Across, -Fall));
	// Flat on the surface as it comes off (the plane's face turned out along +X), then tumbling.
	const FQuat Flat = FRotator(-90.0, 0.0, 0.0).Quaternion();
	const FQuat Tumble(ScrapSpinAxis, FMath::DegreesToRadians(ScrapSpinRate * T));
	Scrap->SetRelativeRotation(Tumble * Flat);
	Scrap->SetCustomPrimitiveDataFloat(FadeData, GetScrapOpacity());
}

void AWantedPoster::EndScrap()
{
	if (Scrap)
	{
		Scrap->DestroyComponent();
		Scrap = nullptr;
	}
	ScrapAge = 0.f;
	SetActorTickEnabled(false);
}
