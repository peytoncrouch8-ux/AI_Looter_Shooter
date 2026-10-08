#include "UI/Inventory/LoadoutParts.h"
#include "Bestiary/Ledger.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/InkedIconData.inl"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Rendering/DrawElements.h"

using namespace LooterUI;

namespace
{
	/** The tier's word on a card, and its color: warmer as the gun earns it, the soul-forged one in the kit's cyan. */
	const TCHAR* NotchTierWord(ENotchTier Tier)
	{
		switch (Tier)
		{
		case ENotchTier::Blooded: return TEXT("Blooded");
		case ENotchTier::Named: return TEXT("Named");
		case ENotchTier::SoulForged: return TEXT("Soul-forged");
		default: return TEXT("");
		}
	}

	FLinearColor NotchTierColor(ENotchTier Tier)
	{
		switch (Tier)
		{
		case ENotchTier::Blooded: return Color::Accent();
		case ENotchTier::Named: return Color::AccentLight();
		case ENotchTier::SoulForged: return Color::CyanText();
		default: return Color::TextDim();
		}
	}

	/** Content in a column of its own width, so the glyphs of different rows line their words up. */
	UWidget* InGlyphColumn(UWidgetTree* Tree, UWidget* Content, float Width, EVerticalAlignment Vertical, float Top)
	{
		UOverlay* Column = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UOverlaySlot* ContentSlot = Column->AddChildToOverlay(Content);
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(Vertical);
		ContentSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		return MakeSized(Tree, Column, Width);
	}
}

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

	UWidget* MakeGunNameLine(UWidgetTree* Tree, const FWeaponInstanceData& Item, int32 FontSize, const FLinearColor& NameColor, int32 LetterSpacing)
	{
		UTextBlock* Name = FittedLabel(Tree, LooterWeaponText::Name(Item), FontSize, NameColor, LetterSpacing);
		if (!WeaponCurses::Of(Item))
		{
			return Name;
		}
		// The coin is about a line of type tall; the name takes the rest of the row and ends in "..." if it doesn't fit.
		const float CoinSize = FMath::Clamp(FontSize * 1.5f, 14.f, 20.f);
		UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const FLinearColor CoinTint(1.f, 1.f, 1.f, Item.bCurseLifted ? 0.55f : 1.f);
		UHorizontalBoxSlot* CoinSlot = Line->AddChildToHorizontalBox(MakeImage(Tree, CrackedCoinBrush(FVector2D(CoinSize, CoinSize), CoinTint)));
		CoinSlot->SetVerticalAlignment(VAlign_Center);
		CoinSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		return Line;
	}

	UWidget* MakeGunIdeasRows(UWidgetTree* Tree, const FWeaponInstanceData& Item, int32 FontSize)
	{
		const FString Notches = LooterWeaponText::NotchesString(Item);
		const FWeaponCurse* Iron = WeaponCurses::Of(Item);
		if (Notches.IsEmpty() && !Iron)
		{
			return nullptr;
		}

		// The glyphs scale with the type but stay where they read; every row keeps a column of that width for its glyph
		// (the perk's and the drawback's arrows too), so all the words start in a line.
		static constexpr float GlyphGap = 7.f;
		const float GlyphSize = FMath::Clamp(FontSize * 1.5f, 14.f, 20.f);
		const FVector2D GlyphBox(GlyphSize, GlyphSize);
		// A wrapped perk keeps its arrow level with the first line: about the middle of a line of this type.
		const float ArrowTop = FontSize * 0.5f;

		UVerticalBox* Rows = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		auto AddLine = [Tree, Rows](float Top)
		{
			UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Rows->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			return Line;
		};
		auto AddGlyph = [](UHorizontalBox* Line, UWidget* Column)
		{
			UHorizontalBoxSlot* GlyphSlot = Line->AddChildToHorizontalBox(Column);
			GlyphSlot->SetVerticalAlignment(VAlign_Top);
			GlyphSlot->SetPadding(FMargin(0.f, 0.f, GlyphGap, 0.f));
		};
		// A line of text that takes the rest of the row and wraps there.
		auto AddWords = [Tree](UHorizontalBox* Line, const FString& Words, int32 Size, const FLinearColor& WordsColor, int32 Spacing)
		{
			UTextBlock* Text = Label(Tree, Words, Size, WordsColor, Spacing);
			Text->SetAutoWrapText(true);
			UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(Text);
			TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TextSlot->SetVerticalAlignment(VAlign_Center);
		};

		// The notches: a tally mark, the count, and the tier's word once the gun has earned one.
		if (!Notches.IsEmpty())
		{
			const ENotchTier Tier = WeaponNotches::TierFor(Item.Kills);
			UHorizontalBox* Line = AddLine(0.f);
			AddGlyph(Line, InGlyphColumn(Tree, MakeImage(Tree, TallyBrush(GlyphBox, NotchTierColor(Tier))), GlyphSize, VAlign_Center, 0.f));
			Line->AddChildToHorizontalBox(Label(Tree, Notches, FontSize, Color::Text(), 100))->SetVerticalAlignment(VAlign_Center);
			if (Tier != ENotchTier::None)
			{
				UHorizontalBoxSlot* TierSlot = Line->AddChildToHorizontalBox(Label(Tree, NotchTierWord(Tier), FontSize, NotchTierColor(Tier), 200));
				TierSlot->SetVerticalAlignment(VAlign_Center);
				TierSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			}
		}

		if (Iron)
		{
			const bool bLifted = Item.bCurseLifted;

			// The cracked coin and the curse's name; the coin fades once the curse is lifted. The rarity color stays on the
			// gun's name, so the curse has its own brass.
			UHorizontalBox* NameLine = AddLine(Notches.IsEmpty() ? 0.f : 6.f);
			AddGlyph(NameLine, InGlyphColumn(Tree, MakeImage(Tree, CrackedCoinBrush(GlyphBox, FLinearColor(1.f, 1.f, 1.f, bLifted ? 0.55f : 1.f))),
				GlyphSize, VAlign_Center, 0.f));
			NameLine->AddChildToHorizontalBox(Label(Tree, Iron->Name.ToString(), FontSize + 2, Color::Curse(), 120))->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* TagSlot = NameLine->AddChildToHorizontalBox(Label(Tree, bLifted ? TEXT("Lifted") : TEXT("Curse"),
				FMath::Max(FontSize - 1, 7), bLifted ? Color::Better() : Color::TextDim(), 220));
			TagSlot->SetVerticalAlignment(VAlign_Center);
			TagSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));

			// What it gives (up, green) and what it costs (down, red). Once lifted the cost stays on the card, dim, so the
			// player can see what they were rid of.
			auto AddEffect = [&](bool bPerk, const FText& Words, const FLinearColor& EffectColor, float Top)
			{
				UHorizontalBox* Line = AddLine(Top);
				AddGlyph(Line, InGlyphColumn(Tree, MakeImage(Tree, IconBrush(bPerk ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(bPerk), 4.f,
					FVector2D(10.f, 8.f), EffectColor)), GlyphSize, VAlign_Top, ArrowTop));
				AddWords(Line, Words.ToString(), FontSize, EffectColor, 60);
			};
			AddEffect(true, Iron->Perk, Color::Better(), 3.f);
			AddEffect(false, Iron->Drawback, bLifted ? Color::TextDim() : Color::Worse(), 2.f);
		}
		return Rows;
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

	void TitlePageTabs(UWidgetTree& Tree, bool bLedger)
	{
		// The bestiary's tab is the second (EInventoryPage order); its title is the first text in its button.
		constexpr int32 BestiaryPage = 1;
		const FText Title = FText::FromString(FString(Ledger::BookName(bLedger)).ToUpper());
		Tree.ForEachWidget([&Title](UWidget* Widget)
		{
			ULooterButton* Tab = Cast<ULooterButton>(Widget);
			if (!Tab || Tab->Action != ActionPage || Tab->Index != BestiaryPage)
			{
				return;
			}
			UTextBlock* TabText = nullptr;
			UWidgetTree::ForWidgetAndChildren(Tab, [&TabText](UWidget* Child)
			{
				TabText = TabText ? TabText : Cast<UTextBlock>(Child);
			});
			if (TabText && !TabText->GetText().EqualTo(Title))
			{
				TabText->SetText(Title);
			}
		});
	}

	UWidget* MakePageTabs(UWidgetTree* Tree, int32 ShownPage, TArray<ULooterButton*>& OutTabs)
	{
		static const TCHAR* const Pages[] = { TEXT("Loadout"), Ledger::BookName(false), TEXT("Missions") };
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
