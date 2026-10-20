#include "UI/World/XpPopupSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTierLook.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/World/XpPopupActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UXpPopupSubsystem* UXpPopupSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UXpPopupSubsystem>() : nullptr;
}

bool UXpPopupSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UXpPopupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!InWorld.IsGameWorld())
	{
		return;
	}
	// The player's progression outlives the level (a local player's): bound for this level only, let go as it ends.
	const UGameInstance* GameInstance = InWorld.GetGameInstance();
	const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	if (UPlayerProgressionSubsystem* Found = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr)
	{
		Progression = Found;
		KillHandle = Found->OnKillXP.AddUObject(this, &UXpPopupSubsystem::HandleKillXP);
	}
}

void UXpPopupSubsystem::Deinitialize()
{
	if (UPlayerProgressionSubsystem* Bound = Progression.Get())
	{
		Bound->OnKillXP.Remove(KillHandle);
	}
	Progression.Reset();
	KillHandle.Reset();
	Pool.Reset();
	Super::Deinitialize();
}

void UXpPopupSubsystem::HandleKillXP(int64 XP, const AActor* Victim)
{
	ShowKill(XP, Victim);
}

int32 UXpPopupSubsystem::NumPooled() const
{
	int32 Count = 0;
	for (const TObjectPtr<AXpPopupActor>& Popup : Pool)
	{
		Count += IsValid(Popup) ? 1 : 0;
	}
	return Count;
}

int32 UXpPopupSubsystem::NumShowing() const
{
	int32 Count = 0;
	for (const TObjectPtr<AXpPopupActor>& Popup : Pool)
	{
		Count += IsValid(Popup) && Popup->IsShowing() ? 1 : 0;
	}
	return Count;
}

AXpPopupActor* UXpPopupSubsystem::Take()
{
	Pool.RemoveAll([](const TObjectPtr<AXpPopupActor>& Popup) { return !IsValid(Popup); });
	AXpPopupActor* Oldest = nullptr;
	for (const TObjectPtr<AXpPopupActor>& Popup : Pool)
	{
		if (!Popup->IsShowing())
		{
			return Popup;
		}
		if (!Oldest || Popup->GetAge() > Oldest->GetAge())
		{
			Oldest = Popup;
		}
	}
	UWorld* World = GetWorld();
	if (Pool.Num() < MaxPopups && World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		if (AXpPopupActor* Made = World->SpawnActor<AXpPopupActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			Pool.Add(Made);
			return Made;
		}
	}
	// All busy: a burst of kills takes over the one that's been up longest (nearly gone anyway).
	return Oldest;
}

AXpPopupActor* UXpPopupSubsystem::ShowKill(int64 Amount, const AActor* Victim)
{
	if (Amount <= 0 || !Victim)
	{
		return nullptr;
	}
	// With its damage numbers: none while they're hidden (a boss between phases).
	const UHealthComponent* Health = Victim->FindComponentByClass<UHealthComponent>();
	if (Health && !Health->bShowDamageNumbers)
	{
		return nullptr;
	}
	// Up on its body, about where the killing shots' numbers rise from.
	float Radius = 0.f;
	float HalfHeight = 0.f;
	Victim->GetSimpleCollisionCylinder(Radius, HalfHeight);
	// A Hallowed or Ethereal kill's reward in its tier's colour (its rank is still the one it died at).
	const ACreatureBase* Creature = Cast<ACreatureBase>(Victim);
	const bool bTier = Creature && CreatureTierLook::TintsKillXP(Creature->GetRank());
	const FLinearColor TierColor = bTier ? CreatureTierLook::TagColor(Creature->GetRank()) : FLinearColor::White;
	return ShowKillAt(Amount, Victim->GetActorLocation() + FVector(0.0, 0.0, HalfHeight * KillSpotHeight), bTier, TierColor);
}

AXpPopupActor* UXpPopupSubsystem::ShowKillAt(int64 Amount, const FVector& Where, bool bTier, const FLinearColor& TierColor)
{
	if (Amount <= 0)
	{
		return nullptr;
	}
	AXpPopupActor* Popup = Take();
	if (Popup)
	{
		Popup->Show(Amount, /*bBig*/ false, Where, KillDelay, bTier, TierColor);
	}
	return Popup;
}

AXpPopupActor* UXpPopupSubsystem::ShowMission(int64 Amount, const APlayerController* Player, float Delay)
{
	const APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
	if (Amount <= 0 || !Camera)
	{
		return nullptr;
	}
	AXpPopupActor* Popup = ShowMissionAt(Amount, Camera->GetCameraLocation(), Delay);
	if (Popup)
	{
		// Before the eye in first person, about the body in third (the camera is behind it): near the player either way.
		Popup->FollowView(FVector(MissionAhead, MissionRight, MissionUp));
	}
	return Popup;
}

AXpPopupActor* UXpPopupSubsystem::ShowMissionAt(int64 Amount, const FVector& Where, float Delay)
{
	if (Amount <= 0)
	{
		return nullptr;
	}
	AXpPopupActor* Popup = Take();
	if (Popup)
	{
		Popup->Show(Amount, /*bBig*/ true, Where, Delay);
	}
	return Popup;
}
