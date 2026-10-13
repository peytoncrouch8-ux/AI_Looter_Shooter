// UHudMinimapWidget's plain rules, apart from its widgets so the tests read them: how much of the world it shows, how its
// markers go round, the zoom it draws at, and where the tracked mission's waypoint shows.

#include "UI/HUD/HudMinimapWidget.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "World/MinimapSubsystem.h"
#include "Engine/LocalPlayer.h"

namespace
{
	/** How much of the world the minimap shows (radius, cm) at zoom 1, at any size. */
	constexpr float BaseRange = 3500.f;
}

float UHudMinimapWidget::RangeAtZoom(float Zoom)
{
	return BaseRange / FMath::Max(Zoom, UGraphicsSettingsSubsystem::MinMinimapZoom);
}

FMinimapMarkerSplit UHudMinimapWidget::SplitMarkers(int32 Pool, int32 Hostiles, int32 Guns, int32 Ammo)
{
	return SplitMarkers(Pool, Hostiles, 0, Guns, Ammo);
}

FMinimapMarkerSplit UHudMinimapWidget::SplitMarkers(int32 Pool, int32 Hostiles, int32 Givers, int32 Guns, int32 Ammo)
{
	FMinimapMarkerSplit Split;
	int32 Left = FMath::Max(Pool, 0);
	Split.Hostiles = FMath::Clamp(Hostiles, 0, Left);
	Left -= Split.Hostiles;
	Split.Givers = FMath::Clamp(Givers, 0, Left);
	Left -= Split.Givers;
	Split.Guns = FMath::Clamp(Guns, 0, Left);
	Left -= Split.Guns;
	Split.Ammo = FMath::Clamp(Ammo, 0, Left);
	return Split;
}

void UHudMinimapWidget::SetPreviewZoom(float Zoom)
{
	PreviewZoom = Zoom > 0.f ? FMath::Clamp(Zoom, UGraphicsSettingsSubsystem::MinMinimapZoom, UGraphicsSettingsSubsystem::MaxMinimapZoom) : 0.f;
}

float UHudMinimapWidget::ZoomFor(const UGraphicsSettingsSubsystem* Settings) const
{
	return PreviewZoom > 0.f ? PreviewZoom : (Settings ? Settings->GetMinimapZoom() : 1.f);
}

float UHudMinimapWidget::GetZoom() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return ZoomFor(LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr);
}

FMinimapWaypoint UHudMinimapWidget::PlaceWaypoint(const FVector& WorldDelta, float ViewYaw, float PixelsPerCm, float InsideRadius, float RimRadius)
{
	FMinimapWaypoint Result;
	const FVector2D Offset = UMinimapSubsystem::ViewOffset(WorldDelta, ViewYaw, PixelsPerCm);
	// Up on screen is -Y, so the angle clockwise from up is atan2(right, up); Slate turns widgets clockwise too.
	Result.Angle = Offset.IsNearlyZero() ? 0.f : static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Offset.X, -Offset.Y)));
	Result.bInside = Offset.Size() <= InsideRadius;
	Result.Position = Result.bInside ? Offset : Offset.GetSafeNormal() * RimRadius;
	return Result;
}
