// Developer console commands for the air and the gusts (not in shipping builds): Looter.Air.Stats and Looter.Gust. The
// switches are console variables: Looter.Air 0/1, Looter.Air.Density and Looter.Air.Glow (World/AmbientAirSubsystem.cpp,
// World/AmbientAirDraw.cpp).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/AmbientAirRules.h"
#include "World/AmbientAirSubsystem.h"
#include "World/GustSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* AirWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return World;
	}

	/** Looter.Air.Stats: what's out of each kind and why, what's near, and what it all costs. */
	void StatsCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = AirWorld(World);
		const UAmbientAirSubsystem* Air = UAmbientAirSubsystem::Get(World);
		if (!Air)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Air.Stats: no air here."));
			return;
		}
		const FAirView& View = Air->GetLastView();
		const FAirSurroundings& Around = Air->GetSurroundings();
		FString Kinds;
		for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
		{
			const EAirKind Kind = static_cast<EAirKind>(Index);
			Kinds += FString::Printf(TEXT("%s%d %s (of %d wanted)"), Index > 0 ? TEXT(", ") : TEXT(""), Air->CountOf(Kind), AmbientAirRules::Name(Kind),
				AmbientAirRules::Target(Kind, View, Around, UAmbientAirSubsystem::GetDensity()));
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Air.Stats: %s; %s, %s; %s. %d of %d out: %s."),
			UAmbientAirSubsystem::IsEnabled() ? TEXT("on") : TEXT("off (Looter.Air 0)"),
			View.bSheltered ? TEXT("sheltered (the mine, an interior)") : (View.bUnderCover ? TEXT("under cover") : TEXT("in the open")),
			View.Lighting.IsNone() ? TEXT("no lighting states (day)") : *View.Lighting.ToString(),
			View.bValid ? TEXT("a view") : TEXT("no view"), Air->NumLive(), AmbientAirRules::Budget, *Kinds);
		UE_LOG(LogLooter, Display, TEXT("Looter.Air.Stats: the place is %.0f%% open grass, %.0f%% in the sun; %d broadleaf trees in the level (%d near), %d lamps (%d lit near). %.3f ms and %.1f traces a frame on average."),
			Around.GrassShare * 100.f, Around.SunShare * 100.f, Air->GetTrees().Num(), Around.NearTrees, Air->GetLamps().Num(), Around.NearLamps,
			Air->GetAverageMilliseconds(), Air->GetAverageTraces());
		if (const UGustSubsystem* Gusts = UGustSubsystem::Get(World))
		{
			const FGustClock& Clock = Gusts->GetClock();
			if (Clock.IsBlowing())
			{
				UE_LOG(LogLooter, Display, TEXT("Looter.Air.Stats: a gust of %.2f toward %.0f degrees, %.1f s into %.1f (push %.2f); %d swings."),
					Clock.GetCurrent().Strength, Clock.GetCurrent().Direction.Rotation().Yaw, Clock.GetAge(), Clock.GetCurrent().Duration,
					Gusts->GetStrengthNow(), Gusts->NumSwings());
			}
			else
			{
				UE_LOG(LogLooter, Display, TEXT("Looter.Air.Stats: no gust; the next in %.0f s (%d so far); %d swings."), Clock.GetSecondsToNext(),
					Clock.GetCount(), Gusts->NumSwings());
			}
		}
	}

	/** Looter.Gust [strength] [seconds]: a gust now. */
	void GustCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = AirWorld(World);
		UGustSubsystem* Gusts = UGustSubsystem::Get(World);
		if (!Gusts)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Gust: no gusts here."));
			return;
		}
		const float Strength = Args.Num() > 0 ? FCString::Atof(*Args[0]) : -1.f;
		const float Seconds = Args.Num() > 1 ? FCString::Atof(*Args[1]) : -1.f;
		if (!UAmbientAirSubsystem::IsEnabled())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Gust: the air is off (Looter.Air 0): the gust would be still."));
		}
		if (Gusts->Trigger(Strength, Seconds))
		{
			const FGustEvent& Gust = Gusts->GetClock().GetCurrent();
			UE_LOG(LogLooter, Display, TEXT("Looter.Gust: %.2f toward %.0f degrees for %.1f s."), Gust.Strength, Gust.Direction.Rotation().Yaw, Gust.Duration);
		}
		else
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Gust: one is blowing already."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs AirStatsCommand(TEXT("Looter.Air.Stats"),
		TEXT("Things in the air: what's out of each kind and wanted, the place's grass and sun, trees and lamps near, the cost; the gusts."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StatsCommand));
	FAutoConsoleCommandWithWorldAndArgs GustNowCommand(TEXT("Looter.Gust"),
		TEXT("A gust now: Looter.Gust [strength 0.4-1] [seconds]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GustCommand));
}

#endif
