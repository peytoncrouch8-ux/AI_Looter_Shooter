// UHudMinimapWidget's roosts: Hob's roosts as the map's roost icon (the same everywhere), at their place when near and the
// nearest found ones on the rim from afar, like a compass.

#include "UI/HUD/HudMinimapWidget.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/MinimapSubsystem.h"
#include "World/RespawnMarker.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "EngineUtils.h"

using namespace LooterUI;

namespace
{
	/** At most this many roosts on the minimap at once (a valley's worth near, three on the rim). */
	constexpr int32 MaxRoostMarkers = 8;
	/** A roost's badge at 100%: on the map a little bigger than a giver's mark, on the rim smaller (it's farther). */
	constexpr float RoostMapSize = 18.f;
	constexpr float RoostRimSize = 13.f;
	/** The icon's share of its badge: bigger than the map page's, so the crow on its post reads at these sizes. */
	constexpr float RoostIconShare = 0.8f;
	/** Gap between the rim and a roost on it (px), as the waypoint's needle keeps. */
	constexpr float RoostRimGap = 4.f;
	/** A roost not found yet shows this faint (and grey), only once it's on the map. */
	constexpr float UnfoundOpacity = 0.6f;
	constexpr float RimOpacity = 0.9f;
	/** How often the level's roosts are looked up again (s): they never move, and one is found only now and then. */
	constexpr float RoostLookSeconds = 0.5f;

	/** Centers Widget on Position inside Canvas at Size. */
	UCanvasPanelSlot* PutOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, float Size)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(FVector2D(Size));
		return CanvasSlot;
	}

	void PlaceAt(UWidget* Widget, const FVector2D& Position, float Size)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(FVector2D(Size));
		}
	}

	void ShowWidget(UWidget* Widget, bool bShow)
	{
		const ESlateVisibility Wanted = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}
}

TArray<FMinimapRoostSpot> UHudMinimapWidget::PlaceRoosts(TConstArrayView<FMinimapRoost> Roosts, const FVector& Player, float ViewYaw, float PixelsPerCm,
	float InsideRadius, float RimRadius, float RimReach, int32 MaxOnRim)
{
	TArray<FMinimapRoostSpot> Spots;
	struct FRimRoost
	{
		int32 Index;
		double Across;
		FVector2D Offset;
	};
	TArray<FRimRoost> Rim;
	for (int32 Index = 0; Index < Roosts.Num(); ++Index)
	{
		const FMinimapRoost& Roost = Roosts[Index];
		const FVector2D Offset = UMinimapSubsystem::ViewOffset(Roost.Location - Player, ViewYaw, PixelsPerCm);
		if (Offset.Size() <= InsideRadius)
		{
			FMinimapRoostSpot& Spot = Spots.AddDefaulted_GetRef();
			Spot.Index = Index;
			Spot.bInside = true;
			Spot.bFound = Roost.bFound;
			Spot.Position = Offset;
			continue;
		}
		// Past the rim, only a roost already found: one still to find is the player's to come across.
		const double Across = FVector::Dist2D(Roost.Location, Player);
		if (Roost.bFound && Across <= RimReach)
		{
			Rim.Add({ Index, Across, Offset });
		}
	}
	Rim.Sort([](const FRimRoost& A, const FRimRoost& B) { return A.Across < B.Across; });
	for (int32 Index = 0; Index < FMath::Min(MaxOnRim, Rim.Num()); ++Index)
	{
		FMinimapRoostSpot& Spot = Spots.AddDefaulted_GetRef();
		Spot.Index = Rim[Index].Index;
		Spot.bInside = false;
		Spot.bFound = true;
		Spot.Position = Rim[Index].Offset.GetSafeNormal() * RimRadius;
	}
	return Spots;
}

void UHudMinimapWidget::BuildRoosts(UCanvasPanel* Layer)
{
	const float Middle = Diameter * 0.5f;
	for (int32 Index = 0; Index < MaxRoostMarkers; ++Index)
	{
		// Each a dark glass badge (a background: the UI transparency setting fades it) with the roost's icon over it.
		UImage* Badge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Badge->SetVisibility(ESlateVisibility::Hidden);
		MarkBackground(Badge);
		PutOnCanvas(Layer, Badge, FVector2D(Middle), RoostMapSize);
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetVisibility(ESlateVisibility::Hidden);
		PutOnCanvas(Layer, Icon, FVector2D(Middle), RoostMapSize * RoostIconShare);
		RoostBadges.Add(Badge);
		RoostIcons.Add(Icon);
		RoostKeys.Add(INDEX_NONE);
	}
}

void UHudMinimapWidget::UpdateRoosts(const FVector& PlayerLocation, float Yaw, float Radius, float PixelsPerCm, float DeltaSeconds)
{
	if (RoostBadges.IsEmpty())
	{
		return;
	}
	RoostLookLeft -= DeltaSeconds;
	if (RoostLookLeft <= 0.f)
	{
		// The class's own list, a dozen at most: where each stands, and whether it's found in the story played here.
		RoostLookLeft = RoostLookSeconds;
		RoostCache.Reset();
		const UWorld* World = GetWorld();
		const UMissionRunner* Runner = UMissionRunner::Get(this);
		const FCampaignRecord* Campaign = Runner && Runner->IsActive() ? &Runner->GetCampaign() : nullptr;
		for (TActorIterator<ARespawnMarker> It(World); It; ++It)
		{
			const ARespawnMarker* Roost = *It;
			if (IsValid(Roost) && !Roost->IsActorBeingDestroyed())
			{
				RoostCache.Add({ Roost->GetActorLocation(), Campaign ? Roost->IsFound(*Campaign) : Roost->bStartActive });
			}
		}
	}

	// Markers grow more gently than the map, as the others do.
	const float MarkerScale = FMath::Sqrt(Scale);
	const float MapSize = RoostMapSize * MarkerScale;
	const float RimSize = RoostRimSize * MarkerScale;
	const TArray<FMinimapRoostSpot> Spots = PlaceRoosts(RoostCache, PlayerLocation, Yaw, PixelsPerCm, Radius - MapSize * 0.5f - 3.f,
		Radius - RimSize * 0.5f - RoostRimGap, RoostRimReach, RoostsOnRim);
	const FLinearColor Lit = MapIcons::Color(EMapPinKind::Grave);
	int32 Used = 0;
	for (const FMinimapRoostSpot& Spot : Spots)
	{
		if (Used >= RoostBadges.Num())
		{
			break;
		}
		UImage* Badge = RoostBadges[Used];
		UImage* Icon = RoostIcons[Used];
		// Its brushes change only when what it shows does: found or not.
		const int32 Key = Spot.bFound ? 1 : 0;
		if (RoostKeys[Used] != Key)
		{
			const FLinearColor Tint = Spot.bFound ? Lit : Color::TextDim();
			Badge->SetBrush(MapIcons::BadgeBrush(Tint, /*bLit*/ false));
			Icon->SetBrush(MapIcons::RoostBrush(RoostMapSize * RoostIconShare, Tint));
			RoostKeys[Used] = Key;
		}
		const float Size = Spot.bInside ? MapSize : RimSize;
		const FVector2D Where = FVector2D(Radius) + Spot.Position;
		PlaceAt(Badge, Where, Size);
		PlaceAt(Icon, Where, Size * RoostIconShare);
		const float Opacity = !Spot.bFound ? UnfoundOpacity : Spot.bInside ? 1.f : RimOpacity;
		Badge->SetRenderOpacity(Opacity);
		Icon->SetRenderOpacity(Opacity);
		ShowWidget(Badge, true);
		ShowWidget(Icon, true);
		++Used;
	}
	for (int32 Index = Used; Index < RoostBadges.Num(); ++Index)
	{
		ShowWidget(RoostBadges[Index], false);
		ShowWidget(RoostIcons[Index], false);
	}
}
