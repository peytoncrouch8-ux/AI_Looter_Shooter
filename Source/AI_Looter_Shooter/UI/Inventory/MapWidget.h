#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "UI/Inventory/MapPins.h"
#include "UI/Inventory/MapPlaces.h"
#include "UI/Inventory/MapView.h"
#include "World/GraveTravelRules.h"
#include "MapWidget.generated.h"

class ALooterHUD;
class ARespawnMarker;
class FSlateWindowElementList;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class ULoadoutPaintLayer;
class ULooterButton;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
struct FGeometry;

/** The map page's layout on the inventory's 1600 x 900 page (LoadoutParts::PageSize), shared by its files. */
namespace MapLayout
{
	/** The map's frame: a chamfered card, the map inside it inset by FrameInset. */
	inline const FVector2D FramePosition(60.0, 100.0);
	inline const FVector2D FrameSize(1030.0, 732.0);
	inline constexpr float FrameInset = 10.f;
	inline const FVector2D MapSize(FrameSize.X - 2.0 * FrameInset, FrameSize.Y - 2.0 * FrameInset);

	/** The legend and the travel card, right of the frame. */
	inline constexpr float SideX = 1120.f;
	inline constexpr float SideWidth = 420.f;

	/** The most pins the map draws at once. */
	inline constexpr int32 MaxPins = 48;

	/** How far a pin catches the pointer beyond its badge (page units). */
	inline constexpr float PinReach = 6.f;

	/** Panning speed for the sticks and keys (page units per second at full tilt; a key press moves KeyPanStep). */
	inline constexpr float StickPanSpeed = 900.f;
	inline constexpr float KeyPanStep = 90.f;

	/** Zoom per wheel notch and per key press, and how fast the triggers zoom (doublings per second at full pull). */
	inline constexpr double WheelZoom = 1.25;
	inline constexpr double TriggerZoomRate = 1.5;
}

/**
 * The map, the inventory's fourth page, laid out like the pages beside it:
 *  - left, framed: the whole level from above, north up, from the minimap's runtime bake (UMinimapSubsystem): the player's
 *    arrow, the tracked mission's objective, missions waiting to be turned in at their givers, the open respawn graves, the
 *    stations, the gunsmith's benches, the chests found, and the places' names (the minor ones once zoomed in); a grid, a
 *    scale bar and the compass's N
 *  - right: the legend with each kind's count, then fast travel: the chosen grave, how far, and Travel or why not (a fight,
 *    a scene, a boss's fight: GraveTravelRules), and the open graves as a list to pick from
 *  - bottom: what the keys do
 * Mouse: drag (either button) pans, the wheel zooms about the pointer, a grave is chosen with a click and travelled to with
 * Travel, E or a double click. Keys: W A S D or the arrows pan, + and - (Page Up, Page Down) zoom, G picks the next grave,
 * E or Enter travels, C centres on the player. Gamepad: either stick pans, the triggers zoom, the D-pad picks the grave that
 * way, A travels, Y centres on the player, the left shoulder turns to the missions. 1 to 3 turn to the other pages; Esc
 * (first letting go of a chosen grave), Tab, I or B close. A travel closes the inventory and fades to the grave
 * (UGraveTravelSubsystem).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD);
	void Close();

	/** What the page shows, for the tests. */
	struct FView
	{
		FMapView Map;
		TArray<FMapPin> Pins;
		/** The chosen grave's marker id (None: none chosen). */
		FName SelectedGrave;
		/** Why Travel can't go for the chosen grave (or at all, with none chosen); None: it can. */
		EGraveTravelBlock Block = EGraveTravelBlock::None;
		/** The travel card's status line. */
		FString Status;
		/** The baked map is showing (not only the pins). */
		bool bHasPicture = false;
	};
	FView GetView() const;

	/** A key as the page takes it (NativeOnKeyDown; tests). True when it used it. */
	bool HandleKey(const FKey& Key);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	/** A pin's two images on the map: its badge and its icon, restyled only when what they show changes. */
	struct FPinSlot
	{
		UImage* Badge = nullptr;
		UImage* Icon = nullptr;
		uint32 StyleKey = 0;
	};

	/** A row of the open graves' list on the travel card. */
	struct FGraveRow
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		UTextBlock* Distance = nullptr;
		FName GraveId;
	};

	// MapWidget.cpp: opening, the level's map and pins, and the layout
	void ResolveLevel();
	void ResetView();
	void RefreshPins();
	void BuildMapFrame(UCanvasPanel& Page);
	void BuildSideColumn(UCanvasPanel& Page);
	UWidget* MakeLegendRow(EMapPinKind Kind, UTextBlock*& OutCount);
	UWidget* MakeYouLegendRow();

	// MapWidgetDraw.cpp: the map, its pins and names placed on the frame each frame, and the grid and route painted under them
	void LayoutMap(float DeltaTime);
	void LayoutPins();
	void LayoutPlaces();
	void LayoutPlayer();
	void LayoutHover();
	void PaintMap(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	FVector2D WorldToFrame(const FVector& World) const;
	FVector2D WorldToUV(const FVector& World) const;

	// MapWidgetInput.cpp: keys, the mouse, the sticks (and NativeTick)
	bool ToFrame(const FVector2D& ScreenPosition, FVector2D& OutFramePoint) const;

	// MapWidgetSelect.cpp: pins under the pointer, choosing a grave, the view sent somewhere, travelling, the buttons
	int32 FindPinAt(const FVector2D& FramePoint) const;
	void SetHovered(int32 PinIndex);
	void SelectGrave(FName GraveId, bool bCenter, bool bSound);
	void SelectNextGrave();
	void SelectGraveToward(const FVector2D& Direction);
	void CenterOn(const FVector2D& UV);
	void CenterOnPlayer();
	void ZoomBy(double Factor);
	void ConfirmTravel();
	const FMapPin* FindGravePin(FName GraveId) const;
	void HandleTabClicked(ULooterButton* Button);
	void HandleTravelClicked(ULooterButton* Button);
	void HandleGraveRowClicked(ULooterButton* Button);
	void HandleGraveRowHovered(ULooterButton* Button);

	// MapWidgetCard.cpp: the legend's counts, the travel card, the graves' list and the prompts
	void RefreshLegend();
	void RefreshTravelCard();
	void RebuildGraveList();
	void RestyleGraveList();
	void RefreshPrompts();
	EGraveTravelBlock CheckTravel(FString& OutStatus) const;
	void FlashStatus();

	TWeakObjectPtr<ALooterHUD> OwningHUD;

	/** The level: its package's short name, its area's name, its places, and the baked square the map covers. */
	FString LevelName;
	FText AreaName;
	TConstArrayView<FMapPlace> Places;
	FBox2D MapBounds = FBox2D(FVector2D(-10000.0), FVector2D(10000.0));
	/** The playable area's corners, for the first view (fitted to them). */
	FBox2D PlayableUV = FBox2D(ForceInit);
	TWeakObjectPtr<UTexture2D> MapTexture;
	FSlateBrush MapBrush;

	FMapView View;
	/** Easing the view's centre to a point (a grave chosen, the player found); a pan or a drag stops it. */
	bool bEasing = false;
	FVector2D EaseTarget = FVector2D(0.5, 0.5);

	TArray<FMapPin> Pins;
	float PinRefreshLeft = 0.f;
	float CardRefreshLeft = 0.f;
	int32 Hovered = INDEX_NONE;
	/** The pointed-at pin came from the graves' list (its row under the pointer), not the map. */
	bool bHoverFromList = false;
	FName SelectedGrave;
	/** The travel card's status flashes when Travel is refused; this counts its seconds down. */
	float StatusFlash = 0.f;
	/** Seconds the page has been open, for the chosen grave's pulse. */
	float OpenTime = 0.f;

	// The mouse: a press on the map is a click until it moves past LoadoutParts::DragStartDistance, then a drag that pans.
	bool bPressing = false;
	bool bDragging = false;
	FVector2D PressScreen = FVector2D::ZeroVector;
	FVector2D LastFramePoint = FVector2D::ZeroVector;

	// The gamepad: either stick pans, the triggers zoom (held values, applied each frame).
	FVector2D LeftStick = FVector2D::ZeroVector;
	FVector2D RightStick = FVector2D::ZeroVector;
	float ZoomInAxis = 0.f;
	float ZoomOutAxis = 0.f;

	// The map frame.
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> MapCanvas;
	UPROPERTY(Transient) TObjectPtr<UImage> MapImage;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> MapPaint;
	UPROPERTY(Transient) TObjectPtr<UImage> SelectGlow;
	UPROPERTY(Transient) TObjectPtr<UImage> SelectRing;
	UPROPERTY(Transient) TObjectPtr<UImage> PlayerGlow;
	UPROPERTY(Transient) TObjectPtr<UImage> PlayerArrow;
	UPROPERTY(Transient) TObjectPtr<UWidget> HoverCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoverName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoverDetail;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LoadingText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AreaTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ScaleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ZoomText;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> PlaceLabels;
	TArray<FPinSlot> PinSlots;

	// The side column.
	/** The legend's count for each kind, by EMapPinKind (null for a kind without a count). */
	TArray<UTextBlock*> LegendCounts;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardDetail;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardStatus;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> TravelButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GraveListHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> GraveList;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	TArray<FGraveRow> GraveRows;
	/** The block the travel card last showed, so it rebuilds its words only when they change. */
	EGraveTravelBlock ShownBlock = EGraveTravelBlock::None;
	FString ShownStatus;
};
