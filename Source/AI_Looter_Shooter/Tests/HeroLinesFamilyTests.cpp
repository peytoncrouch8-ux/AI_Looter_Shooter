#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/CreatureVoiceBarks.h"
#include "Bestiary/BestiaryEntry.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossTypes.h"
#include "Economy/EconomyRules.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Scenes/ColdOpen.h"
#include "Scenes/SitWithPa.h"
#include "Story/HeroLines.h"
#include "Story/HeroLineTable.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/HeroLinesTestKit.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "World/RoostRules.h"
#include "World/TownLifeRules.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/ActorComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/** Misses reported one by one; more are counted. */
	constexpr int32 MaxReported = 60;

	/** A text the player can be shown, and where it was found (for the failure). */
	struct FFoundText
	{
		FString Where;
		FText Text;
	};

	/** The game module's classes, and Blueprints made from them: the ones whose words are the story's. */
	bool IsGameClass(const UClass* Class)
	{
		while (Class && !Class->HasAnyClassFlags(CLASS_Native))
		{
			Class = Class->GetSuperClass();
		}
		return Class && Class->GetOutermost()->GetName() == TEXT("/Script/AI_Looter_Shooter");
	}

	/**
	 * Every FText of Data (an object or a struct of Type), through its structs and arrays, and through the game's own
	 * subobjects of Owner (a mission's objectives, an actor's speaker point): what the data asset or level says aloud.
	 */
	void CollectTexts(const UStruct* Type, const void* Data, const UObject* Owner, const FString& Where, TArray<FFoundText>& Out,
		TSet<const UObject*>& Seen, int32 Depth);

	void CollectValue(const FProperty* Property, const void* Value, const UObject* Owner, const FString& Where, TArray<FFoundText>& Out,
		TSet<const UObject*>& Seen, int32 Depth)
	{
		if (Depth > 12)
		{
			return;
		}
		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			const FText Text = TextProperty->GetPropertyValue(Value);
			if (!Text.IsEmpty())
			{
				Out.Add({ Where, Text });
			}
		}
		else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			CollectTexts(StructProperty->Struct, Value, Owner, Where, Out, Seen, Depth + 1);
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Array(ArrayProperty, Value);
			for (int32 Index = 0; Index < Array.Num(); ++Index)
			{
				CollectValue(ArrayProperty->Inner, Array.GetRawPtr(Index), Owner, FString::Printf(TEXT("%s[%d]"), *Where, Index), Out, Seen,
					Depth + 1);
			}
		}
		else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			const UObject* Child = ObjectProperty->GetObjectPropertyValue(Value);
			if (Child && Owner && Child->IsIn(Owner) && IsGameClass(Child->GetClass()) && !Seen.Contains(Child))
			{
				Seen.Add(Child);
				CollectTexts(Child->GetClass(), Child, Owner, Where + TEXT(".") + Child->GetName(), Out, Seen, Depth + 1);
			}
		}
	}

	void CollectTexts(const UStruct* Type, const void* Data, const UObject* Owner, const FString& Where, TArray<FFoundText>& Out,
		TSet<const UObject*>& Seen, int32 Depth)
	{
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			const FProperty* Property = *It;
			for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
			{
				CollectValue(Property, Property->ContainerPtrToValuePtr<void>(Data, Index), Owner, Where + TEXT(".") + Property->GetName(), Out,
					Seen, Depth);
			}
		}
	}

	void CollectObject(const UObject* Object, const FString& Where, TArray<FFoundText>& Out)
	{
		if (Object)
		{
			TSet<const UObject*> Seen;
			Seen.Add(Object);
			CollectTexts(Object->GetClass(), Object, Object, Where, Out, Seen, 0);
		}
	}

	void CollectLines(const TArray<FStoryLine>& Lines, const FString& Where, TArray<FFoundText>& Out)
	{
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			Out.Add({ FString::Printf(TEXT("%s[%d].Speaker"), *Where, Index), Lines[Index].Speaker });
			Out.Add({ FString::Printf(TEXT("%s[%d].Text"), *Where, Index), Lines[Index].Text });
		}
	}

	/** Every asset of Class (its folder scanned first while the editor is still finding its assets). */
	TArray<UObject*> LoadAllOf(const UClass* Class, const TCHAR* Folder)
	{
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		if (Registry.IsLoadingAssets())
		{
			Registry.ScanPathsSynchronous({ FString(Folder) });
		}
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(Class->GetClassPathName(), Assets, true);
		TArray<UObject*> Loaded;
		for (const FAssetData& Asset : Assets)
		{
			if (UObject* Object = Asset.GetAsset())
			{
				Loaded.Add(Object);
			}
		}
		return Loaded;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroLinesNoFamilyTest, "Looter.Lines.NoFamily",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroLinesNoFamilyTest::RunTest(const FString& Parameters)
{
	// What keeps new lines honest (the user's rule: one story; Pa is the Keeper, Delia the Keeper's mother): for each hero
	// but Ellis, every word the story can show reads without Ellis's family or Ellis's name. The line sets, every mission
	// (offer, turn-in, choices, objectives as shown, items, summary, briefing, givers), every Ledger page, the named guns,
	// Lvl_RansomsRest's placed story things (readables, the porch plate, speaker points, posters), the game's classes'
	// own defaults, and the C++ lines: the cold open, Sit with Pa, Abel's fight and phases, Hob's roosts, the barks, the
	// town's mutters, the heirloom that isn't for sale. A miss is a line that needs its row in Tools/Unreal/hero_lines.json
	// (or a key that drifted from Ellis's words).
	HeroLines::ForgetTable();
	if (!TestNotNull(TEXT("DA_HeroLines loads (Tools/Unreal/create_hero_lines.py makes it)"), HeroLines::GetTable()))
	{
		return false;
	}

	TArray<FFoundText> Texts;
	for (UObject* Set : LoadAllOf(UStoryLineSet::StaticClass(), TEXT("/Game/Data/Story")))
	{
		CollectObject(Set, Set->GetName(), Texts);
	}
	const TArray<UMissionDefinition*> Missions = UMissionDefinition::LoadAll();
	for (const UMissionDefinition* Mission : Missions)
	{
		CollectObject(Mission, Mission->GetName(), Texts);
	}
	for (const UBestiaryEntry* Entry : UBestiaryEntry::LoadAll())
	{
		CollectObject(Entry, Entry->GetName(), Texts);
	}
	const TArray<UNamedWeaponDefinition*> NamedGuns = UNamedWeaponDefinition::LoadAll();
	for (const UNamedWeaponDefinition* Named : NamedGuns)
	{
		CollectObject(Named, Named->GetName(), Texts);
	}
	const int32 FromData = Texts.Num();

	if (FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
	{
		const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
		const ULevel* Level = Map ? Map->PersistentLevel.Get() : nullptr;
		if (TestNotNull(TEXT("Lvl_RansomsRest loads"), Level))
		{
			for (const AActor* Actor : Level->Actors)
			{
				if (Actor && IsGameClass(Actor->GetClass()))
				{
					CollectObject(Actor, TEXT("Lvl_RansomsRest.") + Actor->GetActorNameOrLabel(), Texts);
				}
			}
		}
	}
	else
	{
		AddInfo(TEXT("Lvl_RansomsRest isn't built: its placed words are skipped."));
	}
	// The game's actors' and components' own defaults (lines a constructor sets: Calder's note, Hob's remarks), gathered
	// first: asking for a default object while iterating could make one.
	TArray<const UClass*> GameClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		const UClass* Class = *It;
		if (Class->HasAnyClassFlags(CLASS_Native) && !Class->HasAnyClassFlags(CLASS_NewerVersionExists | CLASS_Deprecated) && IsGameClass(Class)
			&& (Class->IsChildOf(AActor::StaticClass()) || Class->IsChildOf(UActorComponent::StaticClass())))
		{
			GameClasses.Add(Class);
		}
	}
	for (const UClass* Class : GameClasses)
	{
		CollectObject(Class->GetDefaultObject(), Class->GetName() + TEXT(" defaults"), Texts);
	}

	CollectLines(ColdOpen::Lines(), TEXT("ColdOpen::Lines"), Texts);
	CollectLines(SitWithPa::Lines(), TEXT("SitWithPa::Lines"), Texts);
	for (int32 Phase = 0; Phase < 3; ++Phase)
	{
		CollectLines({ AbelRules::PhaseLine(Phase) }, FString::Printf(TEXT("AbelRules::PhaseLine(%d)"), Phase), Texts);
	}
	CollectLines({ AbelRules::BellLine(), AbelRules::HobOnFall() }, TEXT("AbelRules::BellLine, HobOnFall"), Texts);
	const TArray<FBossPhase> Phases = AbelRules::MakePhases();
	for (int32 Phase = 0; Phase < Phases.Num(); ++Phase)
	{
		Texts.Add({ FString::Printf(TEXT("AbelRules::MakePhases[%d].Name"), Phase), Phases[Phase].Name });
	}
	for (int32 Death = 0; Death < RoostRules::NumDeathLines(); ++Death)
	{
		CollectLines({ RoostRules::DeathLine(Death) }, FString::Printf(TEXT("RoostRules::DeathLine(%d)"), Death), Texts);
	}
	CollectLines({ RoostRules::FirstDeathLine(), RoostRules::FirstRoostLine(), RoostRules::FirstTravelLine() }, TEXT("RoostRules::First*"), Texts);
	for (const TownLifeRules::FHousehold& Household : TownLifeRules::Households())
	{
		const TArray<FSpeakerTopic> Topics = TownLifeRules::MutterTopicsFor(Household.Id);
		for (int32 Topic = 0; Topic < Topics.Num(); ++Topic)
		{
			CollectLines(Topics[Topic].Lines, FString::Printf(TEXT("TownLifeRules %s mutters[%d]"), *Household.Id.ToString(), Topic), Texts);
		}
	}
	const TConstArrayView<FCreatureBarkLine> Barks = CreatureBarks::AllLines();
	for (int32 Bark = 0; Bark < Barks.Num(); ++Bark)
	{
		Texts.Add({ FString::Printf(TEXT("CreatureBarks::AllLines()[%d]"), Bark), FText::FromString(Barks[Bark].Text) });
	}
	AddInfo(FString::Printf(TEXT("%d texts: %d from the data assets, %d from the level, the defaults and the code."), Texts.Num(), FromData,
		Texts.Num() - FromData));

	const HeroLinesTestKit::FFamilyWords Family;
	int32 Missed = 0;
	auto Check = [this, &Family, &Missed](FName Hero, const FString& Where, const FString& Words)
	{
		const FString Word = Family.FindIn(Words);
		if (Word.IsEmpty())
		{
			return;
		}
		if (++Missed <= MaxReported)
		{
			AddError(FString::Printf(TEXT("%s reads \"%s\" (Ellis's) at %s: \"%s\""), *Hero.ToString(), *Word, *Where, *Words.Left(100)));
		}
	};
	for (const FName Hero : HeroLinesTestKit::OtherHeroes())
	{
		const HeroLines::FScopedHero AsHero(Hero);
		for (const FFoundText& Found : Texts)
		{
			Check(Hero, Found.Where, HeroLines::Resolve(Found.Text).ToString());
		}
		// What the game builds from the data as it shows it: the objectives' words, the givers, the heirlooms' refusal.
		for (const UMissionDefinition* Mission : Missions)
		{
			Check(Hero, Mission->GetName() + TEXT(" giver"), Mission->GetOfferGiverName().ToString());
			Check(Hero, Mission->GetName() + TEXT(" turn-in"), Mission->GetTurnInText());
			for (int32 Step = 0; Step < Mission->Steps.Num(); ++Step)
			{
				for (int32 Index = 0; Index < Mission->Steps[Step].Objectives.Num(); ++Index)
				{
					if (const UMissionObjective* Objective = Mission->GetObjective(Step, Index))
					{
						const FString Where = FString::Printf(TEXT("%s step %d objective %d"), *Mission->GetName(), Step + 1, Index + 1);
						Check(Hero, Where, Objective->GetDisplayText(nullptr));
						Check(Hero, Where + TEXT(" (short)"), Objective->GetShortText(nullptr));
					}
				}
			}
		}
		for (UNamedWeaponDefinition* Named : NamedGuns)
		{
			FWeaponInstanceData Heirloom;
			Heirloom.Definition = GetMutableDefault<UWeaponDefinition>();
			Heirloom.Named = Named;
			FText Why;
			LooterEconomy::CanSell(Heirloom, &Why);
			Check(Hero, Named->GetName() + TEXT(" not for sale"), Why.ToString());
		}
	}
	if (Missed > MaxReported)
	{
		AddError(FString::Printf(TEXT("...and %d more."), Missed - MaxReported));
	}
	return Missed == 0;
}

#endif
