#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudMinimapWidget.generated.h"

class UCanvasPanel;
class UGraphicsSettingsSubsystem;
class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;

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

/** How many of the minimap's markers go to each kind of thing within its rim (UHudMinimapWidget::SplitMarkers). */
struct FMinimapMarkerSplit
{
	int32 Hostiles = 0;
	/** The givers' ! and ? (UGiverMarkSubsystem). */
	int32 Givers = 0;
	int32 Guns = 0;
	int32 Ammo = 0;
};

/**
 * The HUD's round minimap (top-right) that turns with the view, so the way you look is up: the island from above
 * (baked by UMinimapSubsystem), hostiles as red diamonds, the givers' marks (a gold or white ? where a mission is offered,
 * a ! where one waits to be turned in), loot as dots in its rarity color, ammo as small dots and your arrow in the middle.
 * The tracked mission's waypoint (UMissionSubsystem) is an orange ring on the map when it's in range; farther away, an orange arrow inside the rim points the way like a compass needle, with the distance in meters
 * beside it. Its size is the player's choice (Settings > Interface > Minimap size, 60-150%). Reads the possessed pawn
 * every frame.
 *
 * The map sits in a gunmetal bezel (HudMinimapWidgetFrame.cpp) with ticks every 30 degrees that turn with the view, the
 * N on an orange-ringed disc riding the bezel, a fixed orange notch at the top and a cyan hairline inside; under it, the
 * place's name. The widget's own box is the map's circle (Diameter at 100%): the bezel and the name hang outside it, so
 * the HUD and the settings menu's preview place the map itself.
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
	 * How far the gunmetal bezel reaches past the map's edge at 100% size (it grows with the map). The widget's box is the
	 * map alone, so the real footprint on screen is this much further out all round: the settings menu's size preview
	 * outlines it.
	 */
	static constexpr float BezelReach = 9.f;

	/**
	 * Where a waypoint at WorldDelta from the player shows on a map that turns with the view (ViewYaw up): itself when it
	 * is within InsideRadius pixels of the center, else on a circle RimRadius pixels out, in its direction.
	 */
	static FMinimapWaypoint PlaceWaypoint(const FVector& WorldDelta, float ViewYaw, float PixelsPerCm, float InsideRadius, float RimRadius);

	/** How much of the world the map shows (the radius, in cm) at a zoom: 1 shows 35 m around the player, 2 half that. */
	static float RangeAtZoom(float Zoom);

	/**
	 * How its Pool markers go round what lies within the rim: every hostile first (what a minimap is looked at for), then
	 * the guns, then the ammo with what's left. Loot piles up after long fights, and lies there still after a Save & Quit:
	 * it must never crowd the hostiles off the map.
	 */
	static FMinimapMarkerSplit SplitMarkers(int32 Pool, int32 Hostiles, int32 Guns, int32 Ammo);

	/**
	 * The same with the givers' marks (a ? where a mission is offered, a ! where one waits to be turned in): after every
	 * hostile, before the loot, so a fight never loses its creatures to them and loot never hides a giver.
	 */
	static FMinimapMarkerSplit SplitMarkers(int32 Pool, int32 Hostiles, int32 Givers, int32 Guns, int32 Ammo);

	/**
	 * Makes this one a live copy for the settings menu's zoom preview: the HUD's own is put away under the menu, so the menu
	 * shows a second one in its corner (it draws the same bake, markers and waypoint) while the zoom is being set. A copy
	 * also shows with the minimap switched off in the settings, so the zoom can be set before it is turned on.
	 */
	void SetPreview(bool bInPreview) { bPreview = bInPreview; }

	/** The zoom a preview copy shows, whatever the saved setting is, as the slider moves (kept in range). 0 follows the setting again. */
	void SetPreviewZoom(float Zoom);

	/** The zoom the map shows now: the preview's if it has one, else the player's setting (1 with no settings). */
	float GetZoom() const;

	/**
	 * The tag that marks underground space (the Providence's galleries, Tilly's cellar): any actor carrying it (a trigger box
	 * the build script places, an interior's volume) covers its bounds, and inside them the minimap draws its markers over a
	 * dark plate marked UNDERGROUND instead of the top-down bake, which sees only the ground above.
	 */
	static const FName UndergroundTag;

	/** Where lies underground: inside one of Zones (their boxes). The rule apart from the world, for the tests. */
	static bool IsUnderground(const TArray<FBox>& Zones, const FVector& Where);

	/** The boxes of every actor tagged UndergroundTag in World (their components' bounds; one with none adds nothing). */
	static void GatherUndergroundZones(const UWorld* World, TArray<FBox>& OutZones);

	/** The plate is up (or coming up): the player is underground. */
	bool IsShowingUnderground() const { return bUnderground; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** The zoom to draw at: the preview's if it has one, else Settings' (1 with none). */
	float ZoomFor(const UGraphicsSettingsSubsystem* Settings) const;
	/** Resizes the minimap: the map and its bezel grow with it, the player arrow, markers and the N a little less. */
	void ApplyScale(float NewScale);
	float GetDiameter() const { return Diameter * Scale; }

	/** Shows the tracked mission's waypoint: the ring on the map, or the rim arrow and its distance. */
	void UpdateWaypoint(const FVector& PlayerLocation, float Yaw, float Radius, float PixelsPerCm);

	// --- The frame round the map (HudMinimapWidgetFrame.cpp) ---

	/** Builds the bezel, its ticks, the N on its disc, the notch and the place's two lines into Layer, over the map. */
	void BuildFrame(UCanvasPanel* Layer);
	/** Sizes the frame for Scale: the bezel and its ticks grow with the map, the N's disc and the notch like the markers. */
	void ScaleFrame();
	/** Turns the ticks and moves the N round the bezel, so they keep to the world's directions (ViewYaw up). */
	void TurnFrame(float ViewYaw);
	/**
	 * Names where the player is under the map. With the level's area not found yet it shows the level's file name and looks
	 * again a few times a second; once the area is found its name is shown and it is never looked up again.
	 */
	void UpdatePlace();
	/** The two lines under the map: Place over Area; with no place (the game names none yet), Area on the big line. */
	void ShowPlace(const FText& Place, const FText& Area);

	// --- Underground (HudMinimapWidgetUnderground.cpp) ---

	/** Builds the underground plate (a dark disc, faint rings, UNDERGROUND) into the map's stack, over the bake. */
	void BuildUnderground(UOverlay* Stack);
	/** Looks whether the player is underground and fades the plate in over the bake, or out. */
	void UpdateUnderground(const FVector& PlayerLocation, float DeltaSeconds);

	UPROPERTY(Transient) TObjectPtr<UWidget> UndergroundPlate;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> UndergroundLabel;
	/** The underground spaces' boxes, looked for again every few seconds (they don't move). */
	TArray<FBox> UndergroundZones;
	float UndergroundLookLeft = 0.f;
	/** The plate's fade (0 the bake, 1 the plate) and whether it's heading in. */
	float UndergroundAlpha = 0.f;
	bool bUnderground = false;

	UPROPERTY(Transient) TObjectPtr<USizeBox> SizeBox;
	UPROPERTY(Transient) TObjectPtr<UImage> MapImage;
	UPROPERTY(Transient) TObjectPtr<UImage> Arrow;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Markers;
	/** The tracked mission's waypoint: on the map, or as an arrow on the rim with the distance beside it. */
	UPROPERTY(Transient) TObjectPtr<UImage> Waypoint;
	UPROPERTY(Transient) TObjectPtr<UImage> WaypointArrow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WaypointDistance;

	/** The frame: the bezel (fixed, lit from above), its ticks (turning), the notch (fixed, top), the N on its disc. */
	UPROPERTY(Transient) TObjectPtr<UImage> Bezel;
	UPROPERTY(Transient) TObjectPtr<UImage> Ticks;
	UPROPERTY(Transient) TObjectPtr<UImage> Notch;
	UPROPERTY(Transient) TObjectPtr<UWidget> NorthBadge;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> North;
	/** Under the map: the place (big) over the area it's in (small, dim; collapsed while there's no place). */
	UPROPERTY(Transient) TObjectPtr<UWidget> PlaceBlock;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlaceLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AreaLabel;
	/** The level's area is found and its name shown: it is fixed for the life of the HUD, so it's looked up until then only. */
	bool bPlaceKnown = false;
	/** Seconds until the area is looked for again (while it isn't found). */
	float PlaceRetryLeft = 0.f;

	/** The map picture the brush currently shows (owned by UMinimapSubsystem). */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> MapTexture;
	FSlateBrush MapBrush;

	/** Per marker: what it last showed (kind + color), so brushes only change when needed. */
	TArray<uint32> MarkerKeys;

	/** The meters the distance label shows, so its text only changes when the number does. */
	int32 WaypointMeters = INDEX_NONE;

	/** The minimap's size right now, relative to Diameter. */
	float Scale = 1.f;

	/** A live copy for the settings menu (SetPreview), and the zoom it shows instead of the setting's (0: the setting's). */
	bool bPreview = false;
	float PreviewZoom = 0.f;
};
