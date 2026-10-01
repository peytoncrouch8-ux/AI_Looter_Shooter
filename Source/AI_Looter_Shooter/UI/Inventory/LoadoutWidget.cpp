#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "AI_Looter_Shooter.h"
#include "UI/Inventory/LoadoutStage.h"
#include "UI/Style/LooterButton.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
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
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		// Laid out at 1600 x 900, scaled to fit the screen.
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillOverlaySlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Canvas, 1600.f, 900.f));

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

		// The stand: the far half of its ring behind the stand-in, the picture, then the near half.
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

		// Title tabs: this page and the bestiary.
		TArray<ULooterButton*> Tabs;
		Place(MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Loadout), Tabs), PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleTabClicked);
		}

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
			UHorizontalBoxSlot* RuleSlot = ListHead->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Hex(90, 200, 255, 61))), 0.f, 1.f));
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
	Top->AddChildToHorizontalBox(MakeSized(WidgetTree, Badge, 28.f, 28.f))->SetVerticalAlignment(VAlign_Center);
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
	Card.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, SlotCardHeight), ActionSlot, SlotIndex);
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

	const FString Header = !SlotGun ? TEXT("Backpack · equip into slot")
		: NumSameKind > 0 ? FString::Printf(TEXT("Backpack · %s first"), *KindName(*SlotGun))
		: FString(TEXT("Backpack · swap with slot"));
	ListHeader->SetText(FText::FromString(Header.ToUpper()));
	ListCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Inventory->GetBackpack().Num(), Inventory->BackpackCapacity)));

	// Every slot the backpack has, full or not (the list scrolls when they don't all fit): its guns, the chosen slot's
	// kind first, then the free slots, where the chosen slot's gun can be stored.
	const int32 NumRows = FMath::Max(Inventory->BackpackCapacity, ListOrder.Num());
	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		// Other kinds of gun follow the chosen slot's kind, under their own heading.
		if (Row == NumSameKind && NumSameKind > 0 && NumSameKind < ListOrder.Num())
		{
			if (UScrollBoxSlot* HeadingSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Label(WidgetTree, TEXT("Other weapons"), 8, Color::TextDim(), 220))))
			{
				HeadingSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
			}
		}
		const FCard Card = Row < ListOrder.Num() ? MakeListCard(Row) : MakeFreeListCard(Row);
		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Card.Button)))
		{
			CardSlot->SetPadding(FMargin(0.f, 8.f, 6.f, 0.f));
		}
		ListCards.Add(Card);
	}
}

ULoadoutWidget::FCard ULoadoutWidget::MakeFreeListCard(int32 Row)
{
	// A free backpack slot: the same size as a gun's row, quieter, saying what it's for.
	FCard Card;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* LabelSlot = Line->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Empty"), 9, Hex(143, 179, 204, 150), 200));
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Line->AddChildToHorizontalBox(Label(WidgetTree, FString::FromInt(Row + 1), 9, Hex(143, 179, 204, 110), 60))->SetVerticalAlignment(VAlign_Center);

	UOverlay* Box = MakeCard(WidgetTree, Line, FMargin(16.f, 7.f), 1.3f, Card.Fill, Card.Line);
	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Card.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, ListCardHeight), ActionBackpack, Row);
	Card.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleCardClicked);
	Card.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleCardHovered);
	return Card;
}

ULoadoutWidget::FCard ULoadoutWidget::MakeListCard(int32 Row)
{
	FCard Card;
	const FWeaponInstanceData* Item = ListItem(Row);
	check(Item);
	const FLinearColor Rarity = LooterWeaponText::Color(*Item);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeGunPicture(WidgetTree, *Item, FVector2D(84.f, 28.f), 1.5f,
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
			Mark->AddChildToHorizontalBox(MakeImage(WidgetTree, IconBrush(bUpgrade ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(bUpgrade), 4.f,
				FVector2D(10.f, 8.f), VerdictColor)))->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* WordSlot = Mark->AddChildToHorizontalBox(Label(WidgetTree, bSimilar ? TEXT("Similar") : (bUpgrade ? TEXT("Upgrade") : TEXT("Weaker")), 10, VerdictColor, 60));
		WordSlot->SetVerticalAlignment(VAlign_Center);
		WordSlot->SetPadding(FMargin(bSimilar ? 0.f : 6.f, 0.f, 0.f, 0.f));
		Line->AddChildToHorizontalBox(Mark)->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* Box = MakeCard(WidgetTree, Line, FMargin(12.f, 7.f), 1.3f, Card.Fill, Card.Line);
	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Card.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, ListCardHeight), ActionBackpack, Row);
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
		const bool bFree = !ListOrder.IsValidIndex(Row);
		ListCards[Row].Fill->SetColorAndOpacity(bCursor ? Color::Tile() : (bFree ? Colors::EmptyFill() : Colors::CardFill()));
		ListCards[Row].Line->SetColorAndOpacity(bCursor ? Color::Accent() : (bFree ? Colors::CardLine() * FLinearColor(1.f, 1.f, 1.f, 0.5f) : Colors::CardLine()));
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
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Label(WidgetTree, StatName, 8, Color::TextDim(), 120), 92.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BarSlot = Line->AddChildToHorizontalBox(MakeSegmentBar(WidgetTree, 8, Rating, Color::SegmentOn(), 7.f));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* ValueText = MakeText(WidgetTree, Value, 11, Color::Text());
		ValueText->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, ValueText, 60.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		// The change from the chosen slot's gun, green when it's an upgrade.
		UTextBlock* DeltaText = MakeText(WidgetTree, TEXT(""), 9, Color::TextDim());
		DeltaText->SetJustification(ETextJustify::Right);
		if (B && !FMath::IsNearlyEqual(New, Old, 0.01f))
		{
			DeltaText->SetText(FText::FromString(FormatDelta(New - Old, Decimals)));
			DeltaText->SetColorAndOpacity(FSlateColor(((New > Old) == bHigherIsBetter) ? Color::Better() : Color::Worse()));
		}
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, DeltaText, 46.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		// Nine stats: kept compact so the backpack list below still shows a few rows.
		DetailsBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 20.f))->SetPadding(FMargin(0.f, 1.f));
	};
	// Damage compares the whole shot, so shotguns and rifles line up fairly.
	AddStat(TEXT("Damage"), LooterWeaponText::DamageRating(S), LooterWeaponText::DamageString(S), S.Damage * S.PelletsPerShot,
		B ? B->Damage * B->PelletsPerShot : 0.f, true, 1);
	AddStat(TEXT("Fire rate"), LooterWeaponText::FireRateRating(S), FString::Printf(TEXT("%.0f"), S.FireRate), S.FireRate, B ? B->FireRate : 0.f, true, 0);
	AddStat(TEXT("Magazine"), LooterWeaponText::MagazineRating(S), FString::FromInt(S.MagazineSize), S.MagazineSize, B ? B->MagazineSize : 0.f, true, 0);
	AddStat(TEXT("Reload"), LooterWeaponText::ReloadRating(S), FString::Printf(TEXT("%.2fs"), S.ReloadTime), S.ReloadTime, B ? B->ReloadTime : 0.f, false, 2);
	AddStat(TEXT("Accuracy"), LooterWeaponText::AccuracyRating(S), FString::Printf(TEXT("%.1f°"), S.Spread), S.Spread, B ? B->Spread : 0.f, false, 1);
	// Range is where the damage starts to fall off; recoil and handling are against a plain gun of its kind.
	AddStat(TEXT("Range"), LooterWeaponText::RangeRating(S), FString::Printf(TEXT("%.0f m"), S.Range / 100.f), S.Range / 100.f, B ? B->Range / 100.f : 0.f, true, 0);
	AddStat(TEXT("Recoil"), LooterWeaponText::RecoilRating(S), FString::Printf(TEXT("%.0f%%"), S.Recoil * 100.f), S.Recoil * 100.f, B ? B->Recoil * 100.f : 0.f, false, 0);
	AddStat(TEXT("Handling"), LooterWeaponText::HandlingRating(S), FString::Printf(TEXT("%.0f%%"), S.Handling * 100.f), S.Handling * 100.f, B ? B->Handling * 100.f : 0.f, true, 0);
	AddStat(TEXT("Zoom"), LooterWeaponText::ZoomRating(S), LooterWeaponText::ZoomString(S), S.Zoom, B ? B->Zoom : 0.f, true, 2);
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
		FillOverlaySlot(Bar->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Color::SegmentOff()))));
		const float Level = 44.f * FMath::Clamp(static_cast<float>(Carried) / Max, 0.f, 1.f);
		if (Level > 0.5f)
		{
			UOverlaySlot* LevelSlot = Bar->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SegmentOn())), 0.f, Level));
			LevelSlot->SetHorizontalAlignment(HAlign_Fill);
			LevelSlot->SetVerticalAlignment(VAlign_Bottom);
		}
		UVerticalBoxSlot* BarSlot = Gauge->AddChildToVerticalBox(MakeSized(WidgetTree, Bar, 10.f, 44.f));
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetPadding(FMargin(0.f, 4.f));

		UVerticalBoxSlot* IconSlot = Gauge->AddChildToVerticalBox(MakeImage(WidgetTree, IconBrush(*FString::Printf(TEXT("Ammo%d"), Index), AmmoIcon(Type), 2.f,
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
	const bool bBackpack = !ListCards.IsEmpty();
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
	else if (ListItem(CursorIndex))
	{
		Prompts.Add({ TEXT("E"), SlotItem(ChosenSlot) ? TEXT("Swap in") : TEXT("Equip") });
		Prompts.Add({ TEXT("F"), TEXT("Hold") });
		Prompts.Add({ TEXT("Q"), TEXT("Drop") });
		Prompts.Add({ TEXT("W / S"), TEXT("Browse") });
		Prompts.Add({ TEXT("A"), TEXT("Slots") });
	}
	else
	{
		// A free backpack slot: the chosen slot's gun can be stored in it.
		if (SlotItem(ChosenSlot))
		{
			Prompts.Add({ TEXT("E"), TEXT("Store here") });
		}
		Prompts.Add({ TEXT("W / S"), TEXT("Browse") });
		Prompts.Add({ TEXT("A"), TEXT("Slots") });
	}
	if (!PickedSlot.IsSet())
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
		Prompts.Add({ TEXT("2"), TEXT("Bestiary") });
		Prompts.Add({ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close") });
	}
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
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
