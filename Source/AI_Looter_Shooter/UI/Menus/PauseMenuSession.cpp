// UPauseMenuWidget's session tag, bottom-left (E2b, mockups/e.js "C's session tag"): C's tag on its side with the session's
// number in its gem, SESSION n OF 3 and the serial, how long it's been played and when it was last saved. Played without a
// session (Looter.PlayLevel, a level from the editor) the tag stays and says NO SESSION, Not saved (HUB's call: the card
// keeps the corner balanced, and it tells the player the truth about saving).

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuCardKit.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Style/LooterUIStyle.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Misc/DateTime.h"

using namespace PauseMenuLayout;
using namespace PauseMenuCardKit;
using namespace LooterUI;

namespace
{
	/** Its gem: 46 px, 14 out past the tag's left; its words 52 in, 16 down, clear of the point by 56. */
	constexpr float SessionGem = 46.f;
	constexpr float SessionGemX = -14.f;
	constexpr float SessionPadLeft = 52.f;
	constexpr float SessionPadRight = 56.f;
	constexpr float SessionPadTop = 16.f;
}

void UPauseMenuWidget::MakeSessionCard()
{
	UCanvasPanel* Card = MakeGroup(FVector2D(SessionX, SessionY), FVector2D(SessionWidth, SessionHeight));
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	SessionGroup = Card;
	Put(Card, MakeShape(TEXT("PauseSessionTag"), PauseMenuArt::Tag(SessionWidth, SessionHeight, PauseMenuArt::ELook::Plain, PauseMenuArt::EPart::Fill),
		PauseMenuArt::Tag(SessionWidth, SessionHeight, PauseMenuArt::ELook::Plain, PauseMenuArt::EPart::Lines), FVector2D(SessionWidth, SessionHeight)),
		FVector2D::ZeroVector, FVector2D(SessionWidth, SessionHeight));

	// Its gem on the notch, the session's number in it.
	const FVector2D GemAt(SessionGemX, SessionHeight * 0.5f - SessionGem * 0.5f);
	Put(Card, MakePicture(TEXT("PauseSessionGem"), PauseMenuArt::GemLook(SessionGem, PauseMenuArt::ELook::Plain)), GemAt, FVector2D::ZeroVector);
	UTextBlock* GemText = Words(WidgetTree, 19, true, Color::SlateText());
	GemText->SetJustification(ETextJustify::Center);
	Put(Card, MakeBand(GemText, SessionGem, SessionGem, HAlign_Center), GemAt, FVector2D::ZeroVector);
	Session.Gem = GemText;

	// SESSION n OF 3 and the serial; Played ... Saved ... under them.
	const float Inner = SessionWidth - SessionPadLeft - SessionPadRight;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Title = Words(WidgetTree, 24, true, Color::SlateText(), 140);
	UTextBlock* Of = Words(WidgetTree, 24, false, Color::SlateLabel(), 140);
	AddTo(Top, Title);
	AddTo(Top, Of);
	AddFill(WidgetTree, Top);
	UTextBlock* Serial = Words(WidgetTree, 15, false, Color::SlateIcon(), 300);
	AddTo(Top, Serial, 16.f);
	Column->AddChildToVerticalBox(Top);
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* PlayedLabel = Words(WidgetTree, 20, false, Color::SlateLabel());
	UTextBlock* Played = Words(WidgetTree, 20, true, Color::SlateText());
	UTextBlock* SavedLabel = Words(WidgetTree, 20, false, Color::SlateLabel());
	UTextBlock* Saved = Words(WidgetTree, 20, true, Color::SlateText());
	AddTo(Bottom, PlayedLabel);
	AddTo(Bottom, Played);
	AddTo(Bottom, SavedLabel, 26.f);
	AddTo(Bottom, Saved);
	Column->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	Put(Card, Column, FVector2D(SessionPadLeft, SessionPadTop), FVector2D(Inner, SessionHeight - SessionPadTop));
	Session.Title = Title;
	Session.Of = Of;
	Session.Serial = Serial;
	Session.PlayedLabel = PlayedLabel;
	Session.Played = Played;
	Session.SavedLabel = SavedLabel;
	Session.Saved = Saved;
}

void UPauseMenuWidget::RefreshSession()
{
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const bool bSession = Sessions && Sessions->IsPlayingSession();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int32 Level = Progression ? Progression->GetLevel() : 1;
	const int32 SessionNumber = bSession ? Sessions->GetActiveSession() + 1 : 0;
	// The pause plate's serial, kept from the menu before (the session and the level).
	SetWords(Session.Serial, FString::Printf(TEXT("SER. RV-%04d-%02d"), SessionNumber, FMath::Clamp(Level, 0, 99)));
	if (bSession)
	{
		SetWords(Session.Gem, FString::FromInt(SessionNumber));
		SetWords(Session.Title, FString::Printf(TEXT("SESSION %d"), SessionNumber));
		SetWords(Session.Of, FString::Printf(TEXT(" OF %d"), USessionSubsystem::MaxSessions));
		SetWords(Session.PlayedLabel, TEXT("Played "));
		SetWords(Session.Played, USessionSubsystem::FormatPlayTime(Sessions->GetPlayedSecondsNow(GetWorld())));
		const FSessionSummary Summary = Sessions->GetSummary(Sessions->GetActiveSession());
		SetWords(Session.SavedLabel, TEXT("Saved "));
		SetWords(Session.Saved, Summary.bExists ? USessionSubsystem::FormatSavedTime(Summary.Saved, FDateTime::Now()) : FString(TEXT("Not yet")));
	}
	else
	{
		SetWords(Session.Gem, TEXT("-"));
		SetWords(Session.Title, TEXT("NO SESSION"));
		SetWords(Session.Of, FString());
		SetWords(Session.PlayedLabel, TEXT("Not saved"));
		SetWords(Session.Played, FString());
		SetWords(Session.SavedLabel, FString());
		SetWords(Session.Saved, FString());
	}
}

FString UPauseMenuWidget::GetSessionText() const
{
	auto Text = [](const TWeakObjectPtr<UTextBlock>& Block) { return Block.IsValid() ? Block->GetText().ToString() : FString(); };
	return FString::Printf(TEXT("%s%s | %s%s %s%s"), *Text(Session.Title), *Text(Session.Of), *Text(Session.PlayedLabel), *Text(Session.Played),
		*Text(Session.SavedLabel), *Text(Session.Saved)).TrimEnd();
}
