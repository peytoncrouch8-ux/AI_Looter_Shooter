// UControlHintSubsystem: its life in the level, the profile's memory, and what the HUD asks of it. What it sees of the
// player is ControlHintSubsystemSenses.cpp's.

#include "Tutorial/ControlHintSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Tutorial/ControlHintsSave.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const ULocalPlayer* FindLocalPlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetLocalPlayer() : nullptr;
	}
}

UControlHintSubsystem* UControlHintSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UControlHintSubsystem>() : nullptr;
}

bool UControlHintSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Only where someone plays: the tests drive FControlHintRules by themselves, without a world.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UControlHintSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Behind the main menu nobody plays.
	if (bActive || !InWorld.IsGameWorld() || ALooterMenuGameMode::IsMenuWorld(&InWorld))
	{
		return;
	}
	Load();
	bActive = true;
}

void UControlHintSubsystem::Deinitialize()
{
	bActive = false;
	LastPawn.Reset();
	Benches.Reset();
	Super::Deinitialize();
}

void UControlHintSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SinceLook += DeltaTime;
	if (SinceLook < LookInterval)
	{
		return;
	}
	const float Elapsed = SinceLook;
	SinceLook = 0.f;
	Rules.Update(Look(Elapsed), Elapsed);
	// A show or a lesson: a handful a session, each worth keeping at once (a crash shouldn't bring a learned hint back).
	if (Rules.ConsumeChanged())
	{
		Save();
	}
}

TStatId UControlHintSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UControlHintSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// What the HUD shows
// ---------------------------------------------------------------------------

FName UControlHintSubsystem::GetShownAction() const
{
	const EControlHint Hint = Rules.GetShown();
	if (Hint == EControlHint::Swap)
	{
		// The number key of a gun not in hand says it plainer than the mouse wheel does.
		const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		const UWeaponManagerComponent* Weapons = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		if (Weapons)
		{
			const int32 Slots = FMath::Min(Weapons->GetWeapons().Num(), UKeyBindingSubsystem::NumWeaponSlotKeys);
			for (int32 Slot = 0; Slot < Slots; ++Slot)
			{
				if (Slot != Weapons->GetActiveSlot())
				{
					return UKeyBindingSubsystem::WeaponSlotBindingId(Slot);
				}
			}
		}
	}
	return FControlHintRules::Action(Hint);
}

FText UControlHintSubsystem::GetShownWords() const
{
	const EControlHint Hint = Rules.GetShown();
	const ULocalPlayer* Player = FindLocalPlayer(GetWorld());
	const UKeyBindingSubsystem* Keys = Player ? Player->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const bool bToggle = Keys && Keys->IsToggleMode(FControlHintRules::Action(Hint));
	return FControlHintRules::Words(Hint, bToggle);
}

void UControlHintSubsystem::ResetMemory()
{
	Rules.ResetMemory();
	Rules.ConsumeChanged();
	Save();
	UE_LOG(LogLooter, Log, TEXT("Control hints: every hint forgotten, so each can show again."));
}

void UControlHintSubsystem::ForceShow(EControlHint Hint)
{
	Rules.ForceShow(Hint);
}

// ---------------------------------------------------------------------------
// The profile's memory
// ---------------------------------------------------------------------------

void UControlHintSubsystem::Load()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlot, 0))
	{
		return;
	}
	const ULooterControlHintsSave* Saved = Cast<ULooterControlHintsSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
	if (!Saved)
	{
		return;
	}
	for (const FControlHintRecord& Record : Saved->Hints)
	{
		// A hint the game no longer has is left out.
		const EControlHint Hint = FControlHintRules::FromId(Record.Hint);
		if (Hint != EControlHint::Count)
		{
			FControlHintMemory Memory;
			Memory.Shows = FMath::Max(Record.Shows, 0);
			Memory.bLearned = Record.bLearned;
			Rules.SetMemory(Hint, Memory);
		}
	}
}

void UControlHintSubsystem::Save() const
{
	ULooterControlHintsSave* Out = NewObject<ULooterControlHintsSave>();
	for (int32 Index = 0; Index < static_cast<int32>(EControlHint::Count); ++Index)
	{
		const EControlHint Hint = static_cast<EControlHint>(Index);
		const FControlHintMemory& Memory = Rules.GetMemory(Hint);
		FControlHintRecord& Record = Out->Hints.AddDefaulted_GetRef();
		Record.Hint = FControlHintRules::Id(Hint);
		Record.Shows = Memory.Shows;
		Record.bLearned = Memory.bLearned;
	}
	if (!UGameplayStatics::SaveGameToSlot(Out, SaveSlot, 0))
	{
		UE_LOG(LogLooter, Warning, TEXT("Control hints: the %s slot couldn't be written, so what was learned may show again."), SaveSlot);
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Looter.Hints reset | show <Move|Reload|Melee|Aim|Jump|Sprint|Slide|Swap|Inventory|Bench> */
	void HintsCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			GameWorld = GameWorld ? GameWorld : (Context.World() && Context.World()->IsGameWorld() ? Context.World() : nullptr);
		}
		UControlHintSubsystem* Hints = GameWorld ? GameWorld->GetSubsystem<UControlHintSubsystem>() : nullptr;
		if (!Hints || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Hints reset | show <Move|Reload|Melee|Aim|Jump|Sprint|Slide|Swap|Inventory|Bench>"));
			return;
		}
		if (Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
		{
			Hints->ResetMemory();
		}
		else if (Args[0].Equals(TEXT("show"), ESearchCase::IgnoreCase) && Args.Num() > 1)
		{
			const EControlHint Hint = FControlHintRules::FromId(FName(*Args[1]));
			if (Hint == EControlHint::Count)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Hints: no hint called %s."), *Args[1]);
				return;
			}
			Hints->ForceShow(Hint);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs HintsCommandRegistration(
		TEXT("Looter.Hints"),
		TEXT("The control hints: Looter.Hints reset (forget what was learned) | show <Hint>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HintsCommand));
}
#endif
