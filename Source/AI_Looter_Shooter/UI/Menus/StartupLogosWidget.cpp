#include "UI/Menus/StartupLogosWidget.h"
#include "UI/Menus/StartupLogosSettings.h"
#include "UI/Style/LooterUIStyle.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

using namespace LooterUI;

namespace
{
	TAutoConsoleVariable<int32> CVarLogos(TEXT("Looter.Startup.Logos"), 1,
		TEXT("1: the startup cards (studio, Unreal Engine) play before the main menu once a run. 0: never."));
	TAutoConsoleVariable<int32> CVarInEditor(TEXT("Looter.Startup.InEditor"), 0,
		TEXT("1: the startup cards play in the editor's Play too (off: every PIE start would sit through them)."));
	TAutoConsoleVariable<int32> CVarFirstLaunchSkips(TEXT("Looter.Startup.FirstLaunchSkips"), 0,
		TEXT("1: a key skips the startup cards even on the first launch (the user's rule: the first launch shows them all)."));

	/** The cards' stings (Art/Sounds/recipes/opening.py). */
	const FName CueStudio(TEXT("UI.Startup.Studio"));
	const FName CueEngine(TEXT("UI.Startup.Engine"));

	/** The user's GameUserSettings.ini: whether a run has seen the cards to their end. */
	const TCHAR* SeenSection = TEXT("Looter.Startup");
	const TCHAR* SeenKey = TEXT("LogosSeen");

	/** Once a run: a return to the menu (Save & Quit) goes straight to it. */
	bool GPlayedThisRun = false;

	/** The studio mark's size, and the soft glow behind it. */
	constexpr float StudioMarkSize = 128.f;
	constexpr float StudioGlowSize = 300.f;
	constexpr float StudioGlowOpacity = 0.22f;
	/** The studio card drifts in from this scale to 1 over its show: alive, never enough to notice as a zoom. */
	constexpr float StudioDriftFrom = 0.965f;

	/**
	 * The placeholder studio's mark, original and plain: a hexagon drawn in orange, a small orange diamond at its heart, a
	 * cyan horizon line running through it and out both sides. Easy to swap when the studio has its own.
	 */
	const FPaintedIcon& StudioMark()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(64.f, 64.f);
			const FVector2D Centre(32.f, 32.f);

			FPaintLayer Horizon;
			Horizon.Strokes.Add({ FVector2D(1.f, 32.f), FVector2D(17.f, 32.f) });
			Horizon.Strokes.Add({ FVector2D(47.f, 32.f), FVector2D(63.f, 32.f) });
			Horizon.StrokeWidth = 1.2f;
			Horizon.Color = Color::Hairline();
			Horizon.Opacity = 0.85f;
			Result.Layers.Add(Horizon);

			FPaintLayer Hexagon;
			TArray<FVector2D> Ring;
			for (int32 Corner = 0; Corner <= 6; ++Corner)
			{
				const float Angle = FMath::DegreesToRadians(-90.f + 60.f * (Corner % 6));
				Ring.Add(Centre + 21.f * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
			}
			Hexagon.Strokes.Add(Ring);
			Hexagon.StrokeWidth = 2.6f;
			Hexagon.Color = Color::Accent();
			Result.Layers.Add(Hexagon);

			FPaintLayer Heart;
			Heart.Fills.Add({ FVector2D(32.f, 23.f), FVector2D(39.f, 32.f), FVector2D(32.f, 41.f), FVector2D(25.f, 32.f) });
			Heart.Color = Color::AccentLight();
			Heart.GradientTo = Color::Accent();
			Heart.GradientStart = FVector2D(32.f, 23.f);
			Heart.GradientEnd = FVector2D(32.f, 41.f);
			Result.Layers.Add(Heart);
			return Result;
		}();
		return Picture;
	}

	/** Epic's logo, if it's been imported: looked for quietly, so its absence costs no warning. */
	UTexture2D* FindEngineLogo()
	{
		const FSoftObjectPath& Path = UStartupLogosSettings::Get().EngineLogo;
		if (Path.IsNull() || !FPackageName::DoesPackageExist(Path.GetLongPackageName()))
		{
			return nullptr;
		}
		return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, Path.ToString(), FStringView(), LOAD_NoWarn | LOAD_Quiet));
	}
}

// ---------------------------------------------------------------------------
// When they play
// ---------------------------------------------------------------------------

bool UStartupLogosWidget::ShouldPlay(const UWorld* World)
{
	if (GPlayedThisRun || CVarLogos.GetValueOnGameThread() == 0 || GIsAutomationTesting || FParse::Param(FCommandLine::Get(), TEXT("NoLogos")))
	{
		return false;
	}
	if (World && World->WorldType == EWorldType::PIE && CVarInEditor.GetValueOnGameThread() == 0)
	{
		return false;
	}
	const UStartupLogosSettings& Settings = UStartupLogosSettings::Get();
	return Settings.bShowStudioCard || Settings.bShowEngineCard;
}

bool UStartupLogosWidget::IsFirstLaunch()
{
	bool bSeen = false;
	return !(GConfig && GConfig->GetBool(SeenSection, SeenKey, bSeen, GGameUserSettingsIni) && bSeen);
}

void UStartupLogosWidget::MarkSeen()
{
	// Tests step the cards too, and must leave the player's settings as they were.
	if (GConfig && !GIsAutomationTesting && IsFirstLaunch())
	{
		GConfig->SetBool(SeenSection, SeenKey, true, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

// ---------------------------------------------------------------------------
// The cards
// ---------------------------------------------------------------------------

void UStartupLogosWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// It holds the keyboard while it shows, so any key reaches it.
	SetIsFocusable(true);
}

TSharedRef<SWidget> UStartupLogosWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The dark ground over everything, the menu's world loading under it. Not a panel's glass, so it doesn't follow
		// the UI transparency setting (as the cloud bank's white between places doesn't): faded, the logos would sit over
		// a half-loaded world.
		Cover = MakeImage(WidgetTree, RectBrush(Color::Track()));
		FillOverlaySlot(Root->AddChildToOverlay(Cover));

		const UStartupLogosSettings& Settings = UStartupLogosSettings::Get();
		auto AddCard = [this, Root](UWidget* Card, FName Cue)
		{
			UOverlaySlot* CardSlot = Root->AddChildToOverlay(Card);
			CardSlot->SetHorizontalAlignment(HAlign_Center);
			CardSlot->SetVerticalAlignment(VAlign_Center);
			Card->SetRenderOpacity(0.f);
			Card->SetVisibility(ESlateVisibility::HitTestInvisible);
			Cards.Add(Card);
			CardCues.Add(Cue);
		};
		if (Settings.bShowStudioCard)
		{
			StudioCard = Cards.Num();
			AddCard(MakeStudioCard(), CueStudio);
		}
		if (Settings.bShowEngineCard)
		{
			AddCard(MakeEngineCard(), CueEngine);
		}

		UTextBlock* Hint = MakeText(WidgetTree, TEXT("Press any key to skip"), 11, Color::TextDim(), true, 300);
		SkipHint = Hint;
		SkipHint->SetRenderOpacity(0.f);
		UOverlaySlot* HintSlot = Root->AddChildToOverlay(SkipHint);
		HintSlot->SetHorizontalAlignment(HAlign_Right);
		HintSlot->SetVerticalAlignment(VAlign_Bottom);
		HintSlot->SetPadding(FMargin(0.f, 0.f, 56.f, 44.f));
		Apply();
	}
	return Super::RebuildWidget();
}

UWidget* UStartupLogosWidget::MakeStudioCard()
{
	const UStartupLogosSettings& Settings = UStartupLogosSettings::Get();
	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The mark over a faint warm glow, laid out at the mark's size and drawn larger (it spreads without pushing the name).
	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Glow = MakeImage(WidgetTree, GlowBrush(FVector2D(StudioMarkSize, StudioMarkSize), Color::Accent().CopyWithNewOpacity(StudioGlowOpacity)));
	Glow->SetRenderScale(FVector2D(StudioGlowSize / StudioMarkSize, StudioGlowSize / StudioMarkSize));
	FillOverlaySlot(Mark->AddChildToOverlay(Glow));
	UOverlaySlot* MarkSlot = Mark->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("StudioMark"), StudioMark(), 4.f,
		FVector2D(StudioMarkSize, StudioMarkSize))));
	MarkSlot->SetHorizontalAlignment(HAlign_Center);
	MarkSlot->SetVerticalAlignment(VAlign_Center);
	Card->AddChildToVerticalBox(MakeSized(WidgetTree, Mark, StudioMarkSize, StudioMarkSize))->SetHorizontalAlignment(HAlign_Center);

	StudioNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StudioNameText->SetText(FText::FromString(Settings.StudioName.ToUpper()));
	StudioNameText->SetFont(DisplayFont(40, 600));
	StudioNameText->SetColorAndOpacity(FSlateColor(Color::Text()));
	StudioNameText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(StudioNameText);
	NameSlot->SetHorizontalAlignment(HAlign_Center);
	NameSlot->SetPadding(FMargin(0.f, 34.f, 0.f, 0.f));

	if (!Settings.StudioTagline.IsEmpty())
	{
		UVerticalBoxSlot* TagSlot = Card->AddChildToVerticalBox(MakeText(WidgetTree, Settings.StudioTagline, 13, Color::TextDim(), true, 600));
		TagSlot->SetHorizontalAlignment(HAlign_Center);
		TagSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
	Card->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	return Card;
}

UWidget* UStartupLogosWidget::MakeEngineCard()
{
	const UStartupLogosSettings& Settings = UStartupLogosSettings::Get();
	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTexture2D* Logo = FindEngineLogo();
	bEngineLogoShown = Logo != nullptr;

	if (!Logo || Settings.bEngineCaption)
	{
		Card->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Powered by"), 13, Color::TextDim(), true, 700))->SetHorizontalAlignment(HAlign_Center);
	}
	if (Logo)
	{
		// Epic's logo as it comes: its own shape, untinted, nothing over it.
		const float Height = Settings.EngineLogoHeight;
		const float Aspect = Logo->GetSizeY() > 0 ? float(Logo->GetSizeX()) / float(Logo->GetSizeY()) : 1.f;
		FSlateBrush Brush;
		Brush.SetResourceObject(Logo);
		Brush.ImageSize = FVector2D(Height * Aspect, Height);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		UVerticalBoxSlot* LogoSlot = Card->AddChildToVerticalBox(MakeImage(WidgetTree, Brush));
		LogoSlot->SetHorizontalAlignment(HAlign_Center);
		LogoSlot->SetPadding(FMargin(0.f, Settings.bEngineCaption ? 20.f : 0.f, 0.f, 0.f));
	}
	else
	{
		// Until the logo is imported: the plain words, in the kit's display type (no drawing of Epic's mark).
		UTextBlock* Words = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Words->SetText(FText::FromString(TEXT("UNREAL ENGINE")));
		Words->SetFont(DisplayFont(44, 420));
		Words->SetColorAndOpacity(FSlateColor(Color::Text()));
		UVerticalBoxSlot* WordsSlot = Card->AddChildToVerticalBox(Words);
		WordsSlot->SetHorizontalAlignment(HAlign_Center);
		WordsSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
	return Card;
}

void UStartupLogosWidget::Begin(bool bForceSkippable)
{
	GPlayedThisRun = GPlayedThisRun || !GIsAutomationTesting;
	TakeWidget();
	const bool bSkippable = bForceSkippable || !IsFirstLaunch() || CVarFirstLaunchSkips.GetValueOnGameThread() != 0;
	Rules.Start(Cards.Num(), bSkippable, FStartupLogoTiming::FromConsole());
	bBegun = true;
	bRevealSent = false;
	bFinishedSent = false;
	SetVisibility(ESlateVisibility::Visible);
	UE_LOG(LogLooter, Log, TEXT("Startup: %d cards (%s)."), Cards.Num(), bSkippable ? TEXT("skippable") : TEXT("first launch: shown in full"));
	Advance(0.f);
}

void UStartupLogosWidget::Advance(float DeltaSeconds)
{
	if (!bBegun)
	{
		return;
	}
	Rules.Tick(DeltaSeconds);
	const int32 Started = Rules.TakeStartedCard();
	if (CardCues.IsValidIndex(Started))
	{
		LooterSound::Play2D(this, CardCues[Started]);
	}
	Apply();
	if (Rules.IsRevealing() && !bRevealSent)
	{
		bRevealSent = true;
		// The menu under it takes the mouse from here on.
		SetVisibility(ESlateVisibility::HitTestInvisible);
		MarkSeen();
		OnRevealing.ExecuteIfBound();
	}
	if (Rules.IsDone() && !bFinishedSent)
	{
		bFinishedSent = true;
		OnFinished.ExecuteIfBound();
	}
}

void UStartupLogosWidget::Apply()
{
	const int32 Shown = Rules.GetCard();
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		if (UWidget* Card = Cards[Index])
		{
			Card->SetRenderOpacity(Index == Shown ? Rules.GetCardOpacity() : 0.f);
			if (Index == StudioCard)
			{
				const float Scale = FMath::Lerp(StudioDriftFrom, 1.f, FMath::InterpEaseOut(0.f, 1.f, Rules.GetCardProgress(), 2.f));
				Card->SetRenderScale(FVector2D(Scale, Scale));
			}
		}
	}
	if (Cover)
	{
		Cover->SetRenderOpacity(bBegun ? Rules.GetCover() : 1.f);
	}
	if (SkipHint)
	{
		// Only where a key does something: a skippable run, while a card shows.
		SkipHint->SetRenderOpacity(bBegun && Rules.IsSkippable() && Shown != INDEX_NONE ? 0.7f * Rules.GetCardOpacity() : 0.f);
	}
}

bool UStartupLogosWidget::HandleKey(const FKey& Key, bool bRepeat)
{
	if (!bBegun || Rules.IsRevealing())
	{
		return false;
	}
	// One press, one card: a held key's own repeats skip nothing more.
	if (!bRepeat && Rules.Skip())
	{
		UE_LOG(LogLooter, Verbose, TEXT("Startup: %s skipped a card."), *Key.ToString());
	}
	return true;
}

void UStartupLogosWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

FReply UStartupLogosWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return HandleKey(InKeyEvent.GetKey(), InKeyEvent.IsRepeat()) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UStartupLogosWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleKey(InMouseEvent.GetEffectingButton()) ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FCursorReply UStartupLogosWidget::NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent)
{
	// No pointer over the cards; it's back with the menu.
	return Rules.IsRevealing() ? Super::NativeOnCursorQuery(InGeometry, InCursorEvent) : FCursorReply::Cursor(EMouseCursor::None);
}
