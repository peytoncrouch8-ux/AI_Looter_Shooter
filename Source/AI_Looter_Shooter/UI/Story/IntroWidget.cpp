#include "UI/Story/IntroWidget.h"
#include "Scenes/SceneSkipPromptWidget.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/NewGameIntro.h"
#include "UI/Style/LooterUIStyle.h"
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
#include "Input/Events.h"
#include "InputCoreTypes.h"

using namespace LooterUI;

namespace
{
	// Sizes are mockup px used as Slate sizes, as the HUD's are (the user's call: in-game text a third bigger).

	/** The card's name in the kit's display type (REVENANT's, a size down: it isn't the game's title). */
	constexpr int32 CardTitleSize = 56;
	constexpr int32 CardSubSize = 15;
	constexpr int32 CardSubSpacing = 420;
	/** The rule under a card's name: two cyan hairlines either side of an orange tick. */
	constexpr float RuleLength = 140.f;
	constexpr float RuleThickness = 1.5f;
	constexpr float TickLength = 22.f;
	constexpr float TickThickness = 3.f;
	constexpr float RuleGap = 10.f;
	/** The card sits this far above the middle: the eye's resting line in a wide frame. */
	constexpr float CardLift = 90.f;
	/** A card drifts up this far as it fades in: settling, not popping. */
	constexpr float CardRise = 10.f;

	/** Narration: big enough to read from a couch, wrapped short so a line is read at a glance. */
	constexpr int32 LineSize = 24;
	constexpr int32 LineSpacing = 40;
	constexpr float LineWrap = 1100.f;
	/** The line sits this far above the bottom bar. */
	constexpr float LineAboveBar = 30.f;

	/** A clear fades what shows out this fast. */
	constexpr float ClearSeconds = 0.25f;

	bool IsHoldKey(const FKey& Key)
	{
		return Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom;
	}

	FString HoldKeyLabel(const FKey& Key)
	{
		if (Key == EKeys::Enter)
		{
			return TEXT("ENTER");
		}
		if (Key == EKeys::SpaceBar)
		{
			return TEXT("SPACE");
		}
		if (Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			return TEXT("A");
		}
		return TEXT("E");
	}

	UWidget* MakeRule(UWidgetTree* Tree)
	{
		UHorizontalBox* Rule = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		auto AddPiece = [Tree, Rule](const FLinearColor& Tint, float Width, float Height, float GapAfter)
		{
			UHorizontalBoxSlot* Slot = Rule->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(Tint)), Width, Height));
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(0.f, 0.f, GapAfter, 0.f));
		};
		AddPiece(Color::Hairline(), RuleLength, RuleThickness, RuleGap);
		AddPiece(Color::Accent(), TickLength, TickThickness, RuleGap);
		AddPiece(Color::Hairline(), RuleLength, RuleThickness, 0.f);
		return Rule;
	}
}

// ---------------------------------------------------------------------------
// The fades and the skip keys
// ---------------------------------------------------------------------------

float IntroFade::Alpha(float Elapsed, float Seconds, float FadeIn, float FadeOut)
{
	if (Seconds <= 0.f || Elapsed < 0.f || Elapsed >= Seconds)
	{
		return 0.f;
	}
	const float In = FadeIn > 0.f ? Elapsed / FadeIn : 1.f;
	const float Out = FadeOut > 0.f ? (Seconds - Elapsed) / FadeOut : 1.f;
	return FMath::Clamp(FMath::Min(In, Out), 0.f, 1.f);
}

bool FIntroSkipKeys::CanSkip() const
{
	return Age >= USceneSubsystem::SkipKeysAfter;
}

void FIntroSkipKeys::HoldDown(const FString& Label)
{
	if (!CanSkip())
	{
		return;
	}
	bDown = true;
	Held = 0.f;
	KeyLabel = Label;
	bEscapePrompt = false;
	PromptLeft = USceneSubsystem::SkipPromptSeconds;
}

void FIntroSkipKeys::HoldUp()
{
	bDown = false;
	Held = 0.f;
}

void FIntroSkipKeys::Escape()
{
	if (!CanSkip())
	{
		return;
	}
	if (EscapeLeft > 0.f)
	{
		bSkip = true;
		return;
	}
	EscapeLeft = USceneSubsystem::SkipPromptSeconds;
	PromptLeft = USceneSubsystem::SkipPromptSeconds;
	bEscapePrompt = true;
}

void FIntroSkipKeys::Tick(float DeltaSeconds)
{
	Age += DeltaSeconds;
	EscapeLeft = FMath::Max(0.f, EscapeLeft - DeltaSeconds);
	if (bDown)
	{
		Held += DeltaSeconds;
		PromptLeft = USceneSubsystem::SkipPromptSeconds;
		if (Held >= USceneSubsystem::SkipHoldSeconds)
		{
			bSkip = true;
		}
		return;
	}
	PromptLeft = FMath::Max(0.f, PromptLeft - DeltaSeconds);
}

float FIntroSkipKeys::GetHold() const
{
	return bDown ? FMath::Clamp(Held / USceneSubsystem::SkipHoldSeconds, 0.f, 1.f) : 0.f;
}

FString FIntroSkipKeys::GetLead() const
{
	return bEscapePrompt ? TEXT("PRESS") : TEXT("HOLD");
}

FString FIntroSkipKeys::GetKey() const
{
	return bEscapePrompt ? TEXT("ESC") : KeyLabel;
}

FString FIntroSkipKeys::GetWords() const
{
	return bEscapePrompt ? TEXT("AGAIN TO SKIP") : TEXT("TO SKIP");
}

// ---------------------------------------------------------------------------
// The widget
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UIntroWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));

		// The letterbox: the only backgrounds here (they fade with the UI transparency); 0 tall until they come in.
		auto MakeBar = [this, Root](EVerticalAlignment Edge)
		{
			UImage* Fill = MakeImage(WidgetTree, RectBrush(Color::Ink()));
			MarkBackground(Fill);
			USizeBox* Bar = MakeSized(WidgetTree, Fill, 0.f, 0.f);
			Bar->SetHeightOverride(0.f);
			UOverlaySlot* Slot = Root->AddChildToOverlay(Bar);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(Edge);
			return Bar;
		};
		TopBar = MakeBar(VAlign_Top);
		BottomBar = MakeBar(VAlign_Bottom);

		// The card: a place's name over the rule, a dim line under it.
		UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Card"));
		CardTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CardTitleText->SetFont(DisplayFont(CardTitleSize));
		CardTitleText->SetColorAndOpacity(FSlateColor(Color::Text()));
		CardTitleText->SetJustification(ETextJustify::Center);
		CardBox->AddChildToVerticalBox(CardTitleText)->SetHorizontalAlignment(HAlign_Center);
		UVerticalBoxSlot* RuleSlot = CardBox->AddChildToVerticalBox(MakeRule(WidgetTree));
		RuleSlot->SetHorizontalAlignment(HAlign_Center);
		RuleSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));
		CardSubText = MakeFloatingText(WidgetTree, CardSubSize, Color::TextDim(), CardSubSpacing, ETextJustify::Center);
		CardBox->AddChildToVerticalBox(CardSubText)->SetHorizontalAlignment(HAlign_Center);
		CardBox->SetRenderOpacity(0.f);
		CardBox->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* CardSlot = Root->AddChildToOverlay(CardBox);
		CardSlot->SetHorizontalAlignment(HAlign_Center);
		CardSlot->SetVerticalAlignment(VAlign_Center);
		CardSlot->SetPadding(FMargin(0.f, 0.f, 0.f, CardLift));
		Card = CardBox;

		// The narration, low and centred, riding above the bottom bar.
		LineText = MakeFloatingText(WidgetTree, LineSize, Color::Text(), LineSpacing, ETextJustify::Center);
		LineText->SetAutoWrapText(true);
		LineText->SetWrapTextAt(LineWrap);
		LineText->SetRenderOpacity(0.f);
		LineText->SetVisibility(ESlateVisibility::Collapsed);
		LineSlot = Root->AddChildToOverlay(LineText);
		LineSlot->SetHorizontalAlignment(HAlign_Center);
		LineSlot->SetVerticalAlignment(VAlign_Bottom);
		LineSlot->SetPadding(FMargin(0.f, 0.f, 0.f, LineAboveBar));

		// The scene's own skip prompt, bottom right (shown only while this takes the keys).
		SkipPrompt = WidgetTree->ConstructWidget<USceneSkipPromptWidget>(USceneSkipPromptWidget::StaticClass(), TEXT("SkipPrompt"));
		FillOverlaySlot(Root->AddChildToOverlay(SkipPrompt));

		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Super::RebuildWidget();
}

void UIntroWidget::ShowCard(const FText& Title, const FText& Sub, float Seconds, bool bAccentSub)
{
	if (!CardTitleText)
	{
		return;
	}
	CardTitleText->SetText(Title);
	CardSubText->SetText(Sub);
	CardSubText->SetVisibility(Sub.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	StyleFloatingText(CardSubText, CardSubSize, bAccentSub ? Color::Accent() : Color::TextDim(), CardSubSpacing);
	CardElapsed = 0.f;
	CardSeconds = Seconds;
	ClearLeft = -1.f;
	// One thing on screen at a time: a card replaces a line still fading.
	LineSeconds = 0.f;
}

void UIntroWidget::ShowLine(const FText& Words, float Seconds)
{
	if (!LineText)
	{
		return;
	}
	LineText->SetText(Words);
	LineElapsed = 0.f;
	LineSeconds = Seconds;
	ClearLeft = -1.f;
	CardSeconds = 0.f;
}

void UIntroWidget::ClearWords()
{
	if (ClearLeft < 0.f)
	{
		ClearLeft = ClearSeconds;
	}
}

void UIntroWidget::SetLetterbox(bool bShown, bool bInstant)
{
	LetterboxTarget = bShown ? 1.f : 0.f;
	if (bInstant)
	{
		Letterbox = LetterboxTarget;
	}
}

void UIntroWidget::SetTakesKeys(bool bTakes)
{
	bTakesKeys = bTakes;
	SetIsFocusable(bTakes);
	// Taking the keys, it covers the screen for real: clicks don't reach the menu under it either.
	SetVisibility(bTakes ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
}

bool UIntroWidget::ConsumeSkip()
{
	if (!bSkipTaken && Skip.WantsSkip())
	{
		bSkipTaken = true;
		return true;
	}
	return false;
}

void UIntroWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateLetterbox(MyGeometry, InDeltaTime);
	UpdateWords(InDeltaTime);
	if (bTakesKeys)
	{
		Skip.Tick(InDeltaTime);
	}
	if (SkipPrompt)
	{
		SkipPrompt->Update(bTakesKeys && Skip.IsPromptShown(), Skip.GetLead(), Skip.GetKey(), Skip.GetWords(), Skip.GetHold(), InDeltaTime);
	}
}

void UIntroWidget::UpdateLetterbox(const FGeometry& MyGeometry, float DeltaSeconds)
{
	if (Letterbox != LetterboxTarget)
	{
		const float Step = DeltaSeconds / LetterboxSeconds;
		Letterbox = LetterboxTarget > Letterbox ? FMath::Min(LetterboxTarget, Letterbox + Step) : FMath::Max(LetterboxTarget, Letterbox - Step);
	}
	const float Height = FMath::RoundToFloat(MyGeometry.GetLocalSize().Y * NewGameIntro::LetterboxFraction() * NewGameIntro::Ease(Letterbox));
	if (Height != LastBarHeight && TopBar && BottomBar)
	{
		LastBarHeight = Height;
		TopBar->SetHeightOverride(Height);
		BottomBar->SetHeightOverride(Height);
		TopBar->SetVisibility(Height > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		BottomBar->SetVisibility(Height > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (LineSlot)
		{
			LineSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Height + LineAboveBar));
		}
	}
}

void UIntroWidget::UpdateWords(float DeltaSeconds)
{
	float Cleared = 1.f;
	if (ClearLeft >= 0.f)
	{
		ClearLeft = FMath::Max(0.f, ClearLeft - DeltaSeconds);
		Cleared = ClearLeft / ClearSeconds;
		if (ClearLeft <= 0.f)
		{
			CardSeconds = 0.f;
			LineSeconds = 0.f;
			ClearLeft = -1.f;
		}
	}
	CardElapsed += DeltaSeconds;
	LineElapsed += DeltaSeconds;
	if (Card)
	{
		const float A = IntroFade::Alpha(CardElapsed, CardSeconds, CardFade, CardFade) * Cleared;
		Card->SetRenderOpacity(A);
		Card->SetRenderTranslation(FVector2D(0.f, (1.f - NewGameIntro::Ease(A)) * CardRise));
		Card->SetVisibility(A > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (LineText)
	{
		const float A = IntroFade::Alpha(LineElapsed, LineSeconds, LineFade, LineFade) * Cleared;
		LineText->SetRenderOpacity(A);
		LineText->SetVisibility(A > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

FReply UIntroWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bTakesKeys)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (!InKeyEvent.IsRepeat())
	{
		if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right)
		{
			Skip.Escape();
		}
		else if (IsHoldKey(Key))
		{
			Skip.HoldDown(HoldKeyLabel(Key));
		}
	}
	// Every key stops here: the menu under the intro hears nothing.
	return FReply::Handled();
}

FReply UIntroWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bTakesKeys)
	{
		return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
	}
	if (IsHoldKey(InKeyEvent.GetKey()))
	{
		Skip.HoldUp();
	}
	return FReply::Handled();
}

UIntroWidget::FShown UIntroWidget::GetShown() const
{
	FShown Shown;
	if (Card && CardTitleText)
	{
		Shown.CardTitle = CardTitleText->GetText().ToString();
		Shown.CardAlpha = Card->GetRenderOpacity();
	}
	if (LineText)
	{
		Shown.Line = LineText->GetText().ToString();
		Shown.LineAlpha = LineText->GetRenderOpacity();
	}
	Shown.Letterbox = Letterbox;
	return Shown;
}
