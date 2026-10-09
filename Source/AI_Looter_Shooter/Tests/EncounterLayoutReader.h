#pragma once

// Ransom's Rest's layout as the encounter tests read it (Art/Levels/RansomsRest/layout.json): its boundary, zones,
// obstacles and features, and gameplay.encounters (the keep-outs, the story's fight grounds, and the camps, patrols and
// ambushes between them), seen from above in layout cm (X north, Y east). EncounterCampTests.cpp checks them.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace EncounterLayout
{
	/** The main missions in order: a fight during one is over once a later one is turned in. */
	inline const TArray<FName>& MainOrder()
	{
		static const TArray<FName> Order = { TEXT("Main1"), TEXT("Main2"), TEXT("Main3"), TEXT("Main4"), TEXT("Main5"), TEXT("Main6"), TEXT("Main7") };
		return Order;
	}

	inline TArray<FVector2D> ReadPoints(const TArray<TSharedPtr<FJsonValue>>& Values)
	{
		TArray<FVector2D> Points;
		for (const TSharedPtr<FJsonValue>& Value : Values)
		{
			const TArray<TSharedPtr<FJsonValue>>& XY = Value->AsArray();
			if (XY.Num() >= 2)
			{
				Points.Emplace(XY[0]->AsNumber(), XY[1]->AsNumber());
			}
		}
		return Points;
	}

	inline bool IsInside(const TArray<FVector2D>& Polygon, const FVector2D& Point)
	{
		bool bInside = false;
		for (int32 Index = 0, Previous = Polygon.Num() - 1; Index < Polygon.Num(); Previous = Index++)
		{
			const FVector2D& A = Polygon[Index];
			const FVector2D& B = Polygon[Previous];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	/** How far a point is from a line through Points (closed round to the first point when bClosed), seen from above. */
	inline double DistanceToLine(const TArray<FVector2D>& Points, const FVector2D& Point, bool bClosed)
	{
		const int32 Count = Points.Num();
		if (Count == 1)
		{
			return FVector2D::Distance(Point, Points[0]);
		}
		double Best = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index + 1 < Count + (bClosed ? 1 : 0); ++Index)
		{
			const FVector2D A = Points[Index];
			const FVector2D Along = Points[(Index + 1) % Count] - A;
			const double LengthSquared = Along.SizeSquared();
			const double T = LengthSquared > 0.0 ? FMath::Clamp(FVector2D::DotProduct(Point - A, Along) / LengthSquared, 0.0, 1.0) : 0.0;
			Best = FMath::Min(Best, FVector2D::Distance(Point, A + Along * T));
		}
		return Best;
	}

	inline FVector2D Centroid(const TArray<FVector2D>& Points)
	{
		FVector2D Sum = FVector2D::ZeroVector;
		for (const FVector2D& Point : Points)
		{
			Sum += Point;
		}
		return Points.Num() > 0 ? Sum / Points.Num() : Sum;
	}

	/** A ground seen from above: a polygon, or a radius round a middle. */
	struct FAreaShape
	{
		TArray<FVector2D> Polygon;
		FVector2D Middle = FVector2D::ZeroVector;
		double Radius = 0.0;

		/** How far a point is from it (cm): negative inside. */
		double Distance(const FVector2D& Point) const
		{
			if (Polygon.Num() >= 3)
			{
				const double Edge = DistanceToLine(Polygon, Point, true);
				return IsInside(Polygon, Point) ? -Edge : Edge;
			}
			return FVector2D::Distance(Point, Middle) - Radius;
		}

		/** Points round its edge, for overlaps. */
		TArray<FVector2D> Samples() const
		{
			if (Polygon.Num() >= 3)
			{
				return Polygon;
			}
			TArray<FVector2D> Made;
			for (int32 Step = 0; Step < 16; ++Step)
			{
				const double Angle = 2.0 * UE_DOUBLE_PI * Step / 16.0;
				Made.Add(Middle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			return Made;
		}
	};

	/** A layout obstacle or feature: its id, kind, points (a polygon, or a path with a width), and for a creek its bottom. */
	struct FAreaPiece
	{
		FString Id;
		FString Kind;
		TArray<FVector2D> Points;
		bool bPolygon = false;
		double Width = 0.0;
		double BottomWidth = 0.0;
	};

	/** A scripted fight's ground and when it can happen: during a mission, or from a mission on for good (a roaming group). */
	struct FAreaFight
	{
		FString Id;
		FAreaShape Ground;
		FName During;
		FName After;
	};

	/** One camp, patrol or ambush, as the layout has it (a camp's middle is among its spots). */
	struct FAreaEncounter
	{
		FString Id;
		FString Section;
		TArray<FName> After;
		TArray<FVector2D> Spots;
		TArray<FVector2D> Crates;
		TArray<FVector2D> Route;
		FAreaShape Ground;
		/** The obstacle its ground is (the churchyard's fence, the Webwood): it stands in it by design. */
		FString OwnObstacle;
		int32 Creatures = 0;
		bool bLeader = false;
	};

	struct FAreaLayout
	{
		TArray<FVector2D> Boundary;
		TMap<FString, TArray<FVector2D>> Zones;
		TArray<FAreaPiece> Obstacles;
		TArray<FAreaPiece> Features;
		TArray<FAreaShape> KeepOut;
		TArray<FString> KeepOutNames;
		TArray<FAreaFight> Fights;
		TArray<FAreaEncounter> Encounters;

		const FAreaPiece* Obstacle(const FString& Id) const { return Obstacles.FindByPredicate([&Id](const FAreaPiece& Each) { return Each.Id == Id; }); }
		const FAreaPiece* Feature(const FString& Id) const { return Features.FindByPredicate([&Id](const FAreaPiece& Each) { return Each.Id == Id; }); }
	};

	inline FAreaPiece ReadPiece(const TSharedPtr<FJsonObject>& Object)
	{
		FAreaPiece Piece;
		Object->TryGetStringField(TEXT("id"), Piece.Id);
		if (!Object->TryGetStringField(TEXT("kind"), Piece.Kind))
		{
			Object->TryGetStringField(TEXT("type"), Piece.Kind);
		}
		const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
		Piece.bPolygon = Object->TryGetArrayField(TEXT("polygon"), Points);
		if (Piece.bPolygon || Object->TryGetArrayField(TEXT("path"), Points))
		{
			Piece.Points = ReadPoints(*Points);
		}
		Object->TryGetNumberField(TEXT("width"), Piece.Width);
		const TSharedPtr<FJsonObject>* Bottom = nullptr;
		if (Object->TryGetObjectField(TEXT("bottom"), Bottom))
		{
			(*Bottom)->TryGetNumberField(TEXT("width"), Piece.BottomWidth);
		}
		return Piece;
	}

	/** A shape from a layout entry: its own polygon, an obstacle's or a zone's, the middle between two obstacles, a point. */
	inline FAreaShape ReadShape(const TSharedPtr<FJsonObject>& Object, const FAreaLayout& Layout, const FVector2D& DefaultMiddle)
	{
		FAreaShape Shape;
		Shape.Middle = DefaultMiddle;
		const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
		FString Id;
		if (Object->TryGetArrayField(TEXT("polygon"), Points))
		{
			Shape.Polygon = ReadPoints(*Points);
		}
		else if (Object->TryGetStringField(TEXT("obstacle"), Id) && Layout.Obstacle(Id))
		{
			Shape.Polygon = Layout.Obstacle(Id)->Points;
		}
		else if (Object->TryGetStringField(TEXT("zone"), Id) && Layout.Zones.Contains(Id))
		{
			Shape.Polygon = Layout.Zones[Id];
		}
		if (Object->TryGetArrayField(TEXT("at"), Points) && Points->Num() >= 2)
		{
			Shape.Middle = FVector2D((*Points)[0]->AsNumber(), (*Points)[1]->AsNumber());
		}
		if (Object->TryGetArrayField(TEXT("between"), Points) && Points->Num() == 2)
		{
			const FAreaPiece* First = Layout.Obstacle((*Points)[0]->AsString());
			const FAreaPiece* Second = Layout.Obstacle((*Points)[1]->AsString());
			if (First && Second)
			{
				Shape.Middle = (Centroid(First->Points) + Centroid(Second->Points)) * 0.5;
			}
		}
		Object->TryGetNumberField(TEXT("radius"), Shape.Radius);
		return Shape;
	}

	/** A corridor Width cm either side of a route and past its ends (as build_area_camps.corridor makes it). */
	inline TArray<FVector2D> Corridor(const TArray<FVector2D>& Route, double Width)
	{
		TArray<FVector2D> Left;
		TArray<FVector2D> Right;
		for (int32 Index = 0; Index < Route.Num(); ++Index)
		{
			const bool bLast = Index == Route.Num() - 1;
			const FVector2D Along = (bLast ? Route[Index] - Route[Index - 1] : Route[Index + 1] - Route[Index]).GetSafeNormal();
			const FVector2D Side(-Along.Y, Along.X);
			const FVector2D End = Index == 0 ? -Along * Width : bLast ? Along * Width : FVector2D::ZeroVector;
			Left.Add(Route[Index] + Side * Width + End);
			Right.Insert(Route[Index] - Side * Width + End, 0);
		}
		Left.Append(Right);
		return Left;
	}

	/** One camp, patrol or ambush entry of gameplay.encounters. */
	inline FAreaEncounter ReadEncounter(const TSharedPtr<FJsonObject>& Object, const TCHAR* Section, const FAreaLayout& Layout)
	{
		FAreaEncounter Made;
		Made.Id = Object->GetStringField(TEXT("id"));
		Made.Section = Section;
		const TArray<TSharedPtr<FJsonValue>>* Field = nullptr;
		if (Object->TryGetArrayField(TEXT("after"), Field))
		{
			for (const TSharedPtr<FJsonValue>& Mission : *Field)
			{
				Made.After.Add(FName(*Mission->AsString()));
			}
		}
		if (Object->TryGetArrayField(TEXT("spots"), Field))
		{
			Made.Spots = ReadPoints(*Field);
		}
		if (Object->TryGetArrayField(TEXT("route"), Field))
		{
			Made.Route = ReadPoints(*Field);
		}
		if (Object->TryGetArrayField(TEXT("crate"), Field))
		{
			Made.Crates = ReadPoints(*Field);
		}
		if (Object->TryGetArrayField(TEXT("groups"), Field))
		{
			for (const TSharedPtr<FJsonValue>& Group : *Field)
			{
				Made.Creatures += static_cast<int32>(Group->AsObject()->GetNumberField(TEXT("count")));
				const FString Rank = Group->AsObject()->GetStringField(TEXT("rank"));
				Made.bLeader |= Rank == TEXT("Rare") || Rank == TEXT("Epic");
			}
		}
		FVector2D Middle = Made.Spots.Num() > 0 ? Centroid(Made.Spots) : FVector2D::ZeroVector;
		if (Object->TryGetArrayField(TEXT("center"), Field) && Field->Num() >= 2)
		{
			Middle = FVector2D((*Field)[0]->AsNumber(), (*Field)[1]->AsNumber());
			Made.Spots.Add(Middle);
		}
		const TSharedPtr<FJsonObject>* Ground = nullptr;
		if (Made.Route.Num() >= 2)
		{
			Made.Ground.Polygon = Corridor(Made.Route, Object->GetNumberField(TEXT("corridor")));
		}
		else if (Object->TryGetObjectField(TEXT("ground"), Ground))
		{
			Made.Ground = ReadShape(*Ground, Layout, Middle);
			(*Ground)->TryGetStringField(TEXT("obstacle"), Made.OwnObstacle);
		}
		return Made;
	}

	/** Reads the layout; false (with the test's errors) when it can't. */
	inline bool ReadLayout(FAutomationTestBase& Test, FAreaLayout& Layout)
	{
		FString Text;
		const FString LayoutFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Levels/RansomsRest/layout.json"));
		TSharedPtr<FJsonObject> Root;
		if (!Test.TestTrue(TEXT("layout.json reads"), FFileHelper::LoadFileToString(Text, *LayoutFile))
			|| !Test.TestTrue(TEXT("...as JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid()))
		{
			return false;
		}
		const TSharedPtr<FJsonObject>* Boundary = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (Root->TryGetObjectField(TEXT("boundary"), Boundary) && (*Boundary)->TryGetArrayField(TEXT("polygon"), List))
		{
			Layout.Boundary = ReadPoints(*List);
		}
		if (Root->TryGetArrayField(TEXT("zones"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				const FAreaPiece Zone = ReadPiece(Each->AsObject());
				Layout.Zones.Add(Zone.Id, Zone.Points);
			}
		}
		if (Root->TryGetArrayField(TEXT("obstacles"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				Layout.Obstacles.Add(ReadPiece(Each->AsObject()));
			}
		}
		if (Root->TryGetArrayField(TEXT("features"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				Layout.Features.Add(ReadPiece(Each->AsObject()));
			}
		}
		const TSharedPtr<FJsonObject>* Gameplay = nullptr;
		const TSharedPtr<FJsonObject>* Encounters = nullptr;
		if (!Test.TestTrue(TEXT("...with gameplay.encounters"), Root->TryGetObjectField(TEXT("gameplay"), Gameplay)
			&& (*Gameplay)->TryGetObjectField(TEXT("encounters"), Encounters)))
		{
			return false;
		}
		if ((*Encounters)->TryGetArrayField(TEXT("keepOut"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				Layout.KeepOut.Add(ReadShape(Each->AsObject(), Layout, FVector2D::ZeroVector));
				Layout.KeepOutNames.Add(Each->AsObject()->GetStringField(TEXT("zone")));
			}
		}
		if ((*Encounters)->TryGetArrayField(TEXT("storyFights"), List))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				const TSharedPtr<FJsonObject> Object = Each->AsObject();
				FAreaFight& Fight = Layout.Fights.AddDefaulted_GetRef();
				Fight.Id = Object->GetStringField(TEXT("id"));
				Fight.Ground = ReadShape(Object, Layout, FVector2D::ZeroVector);
				FString Mission;
				Fight.During = Object->TryGetStringField(TEXT("during"), Mission) ? FName(*Mission) : NAME_None;
				Fight.After = Object->TryGetStringField(TEXT("after"), Mission) ? FName(*Mission) : NAME_None;
			}
		}
		for (const TCHAR* Section : { TEXT("camps"), TEXT("patrols"), TEXT("ambushes") })
		{
			if ((*Encounters)->TryGetArrayField(Section, List))
			{
				for (const TSharedPtr<FJsonValue>& Each : *List)
				{
					Layout.Encounters.Add(ReadEncounter(Each->AsObject(), Section, Layout));
				}
			}
		}
		return Test.TestTrue(TEXT("...with a boundary, zones and encounters"), Layout.Boundary.Num() >= 3 && Layout.Zones.Num() > 0
			&& Layout.Encounters.Num() > 0);
	}

	/** Whether a fight's ground may be in use while an encounter that comes on after these missions is. */
	inline bool MayOverlapInTime(const TArray<FName>& After, const FAreaFight& Fight)
	{
		if (!Fight.After.IsNone())
		{
			// A roaming group's ground is in use from then on.
			return true;
		}
		const int32 FightOrder = MainOrder().IndexOfByKey(Fight.During);
		if (FightOrder == INDEX_NONE)
		{
			// A side mission's fight can come any time its mission is open.
			return true;
		}
		return !After.ContainsByPredicate([FightOrder](const FName& Mission) { return MainOrder().IndexOfByKey(Mission) >= FightOrder; });
	}
}

#endif
