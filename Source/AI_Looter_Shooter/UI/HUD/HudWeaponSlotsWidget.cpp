#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Inventory/GunSlotRules.h"
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
	/** The ring round the slot in hand reaches past the circle: its picture has this much room on every side (scaled up to match). */
	constexpr float AccentMargin = UHudWeaponSlotsWidget::AccentMargin;
	/**
	 * The soft orange glow behind the slot in hand, across, and how strong it is at its middle: about a quarter strength
	 * at the circle's edge, gone 16 px out (GlowBrush falls off smoothly to its rim), as in the mockup.
	 */
	constexpr float GlowSize = 96.f;
	constexpr float GlowAlpha = 0.75f;

	// The row left of the circle: the ammo icon, then the key tab, each this far apart.
	constexpr float AmmoSize = 20.f;
	constexpr float RowGap = 8.f;
	constexpr float TabWidth = UHudWeaponSlotsWidget::TabWidth;
	constexpr float TabHeight = UHudWeaponSlotsWidget::TabHeight;
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

	/** A shut slot: its gunmetal ring and padlock dimmed this much (the slot is there, not yet the player's), its tab fainter. */
	constexpr float LockedMetal = 0.42f;
	constexpr float LockedPadlock = 0.85f;
	/** The ring flashing out as a slot opens: it starts on the circle's edge and grows this much as it fades. */
	constexpr float UnlockGrow = 0.42f;

	FLinearColor SlotWithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
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
			Cell.Disc = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Disc));
			// The disc fades with the UI transparency setting; the rings, icons and number don't.
			MarkBackground(Cell.Disc);
			Cell.Rim = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Rim));
			Cell.Metal = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Metal));
			Cell.Arc = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Arc));
			Cell.Hairline = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Hairline));
			Cell.Dashed = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Dashed));
			Cell.AccentRing = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::AccentRing));
			Cell.AccentRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.AccentRing->SetRenderScale(FVector2D((SlotDiameter + AccentMargin * 2.f) / SlotDiameter));
			Cell.AccentRing->SetRenderOpacity(0.f);
			// A shut slot's padlock in the disc's middle (Paint shows it).
			Cell.Lock = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::Lock));
			Cell.Lock->SetVisibility(ESlateVisibility::Hidden);
			// The opening's flash: the accent ring's picture in soul gold, grown and faded out by TickUnlockFlash.
			Cell.UnlockRing = AddSlotLayer(WidgetTree, Layers, PictureBrush(EHudSlotPicture::AccentRing));
			Cell.UnlockRing->SetColorAndOpacity(Color::SoulGold());
			Cell.UnlockRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Cell.UnlockRing->SetVisibility(ESlateVisibility::Hidden);
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
			const FSlateBrush Plate = PictureBrush(EHudSlotPicture::TabPlate);
			Cell.TabGlass = AddSlotLayer(WidgetTree, Tab, Plate);
			Cell.TabGlass->SetColorAndOpacity(SlotWithAlpha(Color::ScreenBg(), 0.85f));
			MarkBackground(Cell.TabGlass);
			Cell.TabLit = AddSlotLayer(WidgetTree, Tab, Plate);
			Cell.TabLit->SetColorAndOpacity(Color::Accent());
			Cell.TabEdge = AddSlotLayer(WidgetTree, Tab, PictureBrush(EHudSlotPicture::TabEdge));
			Cell.TabNumber = MakeFloatingText(WidgetTree, 12, Color::Text(), 0, ETextJustify::Center);
			Cell.TabNumber->SetText(FText::AsNumber(Index + 1));
			UOverlaySlot* NumberSlot = Tab->AddChildToOverlay(Cell.TabNumber);
			NumberSlot->SetHorizontalAlignment(HAlign_Center);
			NumberSlot->SetVerticalAlignment(VAlign_Center);
			// Chakra Petch's line is taller than the 16 px tab and its digits sit low in it: centred by the line, the
			// digit hung out of the plate's bottom edge (seen in the 1080p HUD shots), so it's lifted to the plate's middle.
			Cell.TabNumber->SetRenderTranslation(FVector2D(0.f, -3.f));
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
		// Only an empty slot can be shut (a slot holding an older save's gun is in use: GetSlotLock).
		const EGunSlotLock Lock = Weapon ? EGunSlotLock::Open : Manager->GetSlotLock(Index);
		const bool bLocked = Lock != EGunSlotLock::Open;
		// Shut on the last look and open now: it just opened, and flashes.
		if (Cell.bWasLocked && !bLocked)
		{
			Cell.UnlockAge = 0.f;
		}
		Cell.bWasLocked = bLocked;

		// Repaint only when the slot's gun, its ammo type, whether it's in hand or what shuts it changed.
		uint32 Shown = bInHand ? 1u : 0u;
		if (Weapon)
		{
			const FWeaponInstanceData& Item = Weapon->GetInstance();
			const uint32 AmmoKey = Item.Definition ? static_cast<uint32>(Item.Definition->AmmoType) : MAX_uint8;
			Shown = HashCombine(Shown, HashCombine(GetTypeHash(Item.Definition.Get()), HashCombine(GetTypeHash(Item.Seed),
				HashCombine(static_cast<uint32>(Item.Rarity) + 2u, AmmoKey))));
		}
		else
		{
			Shown = 16u + static_cast<uint32>(Lock);
		}
		if (Shown != Cell.Shown)
		{
			Cell.Shown = Shown;
			Cell.ShownLock = Lock;
			Paint(Cell, Index, Manager, bInHand, Lock);
		}
		TickUnlockFlash(Cell, DeltaTime);

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

void UHudWeaponSlotsWidget::Paint(FSlotWidgets& Cell, int32 Index, const UWeaponManagerComponent* Manager, bool bInHand, EGunSlotLock Lock)
{
	const TArray<AWeaponBase*> Weapons = Manager->GetWeapons();
	const AWeaponBase* Weapon = Weapons.IsValidIndex(Index) ? Weapons[Index] : nullptr;
	auto Show = [](UWidget* Widget, bool bShown)
	{
		Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	};

	if (!Weapon && Lock != EGunSlotLock::Open)
	{
		// Shut: the slot is there but not the player's yet. Its gunmetal ring dimmed round a darker socket, a small padlock in
		// it, no dashes (nothing can go in), the key fainter still than an empty slot's.
		Show(Cell.Glow, false);
		Show(Cell.AccentRing, false);
		Show(Cell.Arc, false);
		Show(Cell.Hairline, false);
		Show(Cell.Gun, false);
		Show(Cell.Ammo, false);
		Show(Cell.TabLit, false);
		Show(Cell.Dashed, false);
		Show(Cell.TabGlass, true);
		Show(Cell.Metal, true);
		Show(Cell.Lock, true);
		Cell.Metal->SetColorAndOpacity(SlotWithAlpha(FLinearColor::White, LockedMetal));
		Cell.Lock->SetColorAndOpacity(SlotWithAlpha(FLinearColor::White, LockedPadlock));
		Cell.Rim->SetColorAndOpacity(SlotWithAlpha(Color::Ink(), 0.5f));
		Cell.Disc->SetColorAndOpacity(SlotWithAlpha(Color::ScreenBg(), 0.72f));
		Cell.TabEdge->SetColorAndOpacity(SlotWithAlpha(Color::Hairline(), 0.3f));
		Cell.TabNumber->SetColorAndOpacity(FSlateColor(SlotWithAlpha(Color::TextDim(), 0.5f)));
		return;
	}
	Show(Cell.Lock, false);
	Cell.Metal->SetColorAndOpacity(FLinearColor::White);

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

void UHudWeaponSlotsWidget::TickUnlockFlash(FSlotWidgets& Cell, float DeltaTime)
{
	if (!Cell.UnlockRing)
	{
		return;
	}
	if (Cell.UnlockAge >= UnlockSeconds)
	{
		if (Cell.UnlockRing->GetVisibility() != ESlateVisibility::Hidden)
		{
			Cell.UnlockRing->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}
	// A soul-gold ring leaves the circle's edge and grows as it fades, eased out: the slot's own small moment under the
	// banner's (the user's rule: every action rewarding).
	const float Alpha = FMath::Clamp(Cell.UnlockAge / UnlockSeconds, 0.f, 1.f);
	const float Eased = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
	Cell.UnlockRing->SetVisibility(ESlateVisibility::HitTestInvisible);
	Cell.UnlockRing->SetRenderScale(FVector2D((SlotDiameter + AccentMargin * 2.f) / SlotDiameter * (1.f + UnlockGrow * Eased)));
	Cell.UnlockRing->SetRenderOpacity(FMath::Square(1.f - Alpha));
	Cell.UnlockAge += DeltaTime;
}

bool UHudWeaponSlotsWidget::IsSlotShown(int32 Index) const
{
	return Slots.IsValidIndex(Index) && Slots[Index].Root && Slots[Index].Root->GetVisibility() != ESlateVisibility::Collapsed;
}

EGunSlotLock UHudWeaponSlotsWidget::GetShownLock(int32 Index) const
{
	return Slots.IsValidIndex(Index) ? Slots[Index].ShownLock : EGunSlotLock::Open;
}

bool UHudWeaponSlotsWidget::IsUnlockFlashing(int32 Index) const
{
	return Slots.IsValidIndex(Index) && Slots[Index].UnlockAge < UnlockSeconds;
}
