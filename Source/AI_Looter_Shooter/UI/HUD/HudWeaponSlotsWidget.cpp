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
	// A slot's circle (reference pixels); the slots sit this far apart, center to center, leaving room for the one in
	// hand to grow.
	constexpr float SlotDiameter = 60.f;
	constexpr float SlotSpacing = 76.f;
	/** Points round a whole circle: at the drawn size (twice that in the texture) the facets stay well under a pixel. */
	constexpr int32 CircleSegments = 64;
	/** The glow ring reaches past the circle, so its picture is this much bigger on every side. */
	constexpr float GlowMargin = 6.f;
	/** The rarity edge runs along the circle's bottom, this many degrees of it (centered on the bottom). */
	constexpr float StripeSweep = 100.f;
	/** The slot in hand: raised and grown, eased over LiftTime seconds. */
	constexpr float LiftHeight = 6.f;
	constexpr float LiftScale = 1.14f;
	constexpr float LiftTime = 0.12f;
	/**
	 * The box a slot's gun icon fits in (keeping its shape): the rifle fills its height, the long shotgun its width. A
	 * circle is short across its middle, so the icon tilts up by GunTilt degrees and lies along the rising diagonal,
	 * where a long gun gets more room and still clears the ring and the rarity arc.
	 */
	const FVector2D GunBox(52.f, 24.f);
	constexpr float GunTilt = -22.f;
	/**
	 * The box the ammo icon under a slot fits in. The ammo icons share one square view box sized for the tall sniper
	 * round, so every type draws at this size and the rounds inside come out about 15-21 px: they keep their sizes
	 * relative to each other, as in the inventory, and the row under the circles stays the same height.
	 */
	const FVector2D AmmoBox(26.f, 26.f);

	/** A gun or ammo icon in a slot that isn't in hand: dimmed a little. */
	const FLinearColor RestingIcon(0.72f, 0.72f, 0.72f, 0.9f);
	const FVector2D TabSize(20.f, 16.f);

	/**
	 * Points along the slot's circle, Inset in from its edge, from angle From to To in degrees (clockwise on screen from
	 * the right, so 90 is the bottom: the order fills and strokes take), both ends included. Offset moves the circle
	 * within a bigger picture.
	 */
	TArray<FVector2D> ArcPoints(float Inset, float From, float To, int32 Segments, const FVector2D& Offset = FVector2D::ZeroVector)
	{
		const float Radius = SlotDiameter * 0.5f - Inset;
		const FVector2D Center = Offset + FVector2D(SlotDiameter * 0.5f, SlotDiameter * 0.5f);
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Index) / static_cast<float>(Segments)));
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	/**
	 * The whole circle, Inset in from the slot's edge, as a convex polygon. The last point (back at the start) is dropped:
	 * a fill's edge of almost no length has no direction, which would spoil the polygon's coverage.
	 */
	TArray<FVector2D> CirclePoints(float Inset, const FVector2D& Offset = FVector2D::ZeroVector)
	{
		TArray<FVector2D> Points = ArcPoints(Inset, 0.f, 360.f, CircleSegments, Offset);
		Points.Pop();
		return Points;
	}

	TArray<FVector2D> Closed(TArray<FVector2D> Points)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	/** The disc (a fill) or a ring StrokeWidth wide that stays inside the slot's box (a stroke). */
	FVectorIcon CircleIcon(bool bFill, float StrokeWidth)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
		Icon.StrokeWidth = StrokeWidth;
		if (bFill)
		{
			Icon.Fills.Add(CirclePoints(1.f));
		}
		else
		{
			Icon.Strokes.Add(Closed(CirclePoints(StrokeWidth * 0.5f + 0.5f)));
		}
		return Icon;
	}

	const FVectorIcon& DiscFill() { static const FVectorIcon Icon = CircleIcon(true, 0.f); return Icon; }
	const FVectorIcon& RingOutline() { static const FVectorIcon Icon = CircleIcon(false, 1.5f); return Icon; }
	const FVectorIcon& RingBoldOutline() { static const FVectorIcon Icon = CircleIcon(false, 2.5f); return Icon; }

	/** A wide, soft ring round the circle (drawn faint, in the accent color). */
	const FVectorIcon& RingGlow()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Ring;
			Ring.ViewBox = FVector2D(SlotDiameter + GlowMargin * 2.f, SlotDiameter + GlowMargin * 2.f);
			Ring.StrokeWidth = 7.f;
			Ring.Strokes.Add(Closed(CirclePoints(0.5f, FVector2D(GlowMargin, GlowMargin))));
			return Ring;
		}();
		return Icon;
	}

	/** The rarity edge: a thick arc along the circle's bottom, just inside the ring. */
	const FVectorIcon& RarityArc()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Band;
			Band.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
			Band.StrokeWidth = 4.f;
			Band.Strokes.Add(ArcPoints(3.5f, 90.f - StripeSweep * 0.5f, 90.f + StripeSweep * 0.5f, 32));
			return Band;
		}();
		return Icon;
	}

	/** An empty slot's outline: short dashes evenly round the circle, one centered on the bottom (so it's symmetric). */
	const FVectorIcon& RingDashed()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Dashes;
			Dashes.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
			Dashes.StrokeWidth = 1.5f;
			// About 5 pixels of dash to 4 of gap round the ring.
			constexpr int32 DashCount = 20;
			constexpr float Step = 360.f / DashCount;
			constexpr float Sweep = Step * 5.f / 9.f;
			for (int32 Index = 0; Index < DashCount; ++Index)
			{
				const float Middle = 90.f + Step * Index;
				Dashes.Strokes.Add(ArcPoints(1.25f, Middle - Sweep * 0.5f, Middle + Sweep * 0.5f, 3));
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
		const FVector2D CircleSize(SlotDiameter, SlotDiameter);
		for (int32 Index = 0; Index < MaxSlots; ++Index)
		{
			FSlotWidgets Cell;
			// The circle: glow, fill, rarity edge, ring (plain, bold or dashed), then the gun's icon (its own ink line
			// carries it over any background).
			UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			// The glow ring reaches past the circle: drawn at the circle's size and scaled up so its ring lands on the slot's.
			Cell.Glow = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotGlow"), RingGlow(), 2.f, CircleSize, FLinearColor::White));
			const float GlowScale = (SlotDiameter + GlowMargin * 2.f) / SlotDiameter;
			Cell.Glow->SetRenderScale(FVector2D(GlowScale, GlowScale));
			Cell.Fill = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotFill"), DiscFill(), 2.f, CircleSize, FLinearColor::White));
			// The fills fade with the UI transparency setting; outlines, silhouettes and text don't.
			MarkBackground(Cell.Fill);
			Cell.Stripe = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotStripe"), RarityArc(), 2.f, CircleSize, FLinearColor::White));
			Cell.Outline = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotOutline"), RingOutline(), 2.f, CircleSize, FLinearColor::White));
			Cell.BoldOutline = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotBold"), RingBoldOutline(), 2.f, CircleSize, FLinearColor::White));
			Cell.Dashed = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotDashed"), RingDashed(), 2.f, CircleSize, FLinearColor::White));
			Cell.Gun = AddLayer(WidgetTree, Layers, FSlateBrush());
			// Tilted about its middle: the brush changes with the gun, the tilt stays.
			Cell.Gun->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.Gun->SetRenderTransformAngle(GunTilt);

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

			USizeBox* CircleBox = MakeSized(WidgetTree, Layers, SlotDiameter, SlotDiameter);
			CircleBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.Lifted = CircleBox;

			// Under it, its ammo's icon. Until a gun shows it draws nothing, but it already takes the icon's room, so the
			// row doesn't change height when the first gun arrives (an empty slot hides it the same way).
			UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Column->AddChildToVerticalBox(CircleBox)->SetHorizontalAlignment(HAlign_Center);
			FSlateBrush NoAmmo;
			NoAmmo.DrawAs = ESlateBrushDrawType::NoDrawType;
			NoAmmo.ImageSize = AmmoBox;
			Cell.Ammo = MakeImage(WidgetTree, NoAmmo);
			Cell.Ammo->SetVisibility(ESlateVisibility::Hidden);
			UVerticalBoxSlot* AmmoSlot = Column->AddChildToVerticalBox(Cell.Ammo);
			AmmoSlot->SetHorizontalAlignment(HAlign_Center);
			AmmoSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

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

		// Repaint only when the slot's gun, its ammo type or whether it's in hand changed.
		uint32 Shown = bInHand ? 1u : 0u;
		if (Weapon)
		{
			const FWeaponInstanceData& Item = Weapon->GetInstance();
			const uint32 AmmoKey = Item.Definition ? static_cast<uint32>(Item.Definition->AmmoType) : MAX_uint8;
			Shown = HashCombine(Shown, HashCombine(GetTypeHash(Item.Definition.Get()), HashCombine(GetTypeHash(Item.Seed),
				HashCombine(static_cast<uint32>(Item.Rarity) + 2u, AmmoKey))));
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
		// Empty: a dark socket with a dashed ring, and the key (it reads on grass and sky alike).
		Show(Cell.Glow, false);
		Show(Cell.Stripe, false);
		Show(Cell.Outline, false);
		Show(Cell.BoldOutline, false);
		Show(Cell.Gun, false);
		Show(Cell.Ammo, false);
		Show(Cell.Dashed, true);
		Cell.Dashed->SetColorAndOpacity(Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.6f));
		Cell.Fill->SetColorAndOpacity(Hex(7, 26, 40, 128));
		Cell.Tab->SetBrush(RectBrush(Color::Plate(), Hex(90, 200, 255, 90), 1.f));
		Cell.TabNumber->SetColorAndOpacity(FSlateColor(Color::TextDim()));
		return;
	}

	const FWeaponInstanceData& Item = Weapon->GetInstance();
	const FLinearColor Rarity = LooterWeaponText::Color(Item);
	const EWeaponKind Kind = KindOf(Item);
	Cell.Gun->SetBrush(InkedIconBrush(LoadoutParts::GunIconName(Kind), LoadoutParts::GunIcon(Kind), GunBox));
	Show(Cell.Gun, true);
	Show(Cell.Dashed, false);
	Show(Cell.Stripe, true);
	Cell.Stripe->SetColorAndOpacity(Rarity);
	// The ammo it takes, as the same Inked icon the inventory's ammo gauges show.
	const bool bHasAmmo = Item.Definition && LooterAmmo::IsValid(Item.Definition->AmmoType);
	if (bHasAmmo)
	{
		const EAmmoType AmmoType = Item.Definition->AmmoType;
		Cell.Ammo->SetBrush(InkedIconBrush(LoadoutParts::AmmoIconName(AmmoType), LoadoutParts::AmmoIcon(AmmoType), AmmoBox));
	}
	Show(Cell.Ammo, bHasAmmo);

	// In hand: an accent ring with a soft glow, tinted with its rarity, the gun at full strength; otherwise ringed in
	// its rarity, the gun a little dimmed.
	Show(Cell.Glow, bInHand);
	Show(Cell.BoldOutline, bInHand);
	Show(Cell.Outline, !bInHand);
	Cell.Glow->SetColorAndOpacity(Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.35f));
	// A legendary's rarity edge is orange too: its ring goes light so the two don't merge.
	Cell.BoldOutline->SetColorAndOpacity(Item.Rarity == EWeaponRarity::Legendary ? Color::Text() : Color::Accent());
	Cell.Outline->SetColorAndOpacity(Rarity);
	Cell.Fill->SetColorAndOpacity(bInHand ? Rarity * FLinearColor(1.f, 1.f, 1.f, 0.33f) : Hex(7, 26, 40, 107));
	Cell.Gun->SetColorAndOpacity(bInHand ? FLinearColor::White : RestingIcon);
	Cell.Ammo->SetColorAndOpacity(bInHand ? FLinearColor::White : RestingIcon);
	Cell.Tab->SetBrush(bInHand ? RectBrush(Color::Accent()) : RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f));
	Cell.TabNumber->SetColorAndOpacity(FSlateColor(bInHand ? Color::AccentDark() : Color::TextDim()));
}
