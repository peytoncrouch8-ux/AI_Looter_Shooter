#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/InkedIconData.inl"
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

namespace LoadoutParts
{
	const FInkedIcon& GunIcon(EWeaponKind Kind)
	{
		static const FInkedIcon Rifle = InkedIconData::Rifle();
		static const FInkedIcon Shotgun = InkedIconData::Shotgun();
		return Kind == EWeaponKind::Shotgun ? Shotgun : Rifle;
	}

	FName GunIconName(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Shotgun ? FName(TEXT("InkedShotgun")) : FName(TEXT("InkedRifle"));
	}

	const FInkedIcon& AmmoIcon(EAmmoType Type)
	{
		static const FInkedIcon Icons[] = {
			InkedIconData::AmmoAssaultRifle(),
			InkedIconData::AmmoShotgun(),
			InkedIconData::AmmoPistol(),
			InkedIconData::AmmoSMG(),
			InkedIconData::AmmoSniper(),
		};
		static_assert(static_cast<int32>(UE_ARRAY_COUNT(Icons)) == LooterAmmo::NumTypes, "An Inked icon for every ammo type, in EAmmoType order.");
		return Icons[FMath::Clamp(static_cast<int32>(Type), 0, static_cast<int32>(UE_ARRAY_COUNT(Icons)) - 1)];
	}

	FName AmmoIconName(EAmmoType Type)
	{
		return FName(*FString::Printf(TEXT("InkedAmmo%d"), static_cast<int32>(Type)));
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

	UWidget* MakeGunPicture(UWidgetTree* Tree, const FWeaponInstanceData& Item, const FVector2D& Size, const FLinearColor& Tint)
	{
		const EWeaponKind Kind = Item.Definition ? Item.Definition->Kind : EWeaponKind::Rifle;
		UOverlay* Picture = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		// Centered at its own shape, so a box of any proportions around it never stretches it.
		UOverlaySlot* ImageSlot = Picture->AddChildToOverlay(MakeImage(Tree, InkedIconBrush(GunIconName(Kind), GunIcon(Kind), Size, Tint)));
		ImageSlot->SetHorizontalAlignment(HAlign_Center);
		ImageSlot->SetVerticalAlignment(VAlign_Center);
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

	UWidget* MakePageTabs(UWidgetTree* Tree, int32 ShownPage, TArray<ULooterButton*>& OutTabs)
	{
		static const TCHAR* const Pages[] = { TEXT("Loadout"), TEXT("Bestiary"), TEXT("Missions") };
		UHorizontalBox* Strip = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		OutTabs.Reset();
		for (int32 Page = 0; Page < UE_ARRAY_COUNT(Pages); ++Page)
		{
			// The page on screen is the title, as bright as the old single title tab; the other waits, dimmer, beside it.
			const bool bShown = Page == ShownPage;
			UBorder* Plate = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Plate->SetBrush(bShown ? RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f) : RectBrush(Hex(7, 26, 40, 120), Hex(90, 200, 255, 60), 1.f));
			Plate->SetPadding(FMargin(26.f, 5.f));
			Plate->SetContent(Label(Tree, Pages[Page], bShown ? 14 : 12, bShown ? Color::Title() : Color::TextDim(), 350));
			UWidget* Tab = MakeShapeBox(Tree, EShape::Tab, bShown ? Color::FrameFill() : Color::FrameFill() * FLinearColor(1.f, 1.f, 1.f, 0.55f),
				bShown ? Color::FrameEdge() : Color::FrameEdge() * FLinearColor(1.f, 1.f, 1.f, 0.5f), Plate, FMargin(18.f, 5.f, 18.f, 3.f));

			ULooterButton* Button = Tree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
			Button->SetupContent(Tab, ActionPage, Page);
			UHorizontalBoxSlot* TabSlot = Strip->AddChildToHorizontalBox(Button);
			TabSlot->SetVerticalAlignment(VAlign_Bottom);
			TabSlot->SetPadding(FMargin(Page > 0 ? 12.f : 0.f, 0.f, 0.f, 0.f));
			OutTabs.Add(Button);
		}
		return Strip;
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
