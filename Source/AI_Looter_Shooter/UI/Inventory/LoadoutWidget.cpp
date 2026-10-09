// ULoadoutWidget: opening and closing, the page's layout, refreshing it, and what it reads from the inventory. The rows
// are LoadoutWidgetRows.cpp, the card and the showcase LoadoutWidgetInspect.cpp, the keys and actions
// LoadoutWidgetInput.cpp, dragging LoadoutWidgetDrag.cpp, the showcase's ring and the motion LoadoutWidgetPaint.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "AI_Looter_Shooter.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

using namespace LooterUI;
using namespace LoadoutParts;

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

	// The showcase is spawned the first time, then kept, idle while the screen is closed. Only in a played world: a test's
	// editor world builds the screen without it.
	UWorld* World = GetWorld();
	if (!Stage.IsValid() && World && World->IsGameWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Stage = World->SpawnActor<ALoadoutGunStage>(Params);
	}
	if (ALoadoutGunStage* StagePtr = Stage.Get())
	{
		if (!StageMaterial)
		{
			// The inventory's picture material: it cuts the model out of the capture and tone maps it.
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, StageMaterialPath))
			{
				StageMaterial = UMaterialInstanceDynamic::Create(Base, this);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Loadout screen: %s is missing, so the gun can't be shown."), StageMaterialPath);
			}
		}
		if (StageMaterial)
		{
			StageMaterial->SetTextureParameterValue(TEXT("Capture"), StagePtr->GetRenderTarget());
		}
		ApplyStageBrush();
		StagePtr->SetActive(true);
	}

	// The guns carried the first time it opens aren't news; anything found after is NEW until the cursor has been on it.
	if (!bSeenPrimed && InManager)
	{
		for (const AWeaponBase* Weapon : InManager->GetWeapons())
		{
			if (Weapon)
			{
				SeenGuns.Add(LoadoutRules::GunIdentity(Weapon->GetInstance()));
			}
		}
		for (const FWeaponInstanceData& Gun : InManager->GetBackpack())
		{
			SeenGuns.Add(LoadoutRules::GunIdentity(Gun));
		}
		bSeenPrimed = true;
	}

	// Start on the gun in hand (the swap target too), at a glance, nothing picked up or dragged.
	PickedSlot.Reset();
	bPressPending = bItemDrag = false;
	bShowcasePress = bTurning = false;
	bInspecting = false;
	Zone = EZone::Slots;
	TargetSlot = CursorIndex = InManager ? FMath::Max(InManager->GetActiveSlot(), 0) : 0;
	CardIdentity = 0;
	Clock = 0.f;
	ApplyMode();
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
	// Off screen: the showcase stops rendering.
	bPressPending = bItemDrag = false;
	bShowcasePress = bTurning = false;
	TurnInput = 0.f;
	if (ALoadoutGunStage* StagePtr = Stage.Get())
	{
		StagePtr->SetActive(false);
	}
	Super::NativeDestruct();
}

void ULoadoutWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickMotion(InDeltaTime);

	ALoadoutGunStage* StagePtr = Stage.Get();
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
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		// Laid out at 1600 x 900, scaled to fit the screen.
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillOverlaySlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Canvas, PageSize.X, PageSize.Y));
		Page = Canvas;

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
			return CanvasSlot;
		};

		// The title tabs: this page, the ledger and the missions.
		TArray<ULooterButton*> Tabs;
		Place(MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Loadout), Tabs), PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleTabClicked);
		}

		// The card's rarity glow sits behind everything on the right, and the showcase's ring behind its gun.
		CardGlow = MakeImage(WidgetTree, GlowBrush(FVector2D(560.f, 320.f), FLinearColor::White));
		CardGlow->SetColorAndOpacity(FLinearColor::Transparent);
		MarkBackground(CardGlow);
		Place(CardGlow, FVector2D(LoadoutLayout::CardX - 80.f, LoadoutLayout::ColumnTop - 90.f), FVector2D(560.f, 320.f));

		// The showcase: the far half of its ring, the gun's picture, then the near half.
		BackLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		BackLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintBack(Geometry, Elements, LayerId); });
		BackLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(BackLayer, FVector2D::ZeroVector, PageSize);

		StageImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Showcase"));
		StageImage->SetBrush(RectBrush(FLinearColor::Transparent));
		StageImage->SetCursor(EMouseCursor::GrabHand);
		StageSlot = Place(StageImage, LoadoutLayout::ShowcaseTopLeft, LoadoutLayout::ShowcaseSize);

		FrontLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		FrontLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintFront(Geometry, Elements, LayerId); });
		FrontLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(FrontLayer, FVector2D::ZeroVector, PageSize);

		ShowcaseHint = Label(WidgetTree, TEXT(""), 8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.8f), 220);
		ShowcaseHint->SetVisibility(ESlateVisibility::HitTestInvisible);
		ShowcaseHintSlot = Place(ShowcaseHint, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Left: everything carried, as one list: the equip slots, the backpack, then the ammo.
		{
			UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			auto Header = [this](const TCHAR* Title, TObjectPtr<UTextBlock>& OutCount)
			{
				UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				Line->AddChildToHorizontalBox(Label(WidgetTree, Title, 10, Color::Accent(), 300))->SetVerticalAlignment(VAlign_Center);
				OutCount = Label(WidgetTree, TEXT(""), 10, Color::TextDim(), 200);
				UHorizontalBoxSlot* CountSlot = Line->AddChildToHorizontalBox(OutCount);
				CountSlot->SetVerticalAlignment(VAlign_Center);
				CountSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
				UHorizontalBoxSlot* RuleSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::RowLine())), 0.f, 1.f));
				RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				RuleSlot->SetVerticalAlignment(VAlign_Center);
				RuleSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
				return Line;
			};
			Column->AddChildToVerticalBox(Header(TEXT("Equipped"), EquippedCount));
			SlotList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Column->AddChildToVerticalBox(SlotList)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

			// The backpack's header carries its sort, which a click (or R) changes.
			UHorizontalBox* PackHead = Header(TEXT("Backpack"), BackpackCount);
			SortText = Label(WidgetTree, TEXT(""), 8, Color::TextDim(), 160);
			UHorizontalBox* SortLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			SortLine->AddChildToHorizontalBox(SortText)->SetVerticalAlignment(VAlign_Center);
			SortButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
			SortButton->bPlaysSounds = false;
			SortButton->SetupContent(SortLine, ActionPrompt, static_cast<int32>(EAction::Sort));
			SortButton->SetCursor(EMouseCursor::Hand);
			SortButton->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandlePromptClicked);
			UHorizontalBoxSlot* SortSlot = PackHead->AddChildToHorizontalBox(SortButton);
			SortSlot->SetVerticalAlignment(VAlign_Center);
			SortSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
			Column->AddChildToVerticalBox(PackHead)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

			ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(ListBox);
			UVerticalBoxSlot* ListSlot = Column->AddChildToVerticalBox(ListBox);
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

			// The ammo, on one quiet line: its icon and the rounds carried.
			UHorizontalBox* AmmoLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			AmmoLine->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Ammo"), 8, Color::TextDim(), 300))->SetVerticalAlignment(VAlign_Center);
			AmmoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UHorizontalBoxSlot* AmmoSlot = AmmoLine->AddChildToHorizontalBox(AmmoRow);
			AmmoSlot->SetVerticalAlignment(VAlign_Center);
			AmmoSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
			Column->AddChildToVerticalBox(AmmoLine)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

			ListColumn = Column;
			Place(Column, FVector2D(LoadoutLayout::ListX, LoadoutLayout::ColumnTop),
				FVector2D(LoadoutLayout::ListWidth, LoadoutLayout::ColumnBottom - LoadoutLayout::ColumnTop));
		}

		// Right: the card of the gun under the cursor, always here. Its contents scroll when Inspect opens all of them.
		{
			CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			CardScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(CardScroll);
			CardScroll->AddChild(CardBox);
			USizeBox* Limit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Limit->SetMaxDesiredHeight(LoadoutLayout::ColumnBottom - LoadoutLayout::ColumnTop - 36.f);
			Limit->SetContent(CardScroll);
			UImage* Fill = nullptr;
			UImage* Line = nullptr;
			UOverlay* Card = MakeCard(WidgetTree, Limit, FMargin(20.f, 18.f, 14.f, 18.f), 1.7f, Fill, Line);
			CardFill = Fill;
			CardLine = Line;
			CardFill->SetColorAndOpacity(Colors::InspectFill());
			// The rarity's colour along the card's top edge (clear of its cut corner).
			CardStripe = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
			UOverlaySlot* StripeSlot = Card->AddChildToOverlay(MakeSized(WidgetTree, CardStripe, 0.f, 3.f));
			StripeSlot->SetHorizontalAlignment(HAlign_Fill);
			StripeSlot->SetVerticalAlignment(VAlign_Top);
			StripeSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
			Place(MakeSized(WidgetTree, Card, LoadoutLayout::CardWidth, 0.f), FVector2D(LoadoutLayout::CardX, LoadoutLayout::ColumnTop), FVector2D::ZeroVector);
		}

		// The gun being dragged, following the mouse: its picture and name, and what letting go here would do.
		{
			UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			GhostBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Content->AddChildToVerticalBox(GhostBox);
			GhostAction = Label(WidgetTree, TEXT(""), 8, Color::Accent(), 150);
			Content->AddChildToVerticalBox(GhostAction)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
			UImage* GhostFill = nullptr;
			UImage* GhostLine = nullptr;
			UOverlay* Card = MakeCard(WidgetTree, Content, FMargin(12.f, 8.f), 1.3f, GhostFill, GhostLine);
			GhostFill->SetColorAndOpacity(Colors::InspectFill());
			GhostLine->SetColorAndOpacity(Color::Accent());
			DragGhost = MakeSized(WidgetTree, Card, GhostWidth, 0.f);
			DragGhost->SetVisibility(ESlateVisibility::Collapsed);
			DragGhost->SetRenderOpacity(0.92f);
			GhostSlot = Canvas->AddChildToCanvas(DragGhost);
			GhostSlot->SetAutoSize(true);
		}

		// Bottom: what the keys do right now.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(PromptBar, LoadoutLayout::PromptBarPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		DiscBrush = IconBrush(TEXT("LoadoutDisc"), DiscIcon(), 2.f, FVector2D(64.f, 64.f), FLinearColor::White);

		// Opened before it was first shown: the picture and contents can go in now.
		ApplyStageBrush();
		ApplyMode();
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
		Brush.ImageSize = bInspecting ? LoadoutLayout::InspectShowcaseSize : LoadoutLayout::ShowcaseSize;
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
	TargetSlot = FMath::Clamp(TargetSlot, 0, FMath::Max(NumSlots() - 1, 0));

	RebuildSlots();
	RebuildList();
	if (Zone == EZone::Backpack && ListRows.IsEmpty())
	{
		Zone = EZone::Slots;
		CursorIndex = TargetSlot;
	}
	CursorIndex = FMath::Clamp(CursorIndex, 0, FMath::Max((Zone == EZone::Slots ? SlotRows.Num() : ListRows.Num()) - 1, 0));
	MarkSeen();
	Restyle();
	RefreshCounts();
	RefreshCard();
	RefreshAmmo();
	RefreshPrompts();
	RefreshShowcase();
}

void ULoadoutWidget::HandleAmmoChanged(EAmmoType Type, int32 Carried)
{
	RefreshAmmo();
}

ULoadoutWidget::FView ULoadoutWidget::GetView() const
{
	FView View;
	View.bOnSlot = Zone == EZone::Slots;
	View.Cursor = CursorIndex;
	View.TargetSlot = TargetSlot;
	View.bInspecting = bInspecting;
	View.Sort = Sort;
	View.ListOrder = ListOrder;
	for (const FPrompt& Prompt : CurrentPrompts())
	{
		View.Prompts.Add(FString::Printf(TEXT("%s: %s"), *Prompt.Key, *Prompt.Text));
	}
	return View;
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

const FWeaponInstanceData* ULoadoutWidget::CursorItem() const
{
	return Zone == EZone::Slots ? SlotItem(CursorIndex) : ListItem(CursorIndex);
}

const FWeaponInstanceData* ULoadoutWidget::Baseline() const
{
	// A backpack gun is compared with the gun it would replace, and only when that's the same kind: a shotgun's eight
	// shells against a rifle's thirty rounds would be all red arrows and no news.
	const FWeaponInstanceData* Gun = Zone == EZone::Backpack ? ListItem(CursorIndex) : nullptr;
	const FWeaponInstanceData* Target = SlotItem(TargetSlot);
	return Gun && Target && Gun->Definition && Gun->Definition == Target->Definition ? Target : nullptr;
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

bool ULoadoutWidget::HasBackpackRoom() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory && Inventory->GetBackpack().Num() < Inventory->BackpackCapacity;
}

bool ULoadoutWidget::IsNew(uint32 Identity) const
{
	return Identity != 0 && !SeenGuns.Contains(Identity);
}

FLinearColor ULoadoutWidget::RarityColor(const FWeaponInstanceData* Gun) const
{
	return Gun ? LooterWeaponText::Color(*Gun) : Color::TextDim();
}
