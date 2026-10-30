#include "UI/World/StatusCalloutSubsystem.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Elements/ElementStatusComponent.h"
#include "UI/World/DamageNumberActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The callout's sound (lane E's UI.StatusCallout; a bank without it yet plays nothing and warns once). */
	const FName CalloutCue(TEXT("UI.StatusCallout"));
	/** Under the hit confirm: the word is news, but the guns stay the loudest thing. */
	constexpr float CalloutVolume = 0.8f;
}

bool UStatusCalloutSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the tests build (no local player there, so nothing shows).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UStatusCalloutSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Every world's statuses go through the one delegate: each world's subsystem answers only its own creatures.
	StatusSetHandle = UElementStatusComponent::OnStatusSet.AddUObject(this, &UStatusCalloutSubsystem::HandleStatusSet);
}

void UStatusCalloutSubsystem::Deinitialize()
{
	UElementStatusComponent::OnStatusSet.Remove(StatusSetHandle);
	StatusSetHandle.Reset();
	Recent.Reset();
	Super::Deinitialize();
}

void UStatusCalloutSubsystem::HandleStatusSet(AActor* Target, ELooterElement Element, bool bFirstOnThisCreature)
{
	UWorld* World = GetWorld();
	if (!bFirstOnThisCreature || !Target || Element == ELooterElement::None || !World || Target->GetWorld() != World)
	{
		return;
	}
	// Shown to the player, as the damage numbers are: none without one to see it, none while its numbers are hidden.
	const APlayerController* Player = World->GetFirstPlayerController();
	if (!Player || !Player->IsLocalController())
	{
		return;
	}
	const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
	if (!Health || !Health->bShowDamageNumbers || Health->IsDead())
	{
		return;
	}

	// A second callout on the same creature in the same moment (two guns, a spread rot) sits a slot higher.
	const double Now = World->GetTimeSeconds();
	Recent.RemoveAll([Now](const FRecent& Entry) { return !Entry.Creature.IsValid() || Now - Entry.When > StackSeconds || Now < Entry.When; });
	int32 OnThis = 0;
	for (const FRecent& Entry : Recent)
	{
		OnThis += Entry.Creature.Get() == Target ? 1 : 0;
	}
	Recent.Add({ Target, Now });

	float Radius = 0.f;
	float HalfHeight = 0.f;
	Target->GetSimpleCollisionCylinder(Radius, HalfHeight);
	const FVector Where = Target->GetActorLocation() + FVector(0.0, 0.0, HalfHeight * CalloutHeight);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = Target;
	// The creature's own number class when it names one (a Blueprint's look), the plain one otherwise.
	UClass* NumberClass = Health->DamageNumberClass ? Health->DamageNumberClass.Get() : ADamageNumberActor::StaticClass();
	if (ADamageNumberActor* Callout = World->SpawnActor<ADamageNumberActor>(NumberClass, Where, FRotator::ZeroRotator, Params))
	{
		Callout->ShowCallout(Element, CalloutSlot(OnThis));
		LooterSound::Play2D(this, CalloutCue, CalloutVolume);
	}
}
