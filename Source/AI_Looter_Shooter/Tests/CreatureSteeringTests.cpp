#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureSteerPlanner.h"
#include "Creatures/CreatureUnstick.h"

// The creature steering's planner and its unstick's rules in a flat world of their own, without physics: a creature (a
// disc moving as the walking movement moves a body, sliding along what it bumps into) crossing a field of fence lines,
// getting out of a U-shaped pen, and going round a rail its looks can't see. CreatureSteeringRulesTests.cpp has the
// unstick's timing and the side it keeps; CreatureSteeringPlayTests.cpp checks the looks and the unstick against real geometry.

namespace
{
	using FLookResult = FCreatureSteerPlanner::FLookResult;

	/** A wall of the flat world: a segment, Thickness either side. */
	struct FFlatWall
	{
		FVector2D A = FVector2D::ZeroVector;
		FVector2D B = FVector2D::ZeroVector;
		double Thickness = 5.0;
	};

	FFlatWall FlatWall(double AX, double AY, double BX, double BY)
	{
		FFlatWall Wall;
		Wall.A = FVector2D(AX, AY);
		Wall.B = FVector2D(BX, BY);
		return Wall;
	}

	FVector ToWorld(const FVector2D& Vector)
	{
		return FVector(Vector.X, Vector.Y, 0.0);
	}

	FVector2D ClosestOnSegment(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double Length2 = AB.SizeSquared();
		const double Share = Length2 < 1.e-9 ? 0.0 : FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / Length2, 0.0, 1.0);
		return A + AB * Share;
	}

	struct FFlatHit
	{
		double Time = 0.0;
		FVector2D Normal = FVector2D::ZeroVector;
		bool bStartedInside = false;
	};

	/** The first wall a disc of Radius meets going Distance from Point along Way (unit): the time, and the normal back at the disc. */
	bool SweepDisc(const TArray<FFlatWall>& Walls, const FVector2D& Point, const FVector2D& Way, double Distance, double Radius, FFlatHit& OutHit)
	{
		bool bFound = false;
		for (const FFlatWall& Wall : Walls)
		{
			const double Reach = Radius + Wall.Thickness;
			const FVector2D Closest = ClosestOnSegment(Point, Wall.A, Wall.B);
			if ((Point - Closest).Size() < Reach - 0.01)
			{
				if (!bFound || OutHit.Time > 0.0)
				{
					OutHit.Time = 0.0;
					OutHit.Normal = (Point - Closest).GetSafeNormal();
					OutHit.bStartedInside = true;
					bFound = true;
				}
				continue;
			}
			double First = TNumericLimits<double>::Max();
			const FVector2D AB = Wall.B - Wall.A;
			if (AB.Size() > 1.e-6)
			{
				// Its two long sides...
				const FVector2D Across = FVector2D(-AB.Y, AB.X).GetSafeNormal();
				const double Toward = FVector2D::DotProduct(Way, Across);
				if (FMath::Abs(Toward) > 1.e-9)
				{
					for (const double Sign : { 1.0, -1.0 })
					{
						const double Time = (Sign * Reach - FVector2D::DotProduct(Point - Wall.A, Across)) / Toward;
						if (Time >= 0.0 && Time <= Distance)
						{
							const double Along = FVector2D::DotProduct(Point + Way * Time - Wall.A, AB) / AB.SizeSquared();
							if (Along >= 0.0 && Along <= 1.0)
							{
								First = FMath::Min(First, Time);
							}
						}
					}
				}
			}
			// ...and its round ends.
			for (const FVector2D& End : { Wall.A, Wall.B })
			{
				const FVector2D FromEnd = Point - End;
				const double Half = FVector2D::DotProduct(Way, FromEnd);
				const double Rest = FromEnd.SizeSquared() - Reach * Reach;
				const double Discriminant = Half * Half - Rest;
				if (Discriminant >= 0.0)
				{
					const double Time = -Half - FMath::Sqrt(Discriminant);
					if (Time >= 0.0 && Time <= Distance)
					{
						First = FMath::Min(First, Time);
					}
				}
			}
			if (First <= Distance && (!bFound || First < OutHit.Time))
			{
				const FVector2D At = Point + Way * First;
				OutHit.Time = First;
				OutHit.Normal = (At - ClosestOnSegment(At, Wall.A, Wall.B)).GetSafeNormal();
				OutHit.bStartedInside = false;
				bFound = true;
			}
		}
		return bFound;
	}

	/** A look as the creature's probe takes it (FCreatureSteerProbe::Look), in the flat world. */
	FLookResult LookFlat(const TArray<FFlatWall>& Walls, const FVector2D& From, const FVector& Direction, float Distance, double LookRadius)
	{
		const FVector2D Way = FVector2D(Direction.X, Direction.Y).GetSafeNormal();
		FFlatHit Hit;
		if (!SweepDisc(Walls, From, Way, Distance, LookRadius, Hit))
		{
			return FLookResult::Clear(Distance);
		}
		if (Hit.bStartedInside)
		{
			if (FVector2D::DotProduct(Hit.Normal, Way) < -0.1)
			{
				return FLookResult::Blocked(0.f, ToWorld(Hit.Normal));
			}
			if (!SweepDisc(Walls, From, Way, Distance, LookRadius * 0.5, Hit)
				|| (Hit.bStartedInside && FVector2D::DotProduct(Hit.Normal, Way) >= -0.1))
			{
				return FLookResult::Clear(Distance);
			}
		}
		return FLookResult::Blocked(static_cast<float>(Hit.Time), ToWorld(Hit.Normal));
	}

	/** A creature's body and pace: a full-size spider by default. */
	struct FFlatCreature
	{
		double BodyRadius = 62.0;
		double LookRadius = 52.0;
		float Speed = 459.f;
		float LookLength = 220.f;
		bool bNear = true;
	};

	struct FFlatRun
	{
		bool bReached = false;
		float Seconds = 0.f;
		int32 SideFlips = 0;
		int32 Reversals = 0;
		int32 WallFollows = 0;
		int32 MostFeltWalls = 0;
		FVector2D End = FVector2D::ZeroVector;

		FString Describe() const
		{
			return FString::Printf(TEXT("%s in %.1f s, ending at (%.0f, %.0f): %d side changes, %d turns back, %d walls followed"),
				bReached ? TEXT("there") : TEXT("NOT there"), Seconds, End.X, End.Y, SideFlips, Reversals, WallFollows);
		}
	};

	/**
	 * Runs a creature from Start to within Stop of Goal for at most Seconds: its planner every 0.1 s on looks that see only
	 * Seen; its body stopped (and sliding) at Seen and Unseen alike, telling the planner of blocked moves as
	 * ACreatureBase::MoveBlockedBy does, and its unstick's rules noting when it's blocked.
	 */
	FFlatRun RunFlat(const TArray<FFlatWall>& Seen, const TArray<FFlatWall>& Unseen, const FVector2D& Start, const FVector2D& Goal,
		const FFlatCreature& Body, float Seconds, double Stop = 150.0)
	{
		constexpr float Frame = 1.f / 60.f;
		constexpr float SteerInterval = 0.1f;
		TArray<FFlatWall> Solid = Seen;
		Solid.Append(Unseen);
		FCreatureSteerPlanner Planner;
		FCreatureUnstick Unstick;
		FVector2D Here = Start;
		FVector Heading = FVector::ZeroVector;
		float SteerTimer = 0.f;
		float Clock = 0.f;
		FFlatRun Run;
		auto Ask = [&]()
		{
			FCreatureSteerPlanner::FRequest Request;
			Request.Here = ToWorld(Here);
			Request.Goal = ToWorld(Goal);
			Request.Desired = ToWorld(Goal - Here).GetSafeNormal();
			Request.GoalDistance = static_cast<float>((Goal - Here).Size());
			Request.LookLength = Body.LookLength;
			Request.LookRadius = static_cast<float>(Body.LookRadius);
			Request.Speed = Body.Speed;
			Request.bNear = Body.bNear;
			return Request;
		};
		auto Look = [&](const FVector& Direction, float Distance)
		{
			return LookFlat(Seen, Here, Direction, Distance, Body.LookRadius);
		};
		while (Clock < Seconds)
		{
			if ((Goal - Here).Size() < Stop)
			{
				Run.bReached = true;
				break;
			}
			Planner.Advance(Frame);
			SteerTimer -= Frame;
			if (SteerTimer <= 0.f)
			{
				SteerTimer = SteerInterval;
				Heading = Planner.Plan(Ask(), Look);
			}
			const FVector2D Was = Here;
			FVector2D Move = FVector2D(Heading.X, Heading.Y) * (Body.Speed * Frame);
			for (int32 Slide = 0; Slide < 2 && Move.Size() > 1.e-6; ++Slide)
			{
				const double Length = Move.Size();
				const FVector2D Way = Move / Length;
				FFlatHit Hit;
				if (!SweepDisc(Solid, Here, Way, Length, Body.BodyRadius, Hit))
				{
					Here += Move;
					break;
				}
				// A blocked move, as ACreatureBase::MoveBlockedBy takes it: pressing into something its looks missed is kept.
				const FVector Normal = ToWorld(Hit.Normal);
				if (FVector::DotProduct(Heading, Normal) <= -0.5 && Planner.TakeContactCheck()
					&& LookFlat(Seen, Here, -Normal, static_cast<float>(Body.BodyRadius * 1.5), Body.LookRadius).bClear
					&& Planner.NoteContact(ToWorld(Here - Hit.Normal * Body.BodyRadius), Normal, Heading, static_cast<float>(Body.BodyRadius), 1.f))
				{
					SteerTimer = 0.f;
				}
				if (Hit.bStartedInside)
				{
					if (FVector2D::DotProduct(Move, Hit.Normal) < 0.0)
					{
						Move -= Hit.Normal * FVector2D::DotProduct(Move, Hit.Normal);
						continue;
					}
					Here += Move;
					break;
				}
				const double Advance = FMath::Max(0.0, Hit.Time - 0.5);
				Here += Way * Advance;
				const FVector2D Left = Way * (Length - Advance);
				Move = Left - Hit.Normal * FVector2D::DotProduct(Left, Hit.Normal);
			}
			// Barely moving for its speed while it wants to go: blocked, as ACreatureBase::IsStuck has it.
			FCreatureUnstick::FFrame Step;
			Step.Here = ToWorld(Here);
			Step.bBlocked = !Heading.IsNearlyZero() && (Here - Was).Size() / Frame < Body.Speed * 0.15f;
			Step.bPressing = !Heading.IsNearlyZero();
			Step.DeltaSeconds = Frame;
			if (Unstick.Update(Step) == FCreatureUnstick::EAction::GoRound)
			{
				Planner.NoteStuck(Ask(), Heading, static_cast<float>(Body.BodyRadius));
			}
			Run.MostFeltWalls = FMath::Max(Run.MostFeltWalls, Planner.NumFeltWalls());
			Clock += Frame;
		}
		Run.Seconds = Clock;
		Run.SideFlips = Planner.GetSideFlips();
		Run.Reversals = Planner.GetReversals();
		Run.WallFollows = Planner.GetWallFollows();
		Run.End = Here;
		return Run;
	}

	/** A fence line north to south at X, from FromY to ToY. */
	FFlatWall Fence(double X, double FromY, double ToY)
	{
		return FlatWall(X, FromY, X, ToY);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringFenceFieldTest, "Looter.Creatures.Steering.FenceField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringFenceFieldTest::RunTest(const FString& Parameters)
{
	// Fences, walls and props stand all over both levels now (the user, 2026-10-08: "it feels empty and wonky"; creatures
	// ping-ponged along fences). A creature crossing a field of four long fence lines, their gaps at alternate ends, gets to
	// its goal 40 m on: one way round each fence, never turning back on itself, up close and far away, a spider and a slime.
	const TArray<FFlatWall> Alternate = { Fence(800.0, -1000.0, 300.0), Fence(1600.0, -300.0, 1000.0), Fence(2400.0, -1000.0, 300.0),
		Fence(3200.0, -300.0, 1000.0) };
	const TArray<FFlatWall> Staggered = { Fence(800.0, -600.0, 800.0), Fence(1600.0, -900.0, 500.0), Fence(2400.0, -500.0, 900.0),
		Fence(3200.0, -800.0, 600.0) };
	FFlatCreature Spider;
	FFlatCreature FarSpider;
	FarSpider.bNear = false;
	FFlatCreature Slime;
	Slime.BodyRadius = 50.0;
	Slime.LookRadius = 41.75;
	Slime.Speed = 220.f;
	struct FCase
	{
		const TCHAR* Name;
		const TArray<FFlatWall>* Fences;
		FVector2D Start;
		FVector2D Goal;
		FFlatCreature Body;
		float Seconds;
	};
	const FCase Cases[] = {
		{ TEXT("A spider through fences gapped at alternate ends"), &Alternate, FVector2D(0.0, 0.0), FVector2D(4000.0, 0.0), Spider, 30.f },
		{ TEXT("...one far away, taking fewer looks"), &Alternate, FVector2D(0.0, 0.0), FVector2D(4000.0, 0.0), FarSpider, 30.f },
		{ TEXT("...a slime"), &Alternate, FVector2D(0.0, 0.0), FVector2D(4000.0, 0.0), Slime, 60.f },
		{ TEXT("A spider through staggered fences"), &Staggered, FVector2D(0.0, 100.0), FVector2D(4000.0, -50.0), Spider, 30.f },
		{ TEXT("...a slime"), &Staggered, FVector2D(0.0, 100.0), FVector2D(4000.0, -50.0), Slime, 70.f },
	};
	for (const FCase& Case : Cases)
	{
		const FFlatRun Run = RunFlat(*Case.Fences, {}, Case.Start, Case.Goal, Case.Body, Case.Seconds);
		const FString Seen = FString::Printf(TEXT("%s: %s"), Case.Name, *Run.Describe());
		TestTrue(*FString::Printf(TEXT("%s (gets there)"), *Seen), Run.bReached);
		TestTrue(*FString::Printf(TEXT("%s (no ping-pong: at most one turn back)"), *Seen), Run.Reversals <= 1);
		TestTrue(*FString::Printf(TEXT("%s (a side per fence at most)"), *Seen), Run.SideFlips <= Case.Fences->Num());
		TestTrue(*FString::Printf(TEXT("%s (followed each fence about once)"), *Seen), Run.WallFollows >= 1 && Run.WallFollows <= 2 * Case.Fences->Num());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringCornerTest, "Looter.Creatures.Steering.Corner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringCornerTest::RunTest(const FString& Parameters)
{
	// A creature in a U-shaped pen (a fence corner, a barn's lean-to) with its goal beyond the pen's back wall gets out the
	// open side and round: it used to wedge in the corner with every way blocked and no input. Wherever in the pen it
	// starts, and whatever its size.
	const TArray<FFlatWall> Pen = { FlatWall(300.0, -300.0, 300.0, 300.0), FlatWall(-200.0, 300.0, 300.0, 300.0), FlatWall(-200.0, -300.0, 300.0, -300.0) };
	FFlatCreature Spider;
	FFlatCreature Slime;
	Slime.BodyRadius = 50.0;
	Slime.LookRadius = 41.75;
	Slime.Speed = 220.f;
	FFlatCreature Unpaid;
	Unpaid.BodyRadius = 34.0;
	Unpaid.LookRadius = 30.6;
	Unpaid.Speed = 400.f;
	struct FCase
	{
		const TCHAR* Name;
		FVector2D Start;
		FVector2D Goal;
		FFlatCreature Body;
		float Seconds;
	};
	const FCase Cases[] = {
		{ TEXT("A spider in the middle of the pen"), FVector2D(0.0, 0.0), FVector2D(1500.0, 0.0), Spider, 20.f },
		{ TEXT("A spider in its corner, its goal off to the side"), FVector2D(150.0, 120.0), FVector2D(1500.0, 300.0), Spider, 20.f },
		{ TEXT("A slime in the pen"), FVector2D(0.0, 0.0), FVector2D(1500.0, 0.0), Slime, 30.f },
		{ TEXT("An Unpaid in the pen's corner"), FVector2D(150.0, 120.0), FVector2D(1500.0, 300.0), Unpaid, 20.f },
	};
	for (const FCase& Case : Cases)
	{
		const FFlatRun Run = RunFlat(Pen, {}, Case.Start, Case.Goal, Case.Body, Case.Seconds);
		const FString Seen = FString::Printf(TEXT("%s: %s"), Case.Name, *Run.Describe());
		TestTrue(*FString::Printf(TEXT("%s (gets out and there)"), *Seen), Run.bReached);
		TestTrue(*FString::Printf(TEXT("%s (no ping-pong in the corner)"), *Seen), Run.Reversals <= 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringUnseenTest, "Looter.Creatures.Steering.Unseen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringUnseenTest::RunTest(const FString& Parameters)
{
	// Something its looks miss (a rail under them, a prop's odd collision) is felt where the body bumps into it, and gone
	// round: the old steering took a random detour after pressing on for 0.8 s and came straight back into it, again and again.
	FFlatCreature Spider;
	struct FCase
	{
		const TCHAR* Name;
		TArray<FFlatWall> Seen;
		TArray<FFlatWall> Unseen;
		FVector2D Goal;
	};
	const FCase Cases[] = {
		{ TEXT("A 16 m rail it can't see, across its way"), {}, { Fence(500.0, -800.0, 800.0) }, FVector2D(1200.0, 0.0) },
		{ TEXT("...slanting"), {}, { FlatWall(500.0, -800.0, 700.0, 800.0) }, FVector2D(1200.0, 0.0) },
		{ TEXT("...in front of a fence it sees"), { Fence(900.0, -800.0, 300.0) }, { Fence(400.0, -300.0, 1000.0) }, FVector2D(1500.0, 0.0) },
	};
	for (const FCase& Case : Cases)
	{
		const FFlatRun Run = RunFlat(Case.Seen, Case.Unseen, FVector2D(0.0, 0.0), Case.Goal, Spider, 25.f);
		const FString Seen = FString::Printf(TEXT("%s: %s, %d felt walls at most"), Case.Name, *Run.Describe(), Run.MostFeltWalls);
		TestTrue(*FString::Printf(TEXT("%s (gets round)"), *Seen), Run.bReached);
		TestTrue(*FString::Printf(TEXT("%s (felt it)"), *Seen), Run.MostFeltWalls >= 1);
		TestTrue(*FString::Printf(TEXT("%s (no ping-pong)"), *Seen), Run.Reversals <= 3);
	}
	return true;
}

#endif
