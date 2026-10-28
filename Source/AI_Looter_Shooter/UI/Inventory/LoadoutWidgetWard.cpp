// ULoadoutWidget: the WARD section (SHIELD, 2026-10-22): the ward worn and the spares carried as rows in the left column,
// under the equip slots, and what the keys do on its rows (wear a spare, stow, drop). Its card and showcase icon are
// LoadoutWidgetWardCard.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** A ward's row's button action (its index is the row: 0 the ward worn, then the spares). */
	const FName ActionWard(TEXT("Ward"));
	/** The ward box's icon on a row (the worn one's bigger), and in the showcase. */
	constexpr float WornIconSize = 40.f;
	constexpr float SpareIconSize = 28.f;
	constexpr float ShowcaseIconSize = 240.f;

	FName WardIconName(const FShieldInstanceData* Ward)
	{
		return Ward && Ward->Definition ? Ward->Definition->IconName : FName(TEXT("WardBox"));
	}

	UWidget* WardIcon(UWidgetTree* Tree, const FShieldInstanceData* Ward, float Size, const FLinearColor& Tint = FLinearColor::White)
	{
		const FName Name = WardIconName(Ward);
		if (const FInkedIcon* Inked = FindInkedIcon(Name))
		{
			return MakeSized(Tree, MakeImage(Tree, InkedIconBrush(Name, *Inked, FVector2D(Size, Size), Tint)), Size, Size);
		}
		return MakeSized(Tree, Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass()), Size, Size);
	}
}

// ---------------------------------------------------------------------------
// The rows
// ---------------------------------------------------------------------------

UPlayerShieldComponent* ULoadoutWidget::GetShield() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? UPlayerShieldComponent::Find(Inventory->GetOwner()) : nullptr;
}

const FShieldInstanceData* ULoadoutWidget::WardItem(int32 Row) const
{
	const UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return nullptr;
	}
	if (Row == 0)
	{
		return Shield->HasWard() ? &Shield->GetWard() : nullptr;
	}
	const TArray<FShieldInstanceData>& Spares = Shield->GetSpares();
	return Spares.IsValidIndex(Row - 1) ? &Spares[Row - 1] : nullptr;
}

void ULoadoutWidget::BuildWardSection(UVerticalBox* Column)
{
	if (!Column)
	{
		return;
	}
	// The header as the others are (WARD and its count), then the rows, under the slots (the gaps RowCenterY counts on).
	UTextBlock* Count = nullptr;
	WardHeader = MakeSectionHeader(TEXT("Ward"), Count);
	WardCount = Count;
	WardCount->SetColorAndOpacity(FSlateColor(Color::TextDim()));
	WardList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(WardHeader)->SetPadding(FMargin(0.f, LoadoutLayout::SectionGap, 0.f, 0.f));
	Column->AddChildToVerticalBox(WardList)->SetPadding(FMargin(0.f, LoadoutLayout::HeaderGap, 0.f, 0.f));

	// The showcase's stand-in for a ward: its icon, big, where the gun turns.
	if (Page && !WardShowcase)
	{
		WardShowcase = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		WardShowcase->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot* ShowSlot = Page->AddChildToCanvas(WardShowcase);
		ShowSlot->SetSize(FVector2D(ShowcaseIconSize, ShowcaseIconSize));
		ShowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ShowSlot->SetPosition(LoadoutLayout::ShowcaseTopLeft + LoadoutLayout::ShowcaseSize * 0.5f);
	}
}

void ULoadoutWidget::RebuildWards()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (Shield != BoundShield.Get())
	{
		// The ward's changes refresh the screen as the guns' do.
		if (UPlayerShieldComponent* Old = BoundShield.Get())
		{
			Old->OnWardsChanged.RemoveDynamic(this, &ULoadoutWidget::Refresh);
		}
		BoundShield = Shield;
		if (Shield)
		{
			Shield->OnWardsChanged.AddUniqueDynamic(this, &ULoadoutWidget::Refresh);
		}
	}
	if (!WardList)
	{
		return;
	}
	WardList->ClearChildren();
	WardRows.Reset();
	// No shield on the holder (a stand-in without health): no section at all, so the list walks as it did.
	const int32 NumRows = Shield ? 1 + Shield->GetSpares().Num() : 0;
	const ESlateVisibility Section = Shield ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	WardList->SetVisibility(Section);
	if (WardHeader)
	{
		WardHeader->SetVisibility(Section);
	}
	if (NumRows == 0 && Zone == EZone::Wards)
	{
		Zone = EZone::Slots;
		CursorIndex = TargetSlot;
	}
	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		const FRow Made = MakeWardRow(Row);
		WardList->AddChildToVerticalBox(Made.Button)->SetPadding(FMargin(0.f, Row > 0 ? LoadoutLayout::RowGap : 0.f, 0.f, 0.f));
		WardRows.Add(Made);
	}
	if (WardCount)
	{
		WardCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d spare"), Shield ? Shield->GetSpares().Num() : 0,
			UPlayerShieldComponent::MaxSpares)));
	}
}

ULoadoutWidget::FRow ULoadoutWidget::MakeWardRow(int32 Row)
{
	const FShieldInstanceData* Ward = WardItem(Row);
	const bool bWorn = Row == 0;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const float IconSize = bWorn ? WornIconSize : SpareIconSize;
	Line->AddChildToHorizontalBox(WardIcon(WidgetTree, Ward, IconSize, Ward ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.35f)))
		->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (Ward)
	{
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, ShieldText::Name(*Ward), bWorn ? 12 : 10, ShieldText::Color(*Ward), 50));
		const FString Trait = ShieldText::TraitName(Ward->Trait);
		const FString Under = Trait.IsEmpty() ? ShieldText::KindLine(*Ward) : FString::Printf(TEXT("%s · %s"), *ShieldText::KindLine(*Ward), *Trait);
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, Under, bWorn ? 8 : 7, Trait.IsEmpty() ? Color::TextDim() : Color::CyanText(), 140))
			->SetPadding(FMargin(0.f, bWorn ? 3.f : 1.f, 0.f, 0.f));
	}
	else
	{
		Words->AddChildToVerticalBox(Label(WidgetTree, TEXT("No ward worn"), 11, Color::TextDim(), 200));
		const UPlayerShieldComponent* Shield = GetShield();
		const bool bSpares = Shield && !Shield->GetSpares().IsEmpty();
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, bSpares ? TEXT("E: wear a spare") : TEXT("Wards drop like guns: hold Interact to wear one"),
			8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.7f), 120))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 10.f, 0.f));

	if (Ward)
	{
		// The one number a ward is judged by at a glance: what it holds.
		UVerticalBox* Number = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* Value = MakeText(WidgetTree, ShieldText::CapacityString(Ward->Stats), bWorn ? 17 : 13, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		Number->AddChildToVerticalBox(Value)->SetHorizontalAlignment(HAlign_Right);
		if (bWorn)
		{
			UTextBlock* Word = Label(WidgetTree, TEXT("Ward"), 7, Color::TextDim(), 200);
			Word->SetJustification(ETextJustify::Right);
			Number->AddChildToVerticalBox(Word)->SetHorizontalAlignment(HAlign_Right);
		}
		Line->AddChildToHorizontalBox(Number)->SetVerticalAlignment(VAlign_Center);
		if (!bWorn)
		{
			// Against the ward worn: green up, red down, or level.
			const UPlayerShieldComponent* Shield = GetShield();
			const int32 Verdict = ShieldRules::Compare(*Ward, Shield && Shield->HasWard() ? &Shield->GetWard() : nullptr);
			UWidget* Mark = Verdict == 0 ? static_cast<UWidget*>(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::TextDim())), 10.f, 2.f))
				: static_cast<UWidget*>(MakeImage(WidgetTree, IconBrush(Verdict > 0 ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(Verdict > 0), 4.f, FVector2D(12.f, 10.f),
					Verdict > 0 ? Color::Better() : Color::Worse())));
			UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, 18.f, 0.f));
			MarkSlot->SetVerticalAlignment(VAlign_Center);
			MarkSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
	}

	const FLinearColor Rarity = Ward ? ShieldText::Color(*Ward) : Color::TextDim();
	FRow Made = WrapRow(Line, Rarity, Ward != nullptr, bWorn ? LoadoutLayout::WardRowHeight : LoadoutLayout::BackpackRowHeight, ActionWard, Row);
	// Its own clicks and hovers (a ward isn't a gun the screen's drag can take), sounded by the action.
	Made.Button->bPlaysSounds = false;
	Made.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleWardClicked);
	Made.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleWardHovered);
	Made.Identity = Ward ? ShieldRules::Identity(*Ward) : 0;
	return Made;
}

void ULoadoutWidget::RestyleWards()
{
	for (int32 Row = 0; Row < WardRows.Num(); ++Row)
	{
		const FRow& Made = WardRows[Row];
		const bool bItem = WardItem(Row) != nullptr;
		const bool bCursor = Zone == EZone::Wards && CursorIndex == Row && !bItemDrag;
		ApplyRowState(Made, bCursor ? ELooterRowState::Lit : (bItem ? ELooterRowState::Normal : ELooterRowState::Empty));
		// The ward worn wears a WORN word on its corner, as a slot shows its gun in hand.
		if (Row == 0 && bItem)
		{
			Made.Chip->SetBrush(RectBrush(Color::Hairline()));
			Made.ChipText->SetText(FText::FromString(TEXT("WORN")));
			Made.ChipText->SetColorAndOpacity(FSlateColor(Color::Track()));
			Made.Chip->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Made.Chip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// ---------------------------------------------------------------------------
// The keys and the mouse on a ward's row
// ---------------------------------------------------------------------------

TArray<ULoadoutWidget::FPrompt> ULoadoutWidget::WardPrompts() const
{
	TArray<FPrompt> Prompts;
	const UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return Prompts;
	}
	const bool bSpares = !Shield->GetSpares().IsEmpty();
	if (CursorIndex == 0)
	{
		if (bSpares)
		{
			Prompts.Add({ TEXT("E"), Shield->HasWard() ? TEXT("Choose a spare") : TEXT("Wear a spare"), EAction::Activate });
		}
		if (Shield->HasWard())
		{
			if (Shield->HasSpareRoom())
			{
				Prompts.Add({ TEXT("C"), TEXT("Stow"), EAction::Stow });
			}
			Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
		}
	}
	else if (WardItem(CursorIndex))
	{
		Prompts.Add({ TEXT("E"), TEXT("Wear"), EAction::Activate });
		Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
	}
	return Prompts;
}

bool ULoadoutWidget::RunWardAction(EAction Action)
{
	switch (Action)
	{
	case EAction::Activate:
	case EAction::Hold:
		WardActivate();
		return true;
	case EAction::Stow:
		WardStow();
		return true;
	case EAction::Drop:
		WardDrop();
		return true;
	case EAction::Move:
	case EAction::Inspect:
		// A ward isn't moved between slots or turned on the stand.
		PlayCue(LooterSoundCue::Denied);
		return true;
	default:
		return false;
	}
}

void ULoadoutWidget::WardActivate()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return;
	}
	if (CursorIndex == 0)
	{
		// The ward worn: off to the spares to choose its replacement (or with none worn, the first spare goes on).
		if (Shield->GetSpares().IsEmpty())
		{
			PlayCue(LooterSoundCue::Denied);
			return;
		}
		if (!Shield->HasWard())
		{
			bool bWorn = false;
			{
				TGuardValue<bool> RefreshAfter(bApplyingAction, true);
				bWorn = Shield->EquipSpare(0);
			}
			Refresh();
			FlashRow(EZone::Wards, 0);
			PlayCue(bWorn ? Sounds::Equip : LooterSoundCue::Denied);
			return;
		}
		PlayCue(LooterSoundCue::Click);
		MoveCursorTo(EZone::Wards, 1, false, false);
		return;
	}
	bool bWorn = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bWorn = Shield->EquipSpare(CursorIndex - 1);
	}
	if (bWorn)
	{
		// The cursor follows the ward to the worn row: its card greets it there.
		CursorIndex = 0;
	}
	Refresh();
	if (bWorn)
	{
		FlashRow(EZone::Wards, 0);
	}
	PlayCue(bWorn ? Sounds::Equip : LooterSoundCue::Denied);
}

void ULoadoutWidget::WardStow()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield || CursorIndex != 0)
	{
		return;
	}
	bool bStowed = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bStowed = Shield->StowWard();
	}
	Refresh();
	PlayCue(bStowed ? Sounds::Stow : LooterSoundCue::Denied);
}

void ULoadoutWidget::WardDrop()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return;
	}
	bool bDropped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bDropped = CursorIndex == 0 ? Shield->DropWard() : Shield->DropSpare(CursorIndex - 1);
	}
	Refresh();
	PlayCue(bDropped ? Sounds::Drop : LooterSoundCue::Denied);
}

void ULoadoutWidget::HandleWardHovered(ULooterButton* Button)
{
	if (Button && !bInspecting)
	{
		MoveCursorTo(EZone::Wards, Button->Index, false, false);
	}
}

void ULoadoutWidget::HandleWardClicked(ULooterButton* Button)
{
	HandleWardHovered(Button);
	WardActivate();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the screen's keys.
	SetKeyboardFocus();
}
