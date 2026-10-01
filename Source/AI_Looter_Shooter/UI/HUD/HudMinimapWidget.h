#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudMinimapWidget.generated.h"

class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;

/** Where the tracked mission's waypoint shows on the minimap (UHudMinimapWidget::PlaceWaypoint). */
struct FMinimapWaypoint
{
	/** On the map: Position is the waypoint itself. Past it: Position is on the rim, toward the waypoint. */
	bool bInside = false;
	/** Pixels from the map's center (+X right, +Y down). */
	FVector2D Position = FVector2D::ZeroVector;
	/** Which way the waypoint lies from the center, in degrees clockwise from up: the angle to turn an up-pointing arrow. */
	float Angle = 0.f;
};

/**
 * The HUD's round minimap (top-right) that turns with the view, so the way you look is up: the island from above
 * (baked by UMinimapSubsystem), hostiles as red diamonds, loot as dots in its rarity color, ammo as small dots, your
 * arrow in the middle, and an N that circles the rim. The tracked mission's waypoint (UMissionSubsystem) is an orange
 * ring on the map when it's in range; farther away, an orange arrow on the rim points the way like a compass needle,
 * with the distance in meters beside it. Its size is the player's choice (Settings > Interface > Minimap size,
 * 60-150%). Reads the possessed pawn every frame.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * The minimap's standard size on screen, and its gap from the screen's top-right corner. Its actual size follows the
	 * player's setting, and the settings menu previews it in the same spot.
	 */
	static constexpr float Diameter = 172.f;
	static constexpr float Margin = 36.f;

	/**
	 * Where a waypoint at WorldDelta from the player shows on a map that turns with the view (ViewYaw up): itself when it
	 * is within InsideRadius pixels of the center, else on a circle RimRadius pixels out, in its direction.
	 */
	static FMinimapWaypoint PlaceWaypoint(const FVector& WorldDelta, float ViewYaw, float PixelsPerCm, float InsideRadius, float RimRadius);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Resizes the minimap: the map, its rim and the player arrow grow with it, markers a little less. */
	void ApplyScale(float NewScale);
	float GetDiameter() const { return Diameter * Scale; }

	/** Shows the tracked mission's waypoint: the ring on the map, or the rim arrow and its distance. */
	void UpdateWaypoint(const FVector& PlayerLocation, float Yaw, float Radius, float PixelsPerCm);

	UPROPERTY(Transient) TObjectPtr<USizeBox> SizeBox;
	UPROPERTY(Transient) TObjectPtr<UImage> MapImage;
	UPROPERTY(Transient) TObjectPtr<UImage> Arrow;
	UPROPERTY(Transient) TObjectPtr<UImage> Notch;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> North;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Markers;
	/** The tracked mission's waypoint: on the map, or as an arrow on the rim with the distance beside it. */
	UPROPERTY(Transient) TObjectPtr<UImage> Waypoint;
	UPROPERTY(Transient) TObjectPtr<UImage> WaypointArrow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WaypointDistance;

	/** The map picture the brush currently shows (owned by UMinimapSubsystem). */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> MapTexture;
	FSlateBrush MapBrush;

	/** Per marker: what it last showed (kind + color), so brushes only change when needed. */
	TArray<uint32> MarkerKeys;

	/** The meters the distance label shows, so its text only changes when the number does. */
	int32 WaypointMeters = INDEX_NONE;

	/** The minimap's size right now, relative to Diameter. */
	float Scale = 1.f;
};
