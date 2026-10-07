#include "Story/MisterSexton.h"
#include "AI_Looter_Shooter.h"
#include "Story/SpeakerPointComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AMisterSexton::ModelPath = TEXT("/Game/Art/Characters/SM_MisterSexton.SM_MisterSexton");
const TCHAR* const AMisterSexton::LedgerPath = TEXT("/Game/Art/Characters/SM_SextonLedger.SM_SextonLedger");
const FName AMisterSexton::LedgerSocket(TEXT("Ledger"));
const FName AMisterSexton::SpeakerSocket(TEXT("Speaker"));

namespace
{
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
	// Nothing of him moves: a look a few times a second is enough to see whether he may go.
	PrimaryActorTick.TickInterval = 0.25f;

	// A posed idle only (Docs/Areas/RansomsRest.md, NPCs): he doesn't turn to whoever talks to him, nor breathe.
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
}

bool AMisterSexton::IsWatched() const
{
	if (SpeakerPoint && SpeakerPoint->IsTalking())
	{
		return true;
	}
	return Body && Body->WasRecentlyRendered(SeenSeconds);
}
