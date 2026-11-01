// UHudMinimapWidget's secrets: the places Sexton's secrets bought mark (USecretSubsystem::GetMarks), the map page's gold
// keyhole at their place when near, and the two nearest on the rim from afar, as found lantern posts show.

#include "UI/HUD/HudMinimapWidget.h"
#include "Economy/SecretSubsystem.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

using namespace LooterUI;

namespace
{
	/** At most this many secret marks at once (the Ledger pages' six), two of them on the rim. */
	constexpr int32 MaxSecretMarkers = 6;
	constexpr int32 SecretsOnRim = 2;
	/** A mark's badge at 100%: the size of a roost's on the map, a little smaller on the rim. */
	constexpr float SecretMapSize = 17.f;
	constexpr float SecretRimSize = 12.f;
	constexpr float SecretIconShare = 0.78f;
	constexpr float SecretRimGap = 4.f;
	constexpr float SecretRimOpacity = 0.85f;
	/** How often the marks are asked for again (s): they change only on a buy or a find. */
	constexpr float SecretLookSeconds = 0.5f;

	UCanvasPanelSlot* PutSecretOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, float Size)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(FVector2D(Size));
		return CanvasSlot;
	}

	void PlaceSecretAt(UWidget* Widget, const FVector2D& Position, float Size)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(FVector2D(Size));
		}
	}

	void ShowSecretWidget(UWidget* Widget, bool bShow)
	{
		const ESlateVisibility Wanted = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}
}

void UHudMinimapWidget::BuildSecrets(UCanvasPanel* Layer)
{
	const float Middle = Diameter * 0.5f;
	const FLinearColor Tint = MapIcons::Color(EMapPinKind::Secret);
	for (int32 Index = 0; Index < MaxSecretMarkers; ++Index)
	{
		// The same look for every one (only where they are changes): a dark glass badge (a background) and the keyhole.
		UImage* Badge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Badge->SetVisibility(ESlateVisibility::Hidden);
		Badge->SetBrush(MapIcons::BadgeBrush(Tint, /*bLit*/ false));
		MarkBackground(Badge);
		PutSecretOnCanvas(Layer, Badge, FVector2D(Middle), SecretMapSize);
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetVisibility(ESlateVisibility::Hidden);
		Icon->SetBrush(MapIcons::SecretBrush(SecretMapSize * SecretIconShare, Tint));
		PutSecretOnCanvas(Layer, Icon, FVector2D(Middle), SecretMapSize * SecretIconShare);
		SecretBadges.Add(Badge);
		SecretIcons.Add(Icon);
	}
}

void UHudMinimapWidget::UpdateSecrets(const FVector& PlayerLocation, float Yaw, float Radius, float PixelsPerCm, float DeltaSeconds)
{
	if (SecretBadges.IsEmpty())
	{
		return;
	}
	SecretLookLeft -= DeltaSeconds;
	if (SecretLookLeft <= 0.f)
	{
		SecretLookLeft = SecretLookSeconds;
		SecretCache.Reset();
		if (const USecretSubsystem* Secrets = USecretSubsystem::Get(this))
		{
			for (const FSecretMark& Mark : Secrets->GetMarks())
			{
				// Bought: shown on the rim from afar too, as a found lantern post is.
				SecretCache.Add({ Mark.Location, /*bFound*/ true });
			}
		}
	}

	const float MarkerScale = FMath::Sqrt(Scale);
	const float MapSize = SecretMapSize * MarkerScale;
	const float RimSize = SecretRimSize * MarkerScale;
	const TArray<FMinimapRoostSpot> Spots = PlaceRoosts(SecretCache, PlayerLocation, Yaw, PixelsPerCm, Radius - MapSize * 0.5f - 3.f,
		Radius - RimSize * 0.5f - SecretRimGap, RoostRimReach, SecretsOnRim);
	int32 Used = 0;
	for (const FMinimapRoostSpot& Spot : Spots)
	{
		if (Used >= SecretBadges.Num())
		{
			break;
		}
		const float Size = Spot.bInside ? MapSize : RimSize;
		const FVector2D Where = FVector2D(Radius) + Spot.Position;
		PlaceSecretAt(SecretBadges[Used], Where, Size);
		PlaceSecretAt(SecretIcons[Used], Where, Size * SecretIconShare);
		const float Opacity = Spot.bInside ? 1.f : SecretRimOpacity;
		SecretBadges[Used]->SetRenderOpacity(Opacity);
		SecretIcons[Used]->SetRenderOpacity(Opacity);
		ShowSecretWidget(SecretBadges[Used], true);
		ShowSecretWidget(SecretIcons[Used], true);
		++Used;
	}
	for (int32 Index = Used; Index < SecretBadges.Num(); ++Index)
	{
		ShowSecretWidget(SecretBadges[Index], false);
		ShowSecretWidget(SecretIcons[Index], false);
	}
}
