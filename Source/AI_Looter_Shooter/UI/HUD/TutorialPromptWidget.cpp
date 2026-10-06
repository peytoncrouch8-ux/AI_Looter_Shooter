#include "UI/HUD/TutorialPromptWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "Scenes/SceneSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** Seconds to fade fully in or out. */
	constexpr float FadeSeconds = 0.35f;
	/** Where it sits: centered, a little under the top edge (clear of the minimap on the right). */
	constexpr float TopShare = 0.12f;
	constexpr float WrapWidth = 820.f;
}

TSharedRef<SWidget> UTutorialPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		HeaderText = MakeFloatingText(WidgetTree, 12, Color::Accent(), 220, ETextJustify::Center);
		BodyText = MakeFloatingText(WidgetTree, 20, Color::Text(), 0, ETextJustify::Center);
		BodyText->SetAutoWrapText(true);
		BodyText->SetWrapTextAt(WrapWidth);
		Box->AddChildToVerticalBox(HeaderText)->SetHorizontalAlignment(HAlign_Center);
		UVerticalBoxSlot* BodySlot = Box->AddChildToVerticalBox(BodyText);
		BodySlot->SetHorizontalAlignment(HAlign_Center);
		BodySlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

		UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(Box);
		BoxSlot->SetAnchors(FAnchors(0.5f, TopShare));
		BoxSlot->SetAlignment(FVector2D(0.5f, 0.f));
		BoxSlot->SetAutoSize(true);
		Box->SetRenderOpacity(0.f);
		Box->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void UTutorialPromptWidget::ShowStep(int32 Index, int32 Count, const FString& Text)
{
	DoneTimer = 0.f;
	Present(FString::Printf(TEXT("TUTORIAL  %d / %d"), Index + 1, Count), Text);
}

void UTutorialPromptWidget::ShowDone(const FString& Text, float Seconds)
{
	Present(TEXT("TUTORIAL COMPLETE"), Text);
	DoneTimer = Seconds;
}

void UTutorialPromptWidget::HideNow()
{
	bWanted = false;
	bPending = false;
	DoneTimer = 0.f;
}

void UTutorialPromptWidget::SetSuppressed(bool bInSuppressed)
{
	bSuppressed = bInSuppressed;
}

void UTutorialPromptWidget::Present(const FString& Header, const FString& Text)
{
	// Fade the old line out first; the new one comes in once it's gone (see NativeTick).
	PendingHeader = Header;
	PendingText = Text;
	bPending = true;
	bWanted = true;
}

void UTutorialPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Box)
	{
		return;
	}

	// A menu (the inventory, the pause menu) covers the game: the prompt steps aside, and the "complete" line waits
	// with its time held, so finishing the tutorial by opening the inventory shows it once the inventory closes rather
	// than over the stand-in.
	const bool bCovered = IsCovered();
	if (DoneTimer > 0.f && !bCovered)
	{
		DoneTimer -= InDeltaTime;
		if (DoneTimer <= 0.f)
		{
			bWanted = false;
		}
	}

	// Swap in a pending text once the old one has faded out (or right away if nothing was showing).
	if (bPending && Opacity <= 0.f)
	{
		HeaderText->SetText(FText::FromString(PendingHeader));
		BodyText->SetText(FText::FromString(PendingText));
		bPending = false;
	}

	const bool bShow = bWanted && !bSuppressed && !bPending && !bCovered;
	const float Target = bShow ? 1.f : 0.f;
	if (Opacity == Target)
	{
		return;
	}
	Opacity = FMath::Clamp(Opacity + (bShow ? 1.f : -1.f) * InDeltaTime / FadeSeconds, 0.f, 1.f);
	Box->SetRenderOpacity(Opacity);
	Box->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

bool UTutorialPromptWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	// A scene that holds the player (the skiff ride) puts the gameplay HUD away, and the prompt with it.
	if (USceneSubsystem::HidesGameplayHUD(this))
	{
		return true;
	}
	// The pause menu pauses the game (and with it the tutorial's checks).
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}
