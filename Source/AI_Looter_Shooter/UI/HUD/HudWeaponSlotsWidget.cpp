#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
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
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	// A slot's hexagon (reference pixels), its points at the sides; the slots sit this far apart, center to center.
	constexpr float HexWidth = 68.f;
	constexpr float HexHeight = 52.f;
	constexpr float SlotSpacing = 84.f;
	/** The glow ring reaches past the hex, so its picture is this much bigger on every side. */
	constexpr float GlowMargin = 6.f;
	/** The slot in hand: raised and grown, eased over LiftTime seconds. */
	constexpr float LiftHeight = 6.f;
	constexpr float LiftScale = 1.14f;
	constexpr float LiftTime = 0.12f;
	const FVector2D GunSize(44.f, 15.f);
	const FVector2D TabSize(20.f, 16.f);

	/** The hexagon's corners, Inset in from its box, starting at the left point and going clockwise. */
	TArray<FVector2D> HexPoints(float Inset, const FVector2D& Offset = FVector2D::ZeroVector)
	{
		const float W = HexWidth;
		const float H = HexHeight;
		const float Quarter = W * 0.25f;
		return {
			Offset + FVector2D(Inset, H * 0.5f), Offset + FVector2D(Quarter + Inset * 0.5f, Inset),
			Offset + FVector2D(W - Quarter - Inset * 0.5f, Inset), Offset + FVector2D(W - Inset, H * 0.5f),
			Offset + FVector2D(W - Quarter - Inset * 0.5f, H - Inset), Offset + FVector2D(Quarter + Inset * 0.5f, H - Inset) };
	}

	TArray<FVector2D> Closed(TArray<FVector2D> Points)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	FVectorIcon HexIcon(bool bFill, float StrokeWidth)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(HexWidth, HexHeight);
		Icon.StrokeWidth = StrokeWidth;
		if (bFill)
		{
			Icon.Fills.Add(HexPoints(1.f));
		}
		else
		{
			Icon.Strokes.Add(Closed(HexPoints(StrokeWidth * 0.5f + 0.5f)));
		}
		return Icon;
	}

	const FVectorIcon& HexFill() { static const FVectorIcon Icon = HexIcon(true, 0.f); return Icon; }
	const FVectorIcon& HexOutline() { static const FVectorIcon Icon = HexIcon(false, 1.5f); return Icon; }
	const FVectorIcon& HexBoldOutline() { static const FVectorIcon Icon = HexIcon(false, 2.5f); return Icon; }

	/** A wide, soft ring around the hex (drawn faint, in the accent color). */
	const FVectorIcon& HexGlow()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Ring;
			Ring.ViewBox = FVector2D(HexWidth + GlowMargin * 2.f, HexHeight + GlowMargin * 2.f);
			Ring.StrokeWidth = 7.f;
			Ring.Strokes.Add(Closed(HexPoints(0.5f, FVector2D(GlowMargin, GlowMargin))));
			return Ring;
		}();
		return Icon;
	}

	/** The rarity edge: a band along the two lower edges and the bottom, just inside the outline. */
	const FVectorIcon& HexStripe()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Band;
			Band.ViewBox = FVector2D(HexWidth, HexHeight);
			Band.StrokeWidth = 4.f;
			const TArray<FVector2D> Inner = HexPoints(3.5f);
			Band.Strokes.Add({ Inner[0], Inner[5], Inner[4], Inner[3] });
			return Band;
		}();
		return Icon;
	}

	/** An empty slot's outline: short dashes around the hex. */
	const FVectorIcon& HexDashed()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Dashes;
			Dashes.ViewBox = FVector2D(HexWidth, HexHeight);
			Dashes.StrokeWidth = 1.5f;
			const TArray<FVector2D> Corners = Closed(HexPoints(1.25f));
			constexpr float Dash = 5.f;
			constexpr float Gap = 4.f;
			for (int32 Edge = 0; Edge + 1 < Corners.Num(); ++Edge)
			{
				const FVector2D From = Corners[Edge];
				const FVector2D Along = Corners[Edge + 1] - From;
				const float Length = Along.Size();
				const FVector2D Direction = Along / Length;
				for (float Start = 0.f; Start < Length; Start += Dash + Gap)
				{
					Dashes.Strokes.Add({ From + Direction * Start, From + Direction * FMath::Min(Start + Dash, Length) });
				}
			}
			return Dashes;
		}();
		return Icon;
	}

	UImage* AddLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush, EHorizontalAlignment H = HAlign_Center,
		EVerticalAlignment V = VAlign_Center)
	{
		UImage* Image = MakeImage(Tree, Brush);
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(H);
		LayerSlot->SetVerticalAlignment(V);
		return Image;
	}

	/** The gun's kind of silhouette, for the icon. */
	EWeaponKind KindOf(const FWeaponInstanceData& Item)
	{
		return Item.Definition ? Item.Definition->Kind : EWeaponKind::Rifle;
	}
}

TSharedRef<SWidget> UHudWeaponSlotsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Row;
		Slots.Reset();
		const FVector2D HexSize(HexWidth, HexHeight);
		for (int32 Index = 0; Index < MaxSlots; ++Index)
		{
			FSlotWidgets Cell;
			// The hex: glow, fill, rarity edge, outline (plain, bold or dashed), then the gun's silhouette over a dark copy.
			UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			// The glow ring reaches past the hex: drawn at the hex's size and scaled up so its hex lands on the slot's.
			Cell.Glow = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudHexGlow"), HexGlow(), 2.f, HexSize, FLinearColor::White));
			Cell.Glow->SetRenderScale(FVector2D((HexWidth + GlowMargin * 2.f) / HexWidth, (HexHeight + GlowMargin * 2.f) / HexHeight));
			Cell.Fill = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudHexFill"), HexFill(), 2.f, HexSize, FLinearColor::White));
			// The fills fade with the UI transparency setting; outlines, silhouettes and text don't.
			MarkBackground(Cell.Fill);
			Cell.Stripe = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudHexStripe"), HexStripe(), 2.f, HexSize, FLinearColor::White));
			Cell.Outline = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudHexOutline"), HexOutline(), 2.f, HexSize, FLinearColor::White));
			Cell.BoldOutline = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudHexBold"), HexBoldOutline(), 2.f, HexSize, FLinearColor::White));
			Cell.Dashed = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudHexDashed"), HexDashed(), 2.f, HexSize, FLinearColor::White));
			Cell.GunShadow = AddLayer(WidgetTree, Layers, FSlateBrush());
			Cell.GunShadow->SetRenderTranslation(FVector2D(1.5f, 1.5f));
			Cell.Gun = AddLayer(WidgetTree, Layers, FSlateBrush());

			// The key number, in a tab straddling the top edge.
			Cell.TabNumber = MakeFloatingText(WidgetTree, 11, Color::TextDim(), 0, ETextJustify::Center);
			Cell.TabNumber->SetText(FText::AsNumber(Index + 1));
			Cell.TabNumber->SetShadowColorAndOpacity(FLinearColor::Transparent);
			Cell.Tab = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Cell.Tab->SetBrush(RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f));
			Cell.Tab->SetPadding(FMargin(0.f));
			Cell.Tab->SetHorizontalAlignment(HAlign_Center);
			Cell.Tab->SetVerticalAlignment(VAlign_Center);
			Cell.Tab->SetContent(Cell.TabNumber);
			USizeBox* TabBox = MakeSized(WidgetTree, Cell.Tab, TabSize.X, TabSize.Y);
			TabBox->SetRenderTranslation(FVector2D(0.f, -TabSize.Y * 0.5f));
			UOverlaySlot* TabSlot = Layers->AddChildToOverlay(TabBox);
			TabSlot->SetHorizontalAlignment(HAlign_Center);
			TabSlot->SetVerticalAlignment(VAlign_Top);

			USizeBox* HexBox = MakeSized(WidgetTree, Layers, HexWidth, HexHeight);
			HexBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.Lifted = HexBox;

			// Under it, its ammo class.
			UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Column->AddChildToVerticalBox(HexBox)->SetHorizontalAlignment(HAlign_Center);
			Cell.AmmoClass = MakeFloatingText(WidgetTree, 13, Color::TextDim(), 40, ETextJustify::Center);
			UVerticalBoxSlot* ClassSlot = Column->AddChildToVerticalBox(Cell.AmmoClass);
			ClassSlot->SetHorizontalAlignment(HAlign_Center);
			ClassSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

			Cell.Root = MakeSized(WidgetTree, Column, SlotSpacing, 0.f);
			Row->AddChildToHorizontalBox(Cell.Root);
			Slots.Add(Cell);
		}
	}
	return Super::RebuildWidget();
}

void UHudWeaponSlotsWidget::Update(const UWeaponManagerComponent* Manager, float DeltaTime)
{
	if (!Manager || Slots.IsEmpty())
	{
		return;
	}
	const TArray<AWeaponBase*> Weapons = Manager->GetWeapons();
	const AWeaponBase* Active = Manager->GetActiveWeapon();
	const int32 Count = FMath::Clamp(Manager->MaxWeapons, 0, MaxSlots);
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		FSlotWidgets& Cell = Slots[Index];
		if (Index >= Count)
		{
			Cell.Root->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Cell.Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		const AWeaponBase* Weapon = Weapons.IsValidIndex(Index) ? Weapons[Index] : nullptr;
		const bool bInHand = Weapon && Weapon == Active;

		// Repaint only when the slot's gun or whether it's in hand changed.
		uint32 Shown = bInHand ? 1u : 0u;
		if (Weapon)
		{
			const FWeaponInstanceData& Item = Weapon->GetInstance();
			Shown = HashCombine(Shown, HashCombine(GetTypeHash(Item.Definition.Get()), HashCombine(GetTypeHash(Item.Seed), static_cast<uint32>(Item.Rarity) + 2u)));
		}
		if (Shown != Cell.Shown)
		{
			Cell.Shown = Shown;
			Paint(Cell, Index, Manager, bInHand);
		}

		// The slot in hand rises and grows; the one it took over from settles back.
		const float Wanted = bInHand ? 1.f : 0.f;
		if (Cell.Lift != Wanted)
		{
			Cell.Lift = FMath::FInterpConstantTo(Cell.Lift, Wanted, DeltaTime, 1.f / LiftTime);
			const float Eased = FMath::InterpEaseOut(0.f, 1.f, Cell.Lift, 2.f);
			FWidgetTransform Transform;
			Transform.Translation = FVector2D(0.f, -LiftHeight * Eased);
			Transform.Scale = FVector2D(FMath::Lerp(1.f, LiftScale, Eased));
			Cell.Lifted->SetRenderTransform(Transform);
		}
	}
}

void UHudWeaponSlotsWidget::Paint(FSlotWidgets& Cell, int32 Index, const UWeaponManagerComponent* Manager, bool bInHand)
{
	const TArray<AWeaponBase*> Weapons = Manager->GetWeapons();
	const AWeaponBase* Weapon = Weapons.IsValidIndex(Index) ? Weapons[Index] : nullptr;
	auto Show = [](UWidget* Widget, bool bShown)
	{
		Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	};

	if (!Weapon)
	{
		// Empty: a dark socket with a dashed outline, and the key (it reads on grass and sky alike).
		Show(Cell.Glow, false);
		Show(Cell.Stripe, false);
		Show(Cell.Outline, false);
		Show(Cell.BoldOutline, false);
		Show(Cell.Gun, false);
		Show(Cell.GunShadow, false);
		Show(Cell.Dashed, true);
		Cell.Dashed->SetColorAndOpacity(Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.6f));
		Cell.Fill->SetColorAndOpacity(Hex(7, 26, 40, 128));
		Cell.Tab->SetBrush(RectBrush(Color::Plate(), Hex(90, 200, 255, 90), 1.f));
		Cell.TabNumber->SetColorAndOpacity(FSlateColor(Color::TextDim()));
		Cell.AmmoClass->SetText(FText::GetEmpty());
		return;
	}

	const FWeaponInstanceData& Item = Weapon->GetInstance();
	const FLinearColor Rarity = LooterWeaponText::Color(Item);
	const EWeaponKind Kind = KindOf(Item);
	const FSlateBrush GunBrush = IconBrush(LoadoutParts::GunIconName(Kind, false), LoadoutParts::GunIcon(Kind, false), 1.f, GunSize, FLinearColor::White);
	Cell.Gun->SetBrush(GunBrush);
	Cell.GunShadow->SetBrush(GunBrush);
	Show(Cell.Gun, true);
	Show(Cell.GunShadow, true);
	Show(Cell.Dashed, false);
	Show(Cell.Stripe, true);
	Cell.Stripe->SetColorAndOpacity(Rarity);
	Cell.GunShadow->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Cell.AmmoClass->SetText(FText::FromString(Item.Definition && LooterAmmo::IsValid(Item.Definition->AmmoType)
		? LooterAmmo::GetInfo(Item.Definition->AmmoType).Short : TEXT("")));

	// In hand: an accent outline with a soft glow, tinted with its rarity, the gun white; otherwise outlined in its rarity.
	Show(Cell.Glow, bInHand);
	Show(Cell.BoldOutline, bInHand);
	Show(Cell.Outline, !bInHand);
	Cell.Glow->SetColorAndOpacity(Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.35f));
	// A legendary's rarity edge is orange too: its ring goes light so the two don't merge.
	Cell.BoldOutline->SetColorAndOpacity(Item.Rarity == EWeaponRarity::Legendary ? Color::Text() : Color::Accent());
	Cell.Outline->SetColorAndOpacity(Rarity);
	Cell.Fill->SetColorAndOpacity(bInHand ? Rarity * FLinearColor(1.f, 1.f, 1.f, 0.33f) : Hex(7, 26, 40, 107));
	Cell.Gun->SetColorAndOpacity(bInHand ? Color::Text() : Rarity);
	Cell.Tab->SetBrush(bInHand ? RectBrush(Color::Accent()) : RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f));
	Cell.TabNumber->SetColorAndOpacity(FSlateColor(bInHand ? Color::AccentDark() : Color::TextDim()));
	Cell.AmmoClass->SetColorAndOpacity(FSlateColor(bInHand ? Color::Text() : Color::TextDim()));
}
