// UHudControlHintWidget: the control hint's line (its keycap drawn as the mission tracker's is), following
// UControlHintSubsystem, and its motion: in, the done flash, out.

#include "UI/HUD/HudControlHintWidget.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionText.h"
#include "Scenes/SceneSubsystem.h"
#include "Tutorial/ControlHintSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** Sounds asked of Main (Audio/LooterSoundCues.h and Art/Sounds/cues.json): until they're made they play nothing. */
	const FName HintCue(TEXT("UI.Hint"));
	const FName HintDoneCue(TEXT("UI.HintDone"));

	// --- The keycap, measured as the mission tracker's (HudMissionTrackerWidgetLayout.cpp), so the two read as one kit ---

	/** 22 px high and at least as wide, the key 4 px from its sides, the words 10 px after it. */
	constexpr float KeycapSize = 22.f;
	constexpr float KeycapPadding = 4.f;
	constexpr float KeycapGap = 10.f;
	/** Its top-right and bottom-left corners are cut this far, inside a cyan edge this wide. */
	constexpr float KeycapCut = 6.f;
	constexpr float KeycapEdge = 1.4f;
	/** Its ends stay as drawn while its middle stretches to the key: drawn at 1x, so an end's texels are its pixels. */
	constexpr float KeycapEndWidth = 8.f;
	constexpr float KeycapPixelsPerUnit = 1.f;
	/** The line's height, as the tracker's hint line. */
	constexpr float LineHeight = 26.f;

	/** Type: the key as the tracker's, the words a step larger than its hint (this line stands alone). */
	constexpr int32 KeySize = 13;
	constexpr int32 WordsSize = 16;

	/** It slides in from this far left. */
	constexpr float SlideDistance = 18.f;
	/** The keycap grows this much at the top of its done pop. */
	constexpr float DonePop = 0.18f;
	/** How fast it steps aside under a menu and comes back (opacity per second). */
	constexpr float CoverFadeRate = 8.f;

	/** The keycap's outline with its top-right and bottom-left corners cut, Inset in from its box. */
	TArray<FVector2D> HintKeycapOutline(float Inset)
	{
		const float Low = Inset;
		const float High = KeycapSize - Inset;
		const float CutInset = KeycapCut + Inset * (UE_SQRT_2 - 1.f);
		return { FVector2D(Low, Low), FVector2D(KeycapSize - CutInset, Low), FVector2D(High, CutInset), FVector2D(High, High),
			FVector2D(CutInset, High), FVector2D(Low, KeycapSize - CutInset) };
	}

	/** The keycap's plate, dark blue from the top down: a background. */
	FPaintedIcon MakeHintKeycapPlate()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(KeycapSize, KeycapSize);
		FPaintLayer Plate;
		Plate.Fills.Add(HintKeycapOutline(0.f));
		Plate.Color = Color::KeycapTop();
		Plate.GradientTo = Color::KeycapBottom();
		Plate.GradientStart = FVector2D(KeycapSize * 0.5f, 0.f);
		Plate.GradientEnd = FVector2D(KeycapSize * 0.5f, KeycapSize);
		Icon.Layers.Add(Plate);
		return Icon;
	}

	/** The keycap's cyan edge, just inside its outline. */
	FPaintedIcon MakeHintKeycapEdge()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(KeycapSize, KeycapSize);
		TArray<FVector2D> Edge = HintKeycapOutline(KeycapEdge * 0.5f);
		const FVector2D First = Edge[0];
		Edge.Add(First);
		FPaintLayer Stroke;
		Stroke.Strokes.Add(MoveTemp(Edge));
		Stroke.StrokeWidth = KeycapEdge;
		Stroke.Color = Color::Hairline();
		Icon.Layers.Add(Stroke);
		return Icon;
	}

	const FPaintedIcon& HintKeycapPlateIcon() { static const FPaintedIcon Icon = MakeHintKeycapPlate(); return Icon; }
	const FPaintedIcon& HintKeycapEdgeIcon() { static const FPaintedIcon Icon = MakeHintKeycapEdge(); return Icon; }

	/** A keycap picture drawn as a box: its ends as drawn, its middle stretched to the key's width. */
	FSlateBrush HintKeycapBrush(FName Name, const FPaintedIcon& Icon)
	{
		FSlateBrush Brush = PaintedIconBrush(Name, Icon, KeycapPixelsPerUnit, FVector2D(KeycapSize, KeycapSize));
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(KeycapEndWidth / KeycapSize, 0.f);
		return Brush;
	}
}

// ---------------------------------------------------------------------------
// Building it
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHudControlHintWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UHudControlHintWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHudControlHintWidget::BuildTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Root;

	// The keycap: its plate a background, its cyan edge and the key solid.
	UOverlay* Cap = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Plate = MakeImage(WidgetTree, HintKeycapBrush(TEXT("HudHintKeycapPlate"), HintKeycapPlateIcon()));
	MarkBackground(Plate);
	FillOverlaySlot(Cap->AddChildToOverlay(Plate));
	FillOverlaySlot(Cap->AddChildToOverlay(MakeImage(WidgetTree, HintKeycapBrush(TEXT("HudHintKeycapEdge"), HintKeycapEdgeIcon()))));
	KeyText = MakeFloatingText(WidgetTree, KeySize, FLinearColor::White);
	UOverlaySlot* KeySlot = Cap->AddChildToOverlay(KeyText);
	KeySlot->SetHorizontalAlignment(HAlign_Center);
	KeySlot->SetVerticalAlignment(VAlign_Center);
	KeySlot->SetPadding(FMargin(KeycapPadding, 0.f));
	USizeBox* CapBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CapBox->SetHeightOverride(KeycapSize);
	CapBox->SetMinDesiredWidth(KeycapSize);
	CapBox->AddChild(Cap);
	// The done pop grows it from its middle.
	CapBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Keycap = CapBox;

	// Then what the key does, floating outlined text.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(CapBox)->SetVerticalAlignment(VAlign_Center);
	WordsText = MakeFloatingText(WidgetTree, WordsSize, FLinearColor::White);
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(WordsText);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(KeycapGap, 0.f, 0.f, 0.f));
	USizeBox* LineBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LineBox->SetHeightOverride(LineHeight);
	LineBox->AddChild(Line);

	// Placed by its left end and the middle of its line.
	UCanvasPanelSlot* LineSlot = Root->AddChildToCanvas(LineBox);
	LineSlot->SetAnchors(FAnchors(0.f, 0.f));
	LineSlot->SetAlignment(FVector2D(0.f, 0.5f));
	LineSlot->SetPosition(FVector2D(Left, Middle));
	LineSlot->SetAutoSize(true);
	LineBox->SetRenderOpacity(0.f);
	LineBox->SetVisibility(ESlateVisibility::Collapsed);
	Block = LineBox;
}

// ---------------------------------------------------------------------------
// Following the hints
// ---------------------------------------------------------------------------

void UHudControlHintWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Block)
	{
		return;
	}
	const UControlHintSubsystem* Hints = UControlHintSubsystem::Get(this);
	const EControlHint Now = Hints ? Hints->GetShown() : EControlHint::Count;
	if (Phase == EPhase::Showing && Now != ShownHint)
	{
		// The hint on show went: used (it flashes), or unheeded, turned off or replaced (it fades).
		const FControlHintRules* Rules = Hints ? &Hints->GetRules() : nullptr;
		BeginLeave(Rules && Rules->GetLastEnded() == ShownHint && Rules->GetLastEnd() == EControlHintEnd::Done);
	}
	// The next waits for the last to finish going, so two never overlap.
	if (Phase == EPhase::Idle && Now != EControlHint::Count)
	{
		Present(Now);
	}
	Animate(InDeltaTime);
}

void UHudControlHintWidget::Present(EControlHint Hint)
{
	ShownHint = Hint;
	Phase = EPhase::Showing;
	Age = 0.f;
	// The key as the player bound it ("W A S D", "Left Shift"), read now, so a key rebound since shows right.
	const UControlHintSubsystem* Hints = UControlHintSubsystem::Get(this);
	const FName Action = Hints ? Hints->GetShownAction() : FControlHintRules::Action(Hint);
	KeyText->SetText(FText::FromString(MissionText::KeyName(GetWorld(), Action)));
	WordsText->SetText(Hints ? Hints->GetShownWords() : FControlHintRules::Words(Hint));
	WordsText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Keycap->SetRenderScale(FVector2D(1.f, 1.f));
	Block->SetVisibility(ESlateVisibility::HitTestInvisible);
	// Quiet: a hint is a nudge, not an event.
	LooterSound::Play2D(this, HintCue, 0.6f);
}

void UHudControlHintWidget::BeginLeave(bool bDone)
{
	Age = 0.f;
	if (!bDone)
	{
		Phase = EPhase::Fading;
		return;
	}
	// Used: the words turn the tracker's done cyan, the keycap pops, and a small tick is heard.
	Phase = EPhase::Done;
	WordsText->SetColorAndOpacity(FSlateColor(Color::CyanText()));
	LooterSound::Play2D(this, HintDoneCue);
}

bool UHudControlHintWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	if (USceneSubsystem::HidesGameplayHUD(this))
	{
		return true;
	}
	// A dead player has no use for a nudge about the controls: it waits for them to be back on their feet.
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	const UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	if (Health && Health->IsDead())
	{
		return true;
	}
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}

void UHudControlHintWidget::Animate(float DeltaTime)
{
	Age += DeltaTime;
	if (Phase == EPhase::Done && Age >= DoneSeconds)
	{
		Phase = EPhase::Fading;
		Age = 0.f;
	}
	if (Phase == EPhase::Fading && Age >= FadeSeconds)
	{
		Phase = EPhase::Idle;
		ShownHint = EControlHint::Count;
	}
	CoverOpacity = FMath::FInterpConstantTo(CoverOpacity, IsCovered() ? 0.f : 1.f, DeltaTime, CoverFadeRate);
	if (Phase == EPhase::Idle)
	{
		Block->SetRenderOpacity(0.f);
		Block->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	float Opacity = 1.f;
	float Offset = 0.f;
	float Pop = 1.f;
	switch (Phase)
	{
	case EPhase::Showing:
	{
		// Sliding in from the left and fading up, easing out.
		const float Share = FMath::Clamp(Age / SlideSeconds, 0.f, 1.f);
		Opacity = Share;
		Offset = -SlideDistance * FMath::Pow(1.f - Share, 3.f);
		break;
	}
	case EPhase::Done:
		Pop = 1.f + DonePop * FMath::Sin(UE_PI * FMath::Clamp(Age / DoneSeconds, 0.f, 1.f));
		break;
	case EPhase::Fading:
		Opacity = 1.f - FMath::Clamp(Age / FadeSeconds, 0.f, 1.f);
		break;
	case EPhase::Idle:
		break;
	}
	const float Shown = Opacity * CoverOpacity;
	Block->SetRenderOpacity(Shown);
	Block->SetRenderTranslation(FVector2D(Offset, 0.f));
	Keycap->SetRenderScale(FVector2D(Pop, Pop));
	Block->SetVisibility(Shown > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}
