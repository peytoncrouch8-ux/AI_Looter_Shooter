#include "World/FoliageBlastSubsystem.h"
#include "World/GustSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "AI_Looter_Shooter.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/PackageName.h"

const FName UFoliageBlastSubsystem::CenterParameter(TEXT("BlastCenter"));
const FName UFoliageBlastSubsystem::StrengthParameter(TEXT("BlastStrength"));

namespace
{
	/** The spring-back: how fast its swing dies (per second) and how fast it swings (radians a second). */
	constexpr float SettleDamping = 3.5f;
	constexpr float SettleSwing = 7.f;
	/** The longest step one tick takes (s): after a hitch it catches up without leaping. */
	constexpr float LongestStep = 0.1f;

#if !UE_BUILD_SHIPPING
	/** Looter.Foliage.Blast [radius cm] [strength]: a blast at the player's feet, to see the grass take it. */
	FAutoConsoleCommandWithWorldAndArgs BlastCommand(TEXT("Looter.Foliage.Blast"),
		TEXT("Throws the grass flat in a ring from the player's feet (radius cm, default 690; strength 0-1, default 1), as an Unpaid's shriek does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			// The console may hand the editor's world: the one being played instead.
			if (GEngine && (!World || !World->IsGameWorld()))
			{
				for (const FWorldContext& Context : GEngine->GetWorldContexts())
				{
					if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
					{
						World = Context.World();
						break;
					}
				}
			}
			const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
			if (!Pawn)
			{
				UE_LOG(LogLooter, Warning, TEXT("FOLIAGE BLAST: no player pawn in this world."));
				return;
			}
			const float Radius = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 690.f;
			const float Strength = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 1.f;
			UFoliageBlastSubsystem::Blast(World, Pawn->GetActorLocation(), Radius, 0.6f, Strength);
			UE_LOG(LogLooter, Display, TEXT("FOLIAGE BLAST: %.0f cm at %.1f."), Radius, Strength);
		}));
#endif
}

UFoliageBlastSubsystem* UFoliageBlastSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UFoliageBlastSubsystem>() : nullptr;
}

void UFoliageBlastSubsystem::Blast(const UObject* WorldContextObject, const FVector& InCenter, float InRadius, float InGrowSeconds, float InStrength)
{
	UFoliageBlastSubsystem* Blasts = Get(WorldContextObject);
	if (!Blasts || InRadius <= 0.f || InStrength <= 0.f)
	{
		return;
	}
	Blasts->Center = InCenter;
	Blasts->Radius = InRadius;
	Blasts->GrowSeconds = FMath::Max(InGrowSeconds, 0.05f);
	Blasts->Strength = FMath::Clamp(InStrength, 0.f, 1.f);
	Blasts->Age = 0.f;
	Blasts->bActive = true;
	Blasts->Write(Blasts->Center, StateAt(0.f, Blasts->Radius, Blasts->GrowSeconds, Blasts->Strength));
}

FFoliageBlastState UFoliageBlastSubsystem::StateAt(float Age, float Radius, float GrowSeconds, float Strength)
{
	FFoliageBlastState State;
	if (Age < 0.f || Radius <= 0.f || Strength <= 0.f)
	{
		return State;
	}
	const float Grow = FMath::Max(GrowSeconds, 0.05f);
	if (Age < Grow)
	{
		// The front rolls out fast, slowing as it spends itself (an ease-out), at full strength.
		const float T = Age / Grow;
		State.Front = Radius * (1.f - FMath::Square(1.f - T));
		State.Strength = Strength;
		return State;
	}
	// Stopped at its reach: the plants spring back, swinging a little past upright before they settle.
	const float After = Age - Grow;
	State.Front = Radius;
	State.Strength = After >= SettleSeconds ? 0.f : Strength * FMath::Exp(-SettleDamping * After) * FMath::Cos(SettleSwing * After);
	return State;
}

bool UFoliageBlastSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UFoliageBlastSubsystem::IsTickable() const
{
	return bActive;
}

TStatId UFoliageBlastSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFoliageBlastSubsystem, STATGROUP_Tickables);
}

void UFoliageBlastSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bActive)
	{
		return;
	}
	Age += FMath::Min(DeltaTime, LongestStep);
	if (Age >= GrowSeconds + SettleSeconds)
	{
		bActive = false;
		Write(Center, FFoliageBlastState());
		return;
	}
	Write(Center, StateAt(Age, Radius, GrowSeconds, Strength));
}

void UFoliageBlastSubsystem::Write(const FVector& At, const FFoliageBlastState& State)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!bCollectionLooked)
	{
		bCollectionLooked = true;
		const FString Path(UGustSubsystem::CollectionPath);
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			WindCollection = LoadObject<UMaterialParameterCollection>(nullptr, *Path);
		}
	}
	UMaterialParameterCollectionInstance* Instance = WindCollection ? World->GetParameterCollectionInstance(WindCollection) : nullptr;
	if (!Instance)
	{
		return;
	}
	Instance->SetVectorParameterValue(CenterParameter, FLinearColor(static_cast<float>(At.X), static_cast<float>(At.Y), static_cast<float>(At.Z), State.Front));
	Instance->SetScalarParameterValue(StrengthParameter, State.Strength);
}
