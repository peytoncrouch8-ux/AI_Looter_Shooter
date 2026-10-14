#include "Story/MisterSexton.h"
#include "AI_Looter_Shooter.h"
#include "Story/SpeakerPointComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AMisterSexton::ModelPath = TEXT("/Game/Art/Characters/SM_MisterSexton.SM_MisterSexton");
const TCHAR* const AMisterSexton::LedgerPath = TEXT("/Game/Art/Characters/SM_SextonLedger.SM_SextonLedger");
const FName AMisterSexton::LedgerSocket(TEXT("Ledger"));
const FName AMisterSexton::SpeakerSocket(TEXT("Speaker"));

namespace
{
	/** His breath shows to a player this near (cm): then he ticks every frame; farther, every FarTickSeconds. */
	constexpr float BreathSeenWithin = 2500.f;
	constexpr float FarTickSeconds = 0.25f;

	/** An asset by path when it's in this checkout, else null (as Hob finds his model). */
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
}

AMisterSexton::AMisterSexton()
{
	// Far off, a look a few times a second is enough to see whether he may go; near, his breath ticks every frame (Tick).
	PrimaryActorTick.TickInterval = FarTickSeconds;

	// A posed idle (Docs/Areas/RansomsRest.md, NPCs): he doesn't turn to whoever talks to him (one posed mesh: no head of its
	// own to turn), and his breath is a swell of his body about his seat (UpdatePose), not the story character's bob.
	TurnSpeed = 0.f;
	BreathHeight = 0.f;
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterStory", "SextonName", "Mister Sexton");

	// His hull stops the player's body and finds the Interact key's line (Visibility); bullets and the creatures' pellets
	// pass through him, and the third-person camera doesn't bump on him.
	Body->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2 /*Weapon: bullets*/, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel1 /*Projectile: pellets*/, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	Ledger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ledger"));
	Ledger->SetupAttachment(Body);
	Ledger->SetMobility(EComponentMobility::Movable);
	Ledger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ledger->SetGenerateOverlapEvents(false);

	static UStaticMesh* const Seated = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Book = FindIfMade<UStaticMesh>(LedgerPath);
	if (!Seated)
	{
		// No model in this checkout: the story character's placeholder stands on the rail instead.
		return;
	}
	// The seat point under him is his model's pivot: the body sits on the actor's spot (the Sit socket), unscaled, facing
	// as it does (into the deck).
	Body->SetStaticMesh(Seated);
	Body->SetRelativeLocation(FVector::ZeroVector);
	Body->SetRelativeScale3D(FVector::OneVector);
	if (Book)
	{
		// "Attach SM_SextonLedger to Sexton's Ledger socket, snapped to target" (MisterSexton.py).
		Ledger->SetStaticMesh(Book);
		Ledger->SetupAttachment(Body, Seated->FindSocket(LedgerSocket) ? LedgerSocket : NAME_None);
	}
	if (Seated->FindSocket(SpeakerSocket))
	{
		// His captions come from his chin.
		SpeakerPoint->SetupAttachment(Body, SpeakerSocket);
		SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
	}
}

void AMisterSexton::BeginPlay()
{
	// The story character's start reads the story: as the level begins he's simply there or not, watched or not.
	Super::BeginPlay();
	bBegun = true;
}

void AMisterSexton::RefreshShown()
{
	const bool bWanted = IsStoryShown();
	if (!bWanted && bBegun && IsShown() && IsWatched())
	{
		// Done with him, but the player is looking at him or hearing him out: he goes when they aren't.
		if (!bLeaving)
		{
			UE_LOG(LogLooter, Log, TEXT("%s: the story is done with him (%s); he goes once nobody is looking."), *GetActorNameOrLabel(),
				*ShownWhen.Describe());
		}
		bLeaving = true;
		return;
	}
	bLeaving = false;
	SetShown(bWanted);
}

void AMisterSexton::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Shown, he ticks; leaving, he looks again: gone once unwatched (or staying, if the story brought him back meanwhile).
	if (bLeaving && !IsWatched())
	{
		RefreshShown();
	}
	// His breath is smooth only for a player near enough to see it: every frame then, a few times a second otherwise.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bClose = Body && Player && Body->WasRecentlyRendered(SeenSeconds)
		&& FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= FMath::Square(BreathSeenWithin);
	const float Wanted = bClose ? 0.f : FarTickSeconds;
	if (!FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, Wanted))
	{
		SetActorTickInterval(Wanted);
	}
}

void AMisterSexton::UpdatePose(float DeltaSeconds)
{
	// The story character's first: his body where it was placed (he never turns: TurnSpeed 0; nor bobs: BreathHeight 0).
	Super::UpdatePose(DeltaSeconds);
	if (!Body)
	{
		return;
	}
	if (!bSeatedScaleCaptured)
	{
		bSeatedScaleCaptured = true;
		SeatedScale = Body->GetRelativeScale3D();
	}
	// A slow breath in and out: his body swells a little about his seat (its pivot), most in his height, so his chest and
	// head rise and settle while he stays sat on the rail. One posed mesh, no rig: this is all of him that can move.
	BreathClock = FMath::Fmod(BreathClock + DeltaSeconds, BreathSecondsSeated);
	const float Breath = 0.5f * (1.f - FMath::Cos(2.f * UE_PI * BreathClock / BreathSecondsSeated));
	const float Swell = BreathSwell * Breath;
	BreathScale = FVector(1.f + Swell / 3.f, 1.f + Swell / 3.f, 1.f + Swell);
	Body->SetRelativeScale3D(SeatedScale * BreathScale);
}

bool AMisterSexton::IsWatched() const
{
	if (SpeakerPoint && SpeakerPoint->IsTalking())
	{
		return true;
	}
	return Body && Body->WasRecentlyRendered(SeenSeconds);
}
