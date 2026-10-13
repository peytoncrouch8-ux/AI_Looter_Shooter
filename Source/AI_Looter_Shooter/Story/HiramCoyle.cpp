// AHiramCoyle: the ore lift's dead engineman on his stool by the boiler (Dead Lift).

#include "Story/HiramCoyle.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/DeadMinerCreature.h"
#include "Story/SpeakerPointComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

const FName AHiramCoyle::SpeakerTag(TEXT("Speaker_Hiram"));
const FName AHiramCoyle::TubeTag(TEXT("Speaker_LiftTube"));
const TCHAR* const AHiramCoyle::ModelPath = TEXT("/Game/Art/Creatures/SK_Hiram.SK_Hiram");
const TCHAR* const AHiramCoyle::CapPath = TEXT("/Game/Art/Creatures/SM_HiramCap.SM_HiramCap");

namespace
{
	/** Dead Lift opens once Main 5, "The Keeper's Lantern", is finished: he's by his boiler from then on. */
	const FName OpensAfter(TEXT("Main5"));

	/** The dead miners' tint he wears on the Unpaid's body until his own is made. */
	const TCHAR* const TintPath = TEXT("/Game/Art/Materials/MI_DeadMiner_B.MI_DeadMiner_B");
}

AHiramCoyle::AHiramCoyle()
{
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterHiram", "HiramName", "Hiram Coyle");
	Tags.Add(SpeakerTag);
	ShownWhen.AfterMissions = { OpensAfter };

	static USkeletalMesh* const Model = FindModel(ModelPath);
	static UMaterialInterface* const Tint = FindTint(TintPath);
	static UStaticMesh* const Cap = FindPart(CapPath);
	static UStaticMesh* const MinerHat = FindPart(ADeadMinerCreature::HatPath);
	static UStaticMesh* const UnpaidHat = FindPart(TEXT("/Game/Art/Creatures/SM_UnpaidHat.SM_UnpaidHat"));
	WearModel(Model, Model ? nullptr : Tint);
	WearOnBone(Hat, Cap ? Cap : (MinerHat ? MinerHat : UnpaidHat), DeadFriendPoses::Hat);
	Prop->SetVisibility(false);
}

void AHiramCoyle::BeginPlay()
{
	// The lift head's tube: when anyone talks through it, it's his voice coming up, and he leans to it down here.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TubeTag))
			{
				if (USpeakerPointComponent* Point = It->FindComponentByClass<USpeakerPointComponent>())
				{
					Tube = Point;
					Point->OnTalked.AddUObject(this, &AHiramCoyle::HandleTubeTalked);
				}
				break;
			}
		}
	}
	NextListen = ListenEvery * FMath::FRandRange(0.4f, 1.f);
	Super::BeginPlay();
}

void AHiramCoyle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USpeakerPointComponent* Point = Tube.Get())
	{
		Point->OnTalked.RemoveAll(this);
	}
	Tube.Reset();
	Super::EndPlay(EndPlayReason);
}

void AHiramCoyle::HandleTubeTalked(USpeakerPointComponent& Point, AActor* InListener)
{
	LeanToTube(ListenSeconds);
}

void AHiramCoyle::LeanToTube(float Seconds)
{
	if (!bAtTube)
	{
		LooterSound::PlayAt(this, LooterSoundCue::DeadFriends::Tube, GetActorLocation() + FVector(0.0, 0.0, 120.0), 0.6f);
	}
	bAtTube = true;
	TubeLeft = FMath::Max(TubeLeft, Seconds);
}

void AHiramCoyle::TickFriend(float DeltaSeconds)
{
	// Through the tube's talk he stays at it; otherwise now and then he leans over to listen for the lift head's bell.
	const USpeakerPointComponent* Point = Tube.Get();
	if (Point && Point->IsTalking())
	{
		bAtTube = true;
		TubeLeft = FMath::Max(TubeLeft, 1.f);
	}
	if (bAtTube)
	{
		TubeLeft -= DeltaSeconds;
		// Talked to himself, he turns from the tube to whoever is there.
		if (TubeLeft <= 0.f || IsTalking())
		{
			bAtTube = false;
			TubeLeft = 0.f;
		}
		return;
	}
	NextListen -= DeltaSeconds;
	if (NextListen <= 0.f && !IsTalking())
	{
		NextListen = ListenEvery * FMath::FRandRange(0.8f, 1.2f);
		LeanToTube(ListenSeconds);
	}
}

FDeadFriendPose AHiramCoyle::WantedPose(float Time) const
{
	FDeadFriendPose Wanted = HiramPoses::Make(bAtTube ? EHiramPose::Tube : EHiramPose::Sit, TubeSide);
	// A slow breath in his shoulders, so he never sits like a post.
	Wanted.Bend(DeadFriendPoses::Chest, 1.2f * FMath::Sin(Time * 1.3f));
	return Wanted;
}
