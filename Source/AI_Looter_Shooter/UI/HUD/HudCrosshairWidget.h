#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/SLeafWidget.h"
#include "Weapons/WeaponTypes.h"
#include "HudCrosshairWidget.generated.h"

/** Which part of a crosshair shape a widget paints: its lines (which kick), or its centre dot (which stays for a third-person aim). */
enum class EHudCrosshairPart : uint8
{
	Lines,
	Dot
};

/**
 * The gun kind's crosshair (HudCrosshair::ShapeFor), painted round its middle: every line twice, the dark edge first and
 * the coloured line over it, as the mockup draws it. Open pushes the lines out as the spread grows (the shotgun's ring grows).
 * The HUD keeps two: the lines, which kick, and the dot alone. In UMG it is UHudCrosshairWidget.
 */
class AI_LOOTER_SHOOTER_API SHudCrosshair : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SHudCrosshair) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetKind(EWeaponKind InKind);
	void SetPart(EHudCrosshairPart InPart);
	/** HUD px the lines have moved out past their rest (HudCrosshair::OpenOf). */
	void SetOpen(float InOpen);
	void SetColour(const FLinearColor& InColour);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	EWeaponKind Kind = EWeaponKind::None;
	EHudCrosshairPart Part = EHudCrosshairPart::Lines;
	float Open = 0.f;
	FLinearColor Colour = FLinearColor::White;
};

UCLASS()
class AI_LOOTER_SHOOTER_API UHudCrosshairWidget : public UWidget
{
	GENERATED_BODY()

public:
	void SetKind(EWeaponKind InKind);
	EWeaponKind GetKind() const { return Kind; }
	void SetPart(EHudCrosshairPart InPart);
	EHudCrosshairPart GetPart() const { return Part; }
	void SetOpen(float InOpen);
	float GetOpen() const { return Open; }
	void SetColour(const FLinearColor& InColour);
	const FLinearColor& GetColour() const { return Colour; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SHudCrosshair> Crosshair;
	EWeaponKind Kind = EWeaponKind::None;
	EHudCrosshairPart Part = EHudCrosshairPart::Lines;
	float Open = 0.f;
	FLinearColor Colour = FLinearColor::White;
};
