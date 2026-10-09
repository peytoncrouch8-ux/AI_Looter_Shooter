#pragma once

// What the fauna tests share: a stand-in mesh for every piece, a walking player as a threat, moving an actor on frame by
// frame with a context the test makes (a test level's subsystem doesn't tick), and Looter.Fauna set for a test.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/FaunaActor.h"
#include "World/FaunaTypes.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Templates/Function.h"

namespace FaunaTestWorld
{
	/** One frame of the fauna tests (s). */
	inline constexpr float Frame = 1.f / 30.f;

	/** A stand-in for every piece: the tests check behaviour, not looks. */
	inline UStaticMesh* StandIn()
	{
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}

	/** A walking player at Where. */
	inline FFaunaThreat Walker(const FVector& Where)
	{
		FFaunaThreat Threat;
		Threat.Location = Where;
		Threat.Fear = 1.f;
		Threat.bPlayer = true;
		return Threat;
	}

	/** Moves an actor on by Seconds in frames, on screen, each frame's context made by Make (it may look at the time). */
	inline void Run(AFaunaActor& Actor, float Seconds, double& Now, TFunctionRef<FFaunaContext(double)> Make)
	{
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			const double Since = Now;
			Now += Frame;
			FFaunaTick Tick;
			Tick.DeltaSeconds = Frame;
			Tick.Distance = 1000.f;
			Tick.bOnScreen = true;
			Tick.Since = Since;
			FFaunaContext Context = Make(Now);
			Context.Now = Now;
			Actor.UpdateFauna(Tick, Context);
		}
	}

	/** Nobody about, nothing heard. */
	inline FFaunaContext Quiet(double)
	{
		return FFaunaContext();
	}

	/** Looter.Fauna set for a test, and put back after it. */
	class FFaunaSetting
	{
	public:
		explicit FFaunaSetting(int32 Wanted)
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Fauna")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Variable->Set(Wanted, ECVF_SetByConsole);
			}
		}

		~FFaunaSetting()
		{
			if (Variable)
			{
				Variable->Set(Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		int32 Previous = 1;
	};
}

#endif
