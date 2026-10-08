// UBenchWidget: the chosen gun: its card under the stand (name, notches and curse, stats), the stand itself with the
// chosen part marked on it, the line under the title, and the prompts.

#include "UI/Bench/BenchWidget.h"
#include "Affixes/WeaponRollLibrary.h"
#include "UI/Bench/BenchRules.h"
#include "UI/Bench/BenchStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponPartSwap.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/DrawElements.h"

#define LOCTEXT_NAMESPACE "LooterBench"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The line under the title when nothing has been done yet: what the bench is for. */
	FText BenchHint()
	{
		return LOCTEXT("Hint", "Scrap guns for parts · fit parts to guns of their kind");
	}

	/** The gun with Part fitted (the stand and the card show it so before it's fitted). */
	FWeaponInstanceData WithPart(const FWeaponInstanceData& Gun, const FBoxedWeaponPart* Part)
	{
		FWeaponInstanceData Fitted = Gun;
		if (Part)
		{
			FBoxedWeaponPart Replaced;
			WeaponPartSwap::Fit(Fitted, *Part, Replaced);
		}
		return Fitted;
	}
}

// ---------------------------------------------------------------------------
// The card under the stand
// ---------------------------------------------------------------------------

void UBenchWidget::RefreshGun()
{
	if (!GunName)
	{
		return;
	}
	IdeasBox->ClearChildren();
	StatsBox->ClearChildren();
	const FWeaponInstanceData* Item = ChosenItem();
	if (!Item)
	{
		GunTag->SetText(FText::GetEmpty());
		GunName->SetText(LOCTEXT("NoGun", "NO GUN TO WORK ON"));
		GunName->SetColorAndOpacity(FSlateColor(Color::TextDim()));
		GunSub->SetText(FText::GetEmpty());
		StatsHeader->SetText(FText::GetEmpty());
		IdeasBox->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// The part under the cursor, as if fitted: a swap can rename the gun (its parts' NamePriority), so the name follows.
	const FBoxedWeaponPart* Preview = PreviewPart();
	const FWeaponInstanceData Shown = WithPart(*Item, Preview);
	const FCarriedGun Ref = ChosenRef();
	FString Tag = Ref.bBackpack ? FString(TEXT("In the backpack")) : FString::Printf(TEXT("Slot %d"), Ref.Index + 1);
	if (bScrapping)
	{
		Tag = TEXT("Scrapping");
	}
	else if (Preview)
	{
		Tag = FString::Printf(TEXT("If fitted: %s"), *BenchRules::PartName(*Preview));
	}
	GunTag->SetText(FText::FromString(Tag.ToUpper()));
	GunTag->SetColorAndOpacity(FSlateColor(bScrapping ? Color::Worse() : Color::Accent()));
	GunName->SetText(FText::FromString(LooterWeaponText::Name(Shown).ToUpper()));
	GunName->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(Shown)));
	GunSub->SetText(FText::FromString(FString::Printf(TEXT("Lv %d · %s · %s"), Shown.Level, *LooterWeaponText::FireModeName(Shown),
		*AmmoName(Shown)).ToUpper()));

	// Its notches and its curse stay with it whatever its parts.
	if (UWidget* Ideas = MakeGunIdeasRows(WidgetTree, *Item, 10))
	{
		IdeasBox->AddChildToVerticalBox(Ideas);
	}
	IdeasBox->SetVisibility(IdeasBox->HasAnyChildren() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	// Its stats, rebuilt as the game builds them (notches and curse included); with a part under the cursor, as they'd be
	// with it fitted, and the change from now.
	const FWeaponStats Now = UWeaponRollLibrary::ComputeInstanceStats(*Item);
	const FWeaponStats S = Preview ? WeaponPartSwap::PreviewStats(*Item, *Preview) : Now;
	const FWeaponStats* B = Preview ? &Now : nullptr;
	StatsHeader->SetText(Preview ? LOCTEXT("StatsIfFitted", "STATS IF FITTED") : LOCTEXT("Stats", "STATS"));
	auto AddStat = [this, B](const TCHAR* StatName, float Rating, const FString& Value, float New, float Old, bool bHigherIsBetter, int32 Decimals)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Label(WidgetTree, StatName, 8, Color::TextDim(), 120), 92.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BarSlot = Line->AddChildToHorizontalBox(MakeSegmentBar(WidgetTree, 8, Rating, Color::SegmentOn(), 7.f));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* ValueText = MakeText(WidgetTree, Value, 11, Color::Text());
		ValueText->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, ValueText, 64.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		// The change fitting it makes, green when it's for the better.
		UTextBlock* DeltaText = MakeText(WidgetTree, TEXT(""), 9, Color::TextDim());
		DeltaText->SetJustification(ETextJustify::Right);
		if (B && !FMath::IsNearlyEqual(New, Old, 0.01f))
		{
			DeltaText->SetText(FText::FromString(FormatDelta(New - Old, Decimals)));
			DeltaText->SetColorAndOpacity(FSlateColor(((New > Old) == bHigherIsBetter) ? Color::Better() : Color::Worse()));
		}
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, DeltaText, 50.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		StatsBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 19.f))->SetPadding(FMargin(0.f, 1.f));
	};
	// As the loadout's stats card reads them: damage per shot, so shotguns and rifles line up fairly.
	AddStat(TEXT("Damage"), LooterWeaponText::DamageRating(S), LooterWeaponText::DamageString(S), S.Damage * S.PelletsPerShot,
		B ? B->Damage * B->PelletsPerShot : 0.f, true, 1);
	AddStat(TEXT("Fire rate"), LooterWeaponText::FireRateRating(S), FString::Printf(TEXT("%.0f"), S.FireRate), S.FireRate, B ? B->FireRate : 0.f, true, 0);
	AddStat(TEXT("Magazine"), LooterWeaponText::MagazineRating(S), FString::FromInt(S.MagazineSize), S.MagazineSize, B ? B->MagazineSize : 0.f, true, 0);
	AddStat(TEXT("Reload"), LooterWeaponText::ReloadRating(S), FString::Printf(TEXT("%.2fs"), S.ReloadTime), S.ReloadTime, B ? B->ReloadTime : 0.f, false, 2);
	AddStat(TEXT("Accuracy"), LooterWeaponText::AccuracyRating(S), FString::Printf(TEXT("%.1f°"), S.Spread), S.Spread, B ? B->Spread : 0.f, false, 1);
	AddStat(TEXT("Range"), LooterWeaponText::RangeRating(S), FString::Printf(TEXT("%.0f m"), S.Range / 100.f), S.Range / 100.f, B ? B->Range / 100.f : 0.f, true, 0);
	AddStat(TEXT("Recoil"), LooterWeaponText::RecoilRating(S), FString::Printf(TEXT("%.0f%%"), S.Recoil * 100.f), S.Recoil * 100.f, B ? B->Recoil * 100.f : 0.f, false, 0);
	AddStat(TEXT("Handling"), LooterWeaponText::HandlingRating(S), FString::Printf(TEXT("%.0f%%"), S.Handling * 100.f), S.Handling * 100.f, B ? B->Handling * 100.f : 0.f, true, 0);
	AddStat(TEXT("Zoom"), LooterWeaponText::ZoomRating(S), LooterWeaponText::ZoomString(S), S.Zoom, B ? B->Zoom : 0.f, true, 2);
}

// ---------------------------------------------------------------------------
// The stand
// ---------------------------------------------------------------------------

void UBenchWidget::RefreshStage(bool bResetTurn)
{
	ABenchStage* StagePtr = Stage.Get();
	if (!StagePtr)
	{
		return;
	}
	const FWeaponInstanceData* Item = ChosenItem();
	if (!Item)
	{
		StagePtr->ClearGun();
		return;
	}
	StagePtr->ShowGun(WithPart(*Item, PreviewPart()), bResetTurn);
}

bool UBenchWidget::ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const
{
	const ABenchStage* StagePtr = Stage.Get();
	FVector2D UV;
	if (!StagePtr || !StagePtr->ProjectToImage(WorldLocation, UV))
	{
		return false;
	}
	OutPoint = FVector2f(BenchLayout::StageTopLeft + UV * BenchLayout::StageSize);
	return true;
}

namespace
{
	/** Points around a ring on the floor under the gun, from one angle to another (degrees, 0 = toward the camera). */
	template <typename FProject>
	TArray<FVector2f> BenchRingArc(const ABenchStage& Stage, float Radius, float From, float To, FProject&& Project)
	{
		const FVector Center = Stage.GetFloorCenter();
		const FVector Toward = Stage.GetTowardCamera();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Toward);
		TArray<FVector2f> Points;
		constexpr int32 Steps = 48;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Step) / Steps));
			FVector2f Point;
			if (Project(Center + (Toward * FMath::Cos(Angle) + Side * FMath::Sin(Angle)) * Radius, Point))
			{
				Points.Add(Point);
			}
		}
		return Points;
	}

	/** The inner ring sits this far in from the outer one, as on the loadout's stand. */
	constexpr float BenchInnerRingShare = 32.f / 46.f;

	/** The mark's corners stand off the part a little, and their arms are at most this long (page units). */
	constexpr float MarkMargin = 6.f;
	constexpr float MarkArm = 14.f;
}

void UBenchWidget::PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ABenchStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial || !StagePtr->HasGun())
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };
	const float Radius = StagePtr->GetRingRadius();

	// A soft glow over the whole ring, the post the gun stands over, then the far halves of the rings behind it.
	const TArray<FVector2f> Circle = BenchRingArc(*StagePtr, Radius, 0.f, 360.f, Project);
	if (Circle.Num() > 2)
	{
		const FBox2f Bounds(Circle);
		DrawBox(Elements, LayerId, Geometry, Bounds.Min, Bounds.GetSize(), &DiscBrush, Colors::RingGlow());
	}
	FVector2f Foot;
	FVector2f Top;
	const FVector Floor = StagePtr->GetFloorCenter();
	if (ProjectToPage(Floor, Foot) && ProjectToPage(Floor + FVector(0.f, 0.f, StagePtr->Lift), Top))
	{
		DrawLines(Elements, LayerId, Geometry, { Foot, Top }, Colors::Pillar(), 3.f);
	}
	DrawLines(Elements, LayerId, Geometry, BenchRingArc(*StagePtr, Radius, 90.f, 270.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, BenchRingArc(*StagePtr, Radius * BenchInnerRingShare, 90.f, 270.f, Project), Colors::InnerRing(), 1.5f);
}

void UBenchWidget::PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ABenchStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial || !StagePtr->HasGun())
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };
	const float Radius = StagePtr->GetRingRadius();
	DrawLines(Elements, LayerId, Geometry, BenchRingArc(*StagePtr, Radius, -90.f, 90.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, BenchRingArc(*StagePtr, Radius * BenchInnerRingShare, -90.f, 90.f, Project), Colors::InnerRing(), 1.5f);

	// The part the slots column is about, framed by four corner marks: which piece of the gun a fitting changes (or,
	// scrapping, the one kept).
	TArray<FVector> Corners;
	if (!StagePtr->GetSlotCorners(MarkedSlot(), Corners))
	{
		return;
	}
	FBox2f Box(ForceInit);
	for (const FVector& Corner : Corners)
	{
		FVector2f Point;
		if (ProjectToPage(Corner, Point))
		{
			Box += Point;
		}
	}
	if (!Box.bIsValid)
	{
		return;
	}
	Box = Box.ExpandBy(MarkMargin);
	const FVector2f Size = Box.GetSize();
	const float ArmX = FMath::Min(MarkArm, Size.X * 0.3f);
	const float ArmY = FMath::Min(MarkArm, Size.Y * 0.3f);
	const FLinearColor MarkColor = bScrapping ? Color::Accent() : Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.85f);
	const FVector2f Min = Box.Min;
	const FVector2f Max = Box.Max;
	DrawLines(Elements, LayerId, Geometry, { FVector2f(Min.X, Min.Y + ArmY), Min, FVector2f(Min.X + ArmX, Min.Y) }, MarkColor, 2.f);
	DrawLines(Elements, LayerId, Geometry, { FVector2f(Max.X - ArmX, Min.Y), FVector2f(Max.X, Min.Y), FVector2f(Max.X, Min.Y + ArmY) }, MarkColor, 2.f);
	DrawLines(Elements, LayerId, Geometry, { FVector2f(Max.X, Max.Y - ArmY), Max, FVector2f(Max.X - ArmX, Max.Y) }, MarkColor, 2.f);
	DrawLines(Elements, LayerId, Geometry, { FVector2f(Min.X + ArmX, Max.Y), FVector2f(Min.X, Max.Y), FVector2f(Min.X, Max.Y - ArmY) }, MarkColor, 2.f);
}

// ---------------------------------------------------------------------------
// The line under the title, and the prompts
// ---------------------------------------------------------------------------

void UBenchWidget::SetStatus(const FText& Text, const FLinearColor& TextColor)
{
	if (!StatusText)
	{
		return;
	}
	const bool bHint = Text.IsEmpty();
	StatusText->SetText(FText::FromString((bHint ? BenchHint() : Text).ToString().ToUpper()));
	StatusText->SetColorAndOpacity(FSlateColor(bHint ? Color::TextDim() : TextColor));
}

void UBenchWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	struct FPrompt
	{
		FString Key;
		FString Text;
	};
	TArray<FPrompt, TInlineAllocator<8>> Prompts;
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FString InteractKey = Bindings ? Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString() : FString(TEXT("E"));

	if (Confirm != EConfirm::None)
	{
		Prompts.Add({ TEXT("Enter"), Confirm == EConfirm::Scrap ? TEXT("Scrap") : TEXT("Throw out") });
		Prompts.Add({ TEXT("Esc"), TEXT("Back") });
	}
	else if (bScrapping)
	{
		Prompts.Add({ TEXT("Enter / Click"), TEXT("Keep this part") });
		Prompts.Add({ TEXT("W / S"), TEXT("Choose") });
		Prompts.Add({ TEXT("Esc"), TEXT("Keep the gun") });
	}
	else
	{
		switch (Column)
		{
		case EColumn::Guns:
			Prompts.Add({ TEXT("Enter / Click"), CursorIndex == ChosenGun ? TEXT("Its slots") : TEXT("Work on this gun") });
			Prompts.Add({ TEXT("X"), TEXT("Scrap") });
			Prompts.Add({ TEXT("W / S"), TEXT("Browse") });
			Prompts.Add({ TEXT("D"), TEXT("Slots") });
			break;
		case EColumn::Slots:
			if (!PartRows.IsEmpty())
			{
				Prompts.Add({ TEXT("Enter / D"), TEXT("Parts box") });
			}
			Prompts.Add({ TEXT("X"), TEXT("Scrap") });
			Prompts.Add({ TEXT("W / S"), TEXT("Choose slot") });
			Prompts.Add({ TEXT("A"), TEXT("Guns") });
			break;
		case EColumn::Parts:
			if (PartEntries.IsValidIndex(CursorIndex) && PartEntries[CursorIndex].Fits())
			{
				Prompts.Add({ TEXT("Enter / Click"), TEXT("Fit") });
			}
			Prompts.Add({ TEXT("Q / Right-click"), TEXT("Throw out") });
			Prompts.Add({ TEXT("W / S"), TEXT("Browse") });
			Prompts.Add({ TEXT("A"), TEXT("Slots") });
			break;
		}
		Prompts.Add({ FString::Printf(TEXT("%s / Esc"), *InteractKey), TEXT("Close") });
	}
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
