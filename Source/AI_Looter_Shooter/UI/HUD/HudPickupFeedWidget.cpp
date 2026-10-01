#include "UI/HUD/HudPickupFeedWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr int32 MaxLines = 6;
	constexpr int32 FontSize = 22;
	/** Room each line takes in the stack. */
	constexpr float LineSpacing = 34.f;
	/** How fast the whole stack drifts up (pixels per second). */
	constexpr float DriftSpeed = 20.f;
	/** A new line pops in slightly large and settles over PopSeconds. */
	constexpr float PopSeconds = 0.18f;
	constexpr float PopScale = 1.3f;
	/** Fully visible until FadeStart seconds, gone at LifeSeconds. */
	constexpr float FadeStart = 2.f;
	constexpr float LifeSeconds = 3.2f;

	/**
	 * The engine's heaviest UI typeface (Roboto Black, shipped with every build), white, in a thick round-jointed black
	 * outline: chunky, soft-edged "sticker" lettering that reads over sky, grass or rock.
	 */
	FSlateFontInfo PickupFont()
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Black"), FontSize);
		Font.OutlineSettings.OutlineSize = 3;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Font.OutlineSettings.bSeparateFillAlpha = true;
		return Font;
	}
}

TSharedRef<SWidget> UHudPickupFeedWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		TextPool.Reset();
		const FSlateFontInfo Font = PickupFont();
		for (int32 Index = 0; Index < MaxLines; ++Index)
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Text->SetFont(Font);
			Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Text->SetShadowOffset(FVector2D(0.f, 3.f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f));
			Text->SetJustification(ETextJustify::Right);
			Text->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
			Text->SetVisibility(ESlateVisibility::Collapsed);
			// Anchored at the feed's bottom right; NativeTick lifts each line.
			UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(Text);
			TextSlot->SetAnchors(FAnchors(1.f, 1.f));
			TextSlot->SetAlignment(FVector2D(1.f, 1.f));
			TextSlot->SetAutoSize(true);
			TextPool.Add(Text);
		}
		WidgetTree->RootWidget = Canvas;
	}
	return Super::RebuildWidget();
}

void UHudPickupFeedWidget::NativeDestruct()
{
	if (UWeaponManagerComponent* Manager = BoundManager.Get())
	{
		Manager->OnAmmoPickedUp.RemoveDynamic(this, &UHudPickupFeedWidget::HandleAmmoPickedUp);
	}
	BoundManager.Reset();
	Super::NativeDestruct();
}

void UHudPickupFeedWidget::BindToPawn()
{
	const APlayerController* Controller = GetOwningPlayer();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (BoundManager.Get() == Manager)
	{
		return;
	}
	if (UWeaponManagerComponent* Old = BoundManager.Get())
	{
		Old->OnAmmoPickedUp.RemoveDynamic(this, &UHudPickupFeedWidget::HandleAmmoPickedUp);
	}
	if (Manager)
	{
		Manager->OnAmmoPickedUp.AddUniqueDynamic(this, &UHudPickupFeedWidget::HandleAmmoPickedUp);
	}
	BoundManager = Manager;
}

void UHudPickupFeedWidget::HandleAmmoPickedUp(EAmmoType Type, int32 Amount)
{
	const TCHAR* Name = LooterAmmo::GetInfo(Type).Name;
	AddLine(Amount > 0 ? FString::Printf(TEXT("+%d %s"), Amount, Name) : FString::Printf(TEXT("%s full"), Name));
}

void UHudPickupFeedWidget::AddLine(const FString& Text)
{
	if (TextPool.IsEmpty())
	{
		return;
	}
	// A full feed lets its oldest line go early.
	if (Lines.Num() >= TextPool.Num())
	{
		Lines[0].Text->SetVisibility(ESlateVisibility::Collapsed);
		Lines.RemoveAt(0);
	}
	UTextBlock* Free = nullptr;
	for (UTextBlock* Candidate : TextPool)
	{
		if (!Lines.ContainsByPredicate([Candidate](const FLine& Line) { return Line.Text == Candidate; }))
		{
			Free = Candidate;
			break;
		}
	}
	FLine& Line = Lines.AddDefaulted_GetRef();
	Line.Text = Free;
	Line.Text->SetText(FText::FromString(Text));
	Line.Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	Line.Text->SetRenderOpacity(1.f);
}

void UHudPickupFeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	BindToPawn();

	for (int32 Index = Lines.Num() - 1; Index >= 0; --Index)
	{
		FLine& Line = Lines[Index];
		Line.Age += InDeltaTime;
		if (Line.Age >= LifeSeconds)
		{
			Line.Text->SetVisibility(ESlateVisibility::Collapsed);
			Lines.RemoveAt(Index);
		}
	}

	// Newest at the bottom; each older line sits one step higher, and everything drifts up as it ages.
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		FLine& Line = Lines[Index];
		const int32 Above = Lines.Num() - 1 - Index;
		const float Target = Above * LineSpacing + Line.Age * DriftSpeed;
		Line.Rise = Line.Age <= InDeltaTime ? Target : FMath::FInterpTo(Line.Rise, Target, InDeltaTime, 12.f);
		if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(Line.Text->Slot))
		{
			TextSlot->SetPosition(FVector2D(0.f, -Line.Rise));
		}
		const float Pop = FMath::Clamp(Line.Age / PopSeconds, 0.f, 1.f);
		Line.Text->SetRenderScale(FVector2D(FMath::Lerp(PopScale, 1.f, FMath::InterpEaseOut(0.f, 1.f, Pop, 2.f))));
		Line.Text->SetRenderOpacity(1.f - FMath::Clamp((Line.Age - FadeStart) / (LifeSeconds - FadeStart), 0.f, 1.f));
	}
}
