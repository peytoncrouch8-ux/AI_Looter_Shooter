#include "Scenes/TransitionScreenSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "UI/Style/LooterUIStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** The title card: the kit's display type, about 72 px tall at 1080p and widely spaced. */
	constexpr int32 TitleSize = 72;
	constexpr int32 TitleSpacing = 560;

	/** How far below its place the title starts its rise (Slate units: pixels at 1080p). */
	constexpr float TitleRiseDistance = 56.f;

	/** The accent line under the title at full width, its thickness, and its gap below the letters. */
	constexpr float LineWidth = 420.f;
	constexpr float LineHeight = 3.f;
	constexpr float LineGap = 14.f;
}

UTransitionScreenSubsystem* UTransitionScreenSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UTransitionScreenSubsystem>() : nullptr;
}

void UTransitionScreenSubsystem::Deinitialize()
{
	RemoveOverlay();
	if (TickHandle.IsValid())
	{
		FTSTicker::RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	Super::Deinitialize();
}

FText UTransitionScreenSubsystem::GameTitle()
{
	return NSLOCTEXT("LooterScenes", "GameTitle", "REVENANT");
}

// ---------------------------------------------------------------------------
// White and reveal
// ---------------------------------------------------------------------------

void UTransitionScreenSubsystem::HoldWhite()
{
	if (!Screen.IsHeld())
	{
		UE_LOG(LogLooter, Log, TEXT("Transition: the white holds."));
	}
	Screen.Hold();
	Show();
}

bool UTransitionScreenSubsystem::IsHeld() const
{
	return Screen.IsHeld();
}

void UTransitionScreenSubsystem::Reveal(const FText& Title)
{
	if (!Screen.IsShowing() && Title.IsEmpty())
	{
		return;
	}
	CardTitle = Title;
	Screen.Reveal(!Title.IsEmpty());
	if (Title.IsEmpty())
	{
		UE_LOG(LogLooter, Log, TEXT("Transition: revealed."));
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("Transition: revealed, %s rising through the white."), *Title.ToString());
	}
	Show();
}

void UTransitionScreenSubsystem::SetWhite(float White)
{
	Screen.Drive(White);
	if (Screen.IsShowing())
	{
		Show();
	}
	else
	{
		RemoveOverlay();
	}
}

void UTransitionScreenSubsystem::FadeToWhite(float Seconds)
{
	Screen.FadeIn(Seconds);
	Show();
}

void UTransitionScreenSubsystem::Clear()
{
	Screen.Clear();
	RemoveOverlay();
}

// ---------------------------------------------------------------------------
// The overlay
// ---------------------------------------------------------------------------

bool UTransitionScreenSubsystem::HandleTick(float DeltaSeconds)
{
	if (Screen.Advance(DeltaSeconds))
	{
		UE_LOG(LogLooter, Warning, TEXT("Transition: nothing revealed the white in %.0f s, so it thins by itself (the level arrived at should call Reveal)."),
			FTransitionScreen::HoldTimeout);
	}
	if (!Screen.IsShowing())
	{
		RemoveOverlay();
		// Returning false lets the ticker go; the handle goes with it.
		TickHandle.Reset();
		return false;
	}
	Refresh();
	return true;
}

void UTransitionScreenSubsystem::Show()
{
	// Real time, through pauses and level loads (a load's long frame counts as one ordinary frame: FTransitionScreen).
	if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UTransitionScreenSubsystem::HandleTick));
	}
	const UGameInstance* GameInstance = GetGameInstance();
	UGameViewportClient* Viewport = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;
	if (Viewport && ShownOn.Get() != Viewport)
	{
		RemoveOverlay();
		BuildOverlay();
		Viewport->AddViewportWidgetContent(Overlay.ToSharedRef(), ViewportZOrder);
		ShownOn = Viewport;
	}
	Refresh();
}

void UTransitionScreenSubsystem::RemoveOverlay()
{
	UGameViewportClient* Viewport = ShownOn.Get();
	if (Viewport && Overlay.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
	}
	ShownOn.Reset();
}

void UTransitionScreenSubsystem::BuildOverlay()
{
	if (Overlay.IsValid())
	{
		return;
	}
	// Plain white, tinted by each image: the cloud's white and its fade, the accent line.
	WhiteBrush = FSlateColorBrush(FLinearColor::White);
	LineBrush = FSlateColorBrush(FLinearColor::White);

	// The title is ink on the cloud's white, outlined as floating text with no panel behind it (the gameplay HUD's rule), over
	// a slim accent line that draws out from its middle.
	Overlay = SNew(SOverlay)
	.Visibility(EVisibility::HitTestInvisible)
	+ SOverlay::Slot()
	[
		SAssignNew(WhiteImage, SImage)
		.Image(&WhiteBrush)
	]
	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SAssignNew(TitleBox, SVerticalBox)
		.Visibility(EVisibility::Collapsed)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SAssignNew(TitleText, STextBlock)
			.Font(LooterUI::DisplayFont(TitleSize, TitleSpacing))
			.ColorAndOpacity(FSlateColor(LooterUI::Color::IconInk()))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.f, LineGap, 0.f, 0.f))
		[
			SAssignNew(LineBox, SBox)
			.WidthOverride(0.f)
			.HeightOverride(LineHeight)
			[
				SNew(SImage)
				.Image(&LineBrush)
				.ColorAndOpacity(FSlateColor(LooterUI::Color::Accent()))
			]
		]
	];
}

void UTransitionScreenSubsystem::Refresh()
{
	if (!Overlay.IsValid())
	{
		return;
	}
	FLinearColor Cloud = LooterUI::Color::Cloud();
	Cloud.A = Screen.GetWhite();
	WhiteImage->SetColorAndOpacity(Cloud);

	const float TitleOpacity = Screen.GetTitleOpacity();
	TitleBox->SetVisibility(TitleOpacity > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	if (TitleOpacity > 0.f)
	{
		if (!TitleText->GetText().EqualTo(CardTitle))
		{
			TitleText->SetText(CardTitle);
		}
		TitleBox->SetRenderOpacity(TitleOpacity);
		const FVector2D Offset(0.0, TitleRiseDistance * Screen.GetTitleRise());
		TitleBox->SetRenderTransform(TOptional<FSlateRenderTransform>(FSlateRenderTransform(Offset)));
		LineBox->SetWidthOverride(FOptionalSize(LineWidth * Screen.GetTitleLine()));
	}
}
