#include "UI/HUD/HudCaptionWidget.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** The name: small, letter-spaced capitals over the words, like the HUD's other labels. */
	constexpr int32 SpeakerSize = 13;
	constexpr int32 SpeakerSpacing = 150;
	/** The words: as big as the tutorial's instruction, so a line reads at a glance mid-fight. */
	constexpr int32 WordsSize = 20;
	/** Seconds to step aside for a menu, and to come back. */
	constexpr float PresenceFadeSeconds = 0.2f;
}

TSharedRef<SWidget> UHudCaptionWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		// No backing: outlined text that reads over sky, grass or rock, like the rest of the gameplay HUD.
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		SpeakerText = MakeFloatingText(WidgetTree, SpeakerSize, Color::Accent(), SpeakerSpacing, ETextJustify::Center);
		Box->AddChildToVerticalBox(SpeakerText)->SetHorizontalAlignment(HAlign_Center);
		WordsText = MakeFloatingText(WidgetTree, WordsSize, Color::Text(), 0, ETextJustify::Center);
		// A fixed wrap width only: auto wrap would wrap at whatever width the auto-sized slot settles on, which
		// collapses to a narrow column.
		WordsText->SetWrapTextAt(WrapWidth);
		UVerticalBoxSlot* WordsSlot = Box->AddChildToVerticalBox(WordsText);
		WordsSlot->SetHorizontalAlignment(HAlign_Center);
		WordsSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

		// Its bottom held in place over the experience bar: a line that wraps grows upward.
		UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(Box);
		BoxSlot->SetAnchors(FAnchors(0.5f, 1.f));
		BoxSlot->SetAlignment(FVector2D(0.5f, 1.f));
		BoxSlot->SetPosition(FVector2D(0.f, -BottomGap));
		BoxSlot->SetAutoSize(true);
		Box->SetRenderOpacity(0.f);
		Box->SetVisibility(ESlateVisibility::Collapsed);
		CaptionBox = Box;
		ShownSerial = 0;
		ShownOpacity = -1.f;
	}
	return Super::RebuildWidget();
}

void UHudCaptionWidget::NativeDestruct()
{
	// Gone (the level ending): nothing is left to hold the captions for.
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->SetHeld(false);
	}
	Super::NativeDestruct();
}

void UHudCaptionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UHudCaptionWidget::Advance(float DeltaSeconds)
{
	if (!CaptionBox)
	{
		return;
	}

	// Nobody can read them under a menu: the lines wait there instead of playing on unseen.
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	const bool bCovered = IsMenuOpen();
	if (Captions && Captions->IsHeld() != bCovered)
	{
		Captions->SetHeld(bCovered);
	}

	// The Subtitles setting goes to the captions, where whatever else draws a line's words (a creature's) reads it too. A
	// widget with no player to ask (a test) leaves the captions' switch as it was set.
	if (Captions)
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		if (Settings)
		{
			Captions->SetSubtitlesShown(Settings->AreSubtitlesShown());
		}
	}

	// Subtitles off: someone's line plays out in the queue all the same, and only its words go undrawn. Nothing is set while
	// it plays, so the switch turned on again mid-line puts that line up from where it is. A line with no speaker (a notice,
	// a remark, a time card) is no subtitle and shows either way.
	const FCaptionEntry* Words = Captions ? Captions->GetCurrentShown() : nullptr;
	if (Words && Words->Serial != ShownSerial)
	{
		ShownSerial = Words->Serial;
		ShowLine(Words->Line);
	}

	Presence = FMath::FInterpConstantTo(Presence, bCovered ? 0.f : 1.f, DeltaSeconds, 1.f / PresenceFadeSeconds);
	const float Opacity = Words ? Captions->GetAlpha() * Presence : 0.f;
	if (Opacity != ShownOpacity)
	{
		ShownOpacity = Opacity;
		CaptionBox->SetRenderOpacity(Opacity);
		CaptionBox->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

bool UHudCaptionWidget::AreWordsShown() const
{
	return CaptionBox && CaptionBox->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UHudCaptionWidget::GetShownWords() const
{
	return WordsText ? WordsText->GetText() : FText::GetEmpty();
}

void UHudCaptionWidget::ShowLine(const FStoryLine& Line)
{
	// A line with no speaker (a sign read aloud) shows its words alone.
	const bool bNamed = !Line.Speaker.IsEmpty();
	SpeakerText->SetText(bNamed ? Line.Speaker.ToUpper() : FText::GetEmpty());
	SpeakerText->SetVisibility(bNamed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	WordsText->SetText(Line.Text);
}

bool UHudCaptionWidget::IsMenuOpen() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	// Every menu but an offer card on its own: there the giver's words go on beside the card (2026-10-22).
	if (LooterHUD && LooterHUD->CoversCaptions())
	{
		return true;
	}
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}
