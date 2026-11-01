// AGangSkiffFlyBy: Lucky Ned's dark skiff slipping off Vane's barge and away south over the Hogback (Main 3), with Vane and
// Ned on the radio and Hob's word after.

#include "World/GangSkiffFlyBy.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "Story/VaneBroadcast.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AGangSkiffFlyBy::HullPath = TEXT("/Game/Art/Vehicles/SM_Skiff_A_Gang.SM_Skiff_A_Gang");

namespace
{
	/** It looks whether it's due this often (s). */
	constexpr float FlyByLookSeconds = 0.5f;
}

AGangSkiffFlyBy::AGangSkiffFlyBy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Skiff(HullPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Far off and in the air the whole time: seen, never touched.
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetStaticMesh(Skiff.Object);
	Hull->SetMobility(EComponentMobility::Movable);
	Hull->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Hull->SetCanEverAffectNavigation(false);
}

void AGangSkiffFlyBy::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);
	bFlying = false;
	bRadioSaid = false;
	bAfterSaid = false;
	Clock = 0.f;
	DueSince = -1.0;
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	bDone = !DoneFlag.IsNone() && Runner && Runner->HasFlag(DoneFlag);
	if (!Path.Build(Course, Seconds))
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no course to fly (%d points)."), *GetActorNameOrLabel(), Course.Num());
		return;
	}
	if (!bDone)
	{
		GetWorldTimerManager().SetTimer(DueTimer, this, &AGangSkiffFlyBy::CheckDue, FlyByLookSeconds, /*bLoop*/ true);
	}
}

void AGangSkiffFlyBy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DueTimer);
	}
	LooterSound::Stop(Engine.Get(), 0.f);
	Engine.Reset();
	Super::EndPlay(EndPlayReason);
}

void AGangSkiffFlyBy::CheckDue()
{
	UWorld* World = GetWorld();
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (bDone || bFlying || !World || !Runner)
	{
		return;
	}
	if (!PlayWhen.IsMet(Runner->GetCampaign(), Runner))
	{
		DueSince = -1.0;
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (DueSince < 0.0)
	{
		DueSince = Now;
	}
	// Into a quiet moment (Delia's words first), or after a while anyway.
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	const bool bQuiet = !UVaneBroadcast::IsSpeaking(this) && (!Captions || Captions->GetQueue().IsEmpty());
	if (bQuiet || Now - DueSince >= QuietWaitSeconds)
	{
		Begin();
	}
}

bool AGangSkiffFlyBy::Begin()
{
	if (bFlying || !Path.IsBuilt())
	{
		return false;
	}
	bFlying = true;
	bRadioSaid = false;
	bAfterSaid = false;
	Clock = 0.f;
	SetActorTransform(Path.PoseAt(0.f));
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DueTimer);
	}
	// Its engine's drone, a little higher and harder than the Lamplighter's: a small fast boat.
	Engine = LooterSound::Start(this, LooterSoundCue::ShipEngine, Hull, NAME_None, 1.f, 1.3f);
	UE_LOG(LogLooter, Log, TEXT("%s: casting off (%.0f m in %.0f s)."), *GetActorNameOrLabel(), Path.GetLength() / 100.f, Path.GetDuration());
	return true;
}

void AGangSkiffFlyBy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AGangSkiffFlyBy::Advance(float DeltaSeconds)
{
	if (bFlying)
	{
		Clock += DeltaSeconds;
		SetActorTransform(Path.PoseAt(FMath::Min(Clock, Path.GetDuration())));
		if (!bRadioSaid && Clock >= RadioAt)
		{
			bRadioSaid = true;
			if (RadioLines && !RadioLines->Lines.IsEmpty())
			{
				UVaneBroadcast::Say(this, RadioLines->Lines);
			}
		}
		if (Clock >= Path.GetDuration())
		{
			Land();
		}
	}
	// The words after, once the radio is off the air (they may come after it's out of sight).
	if (bRadioSaid && !bAfterSaid && !UVaneBroadcast::IsSpeaking(this))
	{
		bAfterSaid = true;
		if (AfterLines && !AfterLines->Lines.IsEmpty())
		{
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
			{
				Captions->Play(AfterLines->Lines, ECaptionPlay::Queue);
			}
		}
	}
	if (!bFlying && (bAfterSaid || !bRadioSaid))
	{
		SetActorTickEnabled(false);
	}
}

void AGangSkiffFlyBy::Land()
{
	bFlying = false;
	bDone = true;
	SetActorHiddenInGame(true);
	LooterSound::Stop(Engine.Get(), 0.6f);
	Engine.Reset();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		if (!DoneFlag.IsNone())
		{
			Runner->SetFlag(DoneFlag);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: gone south over the Hogback."), *GetActorNameOrLabel());
}
