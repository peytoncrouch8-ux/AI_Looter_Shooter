#include "Interaction/InteractionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UInteractionSubsystem* UInteractionSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UInteractionSubsystem>() : nullptr;
}

bool UInteractionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UInteractionSubsystem::Register(AActor* Interactable)
{
	if (!Interactable)
	{
		return;
	}
	// Ones destroyed without leaving (a test level torn down) go as others join.
	Interactables.RemoveAll([](const TWeakObjectPtr<AActor>& Entry) { return !Entry.IsValid(); });
	Interactables.AddUnique(Interactable);
}

void UInteractionSubsystem::Unregister(AActor* Interactable)
{
	Interactables.RemoveAll([Interactable](const TWeakObjectPtr<AActor>& Entry) { return !Entry.IsValid() || Entry.Get() == Interactable; });
}
