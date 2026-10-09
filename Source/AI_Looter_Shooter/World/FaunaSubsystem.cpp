#include "World/FaunaSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/FaunaActor.h"
#include "World/FaunaRules.h"
#include "World/LightingStateSubsystem.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace
{
	TAutoConsoleVariable<int32> CVarFauna(TEXT("Looter.Fauna"), 1,
		TEXT("Ambient life: birds, insects, tumbleweeds, dust devils and washing in the wind (0: all hidden and still, to measure it by difference)."));

	/** How often (s) the creatures near the view are looked for again. */
	constexpr double CreatureScanSeconds = 0.5;

	/** Degrees past the screen's edges that still count as on screen: a turning view finds them already moving. */
	constexpr float ViewMargin = 10.f;

	/** A hidden actor still keeps time this often (s), so it comes back where its clock says. */
	constexpr float HiddenInterval = 1.f;

	/** The longest step one update takes (s): after a long wait an actor catches up in steps it can take. */
	constexpr float LongestStep = 1.f;

	/** How quickly the cost's average follows each frame: a share of the new frame. */
	constexpr float CostSmoothing = 0.05f;

	/** Noises of a kind this near each other (cm) within NoiseMergeSeconds are one (a shotgun's pellets, a burst). */
	constexpr float NoiseMergeDistance = 300.f;
	constexpr double NoiseMergeSeconds = 0.1;
}

UFaunaSubsystem* UFaunaSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UFaunaSubsystem>() : nullptr;
}

bool UFaunaSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

bool UFaunaSubsystem::IsEnabled()
{
	return CVarFauna.GetValueOnGameThread() != 0;
}

void UFaunaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Its strikes are noises; it must exist first to be listened to.
	Collection.InitializeDependency<UBulletSubsystem>();
}

void UFaunaSubsystem::Deinitialize()
{
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		Bullets->OnBulletHit.Remove(BulletHitHandle);
	}
	BulletHitHandle.Reset();
	if (AWeaponBase* Weapon = BoundWeapon.Get())
	{
		Weapon->OnFired.RemoveDynamic(this, &UFaunaSubsystem::HandleWeaponFired);
	}
	BoundWeapon.Reset();
	Actors.Reset();
	Super::Deinitialize();
}

void UFaunaSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UBulletSubsystem* Bullets = InWorld.GetSubsystem<UBulletSubsystem>())
	{
		BulletHitHandle = Bullets->OnBulletHit.AddUObject(this, &UFaunaSubsystem::HandleBulletHit);
	}
}

TStatId UFaunaSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFaunaSubsystem, STATGROUP_Tickables);
}

void UFaunaSubsystem::Register(AFaunaActor* Actor)
{
	if (Actor)
	{
		Actors.AddUnique(Actor);
		Actor->NextUpdateTime = 0.0;
		Actor->LastUpdateTime = -1.0;
		if (!IsEnabled())
		{
			Actor->SetFaunaShown(false);
		}
	}
}

void UFaunaSubsystem::Unregister(AFaunaActor* Actor)
{
	Actors.Remove(Actor);
}

void UFaunaSubsystem::ReportNoise(const FVector& Location, float Radius, EFaunaNoise Kind)
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	for (FFaunaNoise& Known : Noises)
	{
		if (Known.Kind == Kind && Now - Known.Time < NoiseMergeSeconds
			&& FVector::DistSquared(Known.Location, Location) < FMath::Square(NoiseMergeDistance))
		{
			Known.Time = Now;
			Known.Radius = FMath::Max(Known.Radius, Radius);
			return;
		}
	}
	FFaunaNoise& Noise = Noises.AddDefaulted_GetRef();
	Noise.Location = Location;
	Noise.Radius = Radius;
	Noise.Time = Now;
	Noise.Kind = Kind;
}

void UFaunaSubsystem::HandleWeaponFired()
{
	if (const AWeaponBase* Weapon = BoundWeapon.Get())
	{
		ReportNoise(Weapon->GetActorLocation(), GunshotRadius, EFaunaNoise::Gunshot);
	}
}

void UFaunaSubsystem::HandleBulletHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	ReportNoise(Hit.ImpactPoint, ImpactRadius, EFaunaNoise::Impact);
}

void UFaunaSubsystem::BindPlayerWeapon()
{
	// The local player's gun in hand: its shots are what startle the birds (the bullets' strikes come from UBulletSubsystem).
	AWeaponBase* Active = nullptr;
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (const APawn* Pawn = Player ? Player->GetPawn() : nullptr)
	{
		if (const UWeaponManagerComponent* Weapons = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Active = Weapons->GetActiveWeapon();
		}
	}
	if (Active == BoundWeapon.Get())
	{
		return;
	}
	if (AWeaponBase* Old = BoundWeapon.Get())
	{
		Old->OnFired.RemoveDynamic(this, &UFaunaSubsystem::HandleWeaponFired);
	}
	BoundWeapon = Active;
	if (Active)
	{
		Active->OnFired.AddUniqueDynamic(this, &UFaunaSubsystem::HandleWeaponFired);
	}
}

FFaunaContext UFaunaSubsystem::GatherContext() const
{
	FFaunaContext Context;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Context;
	}
	Context.Now = World->GetTimeSeconds();
	Context.Noises = Noises;
	if (const ULightingStateSubsystem* Lighting = World->GetSubsystem<ULightingStateSubsystem>())
	{
		Context.Lighting = Lighting->GetState();
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		if (!Player)
		{
			continue;
		}
		if (const APawn* Pawn = Player->GetPawn())
		{
			FFaunaThreat& Threat = Context.Threats.AddDefaulted_GetRef();
			Threat.Location = Pawn->GetActorLocation();
			Threat.Velocity = Pawn->GetVelocity();
			const ACharacter* Character = Cast<ACharacter>(Pawn);
			Threat.Fear = FaunaRules::FearOf(true, Character && Character->bIsCrouched, static_cast<float>(Threat.Velocity.Size2D()));
			Threat.bPlayer = true;
		}
		// The local player's camera (the menu has one and no pawn; the third-person camera trails the pawn).
		if (!Context.View.bValid && Player->IsLocalController() && Player->PlayerCameraManager)
		{
			Context.View.Location = Player->PlayerCameraManager->GetCameraLocation();
			Context.View.Direction = Player->PlayerCameraManager->GetCameraRotation().Vector();
			Context.View.FieldOfView = Player->PlayerCameraManager->GetFOVAngle();
			Context.View.bValid = true;
		}
	}
	for (const TWeakObjectPtr<APawn>& Each : NearbyCreatures)
	{
		if (const APawn* Creature = Each.Get())
		{
			FFaunaThreat& Threat = Context.Threats.AddDefaulted_GetRef();
			Threat.Location = Creature->GetActorLocation();
			Threat.Velocity = Creature->GetVelocity();
			Threat.Fear = FaunaRules::FearOf(false, false, 0.f);
			Threat.bPlayer = false;
		}
	}
	return Context;
}

void UFaunaSubsystem::ApplyToggle(bool bEnabled)
{
	UE_LOG(LogLooter, Display, TEXT("Ambient life %s (Looter.Fauna)."), bEnabled ? TEXT("on") : TEXT("off"));
	for (const TWeakObjectPtr<AFaunaActor>& Weak : Actors)
	{
		if (AFaunaActor* Actor = Weak.Get())
		{
			// Off: hidden and still. On: each shows again as its next update finds it in range.
			if (!bEnabled)
			{
				Actor->SetFaunaShown(false);
			}
			Actor->NextUpdateTime = 0.0;
		}
	}
}

void UFaunaSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const bool bEnabled = IsEnabled();
	if (bEnabled != bWasEnabled)
	{
		bWasEnabled = bEnabled;
		ApplyToggle(bEnabled);
	}
	UWorld* World = GetWorld();
	if (!bEnabled || !World || Actors.Num() == 0)
	{
		UpdatedLastFrame = 0;
		return;
	}
	const uint64 StartCycles = FPlatformTime::Cycles64();
	const double Now = World->GetTimeSeconds();
	Noises.RemoveAll([Now](const FFaunaNoise& Noise) { return Now - Noise.Time > NoiseMemory; });
	BindPlayerWeapon();
	const FFaunaContext Context = GatherContext();

	if (Now - CreatureScanTime > CreatureScanSeconds && Context.View.bValid)
	{
		// The class's own list, not every actor: a level holds a few dozen creatures at most.
		CreatureScanTime = Now;
		NearbyCreatures.Reset();
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			if (!It->IsDead() && FVector::DistSquared(It->GetActorLocation(), Context.View.Location) < FMath::Square(CreatureThreatRange))
			{
				NearbyCreatures.Add(*It);
			}
		}
	}

	Actors.RemoveAll([](const TWeakObjectPtr<AFaunaActor>& Weak) { return !Weak.IsValid(); });
	int32 Updated = 0;
	// A copy: an update may spawn or remove nothing today, but a callback that does must not upset the loop.
	const TArray<TWeakObjectPtr<AFaunaActor>> Snapshot = Actors;
	for (const TWeakObjectPtr<AFaunaActor>& Weak : Snapshot)
	{
		AFaunaActor* Actor = Weak.Get();
		if (!Actor)
		{
			continue;
		}
		const float Distance = Context.View.bValid ? Actor->DistanceFrom(Context.View.Location) : 0.f;
		const bool bShow = Actor->IsInItsLight(Context.Lighting) && FaunaRules::ShouldShow(Distance, Actor->CullDistance, Actor->IsFaunaShown());
		const bool bOnScreen = bShow && (!Context.View.bValid || FCreatureUpdateRate::IsInView(Context.View.Location,
			Context.View.Direction, Context.View.FieldOfView, Actor->GetFaunaCenter(), Actor->GetFaunaRadius(), ViewMargin));
		if (bShow != Actor->IsFaunaShown())
		{
			Actor->SetFaunaShown(bShow);
			Actor->NextUpdateTime = Now;
		}
		// Coming into view, it's moved on now rather than when its off-screen wait ends: nothing is seen standing still.
		const bool bCameIntoView = bOnScreen && !Actor->bOnScreenLast;
		Actor->bOnScreenLast = bOnScreen;
		if (Now < Actor->NextUpdateTime && !bCameIntoView)
		{
			continue;
		}
		const float Interval = bShow ? FaunaRules::UpdateInterval(Distance, bOnScreen, Actor->IsBusy()) : HiddenInterval;
		FFaunaTick Step;
		Step.DeltaSeconds = Actor->LastUpdateTime < 0.0 ? 0.f : FMath::Min(static_cast<float>(Now - Actor->LastUpdateTime), LongestStep);
		Step.Distance = Distance;
		Step.bOnScreen = bOnScreen;
		Step.Since = Actor->LastUpdateTime < 0.0 ? Now - NoiseMemory : Actor->LastUpdateTime;
		Actor->LastUpdateTime = Now;
		Actor->NextUpdateTime = Now + Interval;
		Actor->UpdateFauna(Step, Context);
		++Updated;
	}
	UpdatedLastFrame = Updated;
	const float Milliseconds = static_cast<float>(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - StartCycles));
	AverageMilliseconds = FMath::Lerp(AverageMilliseconds, Milliseconds, CostSmoothing);
}
