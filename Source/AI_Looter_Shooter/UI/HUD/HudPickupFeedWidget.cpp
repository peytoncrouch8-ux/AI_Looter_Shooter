#include "UI/HUD/HudPickupFeedWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootFanfareSubsystem.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr int32 MaxLines = 6;
	/** Smaller than the corner's big numbers: beside the crosshair it should read at a glance, not crowd the aim. */
	constexpr int32 FontSize = 17;
	/** How fast the whole stack drifts up (pixels per second). */
	constexpr float DriftSpeed = 16.f;
	/** A new line pops in slightly large and settles over PopSeconds. */
	constexpr float PopSeconds = 0.18f;
	constexpr float PopScale = 1.3f;
	/** Fully visible until FadeStart seconds, gone at LifeSeconds: brief, since it sits by the crosshair. */
	constexpr float FadeStart = 1.4f;
	constexpr float LifeSeconds = 2.2f;

	/**
	 * A rare drop's line: bigger, popping in harder and white-hot for its first moment, then shimmering toward white in
	 * its rarity's colour, and staying up a second longer than a pickup's.
	 */
	constexpr int32 FlashFontSize = 23;
	constexpr float FlashPopSeconds = 0.26f;
	constexpr float FlashPopScale = 1.9f;
	constexpr float FlashHotSeconds = 0.14f;
	constexpr float FlashFadeStart = 2.4f;
	constexpr float FlashLifeSeconds = 3.2f;
	/** The shimmer: how fast it beats, and how far toward white it goes. */
	constexpr float ShimmerSpeed = 9.f;
	constexpr float ShimmerDepth = 0.35f;

	/**
	 * The engine's heaviest UI typeface (Roboto Black, shipped with every build), white, in a thick round-jointed black
	 * outline: chunky, soft-edged "sticker" lettering that reads over sky, grass or rock.
	 */
	FSlateFontInfo PickupFont(int32 Size)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Black"), Size);
		Font.OutlineSettings.OutlineSize = Size > FontSize ? 3 : 2;
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
		const FSlateFontInfo Font = PickupFont(FontSize);
		for (int32 Index = 0; Index < MaxLines; ++Index)
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Text->SetFont(Font);
			Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Text->SetShadowOffset(FVector2D(0.f, 2.f));
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
	if (ULootFanfareSubsystem* Fanfare = BoundFanfare.Get())
	{
		Fanfare->OnAnnounced.Remove(FanfareHandle);
	}
	BoundFanfare.Reset();
	Super::NativeDestruct();
}

void UHudPickupFeedWidget::BindToPawn()
{
	// The level's loot fanfare, once: its rare drops' names flash here.
	if (!BoundFanfare.IsValid())
	{
		if (ULootFanfareSubsystem* Fanfare = GetWorld() ? GetWorld()->GetSubsystem<ULootFanfareSubsystem>() : nullptr)
		{
			FanfareHandle = Fanfare->OnAnnounced.AddUObject(this, &UHudPickupFeedWidget::HandleLootAnnounced);
			BoundFanfare = Fanfare;
		}
	}

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

void UHudPickupFeedWidget::HandleLootAnnounced(const AWeaponBase* Weapon)
{
	// Its name in its rarity's colour, as every gun's name is shown (no rarity word: the user's call).
	if (Weapon)
	{
		const FWeaponInstanceData& Instance = Weapon->GetInstance();
		AddFlashLine(LooterWeaponText::Name(Instance).ToUpper(), LooterWeaponText::Color(Instance));
	}
}

UHudPickupFeedWidget::FLine* UHudPickupFeedWidget::TakeLine(const FString& Text)
{
	if (TextPool.IsEmpty())
	{
		return nullptr;
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
	return &Line;
}

void UHudPickupFeedWidget::AddLine(const FString& Text)
{
	if (FLine* Line = TakeLine(Text))
	{
		// The pooled text may last have been a flash line: back to a pickup's look.
		Line->Text->SetFont(PickupFont(FontSize));
		Line->Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}

void UHudPickupFeedWidget::AddFlashLine(const FString& Text, const FLinearColor& Color)
{
	if (FLine* Line = TakeLine(Text))
	{
		Line->bFlash = true;
		Line->Color = Color;
		Line->Text->SetFont(PickupFont(FlashFontSize));
		Line->Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}

void UHudPickupFeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	BindToPawn();

	for (int32 Index = Lines.Num() - 1; Index >= 0; --Index)
	{
		FLine& Line = Lines[Index];
		Line.Age += InDeltaTime;
		if (Line.Age >= (Line.bFlash ? FlashLifeSeconds : LifeSeconds))
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
		const float Pop = FMath::Clamp(Line.Age / (Line.bFlash ? FlashPopSeconds : PopSeconds), 0.f, 1.f);
		const float From = Line.bFlash ? FlashPopScale : PopScale;
		Line.Text->SetRenderScale(FVector2D(FMath::Lerp(From, 1.f, FMath::InterpEaseOut(0.f, 1.f, Pop, Line.bFlash ? 3.f : 2.f))));
		const float FadeFrom = Line.bFlash ? FlashFadeStart : FadeStart;
		const float FadeTo = Line.bFlash ? FlashLifeSeconds : LifeSeconds;
		Line.Text->SetRenderOpacity(1.f - FMath::Clamp((Line.Age - FadeFrom) / (FadeTo - FadeFrom), 0.f, 1.f));
		if (Line.bFlash)
		{
			// White-hot as it lands, cooling into its colour, which then shimmers toward white.
			const float Hot = 1.f - FMath::Clamp((Line.Age - FlashHotSeconds) / FlashHotSeconds, 0.f, 1.f);
			const float Shimmer = ShimmerDepth * (0.5f + 0.5f * FMath::Sin(Line.Age * ShimmerSpeed));
			Line.Text->SetColorAndOpacity(FSlateColor(FMath::Lerp(Line.Color, FLinearColor::White, FMath::Max(Hot, Shimmer))));
		}
	}
}
