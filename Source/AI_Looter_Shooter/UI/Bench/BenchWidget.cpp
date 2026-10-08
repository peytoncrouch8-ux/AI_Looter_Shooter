// UBenchWidget: the gunsmith's bench's screen: opening and closing, its layout, and the queries the other files share.

#include "UI/Bench/BenchWidget.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchRules.h"
#include "UI/Bench/BenchStage.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "LooterBench"

using namespace LooterUI;
using namespace LoadoutParts;
// BenchLayout is spelled out: the inventory's layout (LoadoutParts) has names of its own for the stand and the columns.
namespace Layout = BenchLayout;

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UBenchWidget::Open(ALooterHUD* InHUD, UWeaponManagerComponent* InManager, AActor* InBench)
{
	OwningHUD = InHUD;
	Bench = InBench;
	if (Manager.Get() != InManager)
	{
		if (UWeaponManagerComponent* Old = Manager.Get())
		{
			Old->OnInventoryChanged.RemoveDynamic(this, &UBenchWidget::Refresh);
		}
		Manager = InManager;
		if (InManager)
		{
			InManager->OnInventoryChanged.AddUniqueDynamic(this, &UBenchWidget::Refresh);
		}
	}
	SetIsFocusable(true);

	// The stand is spawned the first time, then kept, idle while the screen is closed.
	UWorld* World = GetWorld();
	if (!Stage.IsValid() && World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Stage = World->SpawnActor<ABenchStage>(Params);
	}
	if (ABenchStage* StagePtr = Stage.Get())
	{
		if (!StageMaterial)
		{
			// The loadout's picture material: it cuts the gun out of the capture and tone maps it.
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, StageMaterialPath))
			{
				StageMaterial = UMaterialInstanceDynamic::Create(Base, this);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Bench screen: %s is missing, so the gun can't be shown."), StageMaterialPath);
			}
		}
		if (StageMaterial)
		{
			StageMaterial->SetTextureParameterValue(TEXT("Capture"), StagePtr->GetRenderTarget());
		}
		ApplyStageBrush();
		StagePtr->SetActive(true);
	}

	// Start on the gun in hand, the cursor on it, nothing being scrapped or asked.
	Guns = InManager ? BenchRules::CarriedGuns(*InManager) : TArray<FCarriedGun>();
	const int32 InHand = InManager ? InManager->GetActiveSlot() : INDEX_NONE;
	ChosenGun = FMath::Max(Guns.IndexOfByKey(FCarriedGun::Equipped(InHand)), 0);
	ChosenSlot = 0;
	Column = EColumn::Guns;
	CursorIndex = ChosenGun;
	bScrapping = false;
	bDragging = false;
	TurnInput = 0.f;
	CloseConfirm();
	SetStatus(FText::GetEmpty(), Color::TextDim());
	Refresh();
	RefreshStage(true);
}

void UBenchWidget::Close()
{
	CloseConfirm();
	bScrapping = false;
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseBench();
	}
}

void UBenchWidget::NativeDestruct()
{
	// Off screen: the stand stops rendering.
	bDragging = false;
	TurnInput = 0.f;
	if (ABenchStage* StagePtr = Stage.Get())
	{
		StagePtr->SetActive(false);
	}
	Super::NativeDestruct();
}

void UBenchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ABenchStage* StagePtr = Stage.Get();
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

TSharedRef<SWidget> UBenchWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the screen, dimmed, as behind the inventory.
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
		};

		// The title, as the inventory's shown tab, and under it what the last action did.
		{
			UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Plate->SetBrush(RectBrush(Color::Plate(), Color::ScreenLine(), 1.f));
			MarkBackground(Plate);
			Plate->SetPadding(FMargin(26.f, 5.f));
			Plate->SetContent(Label(WidgetTree, TEXT("Gunsmith's bench"), 14, Color::Title(), 350));
			Place(MakeShapeBox(WidgetTree, EShape::Tab, Color::FrameFill(), Color::FrameEdge(), Plate, FMargin(18.f, 5.f, 18.f, 3.f)),
				PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
			StatusText = Label(WidgetTree, TEXT(""), 9, Color::TextDim(), 160);
			StatusText->SetJustification(ETextJustify::Center);
			Place(StatusText, FVector2D(800.f, 90.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		}

		// The stand: the far half of its ring behind the gun, the picture, then the near half and the chosen part's mark.
		BackLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		BackLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintBack(Geometry, Elements, LayerId); });
		BackLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(BackLayer, FVector2D::ZeroVector, PageSize);

		StageImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Stage"));
		StageImage->SetBrush(RectBrush(FLinearColor::Transparent));
		StageImage->SetCursor(EMouseCursor::GrabHand);
		Place(StageImage, Layout::StageTopLeft, Layout::StageSize);

		FrontLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		FrontLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintFront(Geometry, Elements, LayerId); });
		FrontLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(FrontLayer, FVector2D::ZeroVector, PageSize);

		Place(Label(WidgetTree, TEXT("Drag to turn"), 8, Color::TextDim(), 220),
			FVector2D(Layout::StageTopLeft.X + Layout::StageSize.X * 0.5f, Layout::StageTopLeft.Y + Layout::StageSize.Y + 4.f), FVector2D::ZeroVector,
			FVector2D(0.5f, 0.f));

		// Left: every gun carried.
		{
			UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UTextBlock* Title = nullptr;
			UTextBlock* Count = nullptr;
			Left->AddChildToVerticalBox(MakeColumnHeader(Title, TEXT("Guns"), Count));
			GunsCount = Count;
			GunList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(GunList);
			UVerticalBoxSlot* ListSlot = Left->AddChildToVerticalBox(GunList);
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			Place(Left, FVector2D(Layout::GunsX, Layout::ColumnTop), FVector2D(Layout::GunsWidth, Layout::ColumnHeight));
		}

		// Beside the stand: the chosen gun's slots, and scrapping it.
		{
			UVerticalBox* Slots = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UTextBlock* Title = nullptr;
			UTextBlock* Count = nullptr;
			Slots->AddChildToVerticalBox(MakeColumnHeader(Title, TEXT("On this gun"), Count));
			SlotsTitle = Title;
			SlotsCount = Count;
			SlotList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Slots->AddChildToVerticalBox(SlotList);
			Place(Slots, FVector2D(Layout::SlotsX, Layout::ColumnTop), FVector2D(Layout::SlotsWidth, Layout::ColumnHeight - 60.f));

			UOverlay* Buttons = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			ScrapButton = MakeButton(BenchActions::Scrap, LOCTEXT("ScrapThisGun", "Scrap this gun"), EButtonKind::Danger);
			FillOverlaySlot(Buttons->AddChildToOverlay(ScrapButton));
			StopScrappingButton = MakeButton(BenchActions::StopScrapping, LOCTEXT("KeepTheGun", "Keep the gun"), EButtonKind::Normal);
			StopScrappingButton->SetVisibility(ESlateVisibility::Collapsed);
			FillOverlaySlot(Buttons->AddChildToOverlay(StopScrappingButton));
			Place(Buttons, FVector2D(Layout::SlotsX, Layout::ColumnTop + Layout::ColumnHeight - 44.f), FVector2D(Layout::SlotsWidth, 40.f));
		}

		// Under the stand: the chosen gun's name, level, notches and curse, then its stats.
		{
			UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			GunTag = Label(WidgetTree, TEXT(""), 8, Color::Accent(), 220);
			Card->AddChildToVerticalBox(GunTag);
			GunName = Label(WidgetTree, TEXT(""), 15, Color::Text(), 40);
			GunName->SetAutoWrapText(true);
			Card->AddChildToVerticalBox(GunName)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
			GunSub = Label(WidgetTree, TEXT(""), 9, Color::TextDim(), 140);
			Card->AddChildToVerticalBox(GunSub)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
			IdeasBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Card->AddChildToVerticalBox(IdeasBox)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

			UVerticalBox* Stats = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			StatsHeader = Label(WidgetTree, TEXT(""), 8, Color::Accent(), 220);
			Stats->AddChildToVerticalBox(StatsHeader);
			StatsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Stats->AddChildToVerticalBox(StatsBox)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
			UImage* StatsFill = nullptr;
			UImage* StatsLine = nullptr;
			UOverlay* StatsCard = MakeCard(WidgetTree, Stats, FMargin(16.f, 10.f), 1.7f, StatsFill, StatsLine);
			StatsFill->SetColorAndOpacity(Colors::CardFill());
			StatsLine->SetColorAndOpacity(Colors::CardLine());
			Card->AddChildToVerticalBox(StatsCard)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
			Card->SetVisibility(ESlateVisibility::HitTestInvisible);
			// As wide as the stand; as tall as what it shows (a cursed gun's rows take more room).
			Place(MakeSized(WidgetTree, Card, Layout::StageSize.X, 0.f), FVector2D(Layout::StageTopLeft.X, Layout::GunCardTop), FVector2D::ZeroVector);
		}

		// Right: the parts box, for the chosen slot.
		{
			UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UTextBlock* Title = nullptr;
			UTextBlock* Count = nullptr;
			Right->AddChildToVerticalBox(MakeColumnHeader(Title, TEXT("Parts box"), Count));
			PartsCount = Count;
			PartsSub = Label(WidgetTree, TEXT(""), 8, Color::TextDim(), 220);
			Right->AddChildToVerticalBox(PartsSub)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			PartList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(PartList);
			UVerticalBoxSlot* ListSlot = Right->AddChildToVerticalBox(PartList);
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			Place(Right, FVector2D(Layout::PartsX, Layout::ColumnTop), FVector2D(Layout::PartsWidth, Layout::ColumnHeight));
		}

		// Bottom: what the keys do right now.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(PromptBar, FVector2D(800.f, 852.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Over everything: the confirm, when one asks.
		PopupLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Popup"));
		FillOverlaySlot(Root->AddChildToOverlay(PopupLayer));
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);

		DiscBrush = IconBrush(TEXT("LoadoutDisc"), DiscIcon(), 2.f, FVector2D(64.f, 64.f), FLinearColor::White);

		// Opened before it was first shown: the picture and contents can go in now.
		ApplyStageBrush();
		SetStatus(FText::GetEmpty(), Color::TextDim());
		Refresh();
	}
	return Super::RebuildWidget();
}

UWidget* UBenchWidget::MakeColumnHeader(UTextBlock*& OutTitle, const FString& Title, UTextBlock*& OutCount)
{
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	OutTitle = Label(WidgetTree, Title, 10, Color::Accent(), 300);
	Header->AddChildToHorizontalBox(OutTitle)->SetVerticalAlignment(VAlign_Center);
	OutCount = Label(WidgetTree, TEXT(""), 10, Color::TextDim(), 200);
	UHorizontalBoxSlot* CountSlot = Header->AddChildToHorizontalBox(OutCount);
	CountSlot->SetVerticalAlignment(VAlign_Center);
	CountSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* RuleSlot = Header->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Colors::CardLine())), 0.f, 1.f));
	RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RuleSlot->SetVerticalAlignment(VAlign_Center);
	RuleSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	return Header;
}

void UBenchWidget::ApplyStageBrush()
{
	if (StageImage && StageMaterial)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(StageMaterial);
		Brush.ImageSize = Layout::StageSize;
		StageImage->SetBrush(Brush);
	}
}

// ---------------------------------------------------------------------------
// Contents
// ---------------------------------------------------------------------------

void UBenchWidget::Refresh()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (bApplyingAction || !GunList || !Inventory)
	{
		return;
	}
	// The guns or the box may have changed under the cursor and the choices.
	Guns = BenchRules::CarriedGuns(*Inventory);
	ChosenGun = FMath::Clamp(ChosenGun, 0, FMath::Max(Guns.Num() - 1, 0));
	ChosenSlot = FMath::Clamp(ChosenSlot, 0, FMath::Max(NumSlots() - 1, 0));
	if (bScrapping && !Inventory->CanScrap(ChosenRef()))
	{
		bScrapping = false;
	}

	RebuildGuns();
	RebuildSlots();
	RebuildParts();
	if (Column == EColumn::Parts && PartRows.IsEmpty())
	{
		Column = EColumn::Slots;
		CursorIndex = ChosenSlot;
	}
	if (Column == EColumn::Slots && SlotRows.IsEmpty())
	{
		Column = EColumn::Guns;
		CursorIndex = ChosenGun;
	}
	CursorIndex = FMath::Clamp(CursorIndex, 0, FMath::Max(RowsOf(Column).Num() - 1, 0));
	Restyle();
	RefreshGun();
	RefreshStage(false);
	RefreshPrompts();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const FWeaponInstanceData* UBenchWidget::ChosenItem() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory && Guns.IsValidIndex(ChosenGun) ? Inventory->FindCarriedGun(Guns[ChosenGun]) : nullptr;
}

FCarriedGun UBenchWidget::ChosenRef() const
{
	return Guns.IsValidIndex(ChosenGun) ? Guns[ChosenGun] : FCarriedGun();
}

bool UBenchWidget::CanChange() const
{
	const FWeaponInstanceData* Item = ChosenItem();
	return Item && WeaponPartSwap::CanModify(*Item);
}

int32 UBenchWidget::NumSlots() const
{
	const FWeaponInstanceData* Item = ChosenItem();
	return Item && Item->Definition ? Item->Definition->Parts.Num() : 0;
}

FName UBenchWidget::SlotNameAt(int32 SlotIndex) const
{
	const FWeaponInstanceData* Item = ChosenItem();
	return Item && Item->Definition && Item->Definition->Parts.IsValidIndex(SlotIndex) ? Item->Definition->Parts[SlotIndex].Name : NAME_None;
}

const FBoxedWeaponPart* UBenchWidget::RowPart(int32 Row) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !PartEntries.IsValidIndex(Row))
	{
		return nullptr;
	}
	const TArray<FBoxedWeaponPart>& Box = Inventory->GetPartsBox();
	return Box.IsValidIndex(PartEntries[Row].BoxIndex) ? &Box[PartEntries[Row].BoxIndex] : nullptr;
}

const FBoxedWeaponPart* UBenchWidget::PreviewPart() const
{
	if (Column != EColumn::Parts || bScrapping || !PartEntries.IsValidIndex(CursorIndex) || !PartEntries[CursorIndex].Fits())
	{
		return nullptr;
	}
	return RowPart(CursorIndex);
}

int32 UBenchWidget::MarkedSlot() const
{
	return bScrapping && Column == EColumn::Slots ? CursorIndex : ChosenSlot;
}

void UBenchWidget::PlayCue(const TCHAR* Cue, float Volume) const
{
	LooterSound::Play2D(this, Cue, Volume);
}

#undef LOCTEXT_NAMESPACE
