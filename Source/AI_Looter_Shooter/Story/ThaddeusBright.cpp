// AThaddeusBright: Tilly's dead father at his bench in the cold room, measuring a coffin that isn't there (Knocking Below).

#include "Story/ThaddeusBright.h"
#include "Audio/LooterSound.h"
#include "Story/SpeakerPointComponent.h"
#include "Components/StaticMeshComponent.h"

const FName AThaddeusBright::SpeakerTag(TEXT("Speaker_Thaddeus"));
const TCHAR* const AThaddeusBright::ModelPath = TEXT("/Game/Art/Creatures/SK_Thaddeus.SK_Thaddeus");
const TCHAR* const AThaddeusBright::ApronPath = TEXT("/Game/Art/Creatures/SM_ThaddeusApron.SM_ThaddeusApron");
const TCHAR* const AThaddeusBright::TapePath = TEXT("/Game/Art/Creatures/SM_ThaddeusTape.SM_ThaddeusTape");
const TCHAR* const AThaddeusBright::HatPath = TEXT("/Game/Art/Creatures/SM_ThaddeusHat.SM_ThaddeusHat");

namespace
{
	/** Knocking Below opens once Main 4, "Hallowed Ground", is finished: he's at his bench from then on. */
	const FName OpensAfter(TEXT("Main4"));

	/** His Sunday black on the Unpaid's body until his own is made (the Unpaid's second clothes). */
	const TCHAR* const TintPath = TEXT("/Game/Art/Materials/MI_Ghost_B.MI_Ghost_B");

	/** A knock's two raps: the fist comes down this often (s); the sound with the first. */
	constexpr float RapSeconds = ThaddeusPoses::KnockSeconds * 0.5f;
	constexpr float KnockHeardAt = RapSeconds * 0.5f;

	float Smooth(float Share)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Share, 0.f, 1.f));
	}
}

AThaddeusBright::AThaddeusBright()
{
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterThaddeus", "ThaddeusName", "Thaddeus Bright");
	Tags.Add(SpeakerTag);
	ShownWhen.AfterMissions = { OpensAfter };

	static USkeletalMesh* const Model = FindModel(ModelPath);
	static UMaterialInterface* const Tint = FindTint(TintPath);
	static UStaticMesh* const ApronModel = FindPart(ApronPath);
	static UStaticMesh* const TapeModel = FindPart(TapePath);
	static UStaticMesh* const HatModel = FindPart(HatPath);
	WearModel(Model, Model ? nullptr : Tint);
	// Bare-headed at his work unless he's given a hat; his tape in his left fist; his apron over his chest and lap.
	WearOnBone(Hat, HatModel, DeadFriendPoses::Hat);
	WearOnBone(Prop, TapeModel, DeadFriendPoses::ArmBone(0, 2));
	Apron = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Apron"));
	Apron->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Apron->SetGenerateOverlapEvents(false);
	Apron->SetCanEverAffectNavigation(false);
	Apron->SetCastShadow(false);
	WearOnBone(Apron, ApronModel, DeadFriendPoses::Spine);
}

void AThaddeusBright::BeginPlay()
{
	// Out of step with any other knocking: his first a little while after the cellar's door opens on him.
	NextKnock = KnockEvery * FMath::FRandRange(0.3f, 0.8f);
	MeasureClock = FMath::FRandRange(0.f, ThaddeusPoses::CycleSeconds);
	Super::BeginPlay();
}

EThaddeusPose AThaddeusBright::GetActivity() const
{
	if (IsTalking())
	{
		return EThaddeusPose::Look;
	}
	if (KnockClock >= 0.f)
	{
		return EThaddeusPose::Knock;
	}
	using namespace ThaddeusPoses;
	const float Phase = FMath::Fmod(MeasureClock, CycleSeconds);
	return Phase >= StretchSeconds + HoldSeconds && Phase < StretchSeconds + HoldSeconds + MarkSeconds ? EThaddeusPose::Mark : EThaddeusPose::Measure;
}

void AThaddeusBright::KnockNow()
{
	KnockClock = 0.f;
	RapsHeard = 0;
	++Knocks;
}

void AThaddeusBright::TickFriend(float DeltaSeconds)
{
	// Talking, he stops his work and looks up; after, he goes back to it where he left off.
	if (IsTalking())
	{
		KnockClock = -1.f;
		return;
	}
	using namespace ThaddeusPoses;
	if (KnockClock >= 0.f)
	{
		KnockClock += DeltaSeconds;
		if (RapsHeard == 0 && KnockClock >= KnockHeardAt)
		{
			// Two raps of his knuckles on the board: the knocking.
			RapsHeard = 1;
			LooterSound::PlayAt(this, LooterSoundCue::DeadFriends::BenchKnock, GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.0, 0.0, 90.0));
		}
		if (KnockClock >= KnockSeconds)
		{
			KnockClock = -1.f;
			NextKnock = KnockEvery * FMath::FRandRange(0.75f, 1.25f);
		}
		return;
	}
	const float Before = FMath::Fmod(MeasureClock, CycleSeconds);
	MeasureClock += DeltaSeconds;
	const float After = FMath::Fmod(MeasureClock, CycleSeconds);
	// The tape drawn out at the start of each measure.
	if (After < Before)
	{
		LooterSound::PlayAt(this, LooterSoundCue::DeadFriends::Tape, GetActorLocation() + GetActorForwardVector() * 45.f + FVector(0.0, 0.0, 95.0), 0.5f);
	}
	NextKnock -= DeltaSeconds;
	if (NextKnock <= 0.f)
	{
		KnockNow();
	}
}

FDeadFriendPose AThaddeusBright::WantedPose(float Time) const
{
	using namespace ThaddeusPoses;
	const EThaddeusPose Activity = GetActivity();
	FDeadFriendPose Wanted;
	switch (Activity)
	{
	case EThaddeusPose::Look:
		Wanted = Make(EThaddeusPose::Look);
		break;
	case EThaddeusPose::Knock:
	{
		// Two raps: the fist down, up, down, up.
		const float Rap = 0.5f - 0.5f * FMath::Cos(2.f * UE_PI * KnockClock / RapSeconds);
		Wanted = Make(EThaddeusPose::Knock, Rap);
		break;
	}
	case EThaddeusPose::Mark:
		Wanted = Make(EThaddeusPose::Mark);
		break;
	default:
	{
		// Drawn out, held, (marked,) let go.
		const float Phase = FMath::Fmod(MeasureClock, CycleSeconds);
		float Stretch = 1.f;
		if (Phase < StretchSeconds)
		{
			Stretch = Smooth(Phase / StretchSeconds);
		}
		else if (Phase >= StretchSeconds + HoldSeconds + MarkSeconds)
		{
			Stretch = 1.f - Smooth((Phase - StretchSeconds - HoldSeconds - MarkSeconds) / ReleaseSeconds);
		}
		Wanted = Make(EThaddeusPose::Measure, Stretch);
		break;
	}
	}
	// A slow breath, so he never stands like a post.
	Wanted.Bend(DeadFriendPoses::Chest, 1.f * FMath::Sin(Time * 1.1f));
	return Wanted;
}
