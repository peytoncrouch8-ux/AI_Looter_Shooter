#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryEntry.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBestiaryEntriesTest, "Looter.Bestiary.Entries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBestiaryEntriesTest::RunTest(const FString& Parameters)
{
	// Every page of the bestiary, now and later: written up; an actor page about an actor that exists, with a model for
	// the stand; a story's page (a story character, a Ledger name) with no actor at all, and any model it names there.
	const TArray<UBestiaryEntry*> Entries = UBestiaryEntry::LoadAll();
	TestTrue(TEXT("Has entries"), Entries.Num() >= 2);
	for (const UBestiaryEntry* Entry : Entries)
	{
		const FString Name = Entry->GetName();
		TestFalse(FString::Printf(TEXT("%s: has a name"), *Name), Entry->DisplayName.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s: has a description"), *Name), Entry->Description.IsEmpty());
		if (Entry->NeedsActor())
		{
			TestNotNull(FString::Printf(TEXT("%s: its actor class loads"), *Name), Entry->ActorClass.LoadSynchronous());
			TestNotNull(FString::Printf(TEXT("%s: has a model for the stand"), *Name), Entry->LoadPreviewMesh());
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: a story's page is about no actor"), *Name), Entry->ActorClass.IsNull());
		TestTrue(FString::Printf(TEXT("%s: the model it names loads"), *Name), Entry->PreviewMesh.IsNull() || Entry->LoadPreviewMesh());
		TestTrue(FString::Printf(TEXT("%s: the still model it names loads"), *Name),
			Entry->PreviewStaticMesh.IsNull() || Entry->LoadPreviewStaticMesh());
		for (const FBestiaryStillPart& Part : Entry->PreviewStillParts)
		{
			// Each still part loads and has its socket on the still model, or the stand leaves it off.
			const UStaticMesh* Base = Entry->LoadPreviewStaticMesh();
			TestTrue(FString::Printf(TEXT("%s: its still part on %s loads, and the stand model has that socket"), *Name, *Part.Socket.ToString()),
				Part.Mesh.LoadSynchronous() && Base && Base->FindSocket(Part.Socket));
		}
	}

	// Listed by section, in the sections' order, and within one by order.
	for (int32 Index = 1; Index < Entries.Num(); ++Index)
	{
		TestTrue(TEXT("Sorted by section"), Entries[Index - 1]->Category <= Entries[Index]->Category);
		TestTrue(TEXT("...then by order"), Entries[Index - 1]->Category != Entries[Index]->Category
			|| Entries[Index - 1]->SortOrder <= Entries[Index]->SortOrder);
	}

	// The spider's page reads its numbers from the spider itself.
	const UBestiaryEntry* const* Spider = Entries.FindByPredicate([](const UBestiaryEntry* Entry) { return Entry->Describes(ASpiderCreature::StaticClass()); });
	if (TestNotNull(TEXT("The brown spider has a page"), Spider))
	{
		const ASpiderCreature* Defaults = GetDefault<ASpiderCreature>();
		const FBestiaryStats Stats = (*Spider)->ReadStats();
		TestEqual(TEXT("Its level"), Stats.Level, Defaults->Level);
		TestEqual(TEXT("Its experience, as a kill pays it"), static_cast<int64>(Stats.XPReward),
			UPlayerProgressionSubsystem::GetLevelRules().KillXP(Defaults->XPReward, Defaults->Level, Defaults->Level));
		TestEqual(TEXT("Its attack"), Stats.AttackDamage, Defaults->AttackDamage);
		TestTrue(TEXT("Its health"), Stats.bHasHealth && Stats.Health > 0.f);
		TestEqual(TEXT("A creature"), (*Spider)->Category, EBestiaryCategory::Creature);
	}

	// Each section has a heading.
	for (const EBestiaryCategory Category : { EBestiaryCategory::Creature, EBestiaryCategory::Enemy, EBestiaryCategory::NPC, EBestiaryCategory::Friend })
	{
		TestFalse(TEXT("Section name"), UBestiaryEntry::CategoryName(Category).IsEmpty());
	}
	return true;
}

#endif
