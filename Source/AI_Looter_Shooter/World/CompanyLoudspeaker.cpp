// ACompanyLoudspeaker: a Vane Consolidated loudspeaker post: Vane's voice from it (its cues, or a broadcast asked for), and
// its horns shot out.

#include "World/CompanyLoudspeaker.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "Story/VaneBroadcast.h"
#include "World/LamplighterEffects.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

const FName ACompanyLoudspeaker::LoudspeakerTag(TEXT("CompanyLoudspeaker"));
const TCHAR* const ACompanyLoudspeaker::ModelPath = TEXT("/Game/Art/Tripo/SM_LoudspeakerPost.SM_LoudspeakerPost");

namespace
{
	// The stand-in's parts, one path each (WORLD's instanced posts are built from the same: build_area_landfall.py).
	const TCHAR* LoudspeakerPostPath = TEXT("/Game/Art/Props/SM_FencePost.SM_FencePost");
	const TCHAR* LoudspeakerConePath = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* LoudspeakerCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* LoudspeakerBrassPath = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");
	const TCHAR* LoudspeakerPaintPath = TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack");

	/** The horns: 40 cm under the pole's top, 45 cm out, turned 55 degrees each side of its front; their size (cones' scale). */
	constexpr float HornsBelowTop = 40.f;
	constexpr float HornsOut = 45.f;
	constexpr float HornsTurn = 55.f;
	const FVector HornScale(0.55, 0.55, 0.8);
	/** The plate: 1.5 m under the top, a hand in front of the pole. */
	constexpr float PlateBelowTop = 150.f;
	const FVector PlateScale(0.04, 0.34, 0.24);
	/** The horns' hit: a capsule across both (cm). */
	constexpr float HornsHitRadius = 60.f;
	constexpr float HornsHitHalfHeight = 85.f;

	/** Bullets' trace channel (DefaultEngine.ini's Weapon). */
	constexpr ECollisionChannel LoudspeakerWeaponChannel = ECC_GameTraceChannel2;

	/** Cues are looked at this often (s). */
	constexpr float CueLookSeconds = 0.5f;
	/** Vane's last words, cut, stay this long (s). */
	constexpr float CutLineSeconds = 1.6f;

	/** An asset found only once it's in this checkout (the Tripo model may not be yet). In a constructor. */
	UStaticMesh* LoudspeakerModelIfMade()
	{
		const FString Path(ACompanyLoudspeaker::ModelPath);
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(ACompanyLoudspeaker::ModelPath);
		return Finder.Object;
	}

	/** Part of a word: a letter, a digit or an apostrophe. */
	bool IsWordChar(TCHAR Char)
	{
		return FChar::IsAlnum(Char) || Char == TEXT('\'');
	}
}

ACompanyLoudspeaker::ACompanyLoudspeaker()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(LoudspeakerTag);
	Tags.Add(FName(TEXT("Obstacle")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PostMesh(LoudspeakerPostPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(LoudspeakerConePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(LoudspeakerCubePath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(LoudspeakerBrassPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Paint(LoudspeakerPaintPath);
	static UStaticMesh* const Model = LoudspeakerModelIfMade();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The pole stops the player and the Interact key's line, but bullets pass it: only its horns are the target.
	Post = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->SetStaticMesh(Model ? Model : PostMesh.Object.Get());
	Post->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Post->SetCollisionResponseToChannel(LoudspeakerWeaponChannel, ECR_Ignore);
	Post->SetCanEverAffectNavigation(false);
	if (!Model && PostMesh.Object)
	{
		// The fence post stretched to the pole's height and thickened a little, its foot on the ground.
		const FBoxSphereBounds Bounds = PostMesh.Object->GetBounds();
		const double Tall = FMath::Max(Bounds.BoxExtent.Z * 2.0, 1.0);
		const double Stretch = PostHeight / Tall;
		Post->SetRelativeScale3D(FVector(1.4, 1.4, Stretch));
		Post->SetRelativeLocation(FVector(0.0, 0.0, -(Bounds.Origin.Z - Bounds.BoxExtent.Z) * Stretch));
	}
	// The Tripo post (4 m, its horns and plate on it facing +X, its pivot at the pole): its foot on the ground by its bounds
	// whatever its pivot's height, the pole on the spot; the horns' target at its Horns socket (else where the stand-in's are).
	FVector HornsAt(HornsOut * 0.6, 0.0, PostHeight - HornsBelowTop);
	if (Model)
	{
		const double Lift = -Model->GetBoundingBox().Min.Z;
		Post->SetRelativeLocation(FVector(0.0, 0.0, Lift));
		if (const UStaticMeshSocket* Socket = Model->FindSocket(TEXT("Horns")))
		{
			HornsAt = Socket->RelativeLocation + FVector(0.0, 0.0, Lift);
		}
	}

	auto MakeHorn = [&](const TCHAR* Name, float Side)
	{
		UStaticMeshComponent* Horn = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Horn->SetupAttachment(Root);
		Horn->SetStaticMesh(Cone.Object);
		Horn->SetMaterial(0, Brass.Object);
		// The cone's tip is its +Z: pitched 90 its tip points back at the pole and its bell out, then turned to its side.
		const float Yaw = HornsTurn * Side;
		const FVector Out = FRotator(0.0, Yaw, 0.0).Vector();
		Horn->SetRelativeLocationAndRotation(Out * HornsOut + FVector(0.0, 0.0, PostHeight - HornsBelowTop), FRotator(90.0, Yaw, 0.0));
		Horn->SetRelativeScale3D(HornScale);
		Horn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Horn->SetCanEverAffectNavigation(false);
		Horn->SetMobility(EComponentMobility::Movable);
		return Horn;
	};
	HornLeft = MakeHorn(TEXT("HornLeft"), -1.f);
	HornRight = MakeHorn(TEXT("HornRight"), 1.f);

	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(Root);
	Plate->SetStaticMesh(Cube.Object);
	Plate->SetMaterial(0, Paint.Object);
	Plate->SetRelativeLocation(FVector(12.0, 0.0, PostHeight - PlateBelowTop));
	Plate->SetRelativeScale3D(PlateScale);
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plate->SetCanEverAffectNavigation(false);

	// Shots only, world dynamic so ground traces pass it; lying across both horns (its axis along the pole's Y).
	HornsHit = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HornsHit"));
	HornsHit->SetupAttachment(Root);
	HornsHit->InitCapsuleSize(HornsHitRadius, HornsHitHalfHeight);
	HornsHit->SetRelativeLocationAndRotation(HornsAt, FRotator(0.0, 0.0, 90.0));
	HornsHit->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HornsHit->SetCollisionObjectType(ECC_WorldDynamic);
	HornsHit->SetCollisionResponseToAllChannels(ECR_Ignore);
	HornsHit->SetCollisionResponseToChannel(LoudspeakerWeaponChannel, ECR_Block);
	HornsHit->SetGenerateOverlapEvents(false);
	HornsHit->SetCanEverAffectNavigation(false);
	HornsHit->SetHiddenInGame(true);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = HornsHealth;
}

void ACompanyLoudspeaker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLook();
}

void ACompanyLoudspeaker::ApplyLook()
{
	const bool bModel = Post && Post->GetStaticMesh() && Post->GetStaticMesh()->GetPathName() == FString(ModelPath);
	for (UStaticMeshComponent* Part : { Post.Get(), HornLeft.Get(), HornRight.Get(), Plate.Get() })
	{
		if (!Part)
		{
			continue;
		}
		// The Tripo model is post, horns and plate in one: the stand-in's own horns and plate go.
		const bool bShown = bShowPost && (Part == Post || !bModel) && !(bSilenced && (Part == HornLeft || Part == HornRight));
		Part->SetVisibility(bShown);
		Part->SetHiddenInGame(!bShown);
	}
	if (Post)
	{
		Post->SetCollisionEnabled(bShowPost ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	if (HornsHit)
	{
		HornsHit->SetCollisionEnabled(bShowPost && !bSilenced ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	// A voice only takes no hits at all.
	SetCanBeDamaged(bShowPost && !bSilenced);
}

void ACompanyLoudspeaker::BeginPlay()
{
	// Its horns' health before the health component starts full on it.
	if (Health)
	{
		Health->MaxHealth = HornsHealth;
	}
	Super::BeginPlay();
	bSilenced = false;
	bOnAir = false;
	ApplyLook();
	if (Health)
	{
		Health->OnDeath.AddUniqueDynamic(this, &ACompanyLoudspeaker::HandleDeath);
	}
	CueSaid.Init(false, Cues.Num());
	CueDueSince.Init(-1.0, Cues.Num());
	if (Cues.Num() > 0)
	{
		GetWorldTimerManager().SetTimer(CueTimer, this, &ACompanyLoudspeaker::CheckCues, CueLookSeconds, /*bLoop*/ true);
	}
}

void ACompanyLoudspeaker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CueTimer);
	}
	Super::EndPlay(EndPlayReason);
}

FVector ACompanyLoudspeaker::GetHornsLocation() const
{
	// The horns' target is laid where they are (the Tripo post's socket, or the stand-in's horns).
	return HornsHit ? HornsHit->GetComponentLocation() : GetActorTransform().TransformPosition(FVector(HornsOut * 0.6, 0.0, PostHeight - HornsBelowTop));
}

bool ACompanyLoudspeaker::IsOnAir() const
{
	return bOnAir && UVaneBroadcast::IsSpeaking(this);
}

bool ACompanyLoudspeaker::Broadcast(const TArray<FStoryLine>& Lines)
{
	if (bSilenced || Lines.IsEmpty())
	{
		return false;
	}
	// The radio's crackle, hiss and click are UVaneBroadcast's (heard everywhere, as the valley's every horn carries him).
	UVaneBroadcast::Say(this, Lines);
	bOnAir = true;
	UE_LOG(LogLooter, Log, TEXT("%s: Vane on the loudspeaker (%d lines)."), *GetActorNameOrLabel(), Lines.Num());
	return true;
}

void ACompanyLoudspeaker::CheckCues()
{
	if (bOnAir && !UVaneBroadcast::IsSpeaking(this))
	{
		bOnAir = false;
	}
	UWorld* World = GetWorld();
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (bSilenced || !World || !Runner)
	{
		return;
	}
	const APlayerController* Controller = World->GetFirstPlayerController();
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (!Player || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) > FMath::Square(static_cast<double>(EarshotRadius)))
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	const bool bQuiet = !UVaneBroadcast::IsSpeaking(this) && (!Captions || Captions->GetQueue().IsEmpty());
	bool bAnyLeft = false;
	for (int32 Index = 0; Index < Cues.Num(); ++Index)
	{
		if (!CueSaid.IsValidIndex(Index) || CueSaid[Index] || !Cues[Index].Lines)
		{
			continue;
		}
		bAnyLeft = true;
		if (!Cues[Index].When.IsMet(Runner->GetCampaign(), Runner))
		{
			CueDueSince[Index] = -1.0;
			continue;
		}
		if (CueDueSince[Index] < 0.0)
		{
			CueDueSince[Index] = Now;
		}
		// Into a quiet moment, so nobody's words are dropped; he never cuts himself off; after a while he breaks in anyway.
		if (UVaneBroadcast::IsSpeaking(this) || (!bQuiet && Now - CueDueSince[Index] < QuietWaitSeconds))
		{
			return;
		}
		CueSaid[Index] = true;
		Broadcast(Cues[Index].Lines->Lines);
		return;
	}
	if (!bAnyLeft)
	{
		World->GetTimerManager().ClearTimer(CueTimer);
	}
}

void ACompanyLoudspeaker::HandleDeath(AController* Killer)
{
	Silence(Killer);
}

bool ACompanyLoudspeaker::Silence(AController* By)
{
	if (bSilenced || !bShowPost)
	{
		return false;
	}
	bSilenced = true;
	const FVector Horns = GetHornsLocation();
	// Cut off mid-word: the line on screen, if it's his, ends inside a word with a dash, and the radio clicks out.
	if (bOnAir && UVaneBroadcast::IsSpeaking(this))
	{
		FString Words;
		UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
		const FCaptionEntry* Current = Captions ? Captions->GetCurrent() : nullptr;
		if (Current && Current->Line.Speaker.EqualTo(UVaneBroadcast::SpeakerName()))
		{
			Words = Current->Line.Text.ToString();
		}
		UVaneBroadcast::Stop(this);
		if (Captions && !Words.IsEmpty())
		{
			Captions->Play({ FStoryLine::Make(UVaneBroadcast::SpeakerName(), FText::FromString(CutOff(Words)), CutLineSeconds) },
				ECaptionPlay::Interrupt);
		}
	}
	bOnAir = false;
	// The horns burst: white sparks, a puff of their smoke, the crackle and the clang.
	LamplighterEffects::Sparks(this, Horns, GetActorForwardVector(), 14, 1.2f);
	LamplighterEffects::SmokePuff(this, Horns, FVector(0.0, 0.0, 70.0));
	// (The radio's own click, when he was on the air, is UVaneBroadcast::Stop's.)
	LooterSound::PlayAt(this, LooterSoundCue::DredgerHoseSnap, Horns, 1.f, 1.2f);
	LooterSound::PlayAt(this, LooterSoundCue::ImpactMetal, Horns);
	ApplyLook();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CueTimer);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: its horns shot out%s."), *GetActorNameOrLabel(), By ? TEXT(" by the player") : TEXT(""));
	return true;
}

ACompanyLoudspeaker* ACompanyLoudspeaker::FindTagged(const UObject* Context, FName Tag)
{
	UWorld* World = Context ? Context->GetWorld() : nullptr;
	if (!World || Tag.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<ACompanyLoudspeaker> It(World); It; ++It)
	{
		if (It->ActorHasTag(Tag))
		{
			return *It;
		}
	}
	return nullptr;
}

FString ACompanyLoudspeaker::CutOff(const FString& Words)
{
	const FString Said = Words.TrimStartAndEnd();
	const int32 Length = Said.Len();
	const FString Dash = FString::Chr(static_cast<TCHAR>(0x2014));
	if (Length == 0)
	{
		return Dash;
	}
	// The word four fifths of the way in (or the next one, from a space), or the last word.
	const int32 At = FMath::Clamp(FMath::FloorToInt32(Length * 0.8f), 0, Length - 1);
	int32 Start = INDEX_NONE;
	if (IsWordChar(Said[At]))
	{
		Start = At;
		while (Start > 0 && IsWordChar(Said[Start - 1]))
		{
			--Start;
		}
	}
	else
	{
		for (int32 Index = At + 1; Index < Length && Start == INDEX_NONE; ++Index)
		{
			Start = IsWordChar(Said[Index]) ? Index : INDEX_NONE;
		}
	}
	if (Start == INDEX_NONE)
	{
		// Nothing but marks after it: the last word.
		int32 End = Length;
		while (End > 0 && !IsWordChar(Said[End - 1]))
		{
			--End;
		}
		Start = End;
		while (Start > 0 && IsWordChar(Said[Start - 1]))
		{
			--Start;
		}
	}
	int32 End = Start;
	while (End < Length && IsWordChar(Said[End]))
	{
		++End;
	}
	const int32 WordLength = End - Start;
	// A third of the word, at least a letter, never all of a word of two or more.
	const int32 Keep = WordLength <= 1 ? WordLength : FMath::Clamp(FMath::DivideAndRoundUp(WordLength, 3), 1, WordLength - 1);
	return Said.Left(Start + Keep) + Dash;
}
