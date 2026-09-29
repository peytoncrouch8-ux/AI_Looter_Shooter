#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudMinimapWidget.generated.h"

class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;

/**
 * The HUD's round minimap (top-right) that turns with the view, so the way you look is up: the island from above
 * (baked by UMinimapSubsystem), hostiles as red diamonds, loot as dots in its rarity color, ammo as small dots, your
 * arrow in the middle, and an N that circles the rim. Its size is the player's choice (Settings > Interface > Minimap
 * size, 60-150%). Reads the possessed pawn every frame.
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Resizes the minimap: the map, its rim and the player arrow grow with it, markers a little less. */
	void ApplyScale(float NewScale);
	float GetDiameter() const { return Diameter * Scale; }

	UPROPERTY(Transient) TObjectPtr<USizeBox> SizeBox;
	UPROPERTY(Transient) TObjectPtr<UImage> MapImage;
	UPROPERTY(Transient) TObjectPtr<UImage> Arrow;
	UPROPERTY(Transient) TObjectPtr<UImage> Notch;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> North;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Markers;

	/** The map picture the brush currently shows (owned by UMinimapSubsystem). */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> MapTexture;
	FSlateBrush MapBrush;

	/** Per marker: what it last showed (kind + color), so brushes only change when needed. */
	TArray<uint32> MarkerKeys;

	/** The minimap's size right now, relative to Diameter. */
	float Scale = 1.f;
};
