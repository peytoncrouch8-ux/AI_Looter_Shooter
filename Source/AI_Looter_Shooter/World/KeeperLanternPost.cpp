// AKeeperLanternPost: a keeper's lantern post on the burial boards deck; the deck's lanterns put out and relit in Abel's
// fight, and the Keeper's Lantern hung, then lit by Abel and leaning toward the next saint's light.

#include "World/KeeperLanternPost.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "World/KeepersLantern.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AKeeperLanternPost::DeckTag(TEXT("LanternPost_Deck"));
const FName AKeeperLanternPost::KeepersPostTag(TEXT("LanternPost_Keeper"));

namespace
{
	const TCHAR* PostPath = TEXT("/Game/Art/Props/SM_KeeperLanternPost.SM_KeeperLanternPost");
	const TCHAR* LanternPath = TEXT("/Game/Art/Props/SM_KeepersLantern.SM_KeepersLantern");
	const TCHAR* DarkGlassPath = TEXT("/Game/Art/Materials/MI_HouseTrim.MI_HouseTrim");

	/**
	 * Without the models: where BurialDeck.py builds the post's lantern (its glass, under the arm's hook toward the post's
	 * front), the keeper's hook low on the post's face, and the Keeper's Lantern's grip and flame over its foot (cm).
	 */
	const FVector FallbackGlass(90.5, 0.0, 236.0);
	const FVector FallbackHook(18.0, 0.0, 178.0);
	const FVector FallbackGrip(0.0, 0.0, 42.7);
	const FVector FallbackFlame(0.0, 0.0, 12.2);

	/** The box the Interact line finds: round the lantern and the hook under it, this much past them (cm). */
	constexpr double BoxPad = 22.0;

	/** The lights: the post's (only with bCastsLight) and the lit Keeper's Lantern's, warm and small (candela, cm). */
	constexpr float PostCandela = 8.f;
	constexpr float KeepersCandela = 10.f;
	constexpr float LightRadius = 650.f;
	const FLinearColor LanternWarm(1.f, 0.72f, 0.42f);

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

	/** The glass slot of a lantern mesh: by its name, or the slot whose own material is named for it; INDEX_NONE without one. */
	int32 FindGlassSlot(const UStaticMeshComponent* Mesh, FName Slot)
	{
		const UStaticMesh* Model = Mesh ? Mesh->GetStaticMesh() : nullptr;
		if (!Model)
		{
			return INDEX_NONE;
		}
		const int32 Named = Mesh->GetMaterialIndex(Slot);
		if (Named != INDEX_NONE)
		{
			return Named;
		}
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			const UMaterialInterface* Own = Model->GetMaterial(Index);
			if (Own && Own->GetName().Contains(Slot.ToString()))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** A socket of Mesh in its component space, or Fallback without the model or the socket. */
	FVector SocketIn(const UStaticMeshComponent* Mesh, FName Socket, const FVector& Fallback)
	{
		if (Mesh && Mesh->GetStaticMesh() && Mesh->DoesSocketExist(Socket))
		{
			return Mesh->GetSocketTransform(Socket, RTS_Component).GetLocation();
		}
		return Fallback;
	}

	UPointLightComponent* MakeLight(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, float Candela)
	{
		UPointLightComponent* Light = Owner->CreateDefaultSubobject<UPointLightComponent>(Name);
		Light->SetupAttachment(Parent);
		Light->SetMobility(EComponentMobility::Movable);
		// A lantern's light: small, warm, never a shadow (the area's rule for every lantern).
		Light->SetCastShadows(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Candela);
		Light->SetAttenuationRadius(LightRadius);
		Light->SetLightColor(LanternWarm);
		Light->SetVisibility(false);
		return Light;
	}
}

AKeeperLanternPost::AKeeperLanternPost()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The post and its lantern stop the player and shots, as a post does.
	static UStaticMesh* const PostModel = FindIfMade<UStaticMesh>(PostPath);
	Post = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->SetMobility(EComponentMobility::Movable);
	Post->SetStaticMesh(PostModel);
	Post->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Post->SetCanEverAffectNavigation(false);

	// What the Interact key's line (Visibility) finds; nothing else meets it. World-dynamic: no ground trace lands on it.
	Grip = CreateDefaultSubobject<UBoxComponent>(TEXT("Grip"));
	Grip->SetupAttachment(Root);
	Grip->SetMobility(EComponentMobility::Movable);
	Grip->InitBoxExtent(FVector(BoxPad, BoxPad, 60.0));
	Grip->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Grip->SetCollisionObjectType(ECC_WorldDynamic);
	Grip->SetCollisionResponseToAllChannels(ECR_Ignore);
	Grip->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Grip->SetGenerateOverlapEvents(false);
	Grip->SetCanEverAffectNavigation(false);
	Grip->SetHiddenInGame(true);

	PostLight = MakeLight(this, Post, TEXT("PostLight"), PostCandela);

	// The Keeper's Lantern on its hook: small, no collision, no shadow; hidden until it's hung.
	static UStaticMesh* const LanternModel = FindIfMade<UStaticMesh>(LanternPath);
	KeepersLantern = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeepersLantern"));
	KeepersLantern->SetupAttachment(Root);
	KeepersLantern->SetMobility(EComponentMobility::Movable);
	KeepersLantern->SetStaticMesh(LanternModel);
	KeepersLantern->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	KeepersLantern->SetGenerateOverlapEvents(false);
	KeepersLantern->SetCanEverAffectNavigation(false);
	KeepersLantern->SetCastShadow(false);
	KeepersLantern->SetVisibility(false);

	KeepersLight = MakeLight(this, KeepersLantern, TEXT("KeepersLight"), KeepersCandela);

	static UMaterialInterface* const TrimGlass = FindIfMade<UMaterialInterface>(DarkGlassPath);
	DarkGlass = TrimGlass;
	Tags.Add(DeckTag);
	// Tagged for the minimap like the props on the ground.
	Tags.Add(TEXT("Obstacle"));
}

void AKeeperLanternPost::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bKeepersPost)
	{
		Tags.AddUnique(KeepersPostTag);
	}
	FitToModel();
	ApplyLantern();
}

void AKeeperLanternPost::BeginPlay()
{
	Super::BeginPlay();
	if (bKeepersPost)
	{
		Tags.AddUnique(KeepersPostTag);
	}
	FitToModel();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AKeeperLanternPost::HandleMissionsChanged);
	}
	RefreshStory();
}

void AKeeperLanternPost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Where its pieces are
// ---------------------------------------------------------------------------

void AKeeperLanternPost::FitToModel()
{
	// The box round the post's lantern and the keeper's hook under it.
	const FVector Glass = SocketIn(Post, InteractSocket, FallbackGlass);
	const FVector Hook = SocketIn(Post, HangSocket, FallbackHook);
	const FVector Low = Glass.ComponentMin(Hook);
	const FVector High = Glass.ComponentMax(Hook);
	Grip->SetRelativeLocation(Post->GetRelativeTransform().TransformPosition((Low + High) * 0.5));
	Grip->SetBoxExtent((High - Low) * 0.5 + FVector(BoxPad));
	// The post's light at its flame.
	PostLight->SetRelativeLocation(SocketIn(Post, LightSocket, FallbackGlass));

	// The Keeper's Lantern by its grip from the hook: upright, or once lit leaning on the hook toward the next saint's light.
	const FVector GripOffset = SocketIn(KeepersLantern, GripSocket, FallbackGrip);
	const FTransform HookWorld(GetActorQuat(), Post->GetComponentTransform().TransformPosition(Hook));
	FQuat Hanging = FRotator(0.0, GetActorRotation().Yaw, 0.0).Quaternion();
	if (bLanternLit && LeanDegrees > 0.f)
	{
		// Its body swings toward the bearing as if drawn there: down turned that way by LeanDegrees.
		const FVector Down = -FVector::UpVector;
		const FVector Drawn = (Down * FMath::Cos(FMath::DegreesToRadians(LeanDegrees)) + GetLeanDirection() * FMath::Sin(FMath::DegreesToRadians(LeanDegrees))).GetSafeNormal();
		Hanging = FQuat::FindBetweenNormals(Down, Drawn) * Hanging;
	}
	KeepersLantern->SetWorldLocationAndRotation(HookWorld.GetLocation() - Hanging.RotateVector(GripOffset), Hanging);
	KeepersLight->SetRelativeLocation(SocketIn(KeepersLantern, LightSocket, FallbackFlame));
}

FVector AKeeperLanternPost::GetLeanDirection() const
{
	// Compass bearing: 0 north (+X), 90 east (+Y).
	const float Radians = FMath::DegreesToRadians(FlameBearing);
	return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0);
}

FVector AKeeperLanternPost::GetKeepersLanternGlobe() const
{
	return KeepersLight ? KeepersLight->GetComponentLocation() : GetActorLocation();
}

// ---------------------------------------------------------------------------
// Lit, dark and hung
// ---------------------------------------------------------------------------

void AKeeperLanternPost::ApplyGlass(UStaticMeshComponent* Mesh, bool bLit, TObjectPtr<UMaterialInterface>& Lit)
{
	const int32 Slot = FindGlassSlot(Mesh, GlassSlot);
	if (Slot == INDEX_NONE)
	{
		return;
	}
	if (!Lit)
	{
		Lit = Mesh->GetStaticMesh()->GetMaterial(Slot);
	}
	UMaterialInterface* Wanted = bLit ? Lit.Get() : DarkGlass.Get();
	if (Wanted && Mesh->GetMaterial(Slot) != Wanted)
	{
		Mesh->SetMaterial(Slot, Wanted);
	}
}

void AKeeperLanternPost::ApplyLantern()
{
	ApplyGlass(Post, !bDark, PostGlow);
	PostLight->SetVisibility(bCastsLight && !bDark);
	KeepersLantern->SetVisibility(bHung);
	ApplyGlass(KeepersLantern, bLanternLit, LanternGlow);
	KeepersLight->SetVisibility(bHung && bLanternLit);
	FitToModel();
}

void AKeeperLanternPost::SetDark(bool bInDark)
{
	if (bDark == bInDark)
	{
		return;
	}
	bDark = bInDark;
	ApplyLantern();
}

bool AKeeperLanternPost::Relight(AActor* ByWhom)
{
	if (!bDark)
	{
		return false;
	}
	SetDark(false);
	UE_LOG(LogLooter, Log, TEXT("%s: the keeper's lantern relit%s."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" by %s"), *ByWhom->GetName()) : TEXT(""));
	OnRelit.Broadcast(*this);
	return true;
}

bool AKeeperLanternPost::StoryHolds(const FStoryCondition& Condition) const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

bool AKeeperLanternPost::CanHang() const
{
	if (!bKeepersPost || bHung)
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !AKeepersLantern::IsTaken(Runner->GetCampaign(), Runner))
	{
		return false;
	}
	return HangWhen.IsEmpty() || HangWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AKeeperLanternPost::Hang(AActor* ByWhom, bool bForce)
{
	if (bHung || (!bForce && !CanHang()))
	{
		return false;
	}
	bHung = true;
	ApplyLantern();
	UE_LOG(LogLooter, Log, TEXT("%s: the Keeper's Lantern hung on the keeper's post, dark%s."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" (by %s)"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

void AKeeperLanternPost::LightKeepersLantern()
{
	bHung = true;
	const bool bWasLit = bLanternLit;
	bLanternLit = true;
	ApplyLantern();
	if (!bWasLit)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: the Keeper's Lantern lit; it leans toward bearing %.0f."), *GetActorNameOrLabel(), FlameBearing);
		OnKeepersLanternLit.Broadcast(*this);
	}
}

void AKeeperLanternPost::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && bKeepersPost)
	{
		const bool bStoryHung = HungWhen.ContainsByPredicate([this](const FStoryCondition& When) { return !When.IsEmpty() && StoryHolds(When); });
		const bool bStoryLit = !LitWhen.IsEmpty() && StoryHolds(LitWhen);
		// Hung by the story, or hung here this visit; but the story gone back to its hanging step (the console starting Main
		// 6 over) takes it down again.
		bHung = bStoryHung || bStoryLit || (bHung && !(!HangWhen.IsEmpty() && StoryHolds(HangWhen)));
		bLanternLit = bStoryLit || (bLanternLit && bHung);
	}
	ApplyLantern();
}

void AKeeperLanternPost::HandleMissionsChanged()
{
	RefreshStory();
}

// ---------------------------------------------------------------------------
// The Interact key
// ---------------------------------------------------------------------------

FInteractionOptions AKeeperLanternPost::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.Reach = Reach;
	if (bDark)
	{
		// Relit under pressure: a hold, so a stray tap in the fight does nothing.
		Options.bTap = false;
		Options.bHold = true;
		Options.HoldSeconds = RelightSeconds;
		Options.HoldPrompt = NSLOCTEXT("LooterAbel", "Relight", "Relight the lantern");
		return Options;
	}
	if (CanHang())
	{
		Options.bTap = true;
		Options.bHold = false;
		Options.TapPrompt = NSLOCTEXT("LooterAbel", "HangLantern", "Hang the Keeper's Lantern");
		return Options;
	}
	return FInteractionOptions::None();
}

bool AKeeperLanternPost::Interact(UInteractionComponent& User, bool bHeld)
{
	if (bDark)
	{
		return bHeld && Relight(User.GetOwner());
	}
	return !bHeld && Hang(User.GetOwner());
}

TOptional<FVector> AKeeperLanternPost::GetInteractionLocation() const
{
	return Grip ? TOptional<FVector>(Grip->GetComponentLocation()) : TOptional<FVector>();
}
