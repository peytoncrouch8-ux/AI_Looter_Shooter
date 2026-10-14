// UWindowLifeSubsystem: somebody at home behind a lit window, now and then, as the player looks over (never a face).

#include "World/WindowLifeSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "World/HouseLights.h"
#include "World/LightingStateSubsystem.h"
#include "World/WindowLifeCard.h"
#include "World/WindowShutter.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarWindowLife(TEXT("Looter.WindowLife"), 1,
		TEXT("Somebody at home: shadows, curtains and shutters at the lived-in houses' windows now and then (0: none)."));

	/** The level's shutters are looked for again this often (s): they stand in the level, nothing spawns them. */
	constexpr double ShutterLookSeconds = 30.0;

	/** A shutter rests with the lit house it's on: the nearest house's lights within this (cm). */
	constexpr float ShutterHouseRadius = 1500.f;

	/** The sight line ends this far out in front of the window (cm), so the house's own wall never blocks it. */
	constexpr float SightOut = 40.f;

	/** The lighting state the lamps are lit for. */
	const FName DuskState(TEXT("Dusk"));

	/** One thing that may play: a kind at a house's lit window, or a shutter. */
	struct FOption
	{
		EWindowLifeKind Kind = EWindowLifeKind::None;
		AHouseLights* House = nullptr;
		int32 Window = INDEX_NONE;
		AWindowShutter* Shutter = nullptr;
		FVector Target = FVector::ZeroVector;
	};
}

UWindowLifeSubsystem* UWindowLifeSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UWindowLifeSubsystem>() : nullptr;
}

bool UWindowLifeSubsystem::IsEnabled()
{
	return CVarWindowLife.GetValueOnGameThread() != 0;
}

bool UWindowLifeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UWindowLifeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Timer);
	}
	Houses.Reset();
	Shutters.Reset();
	Card.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Who joins
// ---------------------------------------------------------------------------

void UWindowLifeSubsystem::AddHouse(AHouseLights* House)
{
	if (!House || House->NumLitWindows() == 0)
	{
		return;
	}
	Houses.RemoveAll([](const TWeakObjectPtr<AHouseLights>& Each) { return !Each.IsValid(); });
	Houses.AddUnique(House);
	// The card and its material now, as the level begins, not as the player first looks at a window.
	if (AWindowLifeCard* Stage = EnsureCard())
	{
		Stage->Prepare();
	}
	UpdateTimer();
}

void UWindowLifeSubsystem::RemoveHouse(AHouseLights* House)
{
	Houses.RemoveAll([House](const TWeakObjectPtr<AHouseLights>& Each) { return !Each.IsValid() || Each.Get() == House; });
	UpdateTimer();
}

int32 UWindowLifeSubsystem::NumHouses() const
{
	return Houses.FilterByPredicate([](const TWeakObjectPtr<AHouseLights>& Each) { return Each.IsValid(); }).Num();
}

void UWindowLifeSubsystem::RefreshShutters()
{
	Shutters.Reset();
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AWindowShutter> It(World); It; ++It)
		{
			Shutters.Add(*It);
		}
	}
	bShuttersFound = true;
}

int32 UWindowLifeSubsystem::NumShutters() const
{
	return Shutters.FilterByPredicate([](const TWeakObjectPtr<AWindowShutter>& Each) { return Each.IsValid(); }).Num();
}

void UWindowLifeSubsystem::UpdateTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (Houses.IsEmpty())
	{
		Timers.ClearTimer(Timer);
	}
	else if (!Timers.IsTimerActive(Timer))
	{
		Timers.SetTimer(Timer, this, &UWindowLifeSubsystem::Look, WindowLifeRules::LookSeconds, /*bLoop*/ true,
			Random.FRandRange(0.1f, WindowLifeRules::LookSeconds));
	}
}

AWindowLifeCard* UWindowLifeSubsystem::EnsureCard()
{
	if (AWindowLifeCard* Known = Card.Get())
	{
		return Known;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWindowLifeCard* Made = World->SpawnActor<AWindowLifeCard>(AWindowLifeCard::StaticClass(), FTransform::Identity, Spawn);
	Card = Made;
	return Made;
}

// ---------------------------------------------------------------------------
// The look round
// ---------------------------------------------------------------------------

void UWindowLifeSubsystem::Look()
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	// The menu's camera has no pawn: nobody is walking past.
	if (!Player || !Player->GetPawn() || !Player->PlayerCameraManager)
	{
		return;
	}
	Check(Player->PlayerCameraManager->GetCameraLocation(), Player->PlayerCameraManager->GetCameraRotation().Vector(),
		World->GetTimeSeconds());
}

bool UWindowLifeSubsystem::IsDuskNow() const
{
	const ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this);
	return Lighting && Lighting->GetState() == DuskState;
}

FObjectKey UWindowLifeSubsystem::HouseKeyOf(const AWindowShutter* Shutter) const
{
	const AHouseLights* Nearest = nullptr;
	double Best = FMath::Square(static_cast<double>(ShutterHouseRadius));
	for (const TWeakObjectPtr<AHouseLights>& Each : Houses)
	{
		const AHouseLights* House = Each.Get();
		const double Squared = House ? FVector::DistSquared(House->GetActorLocation(), Shutter->GetActorLocation()) : Best + 1.0;
		if (Squared <= Best)
		{
			Best = Squared;
			Nearest = House;
		}
	}
	return Nearest ? FObjectKey(Nearest) : FObjectKey(Shutter);
}

EWindowLifeKind UWindowLifeSubsystem::Check(const FVector& Eyes, const FVector& ViewDirection, double Now)
{
	// One at a time, with a pause after each: most looks end here.
	const AWindowLifeCard* Stage = Card.Get();
	if (!IsEnabled() || !Clock.IsQuiet(Now) || (Stage && Stage->IsPlaying()))
	{
		return EWindowLifeKind::None;
	}
	TArray<FOption, TInlineAllocator<16>> Options;
	TArray<EWindowLifeKind, TInlineAllocator<4>> Kinds;
	auto Offer = [&Options, &Kinds](const FOption& Option)
	{
		Options.Add(Option);
		Kinds.AddUnique(Option.Kind);
	};

	// The lit windows the player sees, at houses that have rested.
	AWindowLifeCard* Shows = EnsureCard();
	const bool bCards = Shows && Shows->Prepare();
	for (const TWeakObjectPtr<AHouseLights>& Each : Houses)
	{
		AHouseLights* House = Each.Get();
		if (!bCards || !House || House->IsHidden() || !Clock.MayPlay(FObjectKey(House), Now))
		{
			continue;
		}
		const bool bDusk = House->IsDusk();
		for (int32 Index = 0; Index < House->NumLitWindows(); ++Index)
		{
			FTransform Window;
			if (!House->GetLitWindow(Index, Window))
			{
				continue;
			}
			const FVector Out = Window.GetUnitAxis(EAxis::X);
			if (!WindowLifeRules::Sees(Eyes, ViewDirection, Window.GetLocation(), Out))
			{
				continue;
			}
			for (const EWindowLifeKind Kind : { EWindowLifeKind::Shadow, EWindowLifeKind::Curtain })
			{
				if (WindowLifeRules::Allows(Kind, bDusk))
				{
					Offer({ Kind, House, Index, nullptr, Window.GetLocation() + Out * SightOut });
				}
			}
		}
	}

	// The shutters still open that the player sees.
	if (!bShuttersFound || Now >= NextShutterLook)
	{
		RefreshShutters();
		NextShutterLook = Now + ShutterLookSeconds;
	}
	if (WindowLifeRules::Allows(EWindowLifeKind::Shutter, IsDuskNow()))
	{
		for (const TWeakObjectPtr<AWindowShutter>& Each : Shutters)
		{
			AWindowShutter* Shutter = Each.Get();
			if (!Shutter || Shutter->IsHidden() || Shutter->GetState() != EWindowShutterState::Open
				|| !Clock.MayPlay(HouseKeyOf(Shutter), Now))
			{
				continue;
			}
			const FVector Out = Shutter->GetActorForwardVector();
			if (WindowLifeRules::Sees(Eyes, ViewDirection, Shutter->GetActorLocation(), Out))
			{
				Offer({ EWindowLifeKind::Shutter, nullptr, INDEX_NONE, Shutter, Shutter->GetActorLocation() + Out * SightOut });
			}
		}
	}

	if (Options.IsEmpty() || Random.FRand() >= WindowLifeRules::Chance)
	{
		return EWindowLifeKind::None;
	}
	// The kinds offered are those that may play now (each window's own dusk was asked as it was offered).
	const EWindowLifeKind Kind = WindowLifeRules::PickKind(Kinds, /*bDusk*/ true, Random);
	TArray<const FOption*, TInlineAllocator<16>> OfKind;
	for (const FOption& Option : Options)
	{
		if (Option.Kind == Kind)
		{
			OfKind.Add(&Option);
		}
	}
	if (OfKind.IsEmpty())
	{
		return EWindowLifeKind::None;
	}
	const FOption& Chosen = *OfKind[Random.RandRange(0, OfKind.Num() - 1)];
	// Only now a trace: a fence, a tree or another house between them and the window, and nothing plays this look.
	if (bSightChecks && !HasSightLine(Eyes, Chosen.Target))
	{
		return EWindowLifeKind::None;
	}
	const bool bPlayed = Chosen.Shutter ? PlayShutter(Chosen.Shutter, Now) : PlayAt(Chosen.House, Chosen.Window, Kind, Now);
	return bPlayed ? Kind : EWindowLifeKind::None;
}

bool UWindowLifeSubsystem::HasSightLine(const FVector& Eyes, const FVector& Target) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WindowLifeSight), /*bTraceComplex*/ false);
	const APlayerController* Player = World->GetFirstPlayerController();
	if (const APawn* Pawn = Player ? Player->GetPawn() : nullptr)
	{
		Params.AddIgnoredActor(Pawn);
	}
	return !World->LineTraceTestByChannel(Eyes, Target, ECC_Visibility, Params);
}

// ---------------------------------------------------------------------------
// Playing
// ---------------------------------------------------------------------------

bool UWindowLifeSubsystem::PlayAt(AHouseLights* House, int32 Window, EWindowLifeKind Kind, double Now)
{
	FTransform Where;
	if (!House || !House->GetLitWindow(Window, Where) || (Kind != EWindowLifeKind::Shadow && Kind != EWindowLifeKind::Curtain))
	{
		return false;
	}
	AWindowLifeCard* Stage = EnsureCard();
	const FWindowLifeEvent Event = WindowLifeRules::MakeEvent(Kind, House->IsDusk(), Random);
	if (!Stage || !Stage->Play(Event, Where))
	{
		return false;
	}
	const FName Cue = WindowLifeRules::CueFor(Kind);
	if (!Cue.IsNone())
	{
		LooterSound::PlayAt(this, Cue, Where.GetLocation(), WindowLifeRules::CueVolume);
	}
	Clock.Played(FObjectKey(House), Now, Event.Seconds, Random);
	Remember(Kind, House, Window, Now);
	return true;
}

bool UWindowLifeSubsystem::PlayShutter(AWindowShutter* Shutter, double Now)
{
	if (!Shutter || Shutter->GetState() != EWindowShutterState::Open)
	{
		return false;
	}
	// Its own swing and bang (World.ShutterSlam), after a breath: somebody reaching for it.
	Shutter->Slam(Random.FRandRange(0.f, WindowLifeRules::ShutterDelay));
	Clock.Played(HouseKeyOf(Shutter), Now, WindowLifeRules::ShutterSeconds + WindowLifeRules::ShutterDelay, Random);
	Remember(EWindowLifeKind::Shutter, Shutter, INDEX_NONE, Now);
	return true;
}

void UWindowLifeSubsystem::Remember(EWindowLifeKind Kind, AActor* Place, int32 Window, double Now)
{
	LastKind = Kind;
	LastPlace = Place;
	LastWindow = Window;
	LastTime = Now;
	++Events;
	UE_LOG(LogLooter, Verbose, TEXT("Window life: %s at %s (window %d)."), WindowLifeRules::KindName(Kind),
		Place ? *Place->GetActorNameOrLabel() : TEXT("nowhere"), Window);
}

// ---------------------------------------------------------------------------
// For the console
// ---------------------------------------------------------------------------

bool UWindowLifeSubsystem::FindSeenWindow(const FVector& Eyes, const FVector& ViewDirection, AHouseLights*& OutHouse, int32& OutWindow) const
{
	OutHouse = nullptr;
	OutWindow = INDEX_NONE;
	double Best = TNumericLimits<double>::Max();
	for (const TWeakObjectPtr<AHouseLights>& Each : Houses)
	{
		AHouseLights* House = Each.Get();
		for (int32 Index = 0; House && Index < House->NumLitWindows(); ++Index)
		{
			FTransform Window;
			if (House->GetLitWindow(Index, Window) && WindowLifeRules::Sees(Eyes, ViewDirection, Window.GetLocation(), Window.GetUnitAxis(EAxis::X)))
			{
				const double Squared = FVector::DistSquared(Eyes, Window.GetLocation());
				if (Squared < Best)
				{
					Best = Squared;
					OutHouse = House;
					OutWindow = Index;
				}
			}
		}
	}
	return OutHouse != nullptr;
}

AWindowShutter* UWindowLifeSubsystem::FindSeenShutter(const FVector& Eyes, const FVector& ViewDirection) const
{
	AWindowShutter* Found = nullptr;
	double Best = TNumericLimits<double>::Max();
	for (const TWeakObjectPtr<AWindowShutter>& Each : Shutters)
	{
		AWindowShutter* Shutter = Each.Get();
		if (Shutter && Shutter->GetState() == EWindowShutterState::Open
			&& WindowLifeRules::Sees(Eyes, ViewDirection, Shutter->GetActorLocation(), Shutter->GetActorForwardVector()))
		{
			const double Squared = FVector::DistSquared(Eyes, Shutter->GetActorLocation());
			if (Squared < Best)
			{
				Best = Squared;
				Found = Shutter;
			}
		}
	}
	return Found;
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Looter.WindowLife.Show [Shadow|Curtain|Shutter]: plays one now at the lit window (or open shutter) the player looks at. */
	void ShowCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					World = Context.World();
					break;
				}
			}
		}
		UWindowLifeSubsystem* Life = UWindowLifeSubsystem::Get(World);
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		if (!Life || !Player || !Player->PlayerCameraManager)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.WindowLife.Show: play a level first."));
			return;
		}
		const FVector Eyes = Player->PlayerCameraManager->GetCameraLocation();
		const FVector Look = Player->PlayerCameraManager->GetCameraRotation().Vector();
		const FString Asked = Args.IsEmpty() ? FString() : Args[0];
		const double Now = World->GetTimeSeconds();
		if (Asked.Equals(TEXT("Shutter"), ESearchCase::IgnoreCase))
		{
			Life->RefreshShutters();
			AWindowShutter* Shutter = Life->FindSeenShutter(Eyes, Look);
			UE_LOG(LogLooter, Display, TEXT("Looter.WindowLife.Show: %s"), Shutter && Life->PlayShutter(Shutter, Now)
				? TEXT("a shutter swings shut.") : TEXT("no open shutter in view (6-25 m, facing you)."));
			return;
		}
		AHouseLights* House = nullptr;
		int32 Window = INDEX_NONE;
		if (!Life->FindSeenWindow(Eyes, Look, House, Window))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.WindowLife.Show: no lit window in view (6-25 m, facing you)."));
			return;
		}
		const EWindowLifeKind Kind = Asked.Equals(TEXT("Curtain"), ESearchCase::IgnoreCase) || (Asked.IsEmpty() && !House->IsDusk())
			? EWindowLifeKind::Curtain : EWindowLifeKind::Shadow;
		const bool bPlayed = Life->PlayAt(House, Window, Kind, Now);
		UE_LOG(LogLooter, Display, TEXT("Looter.WindowLife.Show: %s at %s, window %d%s"), WindowLifeRules::KindName(Kind),
			*House->GetActorNameOrLabel(), Window + 1, bPlayed ? TEXT(".") : TEXT(": couldn't play (no M_WindowLife yet?)."));
	}

	FAutoConsoleCommandWithWorldAndArgs GShowCommand(TEXT("Looter.WindowLife.Show"),
		TEXT("Plays a window event now where the player looks: Shadow (the default at dusk), Curtain (by day), or Shutter."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowCommand));
}
#endif
