// The boss bar's pips (Main 7, Abel Ransom, Corrupted; MAINS_2_7.md section 11: "His bar reads 'Smog' and drains as he's
// hurt; under it three gold pips, 'Abel', fill as he's called back"): a row of diamond pips under the bar's far end with its
// label beside them, and a word over the bar's far end. Gold (LooterUI's SoulGold, the kit's "selected") lit, a dark track
// with a gold rim unlit; a pip filling pops (big, white-hot, settling) so the call-back lands. No backing panel (the gameplay
// HUD's rule): the pips' dark track is their only background and fades with the UI transparency. Made the first time a boss
// asks for it.

#include "UI/HUD/HudBossBarWidget.h"
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
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

using namespace LooterUI;

namespace
{
	/** Where the boss bar's pieces are (as HudBossBarWidgetLayout.cpp lays them out). */
	constexpr float PipsClusterSide = 60.f;
	constexpr float PipsBarTop = 36.f;
	constexpr float PipsBarHeight = 30.f;
	constexpr float PipsBarRight = PipsClusterSide + UHudBossBarWidget::BarWidth;
	/** The row: under the bar's far end, its right edge 10 px inside it (clear of the clamp's foot), 5 px under the bar. */
	constexpr float PipsRight = PipsBarRight - 10.f;
	constexpr float PipsTop = PipsBarTop + PipsBarHeight + 5.f;
	/** The line under the bar sits this far under the row's foot while the pips show (the type's own leading spaces the two lines). */
	constexpr float PipsLineGap = 1.f;
	/** A pip: a 13 px diamond (a square turned 45 degrees), 2 px gold rim, 9 px apart; the label 13 px, 8 px before them. */
	constexpr float PipSize = 13.f;
	constexpr float PipRim = 2.f;
	constexpr float PipGap = 9.f;
	constexpr int32 PipLabelSize = 13;
	constexpr int32 PipLabelSpacing = 140;
	constexpr float PipLabelGap = 8.f;
	/** The word over the bar's far end: 13 px, its foot 3 px over the bar. */
	constexpr int32 BarWordSize = 13;
	constexpr int32 BarWordSpacing = 260;
	constexpr float BarWordGap = 3.f;
	/** The pop: a filled pip grows to this, flashes white, and settles over PopSeconds; the row fades in or out over FadeSeconds. */
	constexpr float PopScale = 1.9f;
	constexpr float PopSeconds = 0.55f;
	constexpr float FadeSeconds = 0.3f;
	constexpr float TrackOpacity = 0.85f;

	/** A turned diamond reaches PipSize x sqrt 2 tall; a 13 px line in the kit's type is about 24 with its outline (no Slate to ask). */
	constexpr float PipTurnedHeight = PipSize * 1.4143f;
	constexpr float PipLabelFallbackHeight = 24.f;
}

float UHudBossBarWidget::PipRowHeight()
{
	// Measured, not guessed: the label's line in the type the row draws it in (Slate's own measure, cached), so the line under
	// the bar clears it at any font or DPI scale (the HUD scales as a whole; the measure is in its units).
	float LabelHeight = PipLabelFallbackHeight;
	if (FSlateApplication::IsInitialized())
	{
		if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
		{
			LabelHeight = Renderer->GetFontMeasureService()->GetMaxCharacterHeight(FloatingFont(PipLabelSize, PipLabelSpacing));
		}
	}
	return FMath::Max(LabelHeight, PipTurnedHeight);
}

float UHudBossBarWidget::PipRowTop(float ArmourShown)
{
	return PipsTop + ArmourRowDrop * FMath::Clamp(ArmourShown, 0.f, 1.f);
}

float UHudBossBarWidget::PhaseLineTop(float ArmourShown, float PipsShown)
{
	// Under the bar; under the armour's bar as it shows; and under the pips' row as it shows (the row itself steps down under
	// the armour, so the drop for the pips is from where the line would be with the armour alone).
	const float Armour = FMath::Clamp(ArmourShown, 0.f, 1.f);
	const float WithArmour = UnderBarTop + ArmourRowDrop * Armour;
	const float UnderPips = PipRowBottom(Armour) + PipsLineGap;
	return WithArmour + FMath::Max(0.f, UnderPips - WithArmour) * FMath::Clamp(PipsShown, 0.f, 1.f);
}

void UHudBossBarWidget::PlaceUnderBar()
{
	if (UCanvasPanelSlot* LineSlot = PhaseLabel ? Cast<UCanvasPanelSlot>(PhaseLabel->Slot) : nullptr)
	{
		LineSlot->SetPosition(FVector2D(LineSlot->GetPosition().X, PhaseLineTop(ArmourOpacity, PipsRoom)));
	}
	if (UCanvasPanelSlot* RowSlot = PipRow ? Cast<UCanvasPanelSlot>(PipRow->Slot) : nullptr)
	{
		RowSlot->SetPosition(FVector2D(PipsRight, PipRowTop(ArmourOpacity)));
	}
}

void UHudBossBarWidget::SetPips(int32 Total, int32 Filled, const FText& InPipLabel, const FText& InBarLabel)
{
	if (Total <= 0)
	{
		bPipsWanted = false;
		return;
	}
	const bool bRebuild = Total != PipsTotal || !PipRow;
	if (bRebuild)
	{
		PipsTotal = Total;
		PipsFilled = FMath::Clamp(Filled, 0, Total);
		PipPop.Init(0.f, Total);
		BuildPips();
	}
	bPipsWanted = true;
	if (!InPipLabel.EqualTo(PipLabelText) || !InBarLabel.EqualTo(BarLabelText))
	{
		PipLabelText = InPipLabel;
		BarLabelText = InBarLabel;
		if (PipLabel)
		{
			PipLabel->SetText(PipLabelText.ToUpper());
		}
		if (BarWord)
		{
			BarWord->SetText(BarLabelText.ToUpper());
		}
	}
	const int32 Wanted = FMath::Clamp(Filled, 0, PipsTotal);
	if (Wanted == PipsFilled && !bRebuild)
	{
		return;
	}
	// The pips that fill now pop (a call-back's reward on the HUD); ones taken back (the fight started over) just go dark.
	for (int32 Index = PipsFilled; Index < Wanted; ++Index)
	{
		if (PipPop.IsValidIndex(Index))
		{
			PipPop[Index] = 1.f;
		}
	}
	PipsFilled = Wanted;
	ApplyPips();
}

void UHudBossBarWidget::BuildPips()
{
	UCanvasPanel* Box = Cast<UCanvasPanel>(Cluster);
	if (!Box || !WidgetTree)
	{
		return;
	}
	if (PipRow)
	{
		PipRow->RemoveFromParent();
		PipRow = nullptr;
	}
	if (BarWord)
	{
		BarWord->RemoveFromParent();
		BarWord = nullptr;
	}
	PipBoxes.Reset();
	PipFaces.Reset();

	// The row, right-aligned under the bar's far end: the label, then the pips.
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	PipLabel = MakeFloatingText(WidgetTree, PipLabelSize, Color::SoulGold(), PipLabelSpacing, ETextJustify::Right);
	PipLabel->SetText(PipLabelText.ToUpper());
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(PipLabel);
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	LabelSlot->SetPadding(FMargin(0.f, 0.f, PipLabelGap, 0.f));
	for (int32 Index = 0; Index < PipsTotal; ++Index)
	{
		// A diamond: a rimmed square turned 45 degrees; its face is gold when lit, the dark track when not.
		// The rim, the dark track inside it (the only background: it fades with the UI transparency), and the gold fill over
		// the track while lit (solid, as bars are).
		UOverlay* Pip = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillOverlaySlot(Pip->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Color::SoulGold()))));
		UImage* Track = MakeImage(WidgetTree, RectBrush(Color::Track().CopyWithNewOpacity(TrackOpacity)));
		MarkBackground(Track);
		FillOverlaySlot(Pip->AddChildToOverlay(Track), FMargin(PipRim));
		UImage* Face = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		FillOverlaySlot(Pip->AddChildToOverlay(Face), FMargin(PipRim));
		USizeBox* Sized = MakeSized(WidgetTree, Pip, PipSize, PipSize);
		Sized->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Sized->SetRenderTransformAngle(45.f);
		UHorizontalBoxSlot* PipSlot = Row->AddChildToHorizontalBox(Sized);
		PipSlot->SetVerticalAlignment(VAlign_Center);
		PipSlot->SetPadding(FMargin(Index == 0 ? 0.f : PipGap, 0.f, 0.f, 0.f));
		PipBoxes.Add(Sized);
		PipFaces.Add(Face);
	}
	UCanvasPanelSlot* RowSlot = Box->AddChildToCanvas(Row);
	RowSlot->SetAutoSize(true);
	RowSlot->SetAlignment(FVector2D(1.f, 0.f));
	RowSlot->SetPosition(FVector2D(PipsRight, PipRowTop(ArmourOpacity)));

	// The word over the bar's far end (the bar's own name for what drains: SMOG), right-aligned over the clamp.
	BarWord = MakeFloatingText(WidgetTree, BarWordSize, Color::Text(), BarWordSpacing, ETextJustify::Right);
	BarWord->SetText(BarLabelText.ToUpper());
	UCanvasPanelSlot* WordSlot = Box->AddChildToCanvas(BarWord);
	WordSlot->SetAutoSize(true);
	WordSlot->SetAlignment(FVector2D(1.f, 1.f));
	WordSlot->SetPosition(FVector2D(PipsBarRight - 4.f, PipsBarTop - BarWordGap));

	PipRow = Row;
	PipsOpacity = 0.f;
	Row->SetRenderOpacity(0.f);
	BarWord->SetRenderOpacity(0.f);
	PlaceUnderBar();
	ApplyPips();
}

void UHudBossBarWidget::ApplyPips()
{
	for (int32 Index = 0; Index < PipFaces.Num(); ++Index)
	{
		UImage* Face = PipFaces[Index];
		UWidget* Pip = PipBoxes.IsValidIndex(Index) ? PipBoxes[Index].Get() : nullptr;
		if (!Face || !Pip)
		{
			continue;
		}
		const bool bLit = Index < PipsFilled;
		const float Pop = PipPop.IsValidIndex(Index) ? PipPop[Index] : 0.f;
		// Lit: the gold fill, white-hot as it pops; unlit: only the dark track inside its gold rim shows.
		Face->SetColorAndOpacity(FMath::Lerp(Color::SoulGold(), FLinearColor::White, Pop * 0.8f));
		Face->SetVisibility(bLit ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		const float Scale = 1.f + (PopScale - 1.f) * FMath::InterpEaseOut(0.f, 1.f, Pop, 2.f);
		Pip->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UHudBossBarWidget::TickPips(float DeltaSeconds)
{
	if (!PipRow)
	{
		return;
	}
	const float Target = bPipsWanted && !bDefeated ? 1.f : (bPipsWanted ? 0.6f : 0.f);
	if (PipsOpacity != Target)
	{
		PipsOpacity = FMath::FInterpConstantTo(PipsOpacity, Target, DeltaSeconds, 1.f / FadeSeconds);
		PipRow->SetRenderOpacity(PipsOpacity);
		PipRow->SetVisibility(PipsOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (BarWord)
		{
			BarWord->SetRenderOpacity(PipsOpacity);
			BarWord->SetVisibility(PipsOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	// The line under the bar steps down under the row as it comes, and back up as it goes (dimmed at the boss's death, the
	// row still holds its room).
	const float RoomTarget = bPipsWanted ? 1.f : 0.f;
	if (PipsRoom != RoomTarget)
	{
		PipsRoom = FMath::FInterpConstantTo(PipsRoom, RoomTarget, DeltaSeconds, 1.f / FadeSeconds);
		PlaceUnderBar();
	}
	bool bMoved = false;
	for (float& Pop : PipPop)
	{
		if (Pop > 0.f)
		{
			Pop = FMath::Max(0.f, Pop - DeltaSeconds / PopSeconds);
			bMoved = true;
		}
	}
	if (bMoved)
	{
		ApplyPips();
	}
}
