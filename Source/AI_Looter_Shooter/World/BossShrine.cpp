// ABossShrine: a vigil stone's model and the shapes standing in for it, its life in the level (dormant until its boss is
// first beaten), its boss and its state, and being used. BossShrineShow.cpp is its show and frames (the rise, the bell, the
// candles, the summons), BossShrineView.cpp what it says, BossShrineCallBack.cpp brings its boss back.

#include "World/BossShrine.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Core/LooterMenuGameMode.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterSpawner.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/World/BossShrineWidget.h"
#include "Weapons/WeaponFX.h"
#include "World/MinimapSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LooterBossShrine"

const FName ABossShrine::ShrineTag(TEXT("VigilStone"));
const TCHAR* const ABossShrine::ModelPath = TEXT("/Game/Art/Props/SM_VigilStone.SM_VigilStone");
const TCHAR* const ABossShrine::BellPath = TEXT("/Game/Art/Props/SM_VigilStoneBell.SM_VigilStoneBell");
const TCHAR* const ABossShrine::FlamesPath = TEXT("/Game/Art/Props/SM_VigilStoneFlames.SM_VigilStoneFlames");
const FName ABossShrine::InteractSocket(TEXT("Interact"));
const FName ABossShrine::BellSocket(TEXT("Bell"));
const FName ABossShrine::LightSocket(TEXT("Light"));

namespace
{
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** Without the model: a block of the stone's size (the engine's 1 m cube about its middle), 0.62 deep, 1.0 wide, 1.25 high. */
	const FVector StandInSize(62.0, 100.0, 125.0);
	/** A small drum for the bell, 18 cm across and 20 cm high. */
	const FVector StandInBellSize(18.0, 18.0, 20.0);

	/** Where the model's sockets are (VigilStone.py, in the actor's space: +X its front, the ground at Z 0). */
	FTransform StandInSocket(FName Socket)
	{
		if (Socket == ABossShrine::BellSocket)
		{
			// Under the crook's hook, right of the stone as seen from the front (the model's (0.45, -0.14, 1.535) m).
			return FTransform(FVector(14.0, -45.0, 153.5));
		}
		if (Socket == ABossShrine::LightSocket)
		{
			// Over the candles on the front step.
			return FTransform(FVector(20.0, 0.0, 42.0));
		}
		// The front of the stone, at the height hands reach the bell from.
		return FTransform(FVector(42.0, 0.0, 95.0));
	}

	/** A socket of the model in the actor's space, or where it would be on the model without it. */
	FTransform ModelSocket(const UStaticMesh* Model, FName Socket)
	{
		const UStaticMeshSocket* Found = Model ? Model->FindSocket(Socket) : nullptr;
		return Found ? FTransform(Found->RelativeRotation, Found->RelativeLocation, Found->RelativeScale) : StandInSocket(Socket);
	}

	/** An asset found only once it's imported, so the class works, and its tests run, without it. In a constructor. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	/** The candles' warm light (a world colour, not the UI's). */
	const FLinearColor CandleColor = FLinearColor::FromSRGBColor(FColor(0xFF, 0xB8, 0x66));
}

ABossShrine::ABossShrine()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = IdleTickSeconds;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const Model = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const BellModel = FindIfMade<UStaticMesh>(BellPath);
	static UStaticMesh* const FlamesModel = FindIfMade<UStaticMesh>(FlamesPath);
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);
	static UStaticMesh* const Cylinder = FindIfMade<UStaticMesh>(CylinderPath);

	// It rises out of the earth as one: the stone, its bell, flames and light all hang off this, never off the stone's own
	// scale (the stand-in's block is scaled).
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetMobility(EComponentMobility::Movable);

	// Solid, so it's stood against and the Interact key's line finds it; world dynamic, so the traces that look for the ground
	// among world-static things never take it for the ground.
	Stone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));
	Stone->SetupAttachment(Body);
	Stone->SetMobility(EComponentMobility::Movable);
	Stone->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Stone->SetGenerateOverlapEvents(false);
	Stone->SetCanEverAffectNavigation(false);
	Stone->SetStaticMesh(Model ? Model : Cube);
	if (!Model)
	{
		Stone->SetRelativeLocation(FVector(0.0, 0.0, StandInSize.Z * 0.5));
		Stone->SetRelativeScale3D(StandInSize / 100.0);
	}

	Bell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bell"));
	Bell->SetupAttachment(Body);
	Bell->SetMobility(EComponentMobility::Movable);
	Bell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bell->SetGenerateOverlapEvents(false);
	Bell->SetCanEverAffectNavigation(false);
	Bell->SetStaticMesh(BellModel ? BellModel : Cylinder);
	const FTransform BellAt = ModelSocket(Model, BellSocket);
	Bell->SetRelativeTransform(BellAt);
	if (!BellModel)
	{
		// The cylinder's pivot is its middle: it hangs its height under the hook.
		Bell->SetRelativeLocation(BellAt.GetLocation() - FVector(0.0, 0.0, StandInBellSize.Z * 0.5));
		Bell->SetRelativeScale3D(StandInBellSize / 100.0);
	}

	// The flames share the stone's pivot (VigilStone.py builds them in its space); a glowing surface, no light of its own.
	Flames = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flames"));
	Flames->SetupAttachment(Body);
	Flames->SetMobility(EComponentMobility::Movable);
	Flames->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flames->SetGenerateOverlapEvents(false);
	Flames->SetCanEverAffectNavigation(false);
	Flames->SetCastShadow(false);
	Flames->SetStaticMesh(FlamesModel);
	Flames->SetVisibility(false);

	// A small warm light over the candles: shadowless and short, so it costs next to nothing.
	CandleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CandleLight"));
	CandleLight->SetupAttachment(Body);
	CandleLight->SetRelativeTransform(ModelSocket(Model, LightSocket));
	CandleLight->SetCastShadows(false);
	CandleLight->SetIntensityUnits(ELightUnits::Candelas);
	CandleLight->SetIntensity(CandleCandela);
	CandleLight->SetAttenuationRadius(CandleRadius);
	CandleLight->SetLightColor(CandleColor);
	CandleLight->SetVisibility(false);

	Prompt = LOCTEXT("Prompt", "Keep vigil");
	Tags.Add(ShrineTag);
}

void ABossShrine::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; this one it always carries (the map and the console find it by it).
	Tags.AddUnique(ShrineTag);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	BossWonHandle = UBossComponent::OnAnyBossWon.AddUObject(this, &ABossShrine::HandleAnyBossWon);
	BellRest = Bell->GetRelativeRotation().Quaternion();
	CandleLight->SetIntensity(CandleCandela);
	CandleLight->SetAttenuationRadius(CandleRadius);
	FindBoss();
	SetCandles(false, false);
	SetStanding(false);
	// Standing at once when it rose on an earlier visit; else it waits for its boss's defeat and the player.
	Refresh();
	// A lair's boss away for this visit after a Save & Quit comes back once every actor has begun (its lair has decided
	// whether it's away by then).
	UWorld* World = GetWorld();
	if (CallKind == EBossShrineCall::Lair && World && World->IsGameWorld() && !ALooterMenuGameMode::IsMenuWorld(World))
	{
		LoadCheckTimer = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			BringBackAfterLoad();
		}));
	}
}

void ABossShrine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UBossComponent::OnAnyBossWon.Remove(BossWonHandle);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoadCheckTimer);
	}
	// Removed from a level that goes on (a build script, the console) during Abel's called-back fight: his story as it was.
	if (bAbelRematch)
	{
		EndAbelRematch(/*bWon*/ false);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Its boss and its state
// ---------------------------------------------------------------------------

void ABossShrine::FindBoss()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (CallKind == EBossShrineCall::Lair && !Lair && !BossKey.IsNone())
	{
		for (TActorIterator<AEncounterSpawner> It(World); It; ++It)
		{
			if (It->GetSpawnerId() == BossKey || It->LegendaryId == BossKey)
			{
				Lair = *It;
				break;
			}
		}
	}
	if (CallKind == EBossShrineCall::Placed && !PlacedBoss && BossClass)
	{
		for (TActorIterator<ACreatureBase> It(World, BossClass); It; ++It)
		{
			PlacedBoss = *It;
			break;
		}
	}
}

bool ABossShrine::IsOwnBoss(const ACreatureBase& Creature) const
{
	if (CallKind == EBossShrineCall::Placed && PlacedBoss)
	{
		return &Creature == PlacedBoss.Get();
	}
	return BossClass && Creature.IsA(BossClass);
}

void ABossShrine::HandleAnyBossWon(UBossComponent& Boss)
{
	const ACreatureBase* Creature = Boss.GetCreature();
	if (!Creature || Creature->GetWorld() != GetWorld() || !IsOwnBoss(*Creature))
	{
		return;
	}
	// Laid to rest: the session keeps when (its time played, the clock, this load), for the call back.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->NoteBossRecallDefeat(GetWorld(), BossKey, Creature->Level);
	}
	else
	{
		LocalRecord.DefeatedAt = GetPlayedNow();
		LocalRecord.DefeatedOn = FDateTime::Now();
		LocalRecord.LoadsAtDefeat = GetLoadsNow();
		LocalRecord.Level = Creature->Level;
		++LocalRecord.Defeats;
	}
	SinceDefeat = 0.f;
	bLairCallOn = false;
	UE_LOG(LogLooter, Log, TEXT("%s: %s laid to rest%s."), *GetActorNameOrLabel(), *GetBossName().ToString(),
		Boss.IsRematch() ? TEXT(" again") : TEXT(""));
	Refresh();
}

const FBossRecallRecord* ABossShrine::GetRecord() const
{
	if (const USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		return Sessions->FindBossRecall(BossKey);
	}
	return LocalRecord.HasDefeat() || LocalRecord.bRisen ? &LocalRecord : nullptr;
}

double ABossShrine::GetPlayedNow() const
{
	if (TestClock.PlayedSeconds.IsSet())
	{
		return TestClock.PlayedSeconds.GetValue();
	}
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const UWorld* World = GetWorld();
	return Sessions ? Sessions->GetPlayedSecondsNow(World) : (World ? World->GetTimeSeconds() : 0.0);
}

int32 ABossShrine::GetLoadsNow() const
{
	if (TestClock.Loads.IsSet())
	{
		return TestClock.Loads.GetValue();
	}
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	return Sessions ? Sessions->GetSessionLoads() : 0;
}

bool ABossShrine::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

bool ABossShrine::HasBossBeenBeaten() const
{
	if (const FBossRecallRecord* Record = GetRecord(); Record && (Record->HasDefeat() || Record->bRisen))
	{
		return true;
	}
	// A save from before the stones: the campaign's beaten bosses (Abel), the story (BeatenWhen), its lair's Legendary death.
	if (const UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		if (!BossKey.IsNone() && Campaign.DefeatedBosses.Contains(BossKey))
		{
			return true;
		}
		if (!BeatenWhen.IsEmpty() && BeatenWhen.IsMet(Campaign, Runner))
		{
			return true;
		}
	}
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	return CallKind == EBossShrineCall::Lair && Lair && !Lair->LegendaryId.IsNone() && Sessions
		&& Sessions->GetLegendaryDefeatedAt(GetWorld(), Lair->LegendaryId) >= 0.0;
}

bool ABossShrine::IsBossPresent() const
{
	if (CallKind == EBossShrineCall::Lair)
	{
		const AEncounterSpawner* Den = Lair.Get();
		if (!Den)
		{
			return false;
		}
		for (const ACreatureBase* Each : Den->GetAliveCreatures())
		{
			if (Each && IsOwnBoss(*Each))
			{
				return true;
			}
		}
		// Due there as the player comes: its lair on, back this visit, and not cleared (she lurks until someone comes).
		const EEncounterState State = Den->GetState();
		return Den->IsStoryActive() && !Den->IsLegendaryAway() && State != EEncounterState::Cleared && State != EEncounterState::Off;
	}
	const ACreatureBase* Boss = PlacedBoss.Get();
	if (!Boss || Boss->IsDead())
	{
		return false;
	}
	if (const AAbelKeeper* Abel = Cast<AAbelKeeper>(Boss))
	{
		return Abel->IsPresent();
	}
	return !Boss->IsHidden();
}

BossShrineRules::FInputs ABossShrine::ReadInputs() const
{
	const FBossRecallRecord* Record = GetRecord();
	BossShrineRules::FInputs In;
	In.bEverBeaten = HasBossBeenBeaten();
	In.bStoryAllows = StoryHolds(CallableWhen);
	In.bBossPresent = IsBossPresent();
	In.DefeatedAt = Record ? Record->DefeatedAt : -1.0;
	In.PlayedNow = GetPlayedNow();
	In.LoadsAtDefeat = Record ? Record->LoadsAtDefeat : 0;
	In.LoadsNow = GetLoadsNow();
	return In;
}

void ABossShrine::Refresh()
{
	const FBossRecallRecord* Record = GetRecord();
	const BossShrineRules::FInputs In = ReadInputs();
	const EBossShrineState Was = Verdict.State;
	Verdict = BossShrineRules::Evaluate(In);
	if (Verdict.State != Was)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: %s now %d."), *GetActorNameOrLabel(), *BossKey.ToString(), static_cast<int32>(Verdict.State));
	}

	// Standing from the start once it has risen before; otherwise it rises the first time the player comes near after the fight.
	if (!bRisen && In.bEverBeaten)
	{
		if (Record && Record->bRisen)
		{
			Rise(/*bShow*/ false);
		}
		else
		{
			const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
			const APawn* Player = FindPlayer();
			const float Distance = Player ? static_cast<float>(FVector::Dist(Player->GetActorLocation(), GetActorLocation())) : UE_BIG_NUMBER;
			if (BossShrineRules::ShouldRise(In.bEverBeaten, bRisen, Scenes && Scenes->IsPlaying(), In.bBossPresent, SinceDefeat, Distance,
				RiseRadius))
			{
				Rise(/*bShow*/ true);
			}
		}
	}
	if (bLairCallOn)
	{
		MarkLairBosses();
	}
	// Its candles burn while its boss can be called; they go out otherwise (after a summons' flare has had its moment).
	const bool bWantLit = bRisen && !bRising && Verdict.State == EBossShrineState::Ready;
	if (bWantLit != bCandlesLit && (bWantLit || CandlesOutIn <= 0.f))
	{
		SetCandles(bWantLit, /*bShow*/ true);
	}
	UpdateTickPace();
}

APawn* ABossShrine::FindPlayer() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	if (const APlayerController* PC = World->GetFirstPlayerController())
	{
		return PC->GetPawn();
	}
	// A test level has no player controller: its player stand-in is a plain character that isn't a creature.
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		if (!It->IsA<ACreatureBase>())
		{
			return *It;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions ABossShrine::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.bUsable = bRisen && !bRising;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	return Options;
}

bool ABossShrine::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Use(User.GetOwner());
}

TOptional<FVector> ABossShrine::GetInteractionLocation() const
{
	const UStaticMesh* Model = Stone ? Stone->GetStaticMesh() : nullptr;
	const bool bModel = Model && Model->GetPathName() == ModelPath;
	return TOptional<FVector>((ModelSocket(bModel ? Model : nullptr, InteractSocket) * GetActorTransform()).GetLocation());
}

bool ABossShrine::Use(AActor* Player)
{
	APlayerController* PC = Cast<APlayerController>(Player);
	if (!PC)
	{
		const APawn* Pawn = Cast<APawn>(Player);
		PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	}
	if (!PC || !PC->IsLocalController() || !bRisen || bRising)
	{
		return false;
	}
	if (const ALooterHUD* HUD = ALooterHUD::FindFor(PC); HUD && HUD->IsMenuOpen())
	{
		return false;
	}
	Refresh();
	if (!UBossShrineWidget::OpenFor(PC, this))
	{
		return false;
	}
	// Kneeling at it: the candles gust, the stone hums, the bell stirs.
	LooterSound::PlayAt(this, LooterSoundCue::Shrine::Activate, GetInteractionLocation().Get(GetActorLocation()));
	SwingBell(0.2f);
	if (bCandlesLit)
	{
		CandleFlare = FMath::Max(CandleFlare, 0.6f);
	}
	UpdateTickPace();
	UE_LOG(LogLooter, Log, TEXT("%s: kept vigil at (%s)."), *GetActorNameOrLabel(), *GetBossName().ToString());
	return true;
}

bool ABossShrine::CallBack(APawn* Player)
{
	FindBoss();
	Refresh();
	if (Verdict.State != EBossShrineState::Ready)
	{
		LooterSound::Play2D(this, LooterSoundCue::Shrine::Unavailable);
		return false;
	}
	const FVector Arena = GetArenaSpot();
	const bool bCame = CallKind == EBossShrineCall::Lair ? CallFromLair() : CallPlaced();
	if (!bCame)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: %s couldn't be called back (%s)."), *GetActorNameOrLabel(), *GetBossName().ToString(),
			CallKind == EBossShrineCall::Lair ? (Lair ? *Lair->Describe() : TEXT("no lair")) : (PlacedBoss ? TEXT("its fight wouldn't ready")
			: TEXT("no boss placed")));
		LooterSound::Play2D(this, LooterSoundCue::Shrine::Unavailable);
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the bell rung; %s rises again."), *GetActorNameOrLabel(), *GetBossName().ToString());
	PlaySummonShow(Arena);
	Refresh();
	return true;
}

FVector ABossShrine::GetArenaSpot() const
{
	if (CallKind == EBossShrineCall::Placed && PlacedBoss)
	{
		return PlacedBoss->GetHome().GetLocation();
	}
	return Lair ? Lair->GetActorLocation() : GetActorLocation();
}

#undef LOCTEXT_NAMESPACE
