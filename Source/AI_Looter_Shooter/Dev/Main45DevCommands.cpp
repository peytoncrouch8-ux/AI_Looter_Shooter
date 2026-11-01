// Developer console commands for Mains 4 and 5 (the Wardens story; not in shipping builds):
//   Looter.ChapelHose.Pull            the chapel's window hose pulled out now (whatever its story)
//   Looter.Tender.Come [hover]        the chapel's drop tender flies in from the barge now ("hover": hanging there at once)
//   Looter.Intake.Charge [n]          the next n (1) of the intake's clamp spots get a salt charge (the third lights the fuse)
//   Looter.Intake.Choke               the intake blows now: the swing, the limp hose, the pool, the growl
//   Looter.Intake.SecondHose          the barge's second hose let down onto Larkspur High now

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/ChapelWindowHose.h"
#include "World/CompanyTenderDrop.h"
#include "World/IntakeCharge.h"
#include "World/SinkIntake.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindMain45World(UWorld* World)
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
		return nullptr;
	}

	/** The first actor of class T in the level, or null with a word in the log. */
	template <typename T>
	T* FindFirst(UWorld* World, const TCHAR* Command, const TCHAR* What)
	{
		for (TActorIterator<T> It(World); It; ++It)
		{
			return *It;
		}
		UE_LOG(LogLooter, Warning, TEXT("%s: no %s in this level (Ransom's Rest: build_area_main4.py / build_area_main5.py)."), Command, What);
		return nullptr;
	}

	void ChapelHosePull(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain45World(World);
		if (AChapelWindowHose* Hose = World ? FindFirst<AChapelWindowHose>(World, TEXT("Looter.ChapelHose.Pull"), TEXT("window hose")) : nullptr)
		{
			APlayerController* Controller = World->GetFirstPlayerController();
			const bool bPulled = Hose->Pull(Controller, /*bForce*/ true);
			UE_LOG(LogLooter, Display, TEXT("Looter.ChapelHose.Pull: %s"), bPulled ? TEXT("pulled out of the window") : TEXT("already out"));
		}
	}

	void TenderCome(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain45World(World);
		if (ACompanyTenderDrop* Tender = World ? FindFirst<ACompanyTenderDrop>(World, TEXT("Looter.Tender.Come"), TEXT("drop tender")) : nullptr)
		{
			const bool bHover = Args.Num() > 0 && Args[0].Equals(TEXT("hover"), ESearchCase::IgnoreCase);
			if (bHover)
			{
				Tender->HoverNow();
			}
			else
			{
				Tender->Come(/*bForce*/ true);
			}
			UE_LOG(LogLooter, Display, TEXT("Looter.Tender.Come: %s"), bHover ? TEXT("hanging over the drop") : TEXT("flying in from the barge"));
		}
	}

	void IntakeCharge(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain45World(World);
		if (!World)
		{
			return;
		}
		int32 Wanted = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		int32 Set = 0;
		for (TActorIterator<AIntakeCharge> It(World); It && Set < FMath::Max(Wanted, 1); ++It)
		{
			if (!It->IsSet() && It->SetCharge(nullptr, /*bForce*/ true))
			{
				++Set;
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Intake.Charge: %d charge(s) set"), Set);
	}

	void IntakeChoke(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain45World(World);
		if (ASinkIntake* Intake = World ? FindFirst<ASinkIntake>(World, TEXT("Looter.Intake.Choke"), TEXT("intake")) : nullptr)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Intake.Choke: %s"), Intake->Choke() ? TEXT("blown") : TEXT("already choked"));
		}
	}

	void IntakeSecondHose(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain45World(World);
		if (ASinkIntake* Intake = World ? FindFirst<ASinkIntake>(World, TEXT("Looter.Intake.SecondHose"), TEXT("intake")) : nullptr)
		{
			// Its condition forced for this visit: the build's "Main 5 at the rim" holds from then on in the story anyway.
			FStoryCondition Always;
			Intake->SecondHoseWhen = { Always };
			Intake->SecondHoseGoneWhen.Reset();
			Intake->RefreshStory();
			UE_LOG(LogLooter, Display, TEXT("Looter.Intake.SecondHose: let down (%.0f%%)"), Intake->GetSecondHoseShare() * 100.f);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ChapelHosePullRegistration(TEXT("Looter.ChapelHose.Pull"),
		TEXT("Pulls the Company's hose out of the chapel's side window now (Main 4), whatever its story."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ChapelHosePull));

	FAutoConsoleCommandWithWorldAndArgs TenderComeRegistration(TEXT("Looter.Tender.Come"),
		TEXT("The chapel's drop tender flies in from the barge now: Looter.Tender.Come [hover] (hover: hanging over the yard at once)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TenderCome));

	FAutoConsoleCommandWithWorldAndArgs IntakeChargeRegistration(TEXT("Looter.Intake.Charge"),
		TEXT("Sets the next n salt charges on the Sink intake's collar: Looter.Intake.Charge [n] (the third lights the fuse)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntakeCharge));

	FAutoConsoleCommandWithWorldAndArgs IntakeChokeRegistration(TEXT("Looter.Intake.Choke"),
		TEXT("Blows the Sink's intake now (Main 5's set piece): the swing, the limp hose, the pool drained, the growl."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntakeChoke));

	FAutoConsoleCommandWithWorldAndArgs IntakeSecondHoseRegistration(TEXT("Looter.Intake.SecondHose"),
		TEXT("Lets the barge's second hose down onto Larkspur High's air shaft now (Main 5 at the rim)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntakeSecondHose));
}

#endif
