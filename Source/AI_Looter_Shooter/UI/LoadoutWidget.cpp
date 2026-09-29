#include "UI/LoadoutWidget.h"
#include "AI_Looter_Shooter.h"
#include "UI/LoadoutStage.h"
#include "UI/LooterButton.h"
#include "UI/LooterHUD.h"
#include "UI/LooterUIStyle.h"
#include "UI/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/DrawElements.h"
#include "Templates/UnrealTemplate.h"
#include "Widgets/SLeafWidget.h"

using namespace LooterUI;

namespace
{
	const FName ActionSlot(TEXT("Slot"));
	const FName ActionBackpack(TEXT("Backpack"));
	const TCHAR* StageMaterialPath = TEXT("/Game/UI/Loadout/M_UI_LoadoutStage.M_UI_LoadoutStage");

	// The page is laid out at 1600 x 900 and scaled to fit the screen.
	const FVector2D PageSize(1600.f, 900.f);
	const FVector2D StageTopLeft(580.f, 130.f);
	const FVector2D StageSize(440.f, 640.f);
	constexpr float LeftX = 60.f;
	constexpr float LeftWidth = 426.f;
	constexpr float RightX = 1110.f;
	constexpr float RightWidth = 430.f;
	constexpr float ColumnTop = 150.f;
	/** Callouts leave a slot card level, and turn toward the gun here. */
	constexpr float CalloutElbowX = 600.f;
	constexpr float SlotCardHeight = 140.f;
	constexpr float ListCardHeight = 61.f;
	/** Room above each slot card; the SELECTED chip straddles the card's top edge in it. */
	constexpr float SlotGap = 14.f;
	/** The stand's rings on the floor, and how high its pillars rise (cm). */
	constexpr float OuterRingRadius = 46.f;
	constexpr float InnerRingRadius = 32.f;
	constexpr float PillarHeight = 110.f;
	/** Drag speed (degrees per pixel) and stick speed (degrees per second) for turning the stand-in. */
	constexpr float DragTurnRate = 0.45f;
	constexpr float StickTurnRate = 160.f;

	namespace Colors
	{
		FLinearColor Dim()          { return Hex(2, 8, 14, 204); }
		FLinearColor CardFill()     { return Hex(7, 26, 40, 230); }
		FLinearColor EmptyFill()    { return Hex(7, 26, 40, 140); }
		FLinearColor InHandFill()   { return Hex(46, 30, 8, 235); }
		FLinearColor InHandCursor() { return Hex(74, 47, 10, 240); }
		FLinearColor PickedFill()   { return Hex(255, 159, 28, 72); }
		FLinearColor CardLine()     { return Hex(90, 200, 255, 89); }
		FLinearColor Callout()      { return Hex(90, 200, 255, 153); }
		FLinearColor Ring()         { return Hex(92, 202, 255, 191); }
		FLinearColor InnerRing()    { return Hex(92, 202, 255, 115); }
		FLinearColor RingGlow()     { return Hex(92, 202, 255, 30); }
		FLinearColor Pillar()       { return Hex(92, 202, 255, 36); }
		FLinearColor GunBody()      { return Hex(191, 234, 255, 224); }
	}

	// --- Vector art from the mockup ---

	TArray<FVector2D> Rect(double X0, double Y0, double X1, double Y1)
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

	/** A gun's side view (120 x 40), or just the strip on it that's lit in the gun's rarity color. */
	const FVectorIcon& GunIcon(EWeaponModel Model, bool bStrip)
	{
		static const FVectorIcon Rifle = MakeGunIcon({
			{ { 2, 14 }, { 22, 12 }, { 30, 14 }, { 30, 24 }, { 22, 26 }, { 4, 30 }, { 2, 28 } },
			{ { 30, 12 }, { 70, 12 }, { 72, 14 }, { 72, 22 }, { 30, 24 } },
			Rect(72, 13, 96, 21), Rect(96, 15.5, 116, 18.5), Rect(114, 14, 119, 20),
			{ { 50, 22 }, { 58, 22 }, { 62, 36 }, { 54, 36 } },
			{ { 36, 22 }, { 42, 22 }, { 40, 34 }, { 34, 34 } },
			Rect(44, 8, 54, 12) });
		static const FVectorIcon RifleStrip = MakeGunIcon({ Rect(34, 16, 62, 18), Rect(74, 15, 92, 16.5) });
		static const FVectorIcon Shotgun = MakeGunIcon({
			{ { 2, 16 }, { 20, 13 }, { 30, 14 }, { 30, 22 }, { 4, 30 }, { 2, 28 } },
			{ { 30, 12 }, { 54, 12 }, { 56, 14 }, { 56, 22 }, { 30, 22 } },
			Rect(56, 13, 118, 16.5), Rect(56, 17.5, 108, 20.5), Rect(66, 16.5, 88, 23.5),
			{ { 34, 21 }, { 40, 21 }, { 38, 32 }, { 32, 32 } },
			Rect(114, 11.2, 116.5, 13) });
		static const FVectorIcon ShotgunStrip = MakeGunIcon({ Rect(33, 15, 52, 17) });
		if (Model == EWeaponModel::Shotgun)
		{
			return bStrip ? ShotgunStrip : Shotgun;
		}
		return bStrip ? RifleStrip : Rifle;
	}

	FName GunIconName(EWeaponModel Model, bool bStrip)
	{
		const TCHAR* Kind = Model == EWeaponModel::Shotgun ? TEXT("Shotgun") : TEXT("Rifle");
		return FName(*FString::Printf(TEXT("Gun%s%s"), Kind, bStrip ? TEXT("Strip") : TEXT("")));
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

	UTextBlock* Label(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, int32 LetterSpacing = 0)
	{
		return MakeText(Tree, Text, Size, TextColor, true, LetterSpacing);
	}

	/** Uppercase text that ends in "..." rather than spilling out of its space. */
	UTextBlock* FittedLabel(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, int32 LetterSpacing = 0)
	{
		UTextBlock* Block = Label(Tree, Text, Size, TextColor, LetterSpacing);
		Block->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		return Block;
	}

	UImage* MakePicture(UWidgetTree* Tree, const FSlateBrush& Brush)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	UWidget* Sized(UWidgetTree* Tree, UWidget* Content, float Width, float Height)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (Width > 0.f)
		{
			Box->SetWidthOverride(Width);
		}
		if (Height > 0.f)
		{
			Box->SetHeightOverride(Height);
		}
		Box->SetContent(Content);
		return Box;
	}

	void FillSlot(UOverlaySlot* OverlaySlot, const FMargin& Padding = FMargin(0.f))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		OverlaySlot->SetPadding(Padding);
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
			return MakePicture(Tree, Brush);
		};
		UOverlay* Box = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		OutFill = Shape(false);
		// The fill is background: the UI transparency setting fades it, on top of its state tint.
		MarkBackground(OutFill);
		FillSlot(Box->AddChildToOverlay(OutFill));
		OutLine = Shape(true);
		FillSlot(Box->AddChildToOverlay(OutLine));
		FillSlot(Box->AddChildToOverlay(Content), Padding);
		return Box;
	}

	/** A gun's silhouette, optionally with its rarity strip lit. */
	UWidget* MakeGunPicture(UWidgetTree* Tree, const FWeaponInstanceData& Item, const FVector2D& Size, float PixelsPerUnit,
		const FLinearColor& BodyColor, bool bStrip)
	{
		const EWeaponModel Model = Item.Definition ? Item.Definition->ProceduralModel : EWeaponModel::Rifle;
		UOverlay* Picture = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Picture->AddChildToOverlay(MakePicture(Tree, IconBrush(GunIconName(Model, false), GunIcon(Model, false), PixelsPerUnit, Size, BodyColor)));
		if (bStrip)
		{
			Picture->AddChildToOverlay(MakePicture(Tree, IconBrush(GunIconName(Model, true), GunIcon(Model, true), PixelsPerUnit, Size,
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
		Hint->AddChildToHorizontalBox(Sized(Tree, Cap, 0.f, 22.f))->SetVerticalAlignment(VAlign_Center);
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

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

LoadoutRules::EVerdict LoadoutRules::Compare(const FWeaponInstanceData& Candidate, const FWeaponInstanceData* Current)
{
	if (!Current || !Candidate.Definition || Candidate.Definition != Current->Definition)
	{
		return EVerdict::None;
	}
	auto DamagePerSecond = [](const FWeaponStats& Stats)
	{
		return Stats.Damage * Stats.PelletsPerShot * Stats.FireRate / 60.f;
	};
	const float Theirs = DamagePerSecond(Candidate.Stats);
	const float Mine = DamagePerSecond(Current->Stats);
	if (Theirs > Mine * 1.03f)
	{
		return EVerdict::Upgrade;
	}
	return Theirs < Mine * 0.97f ? EVerdict::Weaker : EVerdict::Similar;
}

int32 LoadoutRules::SortForSwap(const TArray<FWeaponInstanceData>& Backpack, const UWeaponDefinition* Kind, TArray<int32>& OutOrder)
{
	OutOrder.Reset(Backpack.Num());
	if (Kind)
	{
		for (int32 Index = 0; Index < Backpack.Num(); ++Index)
		{
			if (Backpack[Index].Definition == Kind)
			{
				OutOrder.Add(Index);
			}
		}
	}
	const int32 NumKind = OutOrder.Num();
	for (int32 Index = 0; Index < Backpack.Num(); ++Index)
	{
		if (!Kind || Backpack[Index].Definition != Kind)
		{
			OutOrder.Add(Index);
		}
	}
	return NumKind;
}

// ---------------------------------------------------------------------------
// Paint layer
// ---------------------------------------------------------------------------

class SLoadoutPaintLayer : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadoutPaintLayer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, ULoadoutPaintLayer* InOwner)
	{
		Owner = InOwner;
		// What it draws moves every frame (the stand-in animates and turns).
		ForceVolatile(true);
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (const ULoadoutPaintLayer* Layer = Owner.Get())
		{
			Layer->Paint(AllottedGeometry, OutDrawElements, LayerId);
		}
		return LayerId + 1;
	}

private:
	TWeakObjectPtr<ULoadoutPaintLayer> Owner;
};

void ULoadoutPaintLayer::Paint(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	if (Painter)
	{
		Painter(Geometry, Elements, LayerId);
	}
}

TSharedRef<SWidget> ULoadoutPaintLayer::RebuildWidget()
{
	TSharedRef<SLoadoutPaintLayer> Widget = SNew(SLoadoutPaintLayer, this);
	Layer = Widget;
	return Widget;
}

void ULoadoutPaintLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Layer.Reset();
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void ULoadoutWidget::Open(ALooterHUD* InHUD, UWeaponManagerComponent* InManager)
{
	OwningHUD = InHUD;
	if (Manager.Get() != InManager)
	{
		if (UWeaponManagerComponent* Old = Manager.Get())
		{
			Old->OnInventoryChanged.RemoveDynamic(this, &ULoadoutWidget::Refresh);
			Old->OnAmmoChanged.RemoveDynamic(this, &ULoadoutWidget::HandleAmmoChanged);
		}
		Manager = InManager;
		if (InManager)
		{
			InManager->OnInventoryChanged.AddUniqueDynamic(this, &ULoadoutWidget::Refresh);
			InManager->OnAmmoChanged.AddUniqueDynamic(this, &ULoadoutWidget::HandleAmmoChanged);
		}
	}
	SetIsFocusable(true);

	// The stand-in is spawned the first time, then kept, idle while the screen is closed.
	UWorld* World = GetWorld();
	if (!Stage.IsValid() && World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Stage = World->SpawnActor<ALoadoutStage>(Params);
	}
	if (ALoadoutStage* StagePtr = Stage.Get())
	{
		if (!StageMaterial)
		{
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, StageMaterialPath))
			{
				StageMaterial = UMaterialInstanceDynamic::Create(Base, this);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Loadout screen: %s is missing, so the character can't be shown."), StageMaterialPath);
			}
		}
		if (StageMaterial)
		{
			StageMaterial->SetTextureParameterValue(TEXT("Capture"), StagePtr->GetRenderTarget());
		}
		ApplyStageBrush();
		StagePtr->ResetTurn();
		StagePtr->SetActive(true);
	}

	// Start on the weapon in hand, with nothing picked up.
	PickedSlot.Reset();
	Zone = EZone::Slots;
	ChosenSlot = CursorIndex = InManager ? FMath::Max(InManager->GetActiveSlot(), 0) : 0;
	Refresh();
}

void ULoadoutWidget::Close()
{
	PickedSlot.Reset();
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

void ULoadoutWidget::NativeDestruct()
{
	// Off screen: the stand-in stops animating and rendering.
	bDragging = false;
	TurnInput = 0.f;
	if (ALoadoutStage* StagePtr = Stage.Get())
	{
		StagePtr->SetActive(false);
	}
	Super::NativeDestruct();
}

void ULoadoutWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ALoadoutStage* StagePtr = Stage.Get();
	if (!StagePtr)
	{
		return;
	}
	if (!FMath::IsNearlyZero(TurnInput))
	{
		StagePtr->AddTurn(TurnInput * StickTurnRate * InDeltaTime);
	}
	if (StageMaterial)
	{
		StageMaterial->SetScalarParameterValue(TEXT("Exposure"), StagePtr->GetExposure());
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> ULoadoutWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the screen, dimmed.
		UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrush(RectBrush(Colors::Dim()));
		MarkBackground(Backdrop);
		FillSlot(Root->AddChildToOverlay(Backdrop));

		// Laid out at 1600 x 900, scaled to fit the screen.
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillSlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(Sized(WidgetTree, Canvas, 1600.f, 900.f));

		auto Place = [Canvas](UWidget* Widget, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment = FVector2D::ZeroVector)
		{
			UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetAlignment(Alignment);
			if (Size.IsZero())
			{
				CanvasSlot->SetAutoSize(true);
			}
			else
			{
				CanvasSlot->SetSize(Size);
			}
		};

		// The stand: the far half of its ring behind the stand-in, the picture, then the near half and the callouts.
		BackLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		BackLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintBack(Geometry, Elements, LayerId); });
		BackLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(BackLayer, FVector2D::ZeroVector, PageSize);

		StageImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Stage"));
		StageImage->SetBrush(RectBrush(FLinearColor::Transparent));
		StageImage->SetCursor(EMouseCursor::GrabHand);
		Place(StageImage, StageTopLeft, StageSize);

		FrontLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		FrontLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintFront(Geometry, Elements, LayerId); });
		FrontLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(FrontLayer, FVector2D::ZeroVector, PageSize);

		// Under the ring the stand-in stands on.
		Place(Label(WidgetTree, TEXT("Drag to turn"), 8, Hex(143, 179, 204, 204), 220), FVector2D(800.f, 786.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Title tab.
		UBorder* TitlePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TitlePlate->SetBrush(RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f));
		TitlePlate->SetPadding(FMargin(30.f, 5.f));
		TitlePlate->SetContent(Label(WidgetTree, TEXT("Loadout"), 14, Color::Title(), 350));
		Place(MakeShapeBox(WidgetTree, EShape::Tab, Color::FrameFill(), Color::FrameEdge(), TitlePlate, FMargin(18.f, 5.f, 18.f, 3.f)),
			FVector2D(800.f, 36.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Left: a card per weapon slot, over the ammo.
		{
			UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Header->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Equipped"), 10, Color::Accent(), 300));
			EquippedCount = Label(WidgetTree, TEXT(""), 10, Color::TextDim(), 200);
			Header->AddChildToHorizontalBox(EquippedCount)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			Left->AddChildToVerticalBox(Header);
			SlotList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Left->AddChildToVerticalBox(SlotList);
			Place(Left, FVector2D(LeftX, ColumnTop), FVector2D(LeftWidth, 545.f));

			UVerticalBox* Ammo = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Ammo->AddChildToVerticalBox(Label(WidgetTree, TEXT("Ammo"), 10, Color::Accent(), 300));
			AmmoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Ammo->AddChildToVerticalBox(AmmoRow)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
			Place(Ammo, FVector2D(LeftX, 712.f), FVector2D(LeftWidth, 125.f));
		}

		// Right: the chosen slot's gun, over the backpack guns that could go in its place.
		{
			UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			DetailsHeader = Label(WidgetTree, TEXT(""), 10, Color::Accent(), 300);
			Right->AddChildToVerticalBox(DetailsHeader);
			DetailsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UImage* DetailsFill = nullptr;
			UImage* DetailsLine = nullptr;
			UOverlay* DetailsCard = MakeCard(WidgetTree, DetailsBox, FMargin(16.f, 14.f), 1.7f, DetailsFill, DetailsLine);
			DetailsFill->SetColorAndOpacity(Colors::CardFill());
			DetailsLine->SetColorAndOpacity(Hex(90, 200, 255, 102));
			Right->AddChildToVerticalBox(DetailsCard)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

			UHorizontalBox* ListHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			ListHeader = Label(WidgetTree, TEXT(""), 8, Color::TextDim(), 220);
			ListHead->AddChildToHorizontalBox(ListHeader)->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* RuleSlot = ListHead->AddChildToHorizontalBox(Sized(WidgetTree, MakePicture(WidgetTree, RectBrush(Hex(90, 200, 255, 61))), 0.f, 1.f));
			RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RuleSlot->SetVerticalAlignment(VAlign_Center);
			RuleSlot->SetPadding(FMargin(10.f, 0.f));
			ListCount = Label(WidgetTree, TEXT(""), 8, Color::TextDim(), 150);
			ListHead->AddChildToHorizontalBox(ListCount)->SetVerticalAlignment(VAlign_Center);
			Right->AddChildToVerticalBox(ListHead)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

			ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(ListBox);
			UVerticalBoxSlot* ListSlot = Right->AddChildToVerticalBox(ListBox);
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
			Place(Right, FVector2D(RightX, ColumnTop), FVector2D(RightWidth, 685.f));
		}

		// Bottom: what the keys do right now.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(PromptBar, FVector2D(800.f, 852.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		DotBrush = CircleBrush(FLinearColor::White);
		DiscBrush = IconBrush(TEXT("LoadoutDisc"), DiscIcon(), 2.f, FVector2D(64.f, 64.f), FLinearColor::White);

		// Opened before it was first shown: the picture and contents can go in now.
		ApplyStageBrush();
		Refresh();
	}
	return Super::RebuildWidget();
}

void ULoadoutWidget::ApplyStageBrush()
{
	if (StageImage && StageMaterial)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(StageMaterial);
		Brush.ImageSize = StageSize;
		StageImage->SetBrush(Brush);
	}
}

// ---------------------------------------------------------------------------
// Contents
// ---------------------------------------------------------------------------

void ULoadoutWidget::Refresh()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (bApplyingAction || !SlotList || !Inventory)
	{
		return;
	}
	// The inventory may have changed under the cursor and the pick.
	if (PickedSlot.IsSet() && !SlotItem(PickedSlot.GetValue()))
	{
		PickedSlot.Reset();
	}
	ChosenSlot = PickedSlot.IsSet() ? PickedSlot.GetValue() : FMath::Clamp(ChosenSlot, 0, FMath::Max(NumSlots() - 1, 0));

	RebuildSlots();
	RebuildList();
	if (Zone == EZone::Backpack && ListCards.IsEmpty())
	{
		Zone = EZone::Slots;
		CursorIndex = ChosenSlot;
	}
	CursorIndex = FMath::Clamp(CursorIndex, 0, FMath::Max((Zone == EZone::Slots ? SlotCards.Num() : ListCards.Num()) - 1, 0));
	Restyle();
	RefreshDetails();
	RefreshAmmo();
	RefreshPrompts();

	if (ALoadoutStage* StagePtr = Stage.Get())
	{
		StagePtr->ShowLoadout(Cast<ACharacter>(Inventory->GetOwner()), Inventory);
	}
}

void ULoadoutWidget::HandleAmmoChanged(EAmmoType Type, int32 Carried)
{
	RefreshAmmo();
}

void ULoadoutWidget::RebuildSlots()
{
	SlotList->ClearChildren();
	SlotCards.Reset();
	EquippedCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), NumWeapons(), NumSlots())));
	for (int32 SlotIndex = 0; SlotIndex < NumSlots(); ++SlotIndex)
	{
		const FCard Card = MakeSlotCard(SlotIndex);
		SlotList->AddChildToVerticalBox(Card.Button)->SetPadding(FMargin(0.f, SlotGap, 0.f, 0.f));
		SlotCards.Add(Card);
	}
}

ULoadoutWidget::FCard ULoadoutWidget::MakeSlotCard(int32 SlotIndex)
{
	FCard Card;
	const FWeaponInstanceData* Item = SlotItem(SlotIndex);
	const ELoadoutCarry Carry = LoadoutCarry::ForSlot(SlotIndex, NumWeapons(), GetActiveSlot());
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// Top: the slot's number, where its gun is carried, and its level and damage.
	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Badge->SetBrush(RectBrush(Color::Plate(), Hex(90, 200, 255, 140), 1.f));
	Badge->SetHorizontalAlignment(HAlign_Center);
	Badge->SetVerticalAlignment(VAlign_Center);
	Badge->SetContent(Label(WidgetTree, FString::FromInt(SlotIndex + 1), 12, Color::Title()));
	Top->AddChildToHorizontalBox(Sized(WidgetTree, Badge, 28.f, 28.f))->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* CarrySlot = Top->AddChildToHorizontalBox(Label(WidgetTree, LoadoutCarry::Label(Carry), 8,
		Carry == ELoadoutCarry::InHand ? Color::Accent() : Color::TextDim(), 180));
	CarrySlot->SetVerticalAlignment(VAlign_Center);
	CarrySlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UTextBlock* Sub = FittedLabel(WidgetTree, Item ? FString::Printf(TEXT("Lv %d · %s dmg"), Item->Level,
		*LooterWeaponText::DamageString(Item->Stats)) : FString(), 9, Color::TextDim(), 100);
	Sub->SetJustification(ETextJustify::Right);
	UHorizontalBoxSlot* SubSlot = Top->AddChildToHorizontalBox(Sub);
	SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SubSlot->SetVerticalAlignment(VAlign_Center);
	SubSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(Top);

	// Middle: the gun's silhouette, its rarity strip lit.
	UWidget* Middle = Item ? MakeGunPicture(WidgetTree, *Item, FVector2D(186.f, 62.f), 3.f, Colors::GunBody(), true)
		: Label(WidgetTree, TEXT("Empty slot"), 9, Hex(143, 179, 204, 150), 200);
	UVerticalBoxSlot* MiddleSlot = Content->AddChildToVerticalBox(Middle);
	MiddleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MiddleSlot->SetHorizontalAlignment(HAlign_Center);
	MiddleSlot->SetVerticalAlignment(VAlign_Center);

	// Bottom: its name, in its rarity color.
	const bool bBackpackEmpty = !Manager.IsValid() || Manager->GetBackpack().IsEmpty();
	Content->AddChildToVerticalBox(Item ? FittedLabel(WidgetTree, LooterWeaponText::Name(*Item), 11, LooterWeaponText::Color(*Item), 50)
		: FittedLabel(WidgetTree, bBackpackEmpty ? TEXT("Free for the next gun you find") : TEXT("Pick a gun from the backpack"), 8, Color::TextDim(), 100));

	UOverlay* Box = MakeCard(WidgetTree, Content, FMargin(14.f, 10.f), 2.f, Card.Fill, Card.Line);

	// SELECTED straddles the top edge while the slot is picked up.
	UTextBlock* Tag = Label(WidgetTree, TEXT("Selected"), 7, Color::AccentDark(), 150);
	Tag->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Card.Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card.Chip->SetBrush(RectBrush(Color::Accent()));
	Card.Chip->SetPadding(FMargin(7.f, 1.f));
	Card.Chip->SetContent(Tag);
	Card.Chip->SetRenderTranslation(FVector2D(0.f, -9.f));
	Card.Chip->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* ChipSlot = Box->AddChildToOverlay(Card.Chip);
	ChipSlot->SetHorizontalAlignment(HAlign_Right);
	ChipSlot->SetVerticalAlignment(VAlign_Top);
	ChipSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));

	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Card.Button->SetupContent(Sized(WidgetTree, Box, 0.f, SlotCardHeight), ActionSlot, SlotIndex);
	Card.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleCardClicked);
	Card.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleCardHovered);
	return Card;
}

void ULoadoutWidget::RebuildList()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !ListBox)
	{
		return;
	}
	ListBox->ClearChildren();
	ListCards.Reset();
	const FWeaponInstanceData* SlotGun = SlotItem(ChosenSlot);
	NumSameKind = LoadoutRules::SortForSwap(Inventory->GetBackpack(), SlotGun ? SlotGun->Definition.Get() : nullptr, ListOrder);

	const FString Header = !SlotGun ? TEXT("Equip from backpack")
		: NumSameKind > 0 ? FString::Printf(TEXT("Swap with · %s in backpack"), *KindName(*SlotGun))
		: FString(TEXT("Swap with · backpack"));
	ListHeader->SetText(FText::FromString(Header.ToUpper()));
	ListCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Inventory->GetBackpack().Num(), Inventory->BackpackCapacity)));

	if (ListOrder.IsEmpty())
	{
		if (UScrollBoxSlot* EmptySlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Label(WidgetTree, TEXT("Backpack empty"), 8, Hex(143, 179, 204, 150), 200))))
		{
			EmptySlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
		}
		return;
	}
	for (int32 Row = 0; Row < ListOrder.Num(); ++Row)
	{
		// Other kinds of gun follow the chosen slot's kind, under their own heading.
		if (Row == NumSameKind && NumSameKind > 0)
		{
			if (UScrollBoxSlot* HeadingSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Label(WidgetTree, TEXT("Other weapons"), 8, Color::TextDim(), 220))))
			{
				HeadingSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
			}
		}
		const FCard Card = MakeListCard(Row);
		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Card.Button)))
		{
			CardSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		}
		ListCards.Add(Card);
	}
}

ULoadoutWidget::FCard ULoadoutWidget::MakeListCard(int32 Row)
{
	FCard Card;
	const FWeaponInstanceData* Item = ListItem(Row);
	check(Item);
	const FLinearColor Rarity = LooterWeaponText::Color(*Item);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	Line->AddChildToHorizontalBox(Sized(WidgetTree, MakeGunPicture(WidgetTree, *Item, FVector2D(84.f, 28.f), 1.5f,
		Rarity * FLinearColor(1.f, 1.f, 1.f, 0.9f), false), 84.f, 28.f))->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Text->AddChildToVerticalBox(FittedLabel(WidgetTree, LooterWeaponText::Name(*Item), 10, Rarity, 50));
	Text->AddChildToVerticalBox(FittedLabel(WidgetTree, FString::Printf(TEXT("Lv %d · %s dmg · %.0f rpm"), Item->Level,
		*LooterWeaponText::DamageString(Item->Stats), Item->Stats.FireRate), 8, Color::TextDim(), 120))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(Text);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);
	TextSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));

	// Better or worse than the gun in the chosen slot (same kind only).
	const LoadoutRules::EVerdict Verdict = LoadoutRules::Compare(*Item, SlotItem(ChosenSlot));
	if (Verdict != LoadoutRules::EVerdict::None)
	{
		const bool bUpgrade = Verdict == LoadoutRules::EVerdict::Upgrade;
		const bool bSimilar = Verdict == LoadoutRules::EVerdict::Similar;
		const FLinearColor VerdictColor = bSimilar ? Color::TextDim() : (bUpgrade ? Color::Better() : Color::Worse());
		UHorizontalBox* Mark = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (!bSimilar)
		{
			Mark->AddChildToHorizontalBox(MakePicture(WidgetTree, IconBrush(bUpgrade ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(bUpgrade), 4.f,
				FVector2D(10.f, 8.f), VerdictColor)))->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* WordSlot = Mark->AddChildToHorizontalBox(Label(WidgetTree, bSimilar ? TEXT("Similar") : (bUpgrade ? TEXT("Upgrade") : TEXT("Weaker")), 10, VerdictColor, 60));
		WordSlot->SetVerticalAlignment(VAlign_Center);
		WordSlot->SetPadding(FMargin(bSimilar ? 0.f : 6.f, 0.f, 0.f, 0.f));
		Line->AddChildToHorizontalBox(Mark)->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* Box = MakeCard(WidgetTree, Line, FMargin(12.f, 7.f), 1.3f, Card.Fill, Card.Line);
	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Card.Button->SetupContent(Sized(WidgetTree, Box, 0.f, ListCardHeight), ActionBackpack, Row);
	Card.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleCardClicked);
	Card.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleCardHovered);
	return Card;
}

void ULoadoutWidget::Restyle()
{
	const int32 ActiveSlot = GetActiveSlot();
	for (int32 SlotIndex = 0; SlotIndex < SlotCards.Num(); ++SlotIndex)
	{
		const FCard& Card = SlotCards[SlotIndex];
		const bool bItem = SlotItem(SlotIndex) != nullptr;
		const bool bInHand = bItem && SlotIndex == ActiveSlot;
		const bool bCursor = Zone == EZone::Slots && CursorIndex == SlotIndex;
		const bool bPicked = PickedSlot.IsSet() && PickedSlot.GetValue() == SlotIndex;

		FLinearColor Fill = bItem ? (bInHand ? Colors::InHandFill() : Colors::CardFill()) : Colors::EmptyFill();
		FLinearColor Line = bInHand ? Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.55f) : Colors::CardLine();
		// Browsing the backpack: the slot a gun would go into stays lit. While a slot is picked up, every other slot is
		// somewhere it can go.
		if ((Zone == EZone::Backpack && SlotIndex == ChosenSlot) || (PickedSlot.IsSet() && !bPicked))
		{
			Line = Color::TileLine();
		}
		if (bCursor)
		{
			Fill = bInHand ? Colors::InHandCursor() : Color::Tile();
			Line = Color::Accent();
		}
		if (bPicked)
		{
			Fill = Colors::PickedFill();
			Line = Color::Accent();
		}
		Card.Fill->SetColorAndOpacity(Fill);
		Card.Line->SetColorAndOpacity(Line);
		Card.Chip->SetVisibility(bPicked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	for (int32 Row = 0; Row < ListCards.Num(); ++Row)
	{
		const bool bCursor = Zone == EZone::Backpack && CursorIndex == Row;
		ListCards[Row].Fill->SetColorAndOpacity(bCursor ? Color::Tile() : Colors::CardFill());
		ListCards[Row].Line->SetColorAndOpacity(bCursor ? Color::Accent() : Colors::CardLine());
	}
}

void ULoadoutWidget::RefreshDetails()
{
	DetailsBox->ClearChildren();
	const FWeaponInstanceData* SlotGun = SlotItem(ChosenSlot);
	// Browsing the backpack shows that gun against the one in the chosen slot.
	const FWeaponInstanceData* Candidate = Zone == EZone::Backpack ? ListItem(CursorIndex) : nullptr;
	const FWeaponInstanceData* Shown = Candidate ? Candidate : SlotGun;
	const FWeaponInstanceData* Baseline = Candidate ? SlotGun : nullptr;

	const ELoadoutCarry Carry = LoadoutCarry::ForSlot(ChosenSlot, NumWeapons(), GetActiveSlot());
	const FString Header = Candidate
		? (SlotGun ? FString::Printf(TEXT("Backpack · vs slot %d"), ChosenSlot + 1) : FString::Printf(TEXT("Backpack · for slot %d"), ChosenSlot + 1))
		: FString::Printf(TEXT("Slot %d · %s"), ChosenSlot + 1, LoadoutCarry::Label(Carry));
	DetailsHeader->SetText(FText::FromString(Header.ToUpper()));

	if (!Shown)
	{
		DetailsBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("Empty slot"), 17, Color::TextDim(), 40));
		UTextBlock* Help = MakeText(WidgetTree, ListOrder.IsEmpty()
			? TEXT("Nothing in the backpack to equip here. The next gun you pick up fills this slot.")
			: TEXT("Pick a gun from the backpack below to equip it in this slot."), 10, Color::TextDim());
		Help->SetAutoWrapText(true);
		DetailsBox->AddChildToVerticalBox(Help)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		return;
	}

	UTextBlock* NameText = Label(WidgetTree, LooterWeaponText::Name(*Shown), 17, LooterWeaponText::Color(*Shown), 40);
	NameText->SetAutoWrapText(true);
	DetailsBox->AddChildToVerticalBox(NameText);
	DetailsBox->AddChildToVerticalBox(Label(WidgetTree, FString::Printf(TEXT("Lv %d · %s · %s"), Shown->Level,
		*LooterWeaponText::FireModeName(*Shown), *AmmoName(*Shown)), 9, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 4.f, 0.f, 10.f));

	const FWeaponStats& S = Shown->Stats;
	const FWeaponStats* B = Baseline ? &Baseline->Stats : nullptr;
	auto AddStat = [this, B](const TCHAR* StatName, float Rating, const FString& Value, float New, float Old, bool bHigherIsBetter, int32 Decimals)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(Sized(WidgetTree, Label(WidgetTree, StatName, 8, Color::TextDim(), 120), 92.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BarSlot = Line->AddChildToHorizontalBox(MakeSegmentBar(WidgetTree, 8, Rating, Color::SegmentOn(), 7.f));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* ValueText = MakeText(WidgetTree, Value, 11, Color::Text());
		ValueText->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(Sized(WidgetTree, ValueText, 60.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		// The change from the chosen slot's gun, green when it's an upgrade.
		UTextBlock* DeltaText = MakeText(WidgetTree, TEXT(""), 9, Color::TextDim());
		DeltaText->SetJustification(ETextJustify::Right);
		if (B && !FMath::IsNearlyEqual(New, Old, 0.01f))
		{
			DeltaText->SetText(FText::FromString(FormatDelta(New - Old, Decimals)));
			DeltaText->SetColorAndOpacity(FSlateColor(((New > Old) == bHigherIsBetter) ? Color::Better() : Color::Worse()));
		}
		Line->AddChildToHorizontalBox(Sized(WidgetTree, DeltaText, 46.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		DetailsBox->AddChildToVerticalBox(Sized(WidgetTree, Line, 0.f, 24.f))->SetPadding(FMargin(0.f, 2.f));
	};
	// Damage compares the whole shot, so shotguns and rifles line up fairly.
	AddStat(TEXT("Damage"), LooterWeaponText::DamageRating(S), LooterWeaponText::DamageString(S), S.Damage * S.PelletsPerShot,
		B ? B->Damage * B->PelletsPerShot : 0.f, true, 1);
	AddStat(TEXT("Fire rate"), LooterWeaponText::FireRateRating(S), FString::Printf(TEXT("%.0f"), S.FireRate), S.FireRate, B ? B->FireRate : 0.f, true, 0);
	AddStat(TEXT("Magazine"), LooterWeaponText::MagazineRating(S), FString::FromInt(S.MagazineSize), S.MagazineSize, B ? B->MagazineSize : 0.f, true, 0);
	AddStat(TEXT("Reload"), LooterWeaponText::ReloadRating(S), FString::Printf(TEXT("%.2fs"), S.ReloadTime), S.ReloadTime, B ? B->ReloadTime : 0.f, false, 2);
	AddStat(TEXT("Accuracy"), LooterWeaponText::AccuracyRating(S), FString::Printf(TEXT("%.1f°"), S.Spread), S.Spread, B ? B->Spread : 0.f, false, 1);
}

void ULoadoutWidget::RefreshAmmo()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!AmmoRow || !Inventory)
	{
		return;
	}
	AmmoRow->ClearChildren();
	const TConstArrayView<EAmmoType> Types = LooterAmmo::AllTypes();
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		const EAmmoType Type = Types[Index];
		const int32 Carried = Inventory->GetAmmo(Type);
		const int32 Max = FMath::Max(Inventory->GetMaxAmmo(Type), 1);
		UVerticalBox* Gauge = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Gauge->AddChildToVerticalBox(MakeText(WidgetTree, FString::FromInt(Carried), 9, Carried > 0 ? Color::Text() : Color::TextDim()))
			->SetHorizontalAlignment(HAlign_Center);

		// A vertical bar filling up from the bottom.
		UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillSlot(Bar->AddChildToOverlay(MakePicture(WidgetTree, RectBrush(Color::SegmentOff()))));
		const float Level = 44.f * FMath::Clamp(static_cast<float>(Carried) / Max, 0.f, 1.f);
		if (Level > 0.5f)
		{
			UOverlaySlot* LevelSlot = Bar->AddChildToOverlay(Sized(WidgetTree, MakePicture(WidgetTree, RectBrush(Color::SegmentOn())), 0.f, Level));
			LevelSlot->SetHorizontalAlignment(HAlign_Fill);
			LevelSlot->SetVerticalAlignment(VAlign_Bottom);
		}
		UVerticalBoxSlot* BarSlot = Gauge->AddChildToVerticalBox(Sized(WidgetTree, Bar, 10.f, 44.f));
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetPadding(FMargin(0.f, 4.f));

		UVerticalBoxSlot* IconSlot = Gauge->AddChildToVerticalBox(MakePicture(WidgetTree, IconBrush(*FString::Printf(TEXT("Ammo%d"), Index), AmmoIcon(Type), 2.f,
			FVector2D(18.f, 18.f), Carried > 0 ? Color::Title() : Hex(143, 179, 204, 128))));
		IconSlot->SetHorizontalAlignment(HAlign_Center);

		UWidget* Box = MakeShapeBox(WidgetTree, EShape::Control, Hex(7, 26, 40, 217), Hex(90, 200, 255, 71), Gauge, FMargin(0.f, 6.f));
		Box->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s: %d / %d"), LooterAmmo::GetInfo(Type).Name, Carried, Max)));
		UHorizontalBoxSlot* BoxSlot = AmmoRow->AddChildToHorizontalBox(Box);
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BoxSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Types.Num() ? 10.f : 0.f, 0.f));
	}
}

void ULoadoutWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	struct FPrompt
	{
		FString Key;
		const TCHAR* Text;
	};
	TArray<FPrompt, TInlineAllocator<8>> Prompts;
	const bool bBackpack = !ListOrder.IsEmpty();
	if (Zone == EZone::Slots)
	{
		const bool bItem = SlotItem(CursorIndex) != nullptr;
		if (PickedSlot.IsSet())
		{
			const bool bSame = PickedSlot.GetValue() == CursorIndex;
			Prompts.Add({ TEXT("E"), bSame ? TEXT("Put back") : (bItem ? TEXT("Swap here") : TEXT("Move here")) });
			if (bBackpack)
			{
				Prompts.Add({ TEXT("D"), TEXT("Swap with backpack") });
			}
			Prompts.Add({ TEXT("Esc"), TEXT("Cancel") });
		}
		else
		{
			if (bItem)
			{
				Prompts.Add({ TEXT("E"), TEXT("Move") });
				if (CursorIndex != GetActiveSlot())
				{
					Prompts.Add({ TEXT("F"), TEXT("Hold") });
				}
				Prompts.Add({ TEXT("Q"), TEXT("Drop") });
			}
			else if (bBackpack)
			{
				Prompts.Add({ TEXT("E"), TEXT("Equip from backpack") });
			}
			Prompts.Add({ TEXT("W / S"), TEXT("Choose slot") });
			if (bBackpack)
			{
				Prompts.Add({ TEXT("D"), TEXT("Backpack") });
			}
		}
	}
	else
	{
		Prompts.Add({ TEXT("E"), SlotItem(ChosenSlot) ? TEXT("Swap in") : TEXT("Equip") });
		Prompts.Add({ TEXT("F"), TEXT("Hold") });
		Prompts.Add({ TEXT("Q"), TEXT("Drop") });
		Prompts.Add({ TEXT("W / S"), TEXT("Browse") });
		Prompts.Add({ TEXT("A"), TEXT("Slots") });
	}
	if (!PickedSlot.IsSet())
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
		Prompts.Add({ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close") });
	}
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
	}
}

// ---------------------------------------------------------------------------
// Cursor and actions
// ---------------------------------------------------------------------------

void ULoadoutWidget::MoveCursor(int32 Columns, int32 Rows)
{
	if (Columns < 0 && Zone == EZone::Backpack)
	{
		MoveCursorTo(EZone::Slots, ChosenSlot, false);
	}
	else if (Columns > 0 && Zone == EZone::Slots)
	{
		if (!ListCards.IsEmpty())
		{
			MoveCursorTo(EZone::Backpack, 0, true);
		}
	}
	else if (Rows != 0)
	{
		const int32 Count = Zone == EZone::Slots ? SlotCards.Num() : ListCards.Num();
		MoveCursorTo(Zone, FMath::Clamp(CursorIndex + Rows, 0, FMath::Max(Count - 1, 0)), true);
	}
}

void ULoadoutWidget::MoveCursorTo(EZone NewZone, int32 NewIndex, bool bScrollIntoView)
{
	if (NewZone == Zone && NewIndex == CursorIndex)
	{
		return;
	}
	Zone = NewZone;
	CursorIndex = NewIndex;
	// On the slots the cursor chooses the slot (unless one is picked up); the backpack list is for that slot.
	if (Zone == EZone::Slots && !PickedSlot.IsSet() && ChosenSlot != CursorIndex)
	{
		ChosenSlot = CursorIndex;
		RebuildList();
	}
	Restyle();
	RefreshDetails();
	RefreshPrompts();
	if (bScrollIntoView && Zone == EZone::Backpack && ListCards.IsValidIndex(CursorIndex))
	{
		ListBox->ScrollWidgetIntoView(ListCards[CursorIndex].Button, false, EDescendantScrollDestination::IntoView, 8.f);
	}
}

void ULoadoutWidget::Activate()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}

	if (Zone == EZone::Slots)
	{
		const int32 SlotIndex = CursorIndex;
		if (PickedSlot.IsSet())
		{
			// Put the picked-up gun here: swap with the gun here, or move it into the free slot (it lands after the last gun).
			const int32 From = PickedSlot.GetValue();
			PickedSlot.Reset();
			if (From != SlotIndex)
			{
				const bool bSwap = SlotItem(SlotIndex) != nullptr;
				bool bMoved = false;
				{
					TGuardValue<bool> RefreshAfter(bApplyingAction, true);
					bMoved = bSwap ? Inventory->SwapSlots(From, SlotIndex) : Inventory->MoveSlot(From, SlotIndex);
				}
				ChosenSlot = CursorIndex = bMoved && !bSwap ? FMath::Max(Inventory->GetWeapons().Num() - 1, 0) : SlotIndex;
			}
			Refresh();
			return;
		}
		if (SlotItem(SlotIndex))
		{
			PickedSlot = SlotIndex;
			ChosenSlot = SlotIndex;
			Restyle();
			RefreshDetails();
			RefreshPrompts();
		}
		else if (!ListCards.IsEmpty())
		{
			// A free slot: choose something from the backpack for it.
			MoveCursorTo(EZone::Backpack, 0, true);
		}
		return;
	}

	// A backpack gun goes into the chosen slot, trading places with the gun there.
	if (!ListOrder.IsValidIndex(CursorIndex))
	{
		return;
	}
	const int32 BackpackIndex = ListOrder[CursorIndex];
	const bool bSlotFilled = SlotItem(ChosenSlot) != nullptr;
	PickedSlot.Reset();
	bool bEquipped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (bSlotFilled)
		{
			Inventory->SwapSlotWithBackpack(ChosenSlot, BackpackIndex);
		}
		else
		{
			bEquipped = Inventory->MoveBackpackToSlot(BackpackIndex);
		}
	}
	if (bEquipped)
	{
		// Into a free slot (the first one): show it there.
		Zone = EZone::Slots;
		ChosenSlot = CursorIndex = FMath::Max(Inventory->GetWeapons().Num() - 1, 0);
	}
	Refresh();
	if (bSlotFilled)
	{
		// The cursor stays on the gun that came out of the slot, so pressing again swaps straight back.
		const int32 Row = ListOrder.IndexOfByKey(BackpackIndex);
		if (Row != INDEX_NONE)
		{
			MoveCursorTo(EZone::Backpack, Row, true);
		}
	}
}

void ULoadoutWidget::Hold()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}
	PickedSlot.Reset();
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (Zone == EZone::Slots)
		{
			if (SlotItem(CursorIndex))
			{
				Inventory->EquipSlot(CursorIndex);
			}
		}
		else if (ListOrder.IsValidIndex(CursorIndex) && Inventory->EquipFromBackpack(ListOrder[CursorIndex]))
		{
			// It's in a slot now, in hand: follow it there.
			Zone = EZone::Slots;
			ChosenSlot = CursorIndex = FMath::Max(Inventory->GetActiveSlot(), 0);
		}
	}
	Refresh();
}

void ULoadoutWidget::Drop()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}
	PickedSlot.Reset();
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (Zone == EZone::Slots)
		{
			if (SlotItem(CursorIndex))
			{
				Inventory->DropSlot(CursorIndex);
			}
		}
		else if (ListOrder.IsValidIndex(CursorIndex))
		{
			Inventory->DropFromBackpack(ListOrder[CursorIndex]);
		}
	}
	Refresh();
}

void ULoadoutWidget::CancelPick()
{
	if (PickedSlot.IsSet())
	{
		PickedSlot.Reset();
		Restyle();
		RefreshDetails();
		RefreshPrompts();
	}
}

void ULoadoutWidget::HandleCardClicked(ULooterButton* Button)
{
	HandleCardHovered(Button);
	Activate();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the screen's keys.
	SetKeyboardFocus();
}

void ULoadoutWidget::HandleCardHovered(ULooterButton* Button)
{
	if (Button)
	{
		MoveCursorTo(Button->Action == ActionSlot ? EZone::Slots : EZone::Backpack, Button->Index, false);
	}
}

FReply ULoadoutWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		// Esc puts down a picked-up slot first, then closes.
		if (PickedSlot.IsSet())
		{
			CancelPick();
		}
		else
		{
			Close();
		}
		return FReply::Handled();
	}
	if (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I))
	{
		Close();
		return FReply::Handled();
	}

	struct FMove { FKey Keys[3]; int32 Columns; int32 Rows; };
	const FMove Moves[] = {
		{ { EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up }, 0, -1 },
		{ { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down }, 0, 1 },
		{ { EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left }, -1, 0 },
		{ { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right }, 1, 0 },
	};
	for (const FMove& Move : Moves)
	{
		if (Key == Move.Keys[0] || Key == Move.Keys[1] || Key == Move.Keys[2])
		{
			MoveCursor(Move.Columns, Move.Rows);
			return FReply::Handled();
		}
	}

	const bool bInteractKey = Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || bInteractKey)
	{
		Activate();
		return FReply::Handled();
	}
	if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top)
	{
		Hold();
		return FReply::Handled();
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_FaceButton_Left)
	{
		Drop();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------
// Turning the stand-in
// ---------------------------------------------------------------------------

FReply ULoadoutWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && PickedSlot.IsSet())
	{
		CancelPick();
		return FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && StageImage && Stage.IsValid()
		&& StageImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		// Dragging right turns the stand-in's front to the right.
		if (ALoadoutStage* StagePtr = Stage.Get())
		{
			StagePtr->AddTurn(-InMouseEvent.GetCursorDelta().X * DragTurnRate);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		SetKeyboardFocus();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULoadoutWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDragging = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

FReply ULoadoutWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightX)
	{
		const float Value = InAnalogEvent.GetAnalogValue();
		TurnInput = FMath::Abs(Value) > 0.2f ? -Value : 0.f;
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

// ---------------------------------------------------------------------------
// The stand's ring and the callouts
// ---------------------------------------------------------------------------

bool ULoadoutWidget::ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const
{
	const ALoadoutStage* StagePtr = Stage.Get();
	FVector2D UV;
	if (!StagePtr || !StagePtr->ProjectToImage(WorldLocation, UV))
	{
		return false;
	}
	OutPoint = FVector2f(StageTopLeft + UV * StageSize);
	return true;
}

namespace
{
	/** Points around a ring on the floor under the stand-in, from one angle to another (degrees, 0 = toward the camera). */
	template <typename FProject>
	TArray<FVector2f> RingArc(const ALoadoutStage& Stage, float Radius, float From, float To, FProject&& Project)
	{
		const FVector Center = Stage.GetFloorCenter();
		const FVector Toward = Stage.GetTowardCamera();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Toward);
		TArray<FVector2f> Points;
		constexpr int32 Steps = 40;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Step) / Steps));
			FVector2f Point;
			if (Project(Center + (Toward * FMath::Cos(Angle) + Side * FMath::Sin(Angle)) * Radius, Point))
			{
				Points.Add(Point);
			}
		}
		return Points;
	}
}

void ULoadoutWidget::PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ALoadoutStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial)
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };

	// A soft glow over the whole ring.
	const TArray<FVector2f> Circle = RingArc(*StagePtr, OuterRingRadius, 0.f, 360.f, Project);
	if (Circle.Num() > 2)
	{
		const FBox2f Bounds(Circle);
		DrawBox(Elements, LayerId, Geometry, Bounds.Min, Bounds.GetSize(), &DiscBrush, Colors::RingGlow());
	}
	// Faint pillars rising from the ring's sides.
	const FVector Center = StagePtr->GetFloorCenter();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, StagePtr->GetTowardCamera());
	for (const float Sign : { -1.f, 1.f })
	{
		const FVector Foot = Center + Side * (Sign * OuterRingRadius);
		FVector2f Bottom, Top;
		if (ProjectToPage(Foot, Bottom) && ProjectToPage(Foot + FVector(0.f, 0.f, PillarHeight), Top))
		{
			DrawLines(Elements, LayerId, Geometry, { Bottom, Top }, Colors::Pillar(), 2.f);
		}
	}
	// The far halves of the rings, behind the stand-in's feet.
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, OuterRingRadius, 90.f, 270.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, InnerRingRadius, 90.f, 270.f, Project), Colors::InnerRing(), 1.5f);
}

void ULoadoutWidget::PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ALoadoutStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial)
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };

	// The near halves of the rings, in front of the feet.
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, OuterRingRadius, -90.f, 90.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, InnerRingRadius, -90.f, 90.f, Project), Colors::InnerRing(), 1.5f);

	// Callouts: from each slot card, level to the elbow, then to where its gun is carried. The gun in hand's is orange.
	const int32 ActiveSlot = GetActiveSlot();
	for (int32 SlotIndex = 0; SlotIndex < SlotCards.Num(); ++SlotIndex)
	{
		FVector Anchor;
		FVector2f End;
		if (!StagePtr->GetSlotAnchor(SlotIndex, Anchor) || !ProjectToPage(Anchor, End))
		{
			continue;
		}
		const FGeometry& CardGeometry = SlotCards[SlotIndex].Button->GetCachedGeometry();
		const FVector2f CardSize(CardGeometry.GetLocalSize());
		if (CardSize.X <= 0.f)
		{
			continue;
		}
		const FVector2f Start(Geometry.AbsoluteToLocal(CardGeometry.LocalToAbsolute(FVector2f(CardSize.X, CardSize.Y * 0.5f))));
		const bool bInHand = SlotIndex == ActiveSlot;
		const FLinearColor LineColor = bInHand ? Color::Accent() : (SlotIndex == ChosenSlot ? Color::TileLine() : Colors::Callout());
		DrawLines(Elements, LayerId, Geometry, { Start, FVector2f(CalloutElbowX, Start.Y), End }, LineColor, bInHand ? 2.f : 1.5f);
		const float Radius = bInHand ? 6.f : 5.f;
		DrawBox(Elements, LayerId + 1, Geometry, End - FVector2f(Radius), FVector2f(Radius * 2.f), &DotBrush,
			bInHand ? Color::Accent() : Color::SegmentOn());
	}
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const FWeaponInstanceData* ULoadoutWidget::SlotItem(int32 SlotIndex) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return nullptr;
	}
	const TArray<AWeaponBase*> Weapons = Inventory->GetWeapons();
	return Weapons.IsValidIndex(SlotIndex) && Weapons[SlotIndex] ? &Weapons[SlotIndex]->GetInstance() : nullptr;
}

const FWeaponInstanceData* ULoadoutWidget::ListItem(int32 Row) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !ListOrder.IsValidIndex(Row))
	{
		return nullptr;
	}
	const TArray<FWeaponInstanceData>& Backpack = Inventory->GetBackpack();
	return Backpack.IsValidIndex(ListOrder[Row]) ? &Backpack[ListOrder[Row]] : nullptr;
}

int32 ULoadoutWidget::NumSlots() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->MaxWeapons : 0;
}

int32 ULoadoutWidget::NumWeapons() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->GetWeapons().Num() : 0;
}

int32 ULoadoutWidget::GetActiveSlot() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->GetActiveSlot() : INDEX_NONE;
}
