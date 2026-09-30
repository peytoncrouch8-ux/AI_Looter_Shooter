#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"

using namespace LooterUI;

namespace
{
	// --- Vector art from the mockup ---

	TArray<FVector2D> RectPoints(double X0, double Y0, double X1, double Y1)
	{
		return { { X0, Y0 }, { X1, Y0 }, { X1, Y1 }, { X0, Y1 } };
	}

	FVectorIcon MakeGunIcon(TArray<TArray<FVector2D>> Fills)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(120.f, 40.f);
		Icon.Fills = MoveTemp(Fills);
		return Icon;
	}

	/** Appends a quadratic curve from the last point, as a few straight steps. */
	void AddCurve(TArray<FVector2D>& Points, const FVector2D& Control, const FVector2D& End)
	{
		const FVector2D Start = Points.Last();
		for (int32 Step = 1; Step <= 4; ++Step)
		{
			const double T = Step / 4.0;
			Points.Add(Start * FMath::Square(1.0 - T) + Control * (2.0 * (1.0 - T) * T) + End * (T * T));
		}
	}
}

namespace LoadoutParts
{
	/** A gun's side view (120 x 40), or just the strip on it that's lit in the gun's rarity color. */
	const FVectorIcon& GunIcon(EWeaponKind Kind, bool bStrip)
	{
		static const FVectorIcon Rifle = MakeGunIcon({
			{ { 2, 14 }, { 22, 12 }, { 30, 14 }, { 30, 24 }, { 22, 26 }, { 4, 30 }, { 2, 28 } },
			{ { 30, 12 }, { 70, 12 }, { 72, 14 }, { 72, 22 }, { 30, 24 } },
			RectPoints(72, 13, 96, 21), RectPoints(96, 15.5, 116, 18.5), RectPoints(114, 14, 119, 20),
			{ { 50, 22 }, { 58, 22 }, { 62, 36 }, { 54, 36 } },
			{ { 36, 22 }, { 42, 22 }, { 40, 34 }, { 34, 34 } },
			RectPoints(44, 8, 54, 12) });
		static const FVectorIcon RifleStrip = MakeGunIcon({ RectPoints(34, 16, 62, 18), RectPoints(74, 15, 92, 16.5) });
		static const FVectorIcon Shotgun = MakeGunIcon({
			{ { 2, 16 }, { 20, 13 }, { 30, 14 }, { 30, 22 }, { 4, 30 }, { 2, 28 } },
			{ { 30, 12 }, { 54, 12 }, { 56, 14 }, { 56, 22 }, { 30, 22 } },
			RectPoints(56, 13, 118, 16.5), RectPoints(56, 17.5, 108, 20.5), RectPoints(66, 16.5, 88, 23.5),
			{ { 34, 21 }, { 40, 21 }, { 38, 32 }, { 32, 32 } },
			RectPoints(114, 11.2, 116.5, 13) });
		static const FVectorIcon ShotgunStrip = MakeGunIcon({ RectPoints(33, 15, 52, 17) });
		if (Kind == EWeaponKind::Shotgun)
		{
			return bStrip ? ShotgunStrip : Shotgun;
		}
		return bStrip ? RifleStrip : Rifle;
	}

	FName GunIconName(EWeaponKind Kind, bool bStrip)
	{
		const TCHAR* Name = Kind == EWeaponKind::Shotgun ? TEXT("Shotgun") : TEXT("Rifle");
		return FName(*FString::Printf(TEXT("Gun%s%s"), Name, bStrip ? TEXT("Strip") : TEXT("")));
	}

	/** Outlined cartridges (24 x 24), one per ammo type, in EAmmoType order. */
	const FVectorIcon& AmmoIcon(EAmmoType Type)
	{
		auto Make = [](TArray<TArray<FVector2D>> Strokes)
		{
			FVectorIcon Icon;
			Icon.ViewBox = FVector2D(24.f, 24.f);
			Icon.Strokes = MoveTemp(Strokes);
			Icon.StrokeWidth = 1.8f;
			return Icon;
		};
		static const FVectorIcon Icons[] = {
			Make({ { { 9, 21 }, { 9, 9 }, { 12, 3 }, { 15, 9 }, { 15, 21 }, { 9, 21 } }, { { 9, 17 }, { 15, 17 } } }),
			[&Make]
			{
				TArray<FVector2D> Shell = { { 7, 21 }, { 7, 7 } };
				AddCurve(Shell, { 7, 4 }, { 10, 4 });
				Shell.Add({ 14, 4 });
				AddCurve(Shell, { 17, 4 }, { 17, 7 });
				Shell.Add({ 17, 21 });
				Shell.Add({ 7, 21 });
				return Make({ Shell, { { 7, 17 }, { 17, 17 } } });
			}(),
			Make({ { { 9, 21 }, { 9, 12 }, { 12, 7 }, { 15, 12 }, { 15, 21 }, { 9, 21 } }, { { 9, 18 }, { 15, 18 } } }),
			Make({ { { 5, 21 }, { 5, 12 }, { 7.5, 8 }, { 10, 12 }, { 10, 21 }, { 5, 21 } },
				{ { 14, 21 }, { 14, 12 }, { 16.5, 8 }, { 19, 12 }, { 19, 21 }, { 14, 21 } } }),
			Make({ { { 10, 22 }, { 10, 8 }, { 12, 2 }, { 14, 8 }, { 14, 22 }, { 10, 22 } }, { { 10, 18 }, { 14, 18 } } }),
		};
		return Icons[FMath::Clamp(static_cast<int32>(Type), 0, static_cast<int32>(UE_ARRAY_COUNT(Icons)) - 1)];
	}

	/** Small solid triangles for upgrade / weaker (10 x 8). */
	const FVectorIcon& ArrowIcon(bool bUp)
	{
		auto Make = [](TArray<FVector2D> Triangle)
		{
			FVectorIcon Icon;
			Icon.ViewBox = FVector2D(10.f, 8.f);
			Icon.Fills = { MoveTemp(Triangle) };
			return Icon;
		};
		static const FVectorIcon Up = Make({ { 5, 0.5 }, { 9.5, 7.5 }, { 0.5, 7.5 } });
		static const FVectorIcon Down = Make({ { 0.5, 0.5 }, { 9.5, 0.5 }, { 5, 7.5 } });
		return bUp ? Up : Down;
	}

	/** A plain disc (stretched to the floor ring's ellipse for its glow). */
	const FVectorIcon& DiscIcon()
	{
		static const FVectorIcon Disc = []
		{
			FVectorIcon Icon;
			Icon.ViewBox = FVector2D(64.f, 64.f);
			TArray<FVector2D>& Circle = Icon.Fills.AddDefaulted_GetRef();
			for (int32 Step = 0; Step < 48; ++Step)
			{
				const double Angle = UE_TWO_PI * Step / 48.0;
				Circle.Add(FVector2D(32.0 + 31.0 * FMath::Cos(Angle), 32.0 + 31.0 * FMath::Sin(Angle)));
			}
			return Icon;
		}();
		return Disc;
	}

	// --- Widget helpers ---

	UTextBlock* Label(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, int32 LetterSpacing)
	{
		return MakeText(Tree, Text, Size, TextColor, true, LetterSpacing);
	}

	/** Uppercase text that ends in "..." rather than spilling out of its space. */
	UTextBlock* FittedLabel(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, int32 LetterSpacing)
	{
		UTextBlock* Block = Label(Tree, Text, Size, TextColor, LetterSpacing);
		Block->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		return Block;
	}

	/**
	 * A chamfered card: fill and outline shapes, white and tinted per state, around padded content. Scale grows the corner
	 * cut and the outline with the card (2 = the slot cards' 14 px cut and 2 px line).
	 */
	UOverlay* MakeCard(UWidgetTree* Tree, UWidget* Content, const FMargin& Padding, float Scale, UImage*& OutFill, UImage*& OutLine)
	{
		auto Shape = [Tree, Scale](bool bOutline)
		{
			FSlateBrush Brush = ShapeBrush(EShape::Control, bOutline, FLinearColor::White);
			Brush.ImageSize *= Scale;
			return MakeImage(Tree, Brush);
		};
		UOverlay* Box = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		OutFill = Shape(false);
		// The fill is background: the UI transparency setting fades it, on top of its state tint.
		MarkBackground(OutFill);
		FillOverlaySlot(Box->AddChildToOverlay(OutFill));
		OutLine = Shape(true);
		FillOverlaySlot(Box->AddChildToOverlay(OutLine));
		FillOverlaySlot(Box->AddChildToOverlay(Content), Padding);
		return Box;
	}

	/** A gun's silhouette, optionally with its rarity strip lit. */
	UWidget* MakeGunPicture(UWidgetTree* Tree, const FWeaponInstanceData& Item, const FVector2D& Size, float PixelsPerUnit,
		const FLinearColor& BodyColor, bool bStrip)
	{
		const EWeaponKind Kind = Item.Definition ? Item.Definition->Kind : EWeaponKind::Rifle;
		UOverlay* Picture = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Picture->AddChildToOverlay(MakeImage(Tree, IconBrush(GunIconName(Kind, false), GunIcon(Kind, false), PixelsPerUnit, Size, BodyColor)));
		if (bStrip)
		{
			Picture->AddChildToOverlay(MakeImage(Tree, IconBrush(GunIconName(Kind, true), GunIcon(Kind, true), PixelsPerUnit, Size,
				LooterWeaponText::Color(Item))));
		}
		return Picture;
	}

	/** A key cap and what the key does: [E] SWAP. The first (main) action's cap is lit. */
	UWidget* MakeKeyHint(UWidgetTree* Tree, const FString& Key, const FString& Text, bool bPrimary)
	{
		UHorizontalBox* Hint = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* KeyText = MakeText(Tree, Key, 9, bPrimary ? Color::AccentDark() : Color::Title(), true, 40);
		if (bPrimary)
		{
			KeyText->SetShadowColorAndOpacity(FLinearColor::Transparent);
		}
		UBorder* Cap = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Cap->SetBrush(bPrimary ? RectBrush(Color::Accent()) : RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f));
		Cap->SetPadding(FMargin(6.f, 2.f));
		Cap->SetHorizontalAlignment(HAlign_Center);
		Cap->SetContent(KeyText);
		Hint->AddChildToHorizontalBox(MakeSized(Tree, Cap, 0.f, 22.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* TextSlot = Hint->AddChildToHorizontalBox(Label(Tree, Text, 9, bPrimary ? Color::Text() : Color::TextDim(), 140));
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		return Hint;
	}

	FString FormatDelta(float Delta, int32 Decimals)
	{
		FNumberFormattingOptions Options;
		Options.UseGrouping = false;
		Options.MinimumFractionalDigits = 0;
		// Big changes read better whole (the values beside them are rounded too).
		Options.MaximumFractionalDigits = FMath::Abs(Delta) >= 10.f ? 0 : Decimals;
		return (Delta > 0.f ? TEXT("+") : TEXT("")) + FText::AsNumber(Delta, &Options).ToString();
	}

	FString AmmoName(const FWeaponInstanceData& Item)
	{
		return Item.Definition ? LooterAmmo::GetInfo(Item.Definition->AmmoType).Name : TEXT("");
	}

	/** "Assault Rifles": what the swap list calls the chosen slot's kind of gun. */
	FString KindName(const FWeaponInstanceData& Item)
	{
		return Item.Definition ? Item.Definition->DisplayName.ToString() + TEXT("s") : FString(TEXT("Weapons"));
	}

	void DrawLines(FSlateWindowElementList& Elements, int32 LayerId, const FGeometry& Geometry, TArray<FVector2f> Points,
		const FLinearColor& LineColor, float Thickness)
	{
		if (Points.Num() >= 2)
		{
			FSlateDrawElement::MakeLines(Elements, LayerId, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, LineColor, true, Thickness);
		}
	}

	void DrawBox(FSlateWindowElementList& Elements, int32 LayerId, const FGeometry& Geometry, const FVector2f& TopLeft, const FVector2f& Size,
		const FSlateBrush* Brush, const FLinearColor& Tint)
	{
		FSlateDrawElement::MakeBox(Elements, LayerId, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft)), Brush, ESlateDrawEffect::None, Tint);
	}
}
