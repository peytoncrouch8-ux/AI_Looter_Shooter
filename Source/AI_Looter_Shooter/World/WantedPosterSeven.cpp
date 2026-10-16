// AWantedPoster: the Seven on the Rangers' board, each face crossed out with an ink X as its ember is paid.

#include "World/WantedPoster.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** M_PosterDecal's cell (U0, V0, U1, V1 of the atlas), as the paper's own decal sets it. */
	const FName CrossCellParameter(TEXT("Cell"));
}

// ---------------------------------------------------------------------------
// The story
// ---------------------------------------------------------------------------

void AWantedPoster::BindStory()
{
	if (Variant != EWantedPosterVariant::Seven)
	{
		UnbindStory();
		return;
	}
	if (BoundRunner.IsValid())
	{
		return;
	}
	// A face is crossed out as its ember is paid, whenever that comes: it looks again whenever the missions change.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		StoryChangedHandle = Runner->OnChanged.AddUObject(this, &AWantedPoster::HandleStoryChanged);
	}
}

void AWantedPoster::UnbindStory()
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(StoryChangedHandle);
	}
	BoundRunner.Reset();
	StoryChangedHandle.Reset();
}

void AWantedPoster::HandleStoryChanged()
{
	RefreshCrosses();
}

bool AWantedPoster::IsFaceCrossedNow(int32 Face) const
{
	if (Variant != EWantedPosterVariant::Seven || !CrossedFlags.IsValidIndex(Face) || CrossedFlags[Face].IsNone())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->HasFlag(CrossedFlags[Face]);
}

bool AWantedPoster::IsFaceCrossShown(int32 Face) const
{
	return Face >= 0 && Face < SevenCount && (ShownCrosses & (1u << Face)) != 0;
}

// ---------------------------------------------------------------------------
// The X
// ---------------------------------------------------------------------------

void AWantedPoster::RefreshCrosses()
{
	uint8 Crossed = 0;
	for (int32 Face = 0; Face < SevenCount; ++Face)
	{
		Crossed |= IsFaceCrossedNow(Face) ? static_cast<uint8>(1u << Face) : 0;
	}
	if (Crossed != ShownCrosses && Variant == EWantedPosterVariant::Seven)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: the Seven, %d of seven faces crossed out."), *GetActorNameOrLabel(), FMath::CountBits(Crossed));
	}
	ShownCrosses = Crossed;
	// Made the first time a face is paid, and never for the papers that have none: most of the game shows no X at all.
	if (Crossed != 0 && Crosses.IsEmpty())
	{
		for (int32 Face = 0; Face < SevenCount; ++Face)
		{
			Crosses.Add(MakeCross(Face));
		}
		PlaceCrosses();
	}
	for (int32 Face = 0; Face < Crosses.Num(); ++Face)
	{
		if (UDecalComponent* Cross = Crosses[Face])
		{
			Cross->SetVisibility(IsFaceCrossShown(Face) && Cross->GetDecalMaterial() != nullptr);
		}
	}
}

UDecalComponent* AWantedPoster::MakeCross(int32 Face)
{
	if (!CrossMaterial)
	{
		// The paper's own material showing the X's cell instead of the Seven's: one instance shared by every X.
		const UMaterialInstanceDynamic* Paper = FindOrMakeDecalMaterial();
		if (UMaterialInterface* Base = Paper ? Paper->Parent.Get() : nullptr)
		{
			CrossMaterial = UMaterialInstanceDynamic::Create(Base, this);
			const FVector4& Cell = Atlas.SevenCross;
			CrossMaterial->SetVectorParameterValue(CrossCellParameter, FLinearColor(static_cast<float>(Cell.X), static_cast<float>(Cell.Y),
				static_cast<float>(Cell.Z), static_cast<float>(Cell.W)));
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no poster decal material, so the Seven's crosses can't be drawn."), *GetActorNameOrLabel());
		}
	}
	UDecalComponent* Cross = NewObject<UDecalComponent>(this, NAME_None, RF_Transient);
	Cross->SetupAttachment(Root);
	Cross->SetMobility(EComponentMobility::Movable);
	// Over the paper it's inked on.
	Cross->SortOrder = Decal ? Decal->SortOrder + 1 : 1;
	Cross->FadeScreenSize = Decal ? Decal->FadeScreenSize : Cross->FadeScreenSize;
	Cross->SetDecalMaterial(CrossMaterial);
	Cross->SetVisibility(false);
	Cross->RegisterComponent();
	return Cross;
}

void AWantedPoster::PlaceCrosses()
{
	if (Crosses.IsEmpty() || !Decal)
	{
		return;
	}
	// Each face's rectangle of the Seven on the wall: U runs to the reader's right (the poster's -Y), V down (its -Z), from
	// the sheet's middle at the poster's origin. Turned and as deep as the paper's own decal.
	const FVector2D Sheet = Atlas.SizeOf(Atlas.Seven) * Scale;
	const FVector Paper = Decal->GetRelativeLocation();
	for (int32 Face = 0; Face < Crosses.Num(); ++Face)
	{
		UDecalComponent* Cross = Crosses[Face];
		if (!Cross)
		{
			continue;
		}
		const FVector4 Rect = Atlas.SevenFaces.IsValidIndex(Face) ? Atlas.SevenFaces[Face] : FVector4(0.0, 0.0, 0.0, 0.0);
		const double MidU = (Rect.X + Rect.Z) * 0.5;
		const double MidV = (Rect.Y + Rect.W) * 0.5;
		const double Across = FMath::Abs(Rect.Z - Rect.X) * Sheet.X;
		const double Down = FMath::Abs(Rect.W - Rect.Y) * Sheet.Y;
		Cross->DecalSize = FVector(Decal->DecalSize.X, FMath::Max(Down, 0.1) * 0.5, FMath::Max(Across, 0.1) * 0.5);
		Cross->SetRelativeLocationAndRotation(FVector(Paper.X, Sheet.X * (0.5 - MidU), Sheet.Y * (0.5 - MidV)), Decal->GetRelativeRotation());
		Cross->MarkRenderStateDirty();
	}
}
