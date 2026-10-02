#pragma once

// What the interaction tests build in their test levels: the player's stand-in, and props to use (a door, a bell, a
// lantern post). A test level's traces don't see new bodies, so its interaction component works from the cone alone.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractableProp.h"
#include "Interaction/InteractionComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/Material.h"

namespace InteractionTestWorld
{
	/** The stand-in stands at the origin; its eyes are this high (APawn's BaseEyeHeight). */
	inline constexpr double EyeHeight = 64.0;

	/** A spot Distance straight ahead of the stand-in looking along +X, at eye height. */
	inline FVector Ahead(double Distance)
	{
		return FVector(Distance, 0.0, EyeHeight);
	}

	/** The player's stand-in: a pawn that stands at the origin looking along its facing, with an interaction component. */
	inline APawn* SpawnPlayer(UWorld* World, UInteractionComponent*& OutInteraction)
	{
		OutInteraction = nullptr;
		APawn* Player = World->SpawnActor<APawn>();
		if (!Player)
		{
			return nullptr;
		}
		USceneComponent* Root = NewObject<USceneComponent>(Player, TEXT("Root"));
		Player->SetRootComponent(Root);
		Root->RegisterComponent();
		OutInteraction = NewObject<UInteractionComponent>(Player, TEXT("Interaction"));
		OutInteraction->RegisterComponent();
		return Player;
	}

	/** Turns the stand-in to look along Yaw (degrees), level. */
	inline void Face(APawn* Player, double Yaw)
	{
		Player->SetActorRotation(FRotator(0.0, Yaw, 0.0));
	}

	/** Play begins for a prop, as it does in a level: it joins the level's interactables. */
	inline AInteractableProp* Begin(AInteractableProp* Prop)
	{
		if (Prop)
		{
			Prop->DispatchBeginPlay();
		}
		return Prop;
	}

	/** A door: a tap opens it and another shuts it, swinging 90 degrees about its hinge (its origin) in 0.4 s. */
	inline AInteractableProp* SpawnDoor(UWorld* World, const FVector& Where, const FVector& LeafOffset = FVector::ZeroVector)
	{
		AInteractableProp* Door = World->SpawnActor<AInteractableProp>(Where, FRotator::ZeroRotator);
		if (!Door)
		{
			return nullptr;
		}
		Door->Prompt = FText::FromString(TEXT("Open the door"));
		Door->PromptWhenOn = FText::FromString(TEXT("Close the door"));
		Door->UseMode = EInteractablePropUse::Toggle;
		Door->Effect = EInteractablePropEffect::Swing;
		Door->OpenAngle = 90.f;
		Door->SwingSeconds = 0.4f;
		Door->CooldownSeconds = 0.5f;
		Door->Mesh->SetRelativeLocation(LeafOffset);
		Door->Tags.Add(FName(TEXT("Door")));
		return Begin(Door);
	}

	/** A bell: held for a second to ring, as often as liked, two seconds apart. */
	inline AInteractableProp* SpawnBell(UWorld* World, const FVector& Where)
	{
		AInteractableProp* Bell = World->SpawnActor<AInteractableProp>(Where, FRotator::ZeroRotator);
		if (!Bell)
		{
			return nullptr;
		}
		Bell->Prompt = FText::FromString(TEXT("Ring the bell"));
		Bell->bHold = true;
		Bell->HoldSeconds = 1.f;
		Bell->UseMode = EInteractablePropUse::Trigger;
		Bell->CooldownSeconds = 2.f;
		Bell->Tags.Add(FName(TEXT("Bell")));
		return Begin(Bell);
	}

	/** The glow a lit lantern's glass takes in the tests: the engine's default material. */
	inline UMaterialInterface* LanternGlow()
	{
		return UMaterial::GetDefaultMaterial(MD_Surface);
	}

	/** A lantern post: held 1.5 s to light, after which it can't be lit again until put out; lit, its glass glows. */
	inline AInteractableProp* SpawnLantern(UWorld* World, const FVector& Where, bool bLit)
	{
		AInteractableProp* Lantern = World->SpawnActor<AInteractableProp>(Where, FRotator::ZeroRotator);
		if (!Lantern)
		{
			return nullptr;
		}
		Lantern->Prompt = FText::FromString(TEXT("Light the lantern"));
		Lantern->bHold = true;
		Lantern->HoldSeconds = 1.5f;
		Lantern->UseMode = EInteractablePropUse::TurnOn;
		Lantern->Effect = EInteractablePropEffect::Glow;
		Lantern->GlowMaterial = LanternGlow();
		Lantern->bStartOn = bLit;
		Lantern->Tags.Add(FName(TEXT("Lantern")));
		return Begin(Lantern);
	}
}

#endif
