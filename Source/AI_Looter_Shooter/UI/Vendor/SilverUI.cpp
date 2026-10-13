#include "UI/Vendor/SilverUI.h"
#include "Economy/EconomyRules.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** A circle's points, clockwise on screen (y down), first not repeated. */
	TArray<FVector2D> Circle(const FVector2D& Middle, float Radius, int32 Points)
	{
		TArray<FVector2D> Out;
		Out.Reserve(Points);
		for (int32 Index = 0; Index < Points; ++Index)
		{
			const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(Points);
			Out.Add(Middle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Out;
	}

	/** A five-pointed star's outline as a closed polyline (the first point repeated), its top point up. */
	TArray<FVector2D> Star(const FVector2D& Middle, float Outer, float Inner)
	{
		TArray<FVector2D> Out;
		for (int32 Index = 0; Index <= 10; ++Index)
		{
			const float Angle = -0.5f * UE_PI + UE_PI * static_cast<float>(Index % 10) / 5.f;
			const float Radius = Index % 2 == 0 ? Outer : Inner;
			Out.Add(Middle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Out;
	}
}

const FInkedIcon& SilverUI::CoinIcon()
{
	static const FInkedIcon Icon = []()
	{
		FInkedIcon Coin;
		Coin.ViewBox = FVector2D(32.f, 32.f);
		const FVector2D Middle(16.f, 16.f);
		// The disc, lit above and shaded under its middle like every Inked icon; its rim struck in a ring, a star in it.
		Coin.Shapes.Add(Circle(Middle, 13.f, 32));
		TArray<FVector2D> Rim = Circle(Middle, 9.6f, 32);
		// A copy: adding a reference to the array's own element asserts (the add may reallocate).
		Rim.Add(FVector2D(Rim[0]));
		Coin.Cuts.Add(Rim);
		Coin.Cuts.Add(Star(Middle, 4.8f, 2.0f));
		Coin.ShadeY = 17.5f;
		Coin.CutWidth = 0.7f;
		Coin.StrokeWidth = 1.f;
		Coin.OutlineWidth = 1.5f;
		Coin.MaxOutlineWidth = 2.4f;
		return Coin;
	}();
	return Icon;
}

FName SilverUI::CoinIconName()
{
	return TEXT("SilverCoin");
}

UImage* SilverUI::MakeCoin(UWidgetTree* Tree, float Size, const FLinearColor& Tint)
{
	return MakeImage(Tree, InkedIconBrush(CoinIconName(), CoinIcon(), FVector2D(Size, Size), Tint));
}

UHorizontalBox* SilverUI::MakeAmount(UWidgetTree* Tree, int32 Silver, int32 FontSize, const FLinearColor& TextColor, bool bFloating,
	UTextBlock** OutText)
{
	UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const float CoinSize = FMath::RoundToFloat(static_cast<float>(FontSize) * 1.25f);
	Line->AddChildToHorizontalBox(MakeCoin(Tree, CoinSize))->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Amount = bFloating ? MakeFloatingText(Tree, FontSize, TextColor, 40, ETextJustify::Left)
		: MakeText(Tree, FString(), FontSize, TextColor, false, 40);
	Amount->SetText(FText::FromString(LooterEconomy::Format(Silver)));
	UHorizontalBoxSlot* AmountSlot = Line->AddChildToHorizontalBox(Amount);
	AmountSlot->SetVerticalAlignment(VAlign_Center);
	AmountSlot->SetPadding(FMargin(FMath::Max(3.f, CoinSize * 0.25f), 0.f, 0.f, 0.f));
	if (OutText)
	{
		*OutText = Amount;
	}
	return Line;
}
