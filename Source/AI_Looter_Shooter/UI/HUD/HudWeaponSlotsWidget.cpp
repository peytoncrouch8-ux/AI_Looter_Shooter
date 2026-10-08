#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/WidgetTree.h"
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
	constexpr float SlotDiameter = UHudWeaponSlotsWidget::SlotDiameter;
	constexpr float SlotRadius = SlotDiameter * 0.5f;
	/** The space between one circle and the next down the column. */
	constexpr float SlotGap = UHudWeaponSlotsWidget::SlotSpacing - SlotDiameter;
	/** Points round a whole circle: at the drawn size (twice that in the texture) the facets stay well under a pixel. */
	constexpr int32 CircleSegments = 64;
	/** The textures are drawn at twice the size they show at, so they stay crisp. */
	constexpr float SlotPixelsPerUnit = 2.f;

	// The circle's layers, as radii from its centre (1080p pixels, the mockup's). The rim reaches under the gunmetal ring,
	// and the disc a little under the ring's inner edge, so no seam shows between them.
	constexpr float RimOuter = 31.8f;
	constexpr float RimInner = 27.5f;
	constexpr float MetalOuter = 30.f;
	constexpr float MetalInner = 27.5f;
	constexpr float DiscRadius = 28.f;
	constexpr float HairlineRadius = 27.9f;
	constexpr float HairlineWidth = 1.f;
	/** The rarity arc: a thick band along the circle's bottom, this many degrees of it (centred on the bottom). */
	constexpr float ArcRadius = 27.25f;
	constexpr float ArcWidth = 4.5f;
	constexpr float ArcSweep = 100.f;
	/** The ring round the slot in hand, over the rim and a little past it. */
	constexpr float AccentRadius = 31.6f;
	constexpr float AccentWidth = 3.f;
	/** That ring reaches past the circle, so its picture has this much room on every side and is scaled up to match. */
	constexpr float AccentMargin = 4.f;
	/** An empty slot's dashed ring: about 4.4 px of dash to 4.4 of gap. */
	constexpr float DashRadius = 28.f;
	constexpr float DashWidth = 2.f;
	constexpr int32 DashCount = 20;
	/**
	 * The soft orange glow behind the slot in hand, across, and how strong it is at its middle: about a quarter strength
	 * at the circle's edge, gone 16 px out (GlowBrush falls off smoothly to its rim), as in the mockup.
	 */
	constexpr float GlowSize = 96.f;
	constexpr float GlowAlpha = 0.75f;

	// The row left of the circle: the ammo icon, then the key tab, each this far apart.
	constexpr float AmmoSize = 20.f;
	constexpr float RowGap = 8.f;
	constexpr float TabWidth = 22.f;
	constexpr float TabHeight = 16.f;
	/** The tab's bottom-right corner is cut from this far down its right side to this far along its bottom. */
	constexpr float TabCutRight = 11.2f;
	constexpr float TabCutBottom = 17.16f;
	constexpr float TabEdgeWidth = 1.4f;
	/** The tab is small, so its pictures are drawn finer than the circle's. */
	constexpr float TabPixelsPerUnit = 3.f;
	constexpr float RowWidth = AmmoSize + RowGap + TabWidth + RowGap + SlotDiameter;

	/** The slot in hand: the row moves left toward the screen's middle and grows about the circle's centre, eased. */
	constexpr float LiftShift = 9.f;
	constexpr float LiftScale = 1.14f;
	constexpr float LiftTime = 0.12f;
	/**
	 * The box a slot's gun icon fits in (keeping its shape): the rifle fills its height (56 px wide), the long shotgun
	 * its width (58 px). A circle is short across its middle, so the icon tilts up by GunTilt degrees and lies along the
	 * rising diagonal, where a long gun gets the most room: its ends reach the ring.
	 */
	const FVector2D GunBox(58.f, 30.f);
	constexpr float GunTilt = -22.f;

	/** A gun or ammo icon in a slot that isn't in hand: dimmed a little (grey dims an Inked icon). */
	const FLinearColor RestingIcon(0.72f, 0.72f, 0.72f, 0.9f);

	FLinearColor SlotWithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
	}

	/**
	 * Points round a circle of Radius about Center, from angle From to To in degrees (clockwise on screen from the right,
	 * so 90 is the bottom: the order fills and strokes take), both ends included.
	 */
	TArray<FVector2D> SlotArc(const FVector2D& Center, float Radius, float From, float To, int32 Segments)
	{
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
	 * The whole circle as a polygon. The last point (back at the start) is dropped: a fill's edge of almost no length has
	 * no direction, which would spoil the polygon's coverage.
	 */
	TArray<FVector2D> SlotCircle(const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points = SlotArc(Center, Radius, 0.f, 360.f, CircleSegments);
		Points.Pop();
		return Points;
	}

	TArray<FVector2D> SlotClosed(TArray<FVector2D> Points)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	/** The circle's centre in its own picture. */
	const FVector2D SlotCenter(SlotRadius, SlotRadius);

	/** A picture the size of the slot: one ring, Radius to its middle and Width wide (or the disc of Radius, filled). */
	FVectorIcon SlotRing(float Radius, float Width, bool bFilled = false)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
		Icon.StrokeWidth = Width;
		if (bFilled)
		{
			Icon.Fills.Add(SlotCircle(SlotCenter, Radius));
		}
		else
		{
			Icon.Strokes.Add(SlotClosed(SlotCircle(SlotCenter, Radius)));
		}
		return Icon;
	}

	const FVectorIcon& SlotDiscIcon() { static const FVectorIcon Icon = SlotRing(DiscRadius, 0.f, true); return Icon; }
	const FVectorIcon& SlotRimIcon() { static const FVectorIcon Icon = SlotRing((RimOuter + RimInner) * 0.5f, RimOuter - RimInner); return Icon; }
	const FVectorIcon& SlotHairlineIcon() { static const FVectorIcon Icon = SlotRing(HairlineRadius, HairlineWidth); return Icon; }

	/**
	 * The gunmetal ring, lit from the top like the player frame's medallion: the upper half runs from MetalHi to MetalMid,
	 * the lower from MetalLow to MetalDeep. The lower tones go under the whole ring first (flat MetalLow above the middle,
	 * nearly MetalMid), so the upper half's edge leaves no seam.
	 */
	const FPaintedIcon& SlotMetalIcon()
	{
		static const FPaintedIcon Icon = []()
		{
			const FVector2D& Center = SlotCenter;
			FPaintedIcon Ring;
			Ring.ViewBox = FVector2D(SlotDiameter, SlotDiameter);

			FPaintLayer Lower;
			// Even-odd: the inner circle cuts the hole.
			Lower.Fills.Add(SlotCircle(Center, MetalOuter));
			Lower.Fills.Add(SlotCircle(Center, MetalInner));
			Lower.Color = Color::MetalLow();
			Lower.GradientTo = Color::MetalDeep();
			Lower.GradientStart = Center;
			Lower.GradientEnd = Center + FVector2D(0.f, MetalOuter);
			Ring.Layers.Add(Lower);

			FPaintLayer Upper;
			TArray<FVector2D> HalfRing = SlotArc(Center, MetalOuter, 180.f, 360.f, CircleSegments / 2);
			HalfRing.Append(SlotArc(Center, MetalInner, 360.f, 180.f, CircleSegments / 2));
			Upper.Fills.Add(HalfRing);
			Upper.Color = Color::MetalHi();
			Upper.GradientTo = Color::MetalMid();
			Upper.GradientStart = Center - FVector2D(0.f, MetalOuter);
			Upper.GradientEnd = Center;
			Ring.Layers.Add(Upper);
			return Ring;
		}();
		return Icon;
	}

	/** The rarity arc along the circle's bottom. Its round ends are pulled in so the band, ends included, spans ArcSweep. */
	const FVectorIcon& SlotArcIcon()
	{
		static const FVectorIcon Icon = []()
		{
			const float CapDegrees = FMath::RadiansToDegrees(ArcWidth * 0.5f / ArcRadius);
			const float Half = ArcSweep * 0.5f - CapDegrees;
			FVectorIcon Band;
			Band.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
			Band.StrokeWidth = ArcWidth;
			Band.Strokes.Add(SlotArc(SlotCenter, ArcRadius, 90.f - Half, 90.f + Half, 32));
			return Band;
		}();
		return Icon;
	}

	/** The accent ring round the slot in hand, in a picture AccentMargin bigger on every side than the slot. */
	const FVectorIcon& SlotAccentIcon()
	{
		static const FVectorIcon Icon = []()
		{
			const float Size = SlotDiameter + AccentMargin * 2.f;
			FVectorIcon Ring;
			Ring.ViewBox = FVector2D(Size, Size);
			Ring.StrokeWidth = AccentWidth;
			Ring.Strokes.Add(SlotClosed(SlotCircle(FVector2D(Size * 0.5f, Size * 0.5f), AccentRadius)));
			return Ring;
		}();
		return Icon;
	}

	/** An empty slot's ring: short dashes evenly round the circle, one centred on the bottom (so it's symmetric). */
	const FVectorIcon& SlotDashedIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Dashes;
			Dashes.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
			Dashes.StrokeWidth = DashWidth;
			// Half of each step is dash, round ends included: the line between the ends is shorter by the stroke's width.
			constexpr float Step = 360.f / DashCount;
			const float CapDegrees = FMath::RadiansToDegrees(DashWidth * 0.5f / DashRadius);
			const float Sweep = FMath::Max(Step * 0.5f - CapDegrees * 2.f, 0.5f);
			for (int32 Index = 0; Index < DashCount; ++Index)
			{
				const float Middle = 90.f + Step * Index;
				Dashes.Strokes.Add(SlotArc(SlotCenter, DashRadius, Middle - Sweep * 0.5f, Middle + Sweep * 0.5f, 3));
			}
			return Dashes;
		}();
		return Icon;
	}

	/** The key tab's plate: a rectangle with its bottom-right corner cut, clockwise on screen from the top-left. */
	TArray<FVector2D> TabPlatePoints()
	{
		return { FVector2D(0.f, 0.f), FVector2D(TabWidth, 0.f), FVector2D(TabWidth, TabCutRight), FVector2D(TabCutBottom, TabHeight),
			FVector2D(0.f, TabHeight) };
	}

	/** A convex polygon (clockwise on screen) with every edge moved Distance inward. */
	TArray<FVector2D> SlotInset(const TArray<FVector2D>& Points, float Distance)
	{
		TArray<FVector2D> Result;
		Result.Reserve(Points.Num());
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2D& Prev = Points[(Index + Points.Num() - 1) % Points.Num()];
			const FVector2D& Here = Points[Index];
			const FVector2D& Next = Points[(Index + 1) % Points.Num()];
			// Clockwise on screen (y down), the inside lies to the right of travel.
			const FVector2D In = (Here - Prev).GetSafeNormal();
			const FVector2D Out = (Next - Here).GetSafeNormal();
			const FVector2D NormalIn(-In.Y, In.X);
			const FVector2D NormalOut(-Out.Y, Out.X);
			Result.Add(Here + (NormalIn + NormalOut) * (Distance / (1.f + FVector2D::DotProduct(NormalIn, NormalOut))));
		}
		return Result;
	}

	const FVectorIcon& TabPlateIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Plate;
			Plate.ViewBox = FVector2D(TabWidth, TabHeight);
			Plate.Fills.Add(TabPlatePoints());
			return Plate;
		}();
		return Icon;
	}

	/** The tab's edge, drawn just inside the plate. */
	const FVectorIcon& TabEdgeIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Edge;
			Edge.ViewBox = FVector2D(TabWidth, TabHeight);
			Edge.StrokeWidth = TabEdgeWidth;
			Edge.Strokes.Add(SlotClosed(SlotInset(TabPlatePoints(), TabEdgeWidth * 0.5f)));
			return Edge;
		}();
		return Icon;
	}

	UImage* AddSlotLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(Tree, Brush);
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Center);
		LayerSlot->SetVerticalAlignment(VAlign_Center);
		return Image;
	}
}

TSharedRef<SWidget> UHudWeaponSlotsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Column->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Column;
		Slots.Reset();
		const FVector2D CircleSize(SlotDiameter, SlotDiameter);
		const FVector2D TabSize(TabWidth, TabHeight);
		for (int32 Index = 0; Index < MaxSlots; ++Index)
		{
			FSlotWidgets Cell;

			// The circle, back to front: the glow, the inner disc, the rim and gunmetal ring, the rarity arc, the cyan
			// hairline, the in-hand ring or an empty slot's dashes, then the gun's icon (its own ink line carries it over
			// any background).
			UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			// Pictures that reach past the circle are laid out at its size and scaled up about its centre, which leaves
			// the layout alone.
			Cell.Glow = AddSlotLayer(WidgetTree, Layers, GlowBrush(CircleSize, SlotWithAlpha(Color::Accent(), GlowAlpha)));
			Cell.Glow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.Glow->SetRenderScale(FVector2D(GlowSize / SlotDiameter));
			Cell.Glow->SetRenderOpacity(0.f);
			Cell.Disc = AddSlotLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotDisc"), SlotDiscIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White));
			// The disc fades with the UI transparency setting; the rings, icons and number don't.
			MarkBackground(Cell.Disc);
			Cell.Rim = AddSlotLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotRim"), SlotRimIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White));
			Cell.Metal = AddSlotLayer(WidgetTree, Layers, PaintedIconBrush(TEXT("HudSlotMetal"), SlotMetalIcon(), SlotPixelsPerUnit, CircleSize));
			Cell.Arc = AddSlotLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotArc"), SlotArcIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White));
			Cell.Hairline = AddSlotLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotHairline"), SlotHairlineIcon(), SlotPixelsPerUnit, CircleSize,
				SlotWithAlpha(Color::Hairline(), 0.55f)));
			Cell.Dashed = AddSlotLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotDashed"), SlotDashedIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White));
			Cell.AccentRing = AddSlotLayer(WidgetTree, Layers, IconBrush(TEXT("HudSlotAccent"), SlotAccentIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White));
			Cell.AccentRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.AccentRing->SetRenderScale(FVector2D((SlotDiameter + AccentMargin * 2.f) / SlotDiameter));
			Cell.AccentRing->SetRenderOpacity(0.f);
			FSlateBrush NoGun;
			NoGun.DrawAs = ESlateBrushDrawType::NoDrawType;
			Cell.Gun = AddSlotLayer(WidgetTree, Layers, NoGun);
			Cell.Gun->SetVisibility(ESlateVisibility::Hidden);
			// Tilted about its middle: the brush changes with the gun, the tilt stays.
			Cell.Gun->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.Gun->SetRenderTransformAngle(GunTilt);
			USizeBox* CircleBox = MakeSized(WidgetTree, Layers, SlotDiameter, SlotDiameter);

			// The key tab: dark glass (a background) or, in hand, a solid orange plate, with its edge and the number.
			UOverlay* Tab = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			const FSlateBrush Plate = IconBrush(TEXT("HudSlotTab"), TabPlateIcon(), TabPixelsPerUnit, TabSize, FLinearColor::White);
			Cell.TabGlass = AddSlotLayer(WidgetTree, Tab, Plate);
			Cell.TabGlass->SetColorAndOpacity(SlotWithAlpha(Color::ScreenBg(), 0.85f));
			MarkBackground(Cell.TabGlass);
			Cell.TabLit = AddSlotLayer(WidgetTree, Tab, Plate);
			Cell.TabLit->SetColorAndOpacity(Color::Accent());
			Cell.TabEdge = AddSlotLayer(WidgetTree, Tab, IconBrush(TEXT("HudSlotTabEdge"), TabEdgeIcon(), TabPixelsPerUnit, TabSize, FLinearColor::White));
			Cell.TabNumber = MakeFloatingText(WidgetTree, 12, Color::Text(), 0, ETextJustify::Center);
			Cell.TabNumber->SetText(FText::AsNumber(Index + 1));
			UOverlaySlot* NumberSlot = Tab->AddChildToOverlay(Cell.TabNumber);
			NumberSlot->SetHorizontalAlignment(HAlign_Center);
			NumberSlot->SetVerticalAlignment(VAlign_Center);
			USizeBox* TabBox = MakeSized(WidgetTree, Tab, TabWidth, TabHeight);

			// The ammo icon. Until a gun shows it draws nothing, but it already takes the icon's room, so the row keeps
			// its width (an empty slot hides it the same way).
			FSlateBrush NoAmmo;
			NoAmmo.DrawAs = ESlateBrushDrawType::NoDrawType;
			NoAmmo.ImageSize = FVector2D(AmmoSize, AmmoSize);
			Cell.Ammo = MakeImage(WidgetTree, NoAmmo);
			Cell.Ammo->SetVisibility(ESlateVisibility::Hidden);

			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UHorizontalBoxSlot* AmmoSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Cell.Ammo, AmmoSize, AmmoSize));
			AmmoSlot->SetVerticalAlignment(VAlign_Center);
			AmmoSlot->SetPadding(FMargin(0.f, 0.f, RowGap, 0.f));
			UHorizontalBoxSlot* TabSlot = Row->AddChildToHorizontalBox(TabBox);
			TabSlot->SetVerticalAlignment(VAlign_Center);
			TabSlot->SetPadding(FMargin(0.f, 0.f, RowGap, 0.f));
			Row->AddChildToHorizontalBox(CircleBox)->SetVerticalAlignment(VAlign_Center);
			// The slot in hand grows about its circle's centre, so the tab and ammo icon swing out with it.
			Row->SetRenderTransformPivot(FVector2D((RowWidth - SlotRadius) / RowWidth, 0.5f));
			Cell.Lifted = Row;
			Cell.Root = Row;

			UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row);
			RowSlot->SetHorizontalAlignment(HAlign_Right);
			RowSlot->SetPadding(FMargin(0.f, Index > 0 ? SlotGap : 0.f, 0.f, 0.f));
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

		// The slot in hand moves left and grows, its ring and glow coming up; the one it took over from settles back.
		const float Wanted = bInHand ? 1.f : 0.f;
		if (Cell.Lift != Wanted)
		{
			Cell.Lift = FMath::FInterpConstantTo(Cell.Lift, Wanted, DeltaTime, 1.f / LiftTime);
			const float Eased = FMath::InterpEaseOut(0.f, 1.f, Cell.Lift, 2.f);
			FWidgetTransform Transform;
			Transform.Translation = FVector2D(-LiftShift * Eased, 0.f);
			Transform.Scale = FVector2D(FMath::Lerp(1.f, LiftScale, Eased));
			Cell.Lifted->SetRenderTransform(Transform);
			Cell.Glow->SetRenderOpacity(Eased);
			Cell.AccentRing->SetRenderOpacity(Eased);
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
		// Empty: a faint rim round a dark socket with a dashed ring, and the key, dimmed (it reads on grass and sky alike).
		Show(Cell.Glow, false);
		Show(Cell.AccentRing, false);
		Show(Cell.Metal, false);
		Show(Cell.Arc, false);
		Show(Cell.Hairline, false);
		Show(Cell.Gun, false);
		Show(Cell.Ammo, false);
		Show(Cell.TabLit, false);
		Show(Cell.TabGlass, true);
		Show(Cell.Dashed, true);
		Cell.Rim->SetColorAndOpacity(SlotWithAlpha(Color::Ink(), 0.55f));
		Cell.Disc->SetColorAndOpacity(SlotWithAlpha(Color::ScreenBg(), 0.6f));
		Cell.Dashed->SetColorAndOpacity(SlotWithAlpha(Color::Text(), 0.6f));
		Cell.TabEdge->SetColorAndOpacity(SlotWithAlpha(Color::Hairline(), 0.55f));
		Cell.TabNumber->SetColorAndOpacity(FSlateColor(SlotWithAlpha(Color::TextDim(), 0.8f)));
		return;
	}

	const FWeaponInstanceData& Item = Weapon->GetInstance();
	const EWeaponKind Kind = Item.Definition ? Item.Definition->Kind : EWeaponKind::Rifle;
	Cell.Gun->SetBrush(InkedIconBrush(LoadoutParts::GunIconName(Kind), LoadoutParts::GunIcon(Kind), GunBox));
	Show(Cell.Gun, true);
	Show(Cell.Metal, true);
	Show(Cell.Arc, true);
	Show(Cell.Hairline, true);
	Show(Cell.Dashed, false);
	Cell.Rim->SetColorAndOpacity(Color::Ink());
	Cell.Disc->SetColorAndOpacity(SlotWithAlpha(Color::ScreenBg(), 0.82f));
	Cell.Arc->SetColorAndOpacity(LooterWeaponText::Color(Item));
	// The ammo it takes, as the same Inked icon the inventory's ammo gauges and the pickup feed show.
	const bool bHasAmmo = Item.Definition && LooterAmmo::IsValid(Item.Definition->AmmoType);
	if (bHasAmmo)
	{
		const EAmmoType AmmoType = Item.Definition->AmmoType;
		Cell.Ammo->SetBrush(InkedIconBrush(LoadoutParts::AmmoIconName(AmmoType), LoadoutParts::AmmoIcon(AmmoType), FVector2D(AmmoSize, AmmoSize)));
	}
	Show(Cell.Ammo, bHasAmmo);

	// The glow and the accent ring come up with the slot (Update eases them); a carried gun keeps them ready. A
	// legendary's rarity arc is orange too: its ring goes light so the two don't merge.
	Show(Cell.Glow, true);
	Show(Cell.AccentRing, true);
	Cell.AccentRing->SetColorAndOpacity(Item.Rarity == EWeaponRarity::Legendary ? Color::Text() : Color::Accent());

	// In hand: the icons at full strength and the tab lit orange with a dark number; otherwise a little dimmed, on glass.
	Cell.Gun->SetColorAndOpacity(bInHand ? FLinearColor::White : RestingIcon);
	Cell.Ammo->SetColorAndOpacity(bInHand ? FLinearColor::White : RestingIcon);
	Show(Cell.TabLit, bInHand);
	Show(Cell.TabGlass, !bInHand);
	Cell.TabEdge->SetColorAndOpacity(bInHand ? Color::Ink() : SlotWithAlpha(Color::Hairline(), 0.7f));
	Cell.TabNumber->SetColorAndOpacity(FSlateColor(bInHand ? Color::AccentDark() : Color::Text()));
}
