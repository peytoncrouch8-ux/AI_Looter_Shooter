#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Ransom's Rest's spider lurks as the layout has them (gameplay.encounters.lurks; Tools/Unreal/build_area_lurks.py places
// them; the spiders' ambush, 2026-10-31): each waits for the story, is a few spiders on perches of their own (walls, ledges,
// trees) at believable heights, brought out inside the boundary (perches hang on its walls by design), out of the safe zones
// and keep-outs, off the story's fights' ground while they can happen, and sharing ground with no other encounter that can be
// on at the same time (the Webwood's lurk comes only once its set piece is won). Read from the layout itself, as
// Looter.Encounters.Camps.Placement reads the camps (EncounterLayoutReader.h).

#include "Tests/EncounterLayoutReader.h"

using namespace EncounterLayout;

namespace
{
	/** Spots inside the boundary at least this far from its edge (cm): less than a camp's 3 m, the walls they hang on are its edge. */
	constexpr double LurkBoundaryClear = 150.0;
	constexpr double KeepOutClear = 200.0;
	constexpr double FightClear = 300.0;
	/** How far out from a wall (or a trunk) a spider is brought out when the layout doesn't say (cm): build_area_lurks.py's. */
	constexpr double WallOut = 250.0;
	constexpr double TreeOut = 200.0;
	/** A perch's height over the ground in front of it (cm): over a player's head, under a spider's leap. */
	constexpr double LowestPerch = 150.0;
	constexpr double HighestPerch = 600.0;

	TSharedPtr<FJsonObject> ReadEncountersJson(FAutomationTestBase& Test)
	{
		FString Text;
		TSharedPtr<FJsonObject> Root;
		const FString LayoutFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Levels/RansomsRest/layout.json"));
		if (!FFileHelper::LoadFileToString(Text, *LayoutFile) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			Test.AddError(TEXT("layout.json doesn't read"));
			return nullptr;
		}
		const TSharedPtr<FJsonObject>* Gameplay = nullptr;
		const TSharedPtr<FJsonObject>* Encounters = nullptr;
		if (!Root->TryGetObjectField(TEXT("gameplay"), Gameplay) || !(*Gameplay)->TryGetObjectField(TEXT("encounters"), Encounters))
		{
			Test.AddError(TEXT("layout.json has no gameplay.encounters"));
			return nullptr;
		}
		return *Encounters;
	}

	TArray<FName> NamesOf(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		TArray<FName> Names;
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (Object.IsValid() && Object->TryGetArrayField(Field, List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				Names.Add(FName(*Each->AsString()));
			}
		}
		return Names;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderLurkPlacementTest, "Looter.Creatures.Spider.Ambush.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderLurkPlacementTest::RunTest(const FString& Parameters)
{
	FAreaLayout Layout;
	if (!ReadLayout(*this, Layout))
	{
		return false;
	}
	const TSharedPtr<FJsonObject> Encounters = ReadEncountersJson(*this);
	if (!Encounters.IsValid())
	{
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Lurks = nullptr;
	if (!Encounters->TryGetArrayField(TEXT("lurks"), Lurks) || Lurks->IsEmpty())
	{
		AddWarning(TEXT("No gameplay.encounters.lurks yet (apply Art/Levels/RansomsRest/patches/spider_lurks.json): nothing to check."));
		return true;
	}
	// The set pieces' done flags by ambush id: a lurk on only with that flag is never on with that set piece.
	TMap<FString, FName> DoneFlags;
	const TArray<TSharedPtr<FJsonValue>>* Ambushes = nullptr;
	if (Encounters->TryGetArrayField(TEXT("ambushes"), Ambushes))
	{
		for (const TSharedPtr<FJsonValue>& Each : *Ambushes)
		{
			const TSharedPtr<FJsonObject>* SetPiece = nullptr;
			FString Flag;
			if (Each->AsObject()->TryGetObjectField(TEXT("setPiece"), SetPiece) && (*SetPiece)->TryGetStringField(TEXT("doneFlag"), Flag))
			{
				DoneFlags.Add(Each->AsObject()->GetStringField(TEXT("id")), FName(*Flag));
			}
		}
	}

	TArray<FAreaShape> LurkGrounds;
	TArray<FString> LurkIds;
	for (const TSharedPtr<FJsonValue>& Value : *Lurks)
	{
		const TSharedPtr<FJsonObject> Lurk = Value->AsObject();
		const FString Id = Lurk->GetStringField(TEXT("id"));
		const TSharedPtr<FJsonObject>* When = nullptr;
		Lurk->TryGetObjectField(TEXT("when"), When);
		const TArray<FName> After = When ? NamesOf(*When, TEXT("after")) : TArray<FName>();
		const TArray<FName> WithFlags = When ? NamesOf(*When, TEXT("withFlags")) : TArray<FName>();
		TestTrue(FString::Printf(TEXT("%s waits for the story (after a mission)"), *Id), After.Num() > 0);

		int32 Spiders = 0;
		bool bAllSpiders = true;
		for (const TSharedPtr<FJsonValue>& Group : Lurk->GetArrayField(TEXT("groups")))
		{
			Spiders += static_cast<int32>(Group->AsObject()->GetNumberField(TEXT("count")));
			bAllSpiders &= Group->AsObject()->GetStringField(TEXT("class")) == TEXT("SpiderCreature");
		}
		TestTrue(FString::Printf(TEXT("%s is a few brown spiders (%d)"), *Id, Spiders), bAllSpiders && Spiders >= 2 && Spiders <= 4);

		const TArray<TSharedPtr<FJsonValue>> Perches = Lurk->GetArrayField(TEXT("perches"));
		TestTrue(FString::Printf(TEXT("%s has a perch for each spider (%d for %d)"), *Id, Perches.Num(), Spiders), Perches.Num() >= Spiders);
		TArray<FVector2D> Spots;
		for (const TSharedPtr<FJsonValue>& PerchValue : Perches)
		{
			const TSharedPtr<FJsonObject> Perch = PerchValue->AsObject();
			const FString Kind = Perch->GetStringField(TEXT("kind"));
			TestTrue(FString::Printf(TEXT("%s's perch is a wall, ledge or tree (%s)"), *Id, *Kind),
				Kind == TEXT("Wall") || Kind == TEXT("Ledge") || Kind == TEXT("Tree"));
			const TArray<TSharedPtr<FJsonValue>> At = Perch->GetArrayField(TEXT("at"));
			double Face = 0.0;
			const bool bFace = Perch->TryGetNumberField(TEXT("face"), Face);
			TestTrue(FString::Printf(TEXT("%s's perch has its place and its way"), *Id), At.Num() >= 2 && bFace);
			if (At.Num() < 2)
			{
				continue;
			}
			double Up = 300.0;
			Perch->TryGetNumberField(TEXT("up"), Up);
			if (Kind != TEXT("Ledge"))
			{
				TestTrue(FString::Printf(TEXT("%s's perch is over the player's head, in a leap's reach (%.0f cm up)"), *Id, Up),
					Up >= LowestPerch && Up <= HighestPerch);
			}
			double Out = Kind == TEXT("Tree") ? TreeOut : WallOut;
			Perch->TryGetNumberField(TEXT("out"), Out);
			const double Radians = FMath::DegreesToRadians(Face);
			Spots.Add(FVector2D(At[0]->AsNumber() + FMath::Cos(Radians) * Out, At[1]->AsNumber() + FMath::Sin(Radians) * Out));
		}
		const FAreaShape Ground = Lurk->HasField(TEXT("ground")) ? ReadShape(Lurk->GetObjectField(TEXT("ground")), Layout, Centroid(Spots)) : FAreaShape();
		TestTrue(FString::Printf(TEXT("%s has its ground"), *Id), Ground.Polygon.Num() >= 3 || Ground.Radius > 0.0);
		for (const FVector2D& Spot : Spots)
		{
			const FString Where = FString::Printf(TEXT("%s's spot (%.0f, %.0f)"), *Id, Spot.X, Spot.Y);
			TestTrue(FString::Printf(TEXT("%s is inside the boundary, 1.5 m from its edge"), *Where),
				IsInside(Layout.Boundary, Spot) && DistanceToLine(Layout.Boundary, Spot, true) >= LurkBoundaryClear);
			for (int32 Zone = 0; Zone < Layout.KeepOut.Num(); ++Zone)
			{
				TestTrue(FString::Printf(TEXT("%s keeps out of %s"), *Where, *Layout.KeepOutNames[Zone]), Layout.KeepOut[Zone].Distance(Spot) >= KeepOutClear);
			}
			for (const FAreaFight& Fight : Layout.Fights)
			{
				if (MayOverlapInTime(After, Fight))
				{
					TestTrue(FString::Printf(TEXT("%s is off %s's ground"), *Where, *Fight.Id), Fight.Ground.Distance(Spot) >= FightClear);
				}
			}
			TestTrue(FString::Printf(TEXT("%s is on its own ground"), *Where), Ground.Distance(Spot) <= 0.0);
		}
		// Its ground is its own: no camp, patrol or ambush that can be on with it shares it, nor another lurk.
		auto Apart = [&Ground](const FAreaShape& Other)
		{
			bool bApart = true;
			for (const FVector2D& Corner : Ground.Samples())
			{
				bApart &= Other.Distance(Corner) > 0.0;
			}
			for (const FVector2D& Corner : Other.Samples())
			{
				bApart &= Ground.Distance(Corner) > 0.0;
			}
			return bApart;
		};
		for (const FAreaEncounter& Other : Layout.Encounters)
		{
			const FName* Done = DoneFlags.Find(Other.Id);
			if (Done && WithFlags.Contains(*Done))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("%s and %s don't share ground"), *Id, *Other.Id), Apart(Other.Ground));
		}
		for (int32 Index = 0; Index < LurkGrounds.Num(); ++Index)
		{
			TestTrue(FString::Printf(TEXT("%s and %s don't share ground"), *Id, *LurkIds[Index]), Apart(LurkGrounds[Index]));
		}
		LurkGrounds.Add(Ground);
		LurkIds.Add(Id);
	}
	AddInfo(FString::Printf(TEXT("%d spider lurks checked."), LurkIds.Num()));
	return true;
}

#endif
