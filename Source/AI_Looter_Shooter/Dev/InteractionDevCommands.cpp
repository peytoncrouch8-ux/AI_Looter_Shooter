// Developer console commands for the interaction system (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Interaction/InteractableProp.h"
#include "Interaction/InteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	APawn* FindPlayerPawn(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	bool IsGreyboxKind(const FString& Kind)
	{
		return Kind == TEXT("door") || Kind == TEXT("bell") || Kind == TEXT("lantern");
	}

	/** Sets up Prop as a greybox door, bell or lantern post. False when Kind is none of those. */
	bool SetUpGreybox(AInteractableProp& Prop, const FString& Kind)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (Kind == TEXT("door"))
		{
			// A 1 m by 2.1 m leaf from the hinge (the prop's origin) to its right.
			Prop.Mesh->SetStaticMesh(Cube);
			Prop.Mesh->SetRelativeLocation(FVector(0.0, 50.0, 105.0));
			Prop.Mesh->SetRelativeScale3D(FVector(0.08, 1.0, 2.1));
			Prop.Prompt = FText::FromString(TEXT("Open the door"));
			Prop.PromptWhenOn = FText::FromString(TEXT("Close the door"));
			Prop.UseMode = EInteractablePropUse::Toggle;
			Prop.Effect = EInteractablePropEffect::Swing;
			Prop.OpenAngle = -95.f;
			Prop.Tags.Add(FName(TEXT("Door")));
			return true;
		}
		if (Kind == TEXT("bell"))
		{
			Prop.Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
			Prop.Mesh->SetRelativeLocation(FVector(0.0, 0.0, 140.0));
			Prop.Mesh->SetRelativeScale3D(FVector(0.6));
			Prop.Prompt = FText::FromString(TEXT("Ring the bell"));
			Prop.bHold = true;
			Prop.HoldSeconds = 1.f;
			Prop.CooldownSeconds = 2.f;
			Prop.Tags.Add(FName(TEXT("Bell")));
			return true;
		}
		if (Kind == TEXT("lantern"))
		{
			// The keeper's lantern post, dark until lit; a plain post with the lantern glow if its model isn't there.
			UStaticMesh* Post = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_KeeperLanternPost.SM_KeeperLanternPost"));
			if (Post)
			{
				Prop.Mesh->SetStaticMesh(Post);
				Prop.GlowSlot = TEXT("LanternGlow");
				Prop.DarkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/MI_LanternIron.MI_LanternIron"));
			}
			else
			{
				Prop.Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
				Prop.Mesh->SetRelativeLocation(FVector(0.0, 0.0, 125.0));
				Prop.Mesh->SetRelativeScale3D(FVector(0.2, 0.2, 2.5));
				Prop.GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/MI_LanternGlow.MI_LanternGlow"));
			}
			Prop.Prompt = FText::FromString(TEXT("Light the lantern"));
			Prop.bHold = true;
			Prop.HoldSeconds = 1.5f;
			Prop.UseMode = EInteractablePropUse::TurnOn;
			Prop.Effect = EInteractablePropEffect::Glow;
			Prop.Tags.Add(FName(TEXT("Lantern")));
			return true;
		}
		return false;
	}

	/** Looter.Interaction.Spawn <door|bell|lantern>: a greybox prop two meters in front of the player, facing them. */
	void SpawnGreybox(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Interaction.Spawn: no player (start the game first)."));
			return;
		}
		const FString Kind = Args.Num() > 0 ? Args[0].ToLower() : FString(TEXT("door"));
		if (!IsGreyboxKind(Kind))
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Interaction.Spawn <door|bell|lantern>"));
			return;
		}
		const FVector Forward = FRotator(0.0, Pawn->GetViewRotation().Yaw, 0.0).Vector();
		const FVector Feet = Pawn->GetActorLocation() - FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight());
		// Its front toward the player. Set up before its play begins, which keeps how it was placed (shut, dark).
		const FTransform Where((-Forward).Rotation(), Feet + Forward * 200.0);
		AInteractableProp* Prop = GameWorld->SpawnActorDeferred<AInteractableProp>(AInteractableProp::StaticClass(), Where);
		if (!Prop)
		{
			return;
		}
		SetUpGreybox(*Prop, Kind);
		Prop->FinishSpawning(Where);
		UE_LOG(LogLooter, Log, TEXT("Looter.Interaction.Spawn: %s placed in front of the player (%s)."), *Kind, *Prop->GetName());
	}

	/** Looter.Interaction.Focus: what the player would use now, how, and how far through a hold. */
	void LogFocus(const TArray<FString>& Args, UWorld* World)
	{
		const APawn* Pawn = FindPlayerPawn(FindGameWorld(World));
		const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
		if (!Interaction)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Interaction.Focus: no player with an interaction component (start the game first)."));
			return;
		}
		const AActor* Focused = Interaction->GetFocusedActor();
		if (!Focused)
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Interaction.Focus: nothing in reach is looked at."));
			return;
		}
		const FInteractionOptions& Options = Interaction->GetFocusedOptions();
		UE_LOG(LogLooter, Log, TEXT("Looter.Interaction.Focus: %s. Tap: %s. Hold: %s. Holding: %.0f%%."), *Focused->GetName(),
			Options.bTap ? *FString::Printf(TEXT("'%s'"), *Options.TapPrompt.ToString()) : TEXT("no"),
			Options.bHold ? *FString::Printf(TEXT("'%s' (%.1f s)"), *Options.HoldPrompt.ToString(), Options.HoldSeconds) : TEXT("no"),
			Interaction->GetHoldProgress() * 100.f);
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnGreyboxCommand(
		TEXT("Looter.Interaction.Spawn"),
		TEXT("Looter.Interaction.Spawn <door|bell|lantern>: a greybox interactable two meters in front of the player (not saved), ")
		TEXT("to try the prompt, a tap (the door) and a hold (the bell, the lantern)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnGreybox));

	FAutoConsoleCommandWithWorldAndArgs LogFocusCommand(
		TEXT("Looter.Interaction.Focus"),
		TEXT("Looter.Interaction.Focus: logs what the player would use now (loot or an interactable), its tap and hold, and the hold's progress."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LogFocus));
}

#endif
