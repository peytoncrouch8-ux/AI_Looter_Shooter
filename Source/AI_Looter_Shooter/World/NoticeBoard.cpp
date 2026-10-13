// ANoticeBoard: a town's notice board as a mission board: reading it, its postings as they stand, tracking and turning in.

#include "World/NoticeBoard.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

const FName ANoticeBoard::BoardTag(TEXT("NoticeBoard"));
const FName ANoticeBoard::GiverTag(TEXT("Speaker_NoticeBoard"));
const FName ANoticeBoard::ReadEvent(TEXT("NoticeBoard.Read"));

namespace
{
	/** The face's box: as wide and tall as SM_NoticeBoard's face, a few cm deep, just behind the point read from. */
	const FVector FaceExtent(3.0, 78.0, 48.0);
	constexpr double FaceBehindPoint = 5.0;

	ENoticePosting StateOf(const UMissionRunner& Runner, const UMissionDefinition& Mission)
	{
		const FName MissionId = Mission.GetMissionId();
		// Running beats finished: one played again (the console) is up again while it runs.
		if (Runner.IsRunning(MissionId))
		{
			return Runner.IsReadyToTurnIn(MissionId) ? ENoticePosting::Ready : ENoticePosting::Open;
		}
		if (Runner.IsCompleted(MissionId))
		{
			return ENoticePosting::Done;
		}
		if (Runner.IsReadyToTurnIn(MissionId))
		{
			return ENoticePosting::Ready;
		}
		return Runner.IsAvailable(MissionId) ? ENoticePosting::Offered : ENoticePosting::Locked;
	}

	/** A bounty board's order for what its tags bring: jobs to take first, then ones to turn in, under way, done. */
	int32 BoardRank(ENoticePosting State)
	{
		switch (State)
		{
		case ENoticePosting::Offered: return 0;
		case ENoticePosting::Ready:   return 1;
		case ENoticePosting::Open:    return 2;
		case ENoticePosting::Done:    return 3;
		case ENoticePosting::Locked:  break;
		}
		return 4;
	}
}

ANoticeBoard::ANoticeBoard()
{
	// It never changes on its own: a screen opens, nothing moves.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Seen only by the Visibility channel (the Interact key's line, its sight check): never by shots, feet or the camera.
	Face = CreateDefaultSubobject<UBoxComponent>(TEXT("Face"));
	Face->SetupAttachment(Root);
	Face->SetMobility(EComponentMobility::Movable);
	Face->SetBoxExtent(FaceExtent);
	Face->SetRelativeLocation(FaceOffset - FVector(FaceBehindPoint + FaceExtent.X, 0.0, 0.0));
	Face->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Face->SetCollisionObjectType(ECC_WorldDynamic);
	Face->SetCollisionResponseToAllChannels(ECR_Ignore);
	Face->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Face->SetGenerateOverlapEvents(false);
	Face->SetCanEverAffectNavigation(false);

	// Skyreach's board, the first and so far the only one: a board elsewhere names its own postings.
	Postings = { TEXT("WebHollow"), TEXT("RangePractice"), TEXT("Wallow"), TEXT("Lookout"), TEXT("BoardSkiff") };
	MainPosting = TEXT("WebHollow");
	Title = NSLOCTEXT("LooterNoticeBoard", "Title", "Notice Board");
	Subtitle = NSLOCTEXT("LooterNoticeBoard", "Subtitle", "Odd jobs, posted by the town");
	LockedNote = NSLOCTEXT("LooterNoticeBoard", "LockedNote", "ARMED HANDS ONLY. There's a rifle on the gun rack in the square. Come back with it.");
	Prompt = NSLOCTEXT("LooterNoticeBoard", "Prompt", "Read the notice board");
	Tags.Add(BoardTag);
	Tags.Add(GiverTag);
}

void ANoticeBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The face follows the point read from, when a board's model asks for another.
	if (Face)
	{
		Face->SetRelativeLocation(FaceOffset - FVector(FaceBehindPoint + FaceExtent.X, 0.0, 0.0));
	}
}

void ANoticeBoard::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; these it always carries.
	Tags.AddUnique(BoardTag);
	Tags.AddUnique(GiverTag);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void ANoticeBoard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions ANoticeBoard::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.bTap = true;
	Options.bHold = false;
	// A posting waiting to be turned in says so on the key, as a giver's door does.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bTurnIn = Runner && Runner->FindTurnInAt(this);
	Options.TapPrompt = bTurnIn ? NSLOCTEXT("LooterNoticeBoard", "TurnInPrompt", "Turn in") : Prompt;
	Options.Reach = Reach;
	return Options;
}

bool ANoticeBoard::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Use(User.GetOwner());
}

TOptional<FVector> ANoticeBoard::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(FaceOffset));
}

bool ANoticeBoard::Use(AActor* Player)
{
	ALooterHUD* HUD = ALooterHUD::FindFor(Player);
	if (!HUD || HUD->IsMenuOpen())
	{
		return false;
	}
	// Read before the screen opens, so it lists the postings this read puts up.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Read(*Runner);
	}
	if (!HUD->OpenNoticeBoard(this))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the notice board is read%s."), *GetActorNameOrLabel(),
		Player ? *FString::Printf(TEXT(" by %s"), *Player->GetName()) : TEXT(""));
	return true;
}

void ANoticeBoard::Read(UMissionRunner& Runner)
{
	const bool bMainWasUp = MainPosting.IsNone() || Runner.IsRunning(MainPosting) || Runner.IsCompleted(MainPosting);
	Runner.NotifyEvent(FMissionEvent::Named(ReadEvent, this));
	if (!bMainWasUp && Runner.IsRunning(MainPosting))
	{
		Runner.TrackMission(MainPosting);
	}
}

// ---------------------------------------------------------------------------
// Postings
// ---------------------------------------------------------------------------

bool ANoticeBoard::Gives(const UMissionDefinition& Mission) const
{
	return Mission.NeedsTurnIn() && ActorHasTag(Mission.TurnIn.SpeakerTag);
}

bool ANoticeBoard::Offers(const UMissionDefinition& Mission) const
{
	return Mission.IsOffered() && ActorHasTag(Mission.Offer.GiverTag);
}

TArray<FNoticeBoardPosting> ANoticeBoard::GatherPostings(const UMissionRunner& Runner) const
{
	TArray<FNoticeBoardPosting> Found;
	const FName Tracked = Runner.GetTrackedMission();
	auto Pin = [&](const UMissionDefinition& Mission, ENoticePosting State)
	{
		const FName MissionId = Mission.GetMissionId();
		FNoticeBoardPosting& Posting = Found.AddDefaulted_GetRef();
		Posting.MissionId = MissionId;
		Posting.Mission = &Mission;
		Posting.State = State;
		Posting.bTurnedInHere = Gives(Mission);
		Posting.bOfferedHere = Offers(Mission);
		Posting.bTracked = State != ENoticePosting::Done && Tracked == MissionId;
		if (State == ENoticePosting::Open)
		{
			// The first objective still to do, as the tracker counts it.
			for (const FMissionObjectiveView& View : Runner.GetObjectiveViews(MissionId))
			{
				if (!View.bDone)
				{
					Posting.Progress = View.Progress;
					break;
				}
			}
		}
	};
	for (const FName MissionId : Postings)
	{
		const UMissionDefinition* Mission = Runner.FindDefinition(MissionId);
		if (!Mission)
		{
			continue;
		}
		const ENoticePosting State = StateOf(Runner, *Mission);
		// One the game starts itself (the skiff, once Web Hollow is turned in) isn't on the board until it's up.
		if (State == ENoticePosting::Locked && Mission->Start == EMissionStart::Manual)
		{
			continue;
		}
		Pin(*Mission, State);
	}

	// The bounty board: what its tags bring that its list doesn't name, the jobs to take first. What isn't offered yet
	// stays off it, as a job not posted yet.
	TArray<TPair<const UMissionDefinition*, ENoticePosting>> Tagged;
	for (const UMissionDefinition* Mission : Runner.GetDefinitions())
	{
		if (!Mission || Postings.Contains(Mission->GetMissionId()) || (!Offers(*Mission) && !Gives(*Mission)))
		{
			continue;
		}
		const ENoticePosting State = StateOf(Runner, *Mission);
		if (State != ENoticePosting::Locked)
		{
			Tagged.Emplace(Mission, State);
		}
	}
	Tagged.StableSort([](const TPair<const UMissionDefinition*, ENoticePosting>& A, const TPair<const UMissionDefinition*, ENoticePosting>& B)
	{
		return BoardRank(A.Value) < BoardRank(B.Value);
	});
	for (const TPair<const UMissionDefinition*, ENoticePosting>& Each : Tagged)
	{
		Pin(*Each.Key, Each.Value);
	}
	return Found;
}

bool ANoticeBoard::TurnIn(UMissionRunner& Runner, FName MissionId) const
{
	const UMissionDefinition* Mission = Runner.FindDefinition(MissionId);
	if (!Mission || !Gives(*Mission) || !Runner.IsRunning(MissionId) || !Runner.IsReadyToTurnIn(MissionId))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s turned in at the board."), *GetActorNameOrLabel(), *MissionId.ToString());
	return Runner.TurnIn(MissionId);
}

bool ANoticeBoard::Track(UMissionRunner& Runner, FName MissionId) const
{
	if (!Runner.IsRunning(MissionId))
	{
		return false;
	}
	Runner.TrackMission(MissionId);
	return Runner.GetTrackedMission() == MissionId;
}
