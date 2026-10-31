// PauseMenuLayout's rules: where each option's tick is on the range arc and the needle's aim at it, the needle's swing, the
// pieces' boxes (the tests check they fit 1080p and keep apart), and the type's room, measured with Slate's own font
// measure so the callouts and the mission card fit their words in the bigger in-game type.

#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Style/LooterUIStyle.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

namespace
{
	/** The swing's overshoot: a back ease-out of this strength peaks about 4% past its end, then settles. */
	constexpr float SwingBack = 1.f;

	/** Without Slate's renderer (never in play): about what Chakra Petch measures, a little wide so nothing is cut. */
	constexpr float FallbackCharWidth = 0.62f;
	constexpr float FallbackLineHeight = 1.75f;

	/** Measured a little narrower than the text blocks wrap at, so a line Slate keeps whole is never counted short. */
	constexpr float MeasureSlack = 2.f;


	FSlateRenderer* Renderer()
	{
		return FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetRenderer() : nullptr;
	}
}

FVector2D PauseMenuLayout::RowArcPoint(int32 Row)
{
	const float Y = RowY[FMath::Clamp(Row, 0, NumRows - 1)];
	const float Down = Y - DialY;
	return FVector2D(DialX + FMath::Sqrt(FMath::Max(FMath::Square(ArcRadius) - FMath::Square(Down), 0.f)), Y);
}

float PauseMenuLayout::RowDegrees(int32 Row)
{
	const FVector2D Point = RowArcPoint(Row);
	return FMath::RadiansToDegrees(FMath::Atan2(Point.Y - DialY, Point.X - DialX));
}

float PauseMenuLayout::NeedleSwing(float Alpha)
{
	const float U = FMath::Clamp(Alpha, 0.f, 1.f) - 1.f;
	return 1.f + (SwingBack + 1.f) * U * U * U + SwingBack * U * U;
}

FBox2D PauseMenuLayout::MakeBox(float X, float Y, float Width, float Height)
{
	return FBox2D(FVector2D(X, Y), FVector2D(X + Width, Y + Height));
}

FBox2D PauseMenuLayout::TagBox(int32 Row, bool bLit)
{
	const float Left = TagX + (bLit ? TagLitSlide : 0.f) + TagGemX;
	const float Width = TagWidth + (bLit ? TagLitGrow : 0.f) - TagGemX;
	return MakeBox(Left, RowY[FMath::Clamp(Row, 0, NumRows - 1)] - TagHeight * 0.5f, Width, TagHeight);
}

FBox2D PauseMenuLayout::CalloutBox(int32 Row, float Height)
{
	return MakeBox(CalloutX, RowY[FMath::Clamp(Row, 0, NumRows - 1)] - CalloutPointY, CalloutWidth, Height);
}

FBox2D PauseMenuLayout::HeroBox()
{
	// Its level gem stands 18 over the card.
	return MakeBox(CardX, HeroY - 18.f, CardWidth, HeroHeight + 18.f);
}

FBox2D PauseMenuLayout::MissionBox(float Height)
{
	return MakeBox(CardX, MissionY - 14.f, CardWidth, Height + 14.f);
}

FBox2D PauseMenuLayout::SessionBox()
{
	// Its gem stands 14 out past its left.
	return MakeBox(SessionX - 14.f, SessionY, SessionWidth + 14.f, SessionHeight);
}

FBox2D PauseMenuLayout::KeysBox()
{
	return MakeBox(RightEdge - KeysWidth, KeysY - KeysHeight * 0.5f, KeysWidth, KeysHeight);
}

FBox2D PauseMenuLayout::TitleBox(float TitleWidth)
{
	return MakeBox(TitleX, TitleY, FMath::Max(TitleWidth, RulerLength), AreaY + AreaBand - TitleY);
}

FBox2D PauseMenuLayout::DialBox()
{
	// The range arc's ends and the ring's long ticks, whichever reach further.
	const float ArcRise = ArcRadius * FMath::Sin(FMath::DegreesToRadians(ArcTo)) + 14.f;
	const float RingReach = DialRadius * 1.21f + 8.f;
	const float Rise = FMath::Max(ArcRise, RingReach);
	return MakeBox(0.f, DialY - Rise, DialX + ArcRadius + 14.f, Rise * 2.f);
}

// ---------------------------------------------------------------------------
// The type's room
// ---------------------------------------------------------------------------

float PauseMenuLayout::MeasureWidth(const FString& Text, const FSlateFontInfo& Font)
{
	if (FSlateRenderer* Slate = Renderer())
	{
		return static_cast<float>(Slate->GetFontMeasureService()->Measure(Text, Font).X);
	}
	return Text.Len() * Font.Size * FallbackCharWidth * (1.f + Font.LetterSpacing / 1000.f);
}

float PauseMenuLayout::LineHeight(const FSlateFontInfo& Font)
{
	if (FSlateRenderer* Slate = Renderer())
	{
		return static_cast<float>(Slate->GetFontMeasureService()->GetMaxCharacterHeight(Font))
			+ static_cast<float>(Font.OutlineSettings.OutlineSize) * 2.f;
	}
	return Font.Size * FallbackLineHeight;
}

int32 PauseMenuLayout::MeasureLines(const FString& Text, const FSlateFontInfo& Font, float WrapWidth)
{
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);
	if (Words.IsEmpty())
	{
		return 0;
	}
	// Whole words to a line, as a text block wraps: a word that doesn't fit starts the next line (one too long for any
	// line stands alone on its own).
	const float Room = FMath::Max(WrapWidth - MeasureSlack, 1.f);
	int32 Lines = 1;
	FString Line;
	for (const FString& Word : Words)
	{
		const FString Longer = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		if (Line.IsEmpty() || MeasureWidth(Longer, Font) <= Room)
		{
			Line = Longer;
		}
		else
		{
			++Lines;
			Line = Word;
		}
	}
	return Lines;
}

float PauseMenuLayout::CalloutHeight(const FString& Help, const FString& Flavor)
{
	using LooterUI::Font;
	const FSlateFontInfo TitleType = Font(CalloutTitleFont, true, CalloutTitleSpacing);
	const FSlateFontInfo HelpType = Font(HelpFont, false);
	const FSlateFontInfo FlavorType = Font(FlavorFont, false);
	const float Height = CalloutPadTop + LineHeight(TitleType) + OrnamentBlock
		+ MeasureLines(Help, HelpType, CalloutTextWidth) * LineHeight(HelpType)
		+ FlavorGap + MeasureLines(Flavor, FlavorType, CalloutTextWidth) * LineHeight(FlavorType) + CalloutPadBottom;
	// Never shorter than its point needs (the point's tip 52 down, 30 each way, then a cut corner).
	return FMath::CeilToFloat(FMath::Max(Height, CalloutPointY + 30.f + 26.f));
}

float PauseMenuLayout::MissionHeight(const FString& Line, const TArray<FString>& Optionals)
{
	using LooterUI::Font;
	float Height = MissionPadTop
		+ FMath::Max(LineHeight(Font(MissionLabelFont, true, MissionLabelSpacing)), LineHeight(Font(MissionCountFont, true))) + MissionLabelGap
		+ LineHeight(Font(MissionNameFont, true, MissionNameSpacing)) + MissionNameGap;
	const FSlateFontInfo LineType = Font(MissionLineFont, false);
	Height += MeasureLines(Line, LineType, MissionTextWidth) * LineHeight(LineType);
	const FSlateFontInfo OptionalType = Font(MissionOptionalFont, false);
	for (const FString& Optional : Optionals)
	{
		Height += MissionOptionalGap + MeasureLines(Optional, OptionalType, MissionTextWidth) * LineHeight(OptionalType);
	}
	return FMath::CeilToFloat(Height + MissionPadBottom);
}
