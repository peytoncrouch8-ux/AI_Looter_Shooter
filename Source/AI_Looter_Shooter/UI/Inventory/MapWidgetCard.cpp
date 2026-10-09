// UMapWidget: the side column's contents (the legend's counts, the travel card, the open graves' list), the prompts, and
// what the page shows for the tests.

#include "UI/Inventory/MapWidget.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
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
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const FName ActionGraveRow(TEXT("Grave"));

	/** A grave's row in the list. */
	constexpr float GraveRowHeight = 44.f;

	/** How far, for people: "85 m", "1.2 km". */
	FString DistanceWords(double Centimetres)
	{
		const double Metres = Centimetres / 100.0;
		return Metres < 1000.0 ? FString::Printf(TEXT("%d m"), FMath::RoundToInt32(Metres)) : FString::Printf(TEXT("%.1f km"), Metres / 1000.0);
	}

	void SetTextIfChanged(UTextBlock* Text, const FString& Words)
	{
		if (Text && !Text->GetText().ToString().Equals(Words, ESearchCase::CaseSensitive))
		{
			Text->SetText(FText::FromString(Words));
		}
	}
}

// ---------------------------------------------------------------------------
// The legend
// ---------------------------------------------------------------------------

void UMapWidget::RefreshLegend()
{
	for (int32 Kind = 0; Kind < LegendCounts.Num(); ++Kind)
	{
		UTextBlock* Count = LegendCounts[Kind];
		if (!Count)
		{
			continue;
		}
		const int32 Found = MapPins::Count(Pins, static_cast<EMapPinKind>(Kind));
		SetTextIfChanged(Count, FString::FromInt(Found));
		// None of a kind yet: its count waits dim.
		Count->SetColorAndOpacity(Found > 0 ? Color::CyanText() : Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.55f));
	}
}

// ---------------------------------------------------------------------------
// The travel card
// ---------------------------------------------------------------------------

EGraveTravelBlock UMapWidget::CheckTravel(FString& OutStatus) const
{
	const UGraveTravelSubsystem* Travel = UGraveTravelSubsystem::Get(this);
	const APawn* Player = GetOwningPlayerPawn();
	const FMapPin* Pin = FindGravePin(SelectedGrave);
	const ARespawnMarker* Grave = Pin ? Pin->Grave.Get() : nullptr;
	EGraveTravelBlock Block = EGraveTravelBlock::NoPlayer;
	if (Travel)
	{
		Block = Grave ? Travel->CheckGrave(Player, *Grave) : Travel->CheckNow(Player);
	}
	if (Block != EGraveTravelBlock::None)
	{
		OutStatus = GraveTravelRules::Reason(Block).ToString();
	}
	else
	{
		OutStatus = Grave ? TEXT("Ready. The grave-wind will carry you there.") : TEXT("");
	}
	return Block;
}

void UMapWidget::RefreshTravelCard()
{
	if (!CardName || !CardDetail || !CardStatus || !TravelButton)
	{
		return;
	}
	FString Status;
	const EGraveTravelBlock Block = CheckTravel(Status);
	const FMapPin* Pin = FindGravePin(SelectedGrave);
	const int32 Graves = MapPins::Count(Pins, EMapPinKind::Grave);
	const APawn* Player = GetOwningPlayerPawn();

	FString Name;
	FString Detail;
	if (Pin)
	{
		Name = Pin->Name.ToString().ToUpper();
		Detail = Player ? FString::Printf(TEXT("Respawn grave · %s from you"), *DistanceWords(FVector::Dist2D(Player->GetActorLocation(), Pin->Location)))
			: FString(TEXT("Respawn grave"));
	}
	else if (Graves > 0)
	{
		Name = TEXT("CHOOSE A GRAVE");
		Detail = TEXT("Click an open respawn grave on the map or in the list below (G, or the D-pad), then Travel.");
		// With none chosen, only a reason that holds whatever the grave is shown.
		if (Block == EGraveTravelBlock::None)
		{
			Status.Reset();
		}
	}
	else
	{
		Name = TEXT("NO GRAVE OPEN");
		Detail = TEXT("Respawn graves open as the story goes on. Once one has, you can travel to it from here.");
		Status.Reset();
	}
	SetTextIfChanged(CardName, Name);
	SetTextIfChanged(CardDetail, Detail);
	SetTextIfChanged(CardStatus, Status);
	CardStatus->SetVisibility(Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// The verdict's colour: green when the way is clear, red when it isn't, blinking for a moment after a refusal.
	FLinearColor StatusColor = Block == EGraveTravelBlock::None ? Color::Better() : Color::Worse();
	if (StatusFlash > 0.f && FMath::Fmod(StatusFlash * 10.f, 2.f) < 1.f)
	{
		StatusColor = Color::Text();
	}
	CardStatus->SetColorAndOpacity(StatusColor);

	const bool bCanGo = Pin && Block == EGraveTravelBlock::None;
	TravelButton->SetIsEnabled(bCanGo);
	TravelButton->SetRenderOpacity(bCanGo ? 1.f : (Pin ? 0.55f : 0.3f));
	ShownBlock = Block;
	ShownStatus = Status;
}

void UMapWidget::FlashStatus()
{
	StatusFlash = 0.6f;
	ShownStatus.Reset();
	RefreshTravelCard();
}

// ---------------------------------------------------------------------------
// The open graves' list
// ---------------------------------------------------------------------------

void UMapWidget::RebuildGraveList()
{
	if (!GraveList || !GraveListHeader)
	{
		return;
	}
	GraveList->ClearChildren();
	GraveRows.Reset();
	const int32 Graves = MapPins::Count(Pins, EMapPinKind::Grave);
	SetTextIfChanged(GraveListHeader, Graves > 0 ? FString::Printf(TEXT("OPEN GRAVES  ·  %d"), Graves) : FString(TEXT("NO OPEN GRAVES YET")));

	for (const FMapPin& Pin : Pins)
	{
		if (Pin.Kind != EMapPinKind::Grave)
		{
			continue;
		}
		FGraveRow& Row = GraveRows.AddDefaulted_GetRef();
		Row.GraveId = Pin.Id;
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const FLinearColor Tint = MapIcons::Color(EMapPinKind::Grave);
		Line->AddChildToHorizontalBox(MakeImage(WidgetTree, MapIcons::IconBrush(EMapPinKind::Grave, 18.f, Tint)))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(FittedLabel(WidgetTree, Pin.Name.ToString(), 10, Color::Text(), 80));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(10.f, 0.f, 8.f, 0.f));
		Row.Distance = Label(WidgetTree, TEXT(""), 9, Color::TextDim(), 80);
		Line->AddChildToHorizontalBox(Row.Distance)->SetVerticalAlignment(VAlign_Center);

		UOverlay* Box = MakeCard(WidgetTree, Line, FMargin(12.f, 6.f), 1.3f, Row.Fill, Row.Line);
		Row.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		// The page sounds its own hovers and choices (they light the grave on the map too).
		Row.Button->bPlaysSounds = false;
		Row.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, GraveRowHeight), ActionGraveRow, GraveRows.Num() - 1);
		Row.Button->OnButtonClicked.BindUObject(this, &UMapWidget::HandleGraveRowClicked);
		Row.Button->OnButtonHovered.BindUObject(this, &UMapWidget::HandleGraveRowHovered);
		GraveList->AddChildToVerticalBox(Row.Button)->SetPadding(FMargin(0.f, 6.f, 6.f, 0.f));
	}
	RestyleGraveList();
}

void UMapWidget::RestyleGraveList()
{
	const APawn* Player = GetOwningPlayerPawn();
	for (FGraveRow& Row : GraveRows)
	{
		const bool bSelected = !SelectedGrave.IsNone() && Row.GraveId == SelectedGrave;
		if (Row.Fill && Row.Line)
		{
			Row.Fill->SetColorAndOpacity(bSelected ? Color::Tile() : Colors::CardFill());
			Row.Line->SetColorAndOpacity(bSelected ? Color::Accent() : Colors::CardLine());
		}
		const FMapPin* Pin = FindGravePin(Row.GraveId);
		SetTextIfChanged(Row.Distance, Player && Pin ? DistanceWords(FVector::Dist2D(Player->GetActorLocation(), Pin->Location)).ToUpper() : FString());
	}
}

// ---------------------------------------------------------------------------
// The prompts
// ---------------------------------------------------------------------------

void UMapWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	struct FPrompt
	{
		FString Key;
		FString Text;
	};
	TArray<FPrompt, TInlineAllocator<8>> Prompts;
	if (!SelectedGrave.IsNone())
	{
		Prompts.Add({ TEXT("E"), TEXT("Travel") });
	}
	if (MapPins::Count(Pins, EMapPinKind::Grave) > 0)
	{
		Prompts.Add({ TEXT("G"), TEXT("Next grave") });
	}
	Prompts.Add({ TEXT("C"), TEXT("Find me") });
	Prompts.Add({ TEXT("Drag"), TEXT("Pan") });
	Prompts.Add({ TEXT("Wheel"), TEXT("Zoom") });
	Prompts.Add({ TEXT("1-4"), TEXT("Pages") });
	Prompts.Add({ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close") });
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
	}
}

// ---------------------------------------------------------------------------
// For the tests
// ---------------------------------------------------------------------------

UMapWidget::FView UMapWidget::GetView() const
{
	FView Shown;
	Shown.Map = View;
	Shown.Pins = Pins;
	Shown.SelectedGrave = SelectedGrave;
	Shown.Block = CheckTravel(Shown.Status);
	Shown.bHasPicture = MapTexture.IsValid();
	return Shown;
}
