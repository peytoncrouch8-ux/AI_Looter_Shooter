// UCarriedLanternSubsystem: Abel's light carried home at Sully's hip (Main 7), the hose-men's lines on it, its HUD meter,
// and the freed souls' trail behind it.

#include "World/CarriedLantern.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "UI/HUD/HudCarriedLanternWidget.h"
#include "World/CarriedLanternRules.h"
#include "World/LamplighterRules.h"
#include "World/SoulFollowers.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The story's carried word is read this often (s): it walks the campaign record. */
	constexpr float CarriedCheckEvery = 0.25f;
}

UCarriedLanternSubsystem* UCarriedLanternSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UCarriedLanternSubsystem>() : nullptr;
}

bool UCarriedLanternSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UCarriedLanternSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Main 7 from its carry step (FreeAbel's end: the lantern taken down lit) until Saint Ada wakes (the lantern on her lid).
	CarryWhen.DuringMission = GivenToTheWind::Mission();
	CarryWhen.FromStep = GivenToTheWind::CarryStep;
	CarryWhen.WithoutFlags = { GivenToTheWind::SaintAdaAwakeFlag() };
}

void UCarriedLanternSubsystem::Deinitialize()
{
	if (Meter)
	{
		Meter->RemoveFromParent();
	}
	Meter = nullptr;
	Followers = nullptr;
	Latched.Reset();
	Super::Deinitialize();
}

TStatId UCarriedLanternSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCarriedLanternSubsystem, STATGROUP_Tickables);
}

bool UCarriedLanternSubsystem::IsTickable() const
{
	// Nothing to do outside Main 7's walk home but a meter fading out; the trail ticks itself.
	return !IsTemplate() && (IsCarried() || Latched.Num() > 0 || (Meter && Meter->IsShown()));
}

void UCarriedLanternSubsystem::Tick(float DeltaTime)
{
	Step(DeltaTime);
}

bool UCarriedLanternSubsystem::StoryHolds(const FStoryCondition& Condition) const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

bool UCarriedLanternSubsystem::IsCarried() const
{
	if (bSetDown)
	{
		return false;
	}
	if (ForcedMode >= 0)
	{
		return ForcedMode > 0;
	}
	// The story's word, read now and then: the hose-men ask every frame they hold a line.
	const UWorld* World = GetWorld();
	const float Now = World ? static_cast<float>(World->GetTimeSeconds()) : 0.f;
	if (CarriedCheckLeft <= 0.f || Now >= CarriedCheckLeft)
	{
		CarriedCheckLeft = Now + CarriedCheckEvery;
		bCarriedCached = StoryHolds(CarryWhen);
	}
	return bCarriedCached;
}

void UCarriedLanternSubsystem::ForceCarried(int32 Mode)
{
	ForcedMode = FMath::Clamp(Mode, -1, 1);
	CarriedCheckLeft = 0.f;
	if (ForcedMode > 0)
	{
		bSetDown = false;
	}
	UE_LOG(LogLooter, Log, TEXT("Abel's lantern: %s."), ForcedMode < 0 ? TEXT("carried as the story says")
		: ForcedMode > 0 ? TEXT("carried (forced)") : TEXT("not carried (forced)"));
}

void UCarriedLanternSubsystem::SetDown()
{
	bSetDown = true;
	// Every line on it lets go: there's nothing at Sully's hip to hold.
	Latched.Reset();
	UE_LOG(LogLooter, Log, TEXT("Abel's lantern is set on the Reliquary (his light %.0f%%)."), Light * 100.f);
}

void UCarriedLanternSubsystem::SetLight(float InLight)
{
	Light = FMath::Clamp(InLight, CarriedLanternRules::MinLight, 1.f);
}

void UCarriedLanternSubsystem::Drain(float Share, AActor* By)
{
	if (!IsCarried())
	{
		return;
	}
	Light = FMath::Max(CarriedLanternRules::MinLight, Light - FMath::Max(Share, 0.f));
	SinceFree = 0.f;
}

void UCarriedLanternSubsystem::SetLatched(AActor* By, bool bLatched)
{
	if (!By)
	{
		return;
	}
	Latched.RemoveAll([By](const TWeakObjectPtr<AActor>& Each) { return !Each.IsValid() || Each.Get() == By; });
	if (!bLatched)
	{
		return;
	}
	Latched.Add(By);
	SinceFree = 0.f;
	// Finch on the tube, the first time ever: why it matters (he's draining Abel, not Sully) and what to do.
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && !Runner->HasFlag(GivenToTheWind::LatchWordFlag()))
	{
		Runner->SetFlag(GivenToTheWind::LatchWordFlag());
		LandfallRules::Say(this, GivenToTheWind::LatchLines(), /*bInterrupt*/ false);
	}
	UE_LOG(LogLooter, Log, TEXT("Abel's lantern: %s's line on it (%d now)."), *By->GetName(), Latched.Num());
}

int32 UCarriedLanternSubsystem::NumLatched() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Each : Latched)
	{
		Count += Each.IsValid() ? 1 : 0;
	}
	return Count;
}

FVector UCarriedLanternSubsystem::LanternPoint(const APawn* Carrier)
{
	if (!Carrier)
	{
		return FVector::ZeroVector;
	}
	const FVector Ahead = Carrier->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Ahead);
	return Carrier->GetActorLocation() + Right * CarriedLanternRules::HipRight + Ahead * CarriedLanternRules::HipAhead
		- FVector(0.0, 0.0, CarriedLanternRules::HipDown);
}

void UCarriedLanternSubsystem::Step(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	Latched.RemoveAll([](const TWeakObjectPtr<AActor>& Each) { return !Each.IsValid(); });
	const bool bCarried = IsCarried();
	if (!bCarried)
	{
		Latched.Reset();
	}
	const int32 Lines = Latched.Num();
	if (Lines > 0)
	{
		Light = CarriedLanternRules::Drained(Light, Lines, DeltaSeconds);
		SinceFree = 0.f;
	}
	else
	{
		SinceFree += DeltaSeconds;
		const float Before = Light;
		Light = CarriedLanternRules::Refilled(Light, SinceFree, DeltaSeconds);
		// Full again: a soft rise of the soul's chime, so the player hears they've won it back.
		if (bCarried && Before < 1.f && Light >= 1.f && Before < 0.999f && SinceFree > CarriedLanternRules::RefillDelay + 0.2f)
		{
			LooterSound::Play2D(this, LooterSoundCue::SoulMote::Pickup, 0.6f, 0.8f);
		}
	}
	UpdateWidget(DeltaSeconds);
}

void UCarriedLanternSubsystem::UpdateWidget(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
	const bool bWanted = IsCarried() && !USceneSubsystem::HidesGameplayHUD(this);
	if (!Meter && bWanted && Viewer && Viewer->IsLocalController())
	{
		Meter = CreateWidget<UHudCarriedLanternWidget>(Viewer, UHudCarriedLanternWidget::StaticClass());
		if (Meter)
		{
			Meter->AddToViewport(UHudCarriedLanternWidget::ViewportZOrder);
		}
	}
	if (Meter)
	{
		Meter->SetShown(bWanted);
		Meter->SetLight(Light, NumLatched() > 0, Clock);
	}
}

ASoulFollowers* UCarriedLanternSubsystem::EnsureFollowers()
{
	if (Followers && IsValid(Followers))
	{
		return Followers;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Followers = World->SpawnActor<ASoulFollowers>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	return Followers;
}

int32 UCarriedLanternSubsystem::AddFollower(const FVector& From, bool bAtOnce)
{
	ASoulFollowers* Trail = EnsureFollowers();
	return Trail ? Trail->AddSoul(From, bAtOnce) : 0;
}

int32 UCarriedLanternSubsystem::NumFollowers() const
{
	return Followers ? Followers->NumFollowing() : 0;
}

void UCarriedLanternSubsystem::ReleaseFollowers(const TArray<FVector>& Extra)
{
	if (ASoulFollowers* Trail = EnsureFollowers())
	{
		Trail->Release(Extra);
	}
}
