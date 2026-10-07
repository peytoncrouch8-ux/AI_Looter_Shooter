// UBestiaryWidget: the chosen entry's side of the page (its card of numbers, its description and field notes), for each
// kind of page: an actor met in the world, a story character, a Ledger name. The list and the stand are BestiaryWidget.cpp's.

#include "UI/Bestiary/BestiaryWidget.h"
#include "Bestiary/BestiaryEntry.h"
#include "Bestiary/Ledger.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/Bestiary/BestiaryStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const TCHAR* const Unknown = TEXT("???");

	/** "Creature", "Enemy", "NPC", "Friend": the details header's word for one entry. */
	FString CategoryWord(EBestiaryCategory Category)
	{
		switch (Category)
		{
		case EBestiaryCategory::Creature: return TEXT("Creature");
		case EBestiaryCategory::Enemy:    return TEXT("Enemy");
		case EBestiaryCategory::NPC:      return TEXT("NPC");
		case EBestiaryCategory::Friend:   return TEXT("Friend");
		}
		return FString();
	}

	FString FormatWhole(float Value)
	{
		return FText::AsNumber(FMath::RoundToInt(Value)).ToString();
	}
}

void UBestiaryWidget::RefreshDetails()
{
	if (!DetailsBox)
	{
		return;
	}
	DetailsBox->ClearChildren();
	NotesBox->ClearChildren();
	const Ledger::FWords& Words = Ledger::Words(bLedger);
	const UBestiaryEntry* Entry = GetSelected();
	const ABestiaryStage* StagePtr = Stage.Get();
	StageHint->SetVisibility(StagePtr && StagePtr->HasModel() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (!Entry)
	{
		DetailsHeader->SetText(FText::FromString(Words.Empty.ToUpper()));
		DetailsBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("No entries yet"), 17, Color::TextDim(), 40));
		UTextBlock* Help = MakeText(WidgetTree, Words.EmptyHelp, 10, Color::TextDim());
		Help->SetAutoWrapText(true);
		DetailsBox->AddChildToVerticalBox(Help)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		return;
	}

	// Not met yet: only its section is known, and the page says how to fill it in.
	const bool bKnown = IsKnown(*Entry);
	bSelectedKnown = bKnown;
	const bool bName = Entry->Page == EBestiaryPage::LedgerName;
	const FString Kind = bKnown ? Entry->Kind.ToString() : FString();
	DetailsHeader->SetText(FText::FromString((Kind.IsEmpty() ? CategoryWord(Entry->Category) : CategoryWord(Entry->Category) + TEXT(" · ") + Kind).ToUpper()));

	UTextBlock* NameText = Label(WidgetTree, bKnown ? Entry->DisplayName.ToString() : FString(Unknown), 17, bKnown ? Color::Text() : Color::TextDim(), 40);
	NameText->SetAutoWrapText(true);
	DetailsBox->AddChildToVerticalBox(NameText);
	// Where it's found; a Ledger name's whereabouts are a line of their own below, blank until the story finds them.
	const FString Habitat = bName ? FString() : bKnown ? Entry->Habitat.ToString() : Words.NotMet;
	if (!Habitat.IsEmpty())
	{
		DetailsBox->AddChildToVerticalBox(Label(WidgetTree, Habitat, 9, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	auto AddStat = [this](const TCHAR* StatName, const FString& Value, bool bLit)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Label(WidgetTree, StatName, 8, Color::TextDim(), 120));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* ValueText = MakeText(WidgetTree, Value, 11, bLit ? Color::Text() : Color::TextDim());
		ValueText->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(ValueText)->SetVerticalAlignment(VAlign_Center);
		DetailsBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 24.f))->SetPadding(FMargin(0.f, 2.f));
	};
	DetailsBox->AddChildToVerticalBox(MakeSized(WidgetTree, nullptr, 0.f, 8.f));
	if (!bKnown)
	{
		// Something to meet out there has its numbers blank; someone of the story has none to show.
		if (Entry->NeedsActor())
		{
			for (const TCHAR* StatName : { TEXT("Level"), TEXT("Health"), TEXT("Attack"), TEXT("Experience") })
			{
				AddStat(StatName, Unknown, false);
			}
			AddStat(TEXT("Defeated"), TEXT("0"), false);
		}
		UTextBlock* Help = MakeText(WidgetTree, Entry->NeedsActor() ? Words.NotMetHelp : Words.NotMetPersonHelp, 10, Color::TextDim());
		Help->SetAutoWrapText(true);
		NotesBox->AddChildToVerticalBox(Help);
		return;
	}
	if (bName)
	{
		// The Ledger's names: where they are, once the Keeper's Lantern has found them.
		const bool bFound = IsFound(*Entry);
		AddStat(TEXT("Whereabouts"), bFound ? Entry->Habitat.ToString() : Words.BlankWhereabouts, bFound);
	}
	else if (Entry->NeedsActor())
	{
		// The numbers, read from the actor itself.
		const FBestiaryStats Stats = Entry->ReadStats();
		if (Stats.Level > 0)
		{
			AddStat(TEXT("Level"), FString::FromInt(Stats.Level), true);
		}
		if (Stats.bHasHealth)
		{
			AddStat(TEXT("Health"), FormatWhole(Stats.Health), true);
		}
		AddStat(TEXT("Attack"), Stats.bAttacks ? FString::Printf(TEXT("%s dmg"), *FormatWhole(Stats.AttackDamage)) : FString(TEXT("Harmless")), Stats.bAttacks);
		AddStat(TEXT("Experience"), Stats.XPReward > 0 ? FString::Printf(TEXT("%d XP"), Stats.XPReward) : FString(TEXT("None")), Stats.XPReward > 0);
		const int32 Defeated = GetDefeated(*Entry);
		AddStat(TEXT("Defeated"), FText::AsNumber(Defeated).ToString(), Defeated > 0);
	}

	// Under the card: the description, then the field notes.
	if (!Entry->Description.IsEmpty())
	{
		UTextBlock* Description = MakeText(WidgetTree, Entry->Description.ToString(), 10, Color::Text());
		Description->SetAutoWrapText(true);
		NotesBox->AddChildToVerticalBox(Description);
	}
	if (!Entry->Notes.IsEmpty())
	{
		NotesBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("Field notes"), 10, Color::Accent(), 300))->SetPadding(FMargin(0.f, 18.f, 0.f, 4.f));
		for (const FText& Note : Entry->Notes)
		{
			UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UHorizontalBoxSlot* BulletSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 5.f, 5.f));
			BulletSlot->SetVerticalAlignment(VAlign_Top);
			BulletSlot->SetPadding(FMargin(2.f, 6.f, 10.f, 0.f));
			UTextBlock* NoteText = MakeText(WidgetTree, Note.ToString(), 10, Color::Text());
			NoteText->SetAutoWrapText(true);
			Line->AddChildToHorizontalBox(NoteText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NotesBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 4.f, 8.f, 0.f));
		}
	}
}

bool UBestiaryWidget::IsFound(const UBestiaryEntry& Entry) const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner ? Entry.IsFound(Runner->GetCampaign(), Runner) : Entry.IsFound(FCampaignRecord());
}
