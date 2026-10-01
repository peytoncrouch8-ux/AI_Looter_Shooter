#include "Tutorial/TutorialDirector.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Core/LooterMenuGameMode.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionSubsystem.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/TutorialPromptWidget.h"
#include "Weapons/WeaponBase.h"
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

	UMissionSubsystem* GetMissions(const UWorld* World)
	{
		return World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	}

	/** The living creature of kind T nearest to Where (on the map, so height doesn't count), or null. */
	template <typename T>
	const T* NearestLiving(const UWorld* World, const FVector& Where)
	{
		const T* Nearest = nullptr;
		double NearestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<T> It(World); It; ++It)
		{
			const double Distance = FVector::DistSquared2D(It->GetActorLocation(), Where);
			if (!It->IsDead() && Distance < NearestDistance)
			{
				Nearest = *It;
				NearestDistance = Distance;
			}
		}
		return Nearest;
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
	MissionTitle = TEXT("Welcome to Skyreach");
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	// Behind the main menu there's no one to teach.
	if (ALooterMenuGameMode::IsMenuWorld(GetWorld()))
	{
		SetActorTickEnabled(false);
		return;
	}
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
	EndMission();
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
	// The waypoint follows its target (a spider on the move, the rifle once it's taken), and the text the key bindings.
	SyncMission();
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

void ATutorialDirector::ResumeAtStep(int32 Index)
{
	if (Current != INDEX_NONE && Steps.IsValidIndex(Index))
	{
		StartStep(Index);
	}
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
	SyncMission();
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
	EndMission();
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

void ATutorialDirector::SyncMission()
{
	UMissionSubsystem* Missions = GetMissions(GetWorld());
	if (!Missions || !Steps.IsValidIndex(Current))
	{
		return;
	}
	if (MissionId == INDEX_NONE)
	{
		// Behind the main menu no one plays, so there's no mission to guide them (a restart from the console there too).
		if (ALooterMenuGameMode::IsMenuWorld(GetWorld()))
		{
			return;
		}
		MissionId = Missions->AddMission(FText::FromString(MissionTitle));
	}
	// Unchanged text and a waypoint that only moved don't wake the mission's listeners, so this is cheap to repeat.
	const FTutorialStep& Step = Steps[Current];
	Missions->SetObjective(MissionId, FText::FromString(ResolveKeys(Step.Text)), FindWaypoint(Step));
}

void ATutorialDirector::EndMission()
{
	if (MissionId == INDEX_NONE)
	{
		return;
	}
	if (UMissionSubsystem* Missions = GetMissions(GetWorld()))
	{
		Missions->RemoveMission(MissionId);
	}
	MissionId = INDEX_NONE;
}

TOptional<FVector> ATutorialDirector::FindWaypoint(const FTutorialStep& Step) const
{
	const UWorld* World = GetWorld();
	switch (Step.Goal)
	{
	case ETutorialGoal::Move:
	case ETutorialGoal::ReachRack:
	{
		// The road leads to the village and its gun rack, so the first steps already point that way.
		TActorIterator<AWeaponRack> Rack(World);
		return Rack ? TOptional<FVector>(Rack->GetActorLocation()) : TOptional<FVector>();
	}
	case ETutorialGoal::HoldWeapon:
	{
		// The rifle itself while it lies there; once it's gone (taken, restocking), the rack.
		TActorIterator<AWeaponRack> Rack(World);
		if (!Rack)
		{
			return TOptional<FVector>();
		}
		const AWeaponBase* Offered = Rack->IsWeaponOffered() ? Rack->GetOfferedWeapon() : nullptr;
		return Offered ? Offered->GetActorLocation() : Rack->GetActorLocation();
	}
	case ETutorialGoal::HitDummies:
	{
		// The middle of the training ground, not one dummy: any of them counts.
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;
		for (TActorIterator<ATargetDummy> It(World); It; ++It)
		{
			Sum += It->GetActorLocation();
			++Count;
		}
		return Count > 0 ? TOptional<FVector>(Sum / Count) : TOptional<FVector>();
	}
	case ETutorialGoal::KillCreatures:
	{
		// The step asks for spiders, so the nearest one; any creature counts, so with no spider left, the nearest of those.
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			return TOptional<FVector>();
		}
		const FVector From = Pawn->GetActorLocation();
		const ACreatureBase* Target = NearestLiving<ASpiderCreature>(World, From);
		Target = Target ? Target : NearestLiving<ACreatureBase>(World, From);
		return Target ? TOptional<FVector>(Target->GetActorLocation()) : TOptional<FVector>();
	}
	case ETutorialGoal::OpenInventory:
		break;
	}
	return TOptional<FVector>();
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
