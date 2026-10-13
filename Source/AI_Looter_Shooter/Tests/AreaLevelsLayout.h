#pragma once

// Ransom's Rest's creature levels as its layout gives them (Art/Levels/RansomsRest/layout.json gameplay.levels): the band,
// the zones (each a layout zone, an obstacle, its own polygon or a radius round a point, read as EncounterLayout reads a
// fight's ground) and the kinds at set levels, made into a transient UAreaDefinition the way
// Tools/Unreal/create_area_assets.py writes DA_Area_RansomsRest. LevelBandTests.cpp checks the asset against it and the
// spots' levels; ProgressionPacingTests.cpp plays the story on it.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/EncounterLayoutReader.h"
#include "UObject/Package.h"
#include "UObject/TopLevelAssetPath.h"

namespace AreaLevelsLayout
{
	/** A spot on a fight or a zone of the layout, seen from above: a polygon's middle, or a circle's. */
	inline FVector2D MiddleOf(const EncounterLayout::FAreaShape& Shape)
	{
		return Shape.Polygon.Num() >= 3 ? EncounterLayout::Centroid(Shape.Polygon) : Shape.Middle;
	}

	/**
	 * Where a named place of the layout stands: a story fight's ground (gameplay.encounters.storyFights: "BluffNest",
	 * "Abel") or a layout zone ("sink"). False when the layout has neither.
	 */
	inline bool FindPlace(const EncounterLayout::FAreaLayout& Layout, const FString& Name, FVector& OutSpot)
	{
		if (const EncounterLayout::FAreaFight* Fight = Layout.Fights.FindByPredicate([&Name](const EncounterLayout::FAreaFight& Each) { return Each.Id == Name; }))
		{
			const FVector2D Middle = MiddleOf(Fight->Ground);
			OutSpot = FVector(Middle.X, Middle.Y, 0.0);
			return true;
		}
		if (const TArray<FVector2D>* Zone = Layout.Zones.Find(Name))
		{
			const FVector2D Middle = EncounterLayout::Centroid(*Zone);
			OutSpot = FVector(Middle.X, Middle.Y, 0.0);
			return true;
		}
		return false;
	}

	/** Where one camp, patrol or ambush stands: the middle of its spots (a camp's middle among them), or of its route. */
	inline FVector EncounterSpot(const EncounterLayout::FAreaEncounter& Encounter)
	{
		const FVector2D Middle = EncounterLayout::Centroid(Encounter.Spots.Num() > 0 ? Encounter.Spots : Encounter.Route);
		return FVector(Middle.X, Middle.Y, 0.0);
	}

	/**
	 * The layout's gameplay.levels as a transient area (no promotions: they're create_area_assets.py's own numbers), or null
	 * with the test's errors. Layout is the same file read by EncounterLayout::ReadLayout, for the zones' shapes.
	 */
	inline UAreaDefinition* ReadArea(FAutomationTestBase& Test, const EncounterLayout::FAreaLayout& Layout)
	{
		FString Text;
		TSharedPtr<FJsonObject> Root;
		const TSharedPtr<FJsonObject>* Gameplay = nullptr;
		const TSharedPtr<FJsonObject>* Levels = nullptr;
		const FString LayoutFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Levels/RansomsRest/layout.json"));
		if (!Test.TestTrue(TEXT("layout.json reads with gameplay.levels"), FFileHelper::LoadFileToString(Text, *LayoutFile)
			&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid()
			&& Root->TryGetObjectField(TEXT("gameplay"), Gameplay) && (*Gameplay)->TryGetObjectField(TEXT("levels"), Levels)))
		{
			return nullptr;
		}
		UAreaDefinition* Area = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (!Test.TestTrue(TEXT("...with its band"), (*Levels)->TryGetArrayField(TEXT("band"), List) && List->Num() == 2))
		{
			return nullptr;
		}
		Area->MinLevel = static_cast<int32>((*List)[0]->AsNumber());
		Area->MaxLevel = static_cast<int32>((*List)[1]->AsNumber());
		if ((*Levels)->TryGetArrayField(TEXT("zones"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				const TSharedPtr<FJsonObject> Object = Each->AsObject();
				const TArray<TSharedPtr<FJsonValue>>* Range = nullptr;
				FAreaLevelZone& Zone = Area->LevelZones.AddDefaulted_GetRef();
				Zone.Id = FName(*Object->GetStringField(TEXT("id")));
				Zone.Outline = EncounterLayout::ReadShape(Object, Layout, FVector2D::ZeroVector).Samples();
				if (Test.TestTrue(FString::Printf(TEXT("Zone %s has its levels"), *Zone.Id.ToString()),
					Object->TryGetArrayField(TEXT("levels"), Range) && Range->Num() == 2))
				{
					Zone.MinLevel = static_cast<int32>((*Range)[0]->AsNumber());
					Zone.MaxLevel = static_cast<int32>((*Range)[1]->AsNumber());
				}
			}
		}
		if ((*Levels)->TryGetArrayField(TEXT("kinds"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				const TSharedPtr<FJsonObject> Object = Each->AsObject();
				const FString ClassName = Object->GetStringField(TEXT("class"));
				UClass* Class = FindObject<UClass>(FTopLevelAssetPath(TEXT("/Script/AI_Looter_Shooter"), FName(*ClassName)));
				if (Test.TestTrue(FString::Printf(TEXT("%s, set at a level, is a creature class"), *ClassName),
					Class && Class->IsChildOf(ACreatureBase::StaticClass())))
				{
					FAreaKindLevel& Kind = Area->KindLevels.AddDefaulted_GetRef();
					Kind.Kind = Class;
					Kind.Level = static_cast<int32>(Object->GetNumberField(TEXT("level")));
				}
			}
		}
		return Area;
	}
}

#endif
