#include "Tutorial/TutorialDirector.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Creatures/CreatureBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/WeaponRack.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/TutorialPromptWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Checking the player a few times a second is plenty for a tutorial. */
	constexpr float CheckInterval = 0.2f;

	UPlayerProgressionSubsystem* GetProgression(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
		return Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	}
}

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = CheckInterval;

	Steps = {
		{ TEXT("Welcome to Skyreach. Move with {Move} and look around with the mouse."), ETutorialGoal::Move, 600.f },
		{ TEXT("Hold {Sprint} to run. Follow the road to the village."), ETutorialGoal::ReachRack, 900.f },
		{ TEXT("Grab the rifle on the gun rack: look at it and press {Interact}."), ETutorialGoal::HoldWeapon, 1.f },
		{ TEXT("Shoot the target dummies in the meadow under the windmill. {Reload} reloads."), ETutorialGoal::HitDummies, 5.f },
		{ TEXT("Spiders nest in the woods past the pond. Hunt down two of them."), ETutorialGoal::KillCreatures, 2.f },
		{ TEXT("Press {Inventory} to see your loadout and your weapons' stats."), ETutorialGoal::OpenInventory, 1.f },
	};
	DoneText = TEXT("You're ready. Explore the island, and climb to the lookout on the plateau for the view.");
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	BindTargets();
	const UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld());
	if (Progression && Progression->IsTutorialDone())
	{
		SetActorTickEnabled(false);
		return;
	}
	StartStep(0);
}

void ATutorialDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Prompt)
	{
		Prompt->RemoveFromParent();
		Prompt = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ATutorialDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Pawn && !StepStart.IsSet())
	{
		StepStart = Pawn->GetActorLocation();
	}
	if (UTutorialPromptWidget* Widget = GetPrompt())
	{
		if (!bStepShown && Steps.IsValidIndex(Current))
		{
			Widget->ShowStep(Current, Steps.Num(), ResolveKeys(Steps[Current].Text));
			bStepShown = true;
		}
		const ALooterHUD* HUD = Controller ? Cast<ALooterHUD>(Controller->GetHUD()) : nullptr;
		Widget->SetSuppressed(HUD && HUD->IsMenuOpen() && !(Steps.IsValidIndex(Current) && Steps[Current].Goal == ETutorialGoal::OpenInventory));
	}
	// Several steps can be done at once (the player already carries a gun): pass them all.
	while (Steps.IsValidIndex(Current) && IsStepDone(Steps[Current]))
	{
		StartStep(Current + 1);
	}
}

void ATutorialDirector::Restart()
{
	if (UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld()))
	{
		Progression->SetTutorialDone(false);
	}
	SetActorTickEnabled(true);
	StartStep(0);
}

void ATutorialDirector::Skip()
{
	Finish(/*bShowDone*/ false);
}

void ATutorialDirector::StartStep(int32 Index)
{
	if (!Steps.IsValidIndex(Index))
	{
		Finish(/*bShowDone*/ true);
		return;
	}
	Current = Index;
	Hits = 0;
	Kills = 0;
	bStepShown = false;
	const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	StepStart.Reset();
	if (Pawn)
	{
		StepStart = Pawn->GetActorLocation();
	}
	UE_LOG(LogLooter, Log, TEXT("Tutorial step %d/%d"), Index + 1, Steps.Num());
}

bool ATutorialDirector::IsStepDone(const FTutorialStep& Step) const
{
	const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}
	switch (Step.Goal)
	{
	case ETutorialGoal::Move:
		return StepStart.IsSet() && FVector::Dist2D(Pawn->GetActorLocation(), *StepStart) >= Step.Amount;
	case ETutorialGoal::ReachRack:
	{
		// No rack in this level: nothing to walk to.
		TActorIterator<AWeaponRack> Rack(GetWorld());
		return !Rack || FVector::Dist2D(Pawn->GetActorLocation(), Rack->GetActorLocation()) <= Step.Amount;
	}
	case ETutorialGoal::HoldWeapon:
	{
		const UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>();
		return Manager && !Manager->GetWeapons().IsEmpty();
	}
	case ETutorialGoal::HitDummies:
		return Hits >= Step.Amount || !TActorIterator<ATargetDummy>(GetWorld());
	case ETutorialGoal::KillCreatures:
		return Kills >= Step.Amount || !TActorIterator<ACreatureBase>(GetWorld());
	case ETutorialGoal::OpenInventory:
	{
		const ALooterHUD* HUD = Cast<ALooterHUD>(Controller->GetHUD());
		return HUD && HUD->IsInventoryOpen();
	}
	}
	return true;
}

void ATutorialDirector::Finish(bool bShowDone)
{
	Current = INDEX_NONE;
	SetActorTickEnabled(false);
	if (UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld()))
	{
		Progression->SetTutorialDone(true);
	}
	if (UTutorialPromptWidget* Widget = GetPrompt())
	{
		if (bShowDone)
		{
			Widget->SetSuppressed(false);
			Widget->ShowDone(ResolveKeys(DoneText), DoneSeconds);
		}
		else
		{
			Widget->HideNow();
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Tutorial %s"), bShowDone ? TEXT("complete") : TEXT("skipped"));
}

void ATutorialDirector::BindTargets()
{
	// Dummies count hits, creatures count kills; both keep their health components through respawns.
	for (TActorIterator<ATargetDummy> It(GetWorld()); It; ++It)
	{
		if (UHealthComponent* Health = It->FindComponentByClass<UHealthComponent>())
		{
			Health->OnDamaged.AddUniqueDynamic(this, &ATutorialDirector::HandleDummyDamaged);
		}
	}
	for (TActorIterator<ACreatureBase> It(GetWorld()); It; ++It)
	{
		if (UHealthComponent* Health = It->FindComponentByClass<UHealthComponent>())
		{
			Health->OnDeath.AddUniqueDynamic(this, &ATutorialDirector::HandleCreatureDeath);
		}
	}
}

void ATutorialDirector::HandleDummyDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Cast<APlayerController>(InstigatedBy))
	{
		++Hits;
	}
}

void ATutorialDirector::HandleCreatureDeath(AController* Killer)
{
	if (Cast<APlayerController>(Killer))
	{
		++Kills;
	}
}

UTutorialPromptWidget* ATutorialDirector::GetPrompt()
{
	if (!Prompt)
	{
		APlayerController* Controller = GetWorld()->GetFirstPlayerController();
		if (Controller && Controller->IsLocalController())
		{
			Prompt = CreateWidget<UTutorialPromptWidget>(Controller, UTutorialPromptWidget::StaticClass());
			if (Prompt)
			{
				Prompt->AddToViewport(/*ZOrder*/ 5);
			}
		}
	}
	return Prompt;
}

FString ATutorialDirector::ResolveKeys(const FString& Text) const
{
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UKeyBindingSubsystem* Keys = Player ? Player->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	auto KeyName = [Keys](const TCHAR* Id)
	{
		const FKey Key = Keys ? Keys->GetKey(Id) : FKey();
		return Key.IsValid() ? Key.GetDisplayName(/*bLongDisplayName*/ false).ToString() : FString(Id);
	};

	FString Result;
	int32 Index = 0;
	while (Index < Text.Len())
	{
		const int32 Open = Text.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);
		const int32 Close = Open == INDEX_NONE ? INDEX_NONE : Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open);
		if (Close == INDEX_NONE)
		{
			Result += Text.Mid(Index);
			break;
		}
		Result += Text.Mid(Index, Open - Index);
		const FString Id = Text.Mid(Open + 1, Close - Open - 1);
		if (Id == TEXT("Move"))
		{
			Result += FString::Printf(TEXT("[%s %s %s %s]"), *KeyName(TEXT("MoveForward")), *KeyName(TEXT("MoveLeft")),
				*KeyName(TEXT("MoveBackward")), *KeyName(TEXT("MoveRight")));
		}
		else
		{
			Result += FString::Printf(TEXT("[%s]"), *KeyName(*Id));
		}
		Index = Close + 1;
	}
	return Result;
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Looter.Tutorial restart|skip */
	void TutorialCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			GameWorld = GameWorld ? GameWorld : (Context.World() && Context.World()->IsGameWorld() ? Context.World() : nullptr);
		}
		ATutorialDirector* Director = nullptr;
		for (TActorIterator<ATutorialDirector> It(GameWorld); GameWorld && It; ++It)
		{
			Director = *It;
		}
		if (!Director || Args.Num() != 1)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game with a tutorial): Looter.Tutorial restart|skip"));
			return;
		}
		if (Args[0].Equals(TEXT("restart"), ESearchCase::IgnoreCase))
		{
			Director->Restart();
		}
		else if (Args[0].Equals(TEXT("skip"), ESearchCase::IgnoreCase))
		{
			Director->Skip();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs TutorialCommandRegistration(
		TEXT("Looter.Tutorial"),
		TEXT("Restarts or skips the level's tutorial: Looter.Tutorial restart|skip"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TutorialCommand));
}
#endif
