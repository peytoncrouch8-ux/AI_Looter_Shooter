// Developer console commands for the level's geometry, its playable area, and measuring what groups of it cost (not in
// shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/PlayableArea.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UnrealType.h"

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
		return World;
	}

	/**
	 * Looter.InstanceCollision [mesh name part]: for every instanced mesh in the world, whether its instances have physics
	 * bodies to collide with, and what decides it (the component's collision, its owner's, the mesh's hulls). Scattered
	 * trees once said they collided and still had no bodies.
	 */
	void InstanceCollisionCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FindGameWorld(World);
		if (!World)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.InstanceCollision: no world."));
			return;
		}
		const FString Filter = Args.Num() > 0 ? Args[0] : FString();
		const FBoolProperty* DisableCollision = FindFProperty<FBoolProperty>(UInstancedStaticMeshComponent::StaticClass(), TEXT("bDisableCollision"));
		int32 Listed = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UInstancedStaticMeshComponent*> Components(*It);
			for (const UInstancedStaticMeshComponent* Component : Components)
			{
				const UStaticMesh* Mesh = Component->GetStaticMesh();
				if (!Mesh || (!Filter.IsEmpty() && !Mesh->GetName().Contains(Filter)))
				{
					continue;
				}
				int32 Bodies = 0;
				for (const FBodyInstance* Body : Component->GetInstanceBodies())
				{
					Bodies += Body && Body->IsValidBodyInstance() ? 1 : 0;
				}
				const UBodySetup* Setup = Mesh->GetBodySetup();
				UE_LOG(LogLooter, Display,
					TEXT("Looter.InstanceCollision: %s on %s: %d instances, %d bodies (%d slots); collision %d (body %d, actor %s), profile %s, ")
					TEXT("disabled %s, physics state %s, registered %s, mesh compiling %s; mesh shapes %d, trace flag %d, cooked %s, cook failed %s"),
					*Mesh->GetName(), *It->GetName(), Component->GetInstanceCount(), Bodies, Component->GetInstanceBodies().Num(),
					static_cast<int32>(Component->GetCollisionEnabled()), static_cast<int32>(Component->BodyInstance.GetCollisionEnabled(false)),
					It->GetActorEnableCollision() ? TEXT("on") : TEXT("off"), *Component->GetCollisionProfileName().ToString(),
					DisableCollision && DisableCollision->GetPropertyValue_InContainer(Component) ? TEXT("yes") : TEXT("no"),
					Component->IsPhysicsStateCreated() ? TEXT("yes") : TEXT("no"), Component->IsRegistered() ? TEXT("yes") : TEXT("no"),
					Mesh->IsCompiling() ? TEXT("yes") : TEXT("no"), Setup ? Setup->AggGeom.GetElementCount() : -1,
					Setup ? static_cast<int32>(Setup->CollisionTraceFlag.GetValue()) : -1,
					Setup && Setup->bCreatedPhysicsMeshes ? TEXT("yes") : TEXT("no"), Setup && Setup->bFailedToCreatePhysicsMeshes ? TEXT("yes") : TEXT("no"));
				++Listed;
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.InstanceCollision: %d instanced meshes in %s."), Listed, *World->GetName());
	}

	FAutoConsoleCommandWithWorldAndArgs InstanceCollisionCommandRegistration(
		TEXT("Looter.InstanceCollision"),
		TEXT("Lists the world's instanced meshes and whether their instances have collision bodies: Looter.InstanceCollision [mesh name part]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&InstanceCollisionCommand));

	/** The line batch Looter.World.Bounds draws into (lines in it stay until it's cleared), and the world it's on in. */
	constexpr uint32 BoundsBatchId = 0x426F756E;
	TWeakObjectPtr<UWorld> BoundsShownIn;

	/**
	 * Looter.World.Bounds [1|0]: draws every playable area's boundary (closed edges orange, open edges cyan) and where
	 * its walls stand, in the game or, with no game running, in the editor; again (or 0) clears it. Logs each area's
	 * corners, open edges and walls, and whether the walls' collision profile is there.
	 */
	void BoundsCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FindGameWorld(World);
		ULineBatchComponent* Lines = World ? World->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent) : nullptr;
		if (!Lines)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.World.Bounds: no world to draw in."));
			return;
		}
		const bool bShow = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : BoundsShownIn.Get() != World;
		Lines->ClearBatch(BoundsBatchId);
		BoundsShownIn = bShow ? World : nullptr;
		if (!bShow)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.World.Bounds: off."));
			return;
		}

		FCollisionResponseTemplate Profile;
		const bool bProfile = UCollisionProfile::Get()->GetProfileTemplate(APlayableArea::WallProfile, Profile);
		int32 Areas = 0;
		for (TActorIterator<APlayableArea> It(World); It; ++It)
		{
			It->DrawBounds(*Lines, BoundsBatchId);
			const FPlayableBoundary& Outline = It->GetBoundary();
			int32 OpenCount = 0;
			for (int32 Edge = 0; Edge < Outline.NumEdges(); ++Edge)
			{
				OpenCount += Outline.IsOpen(Edge) ? 1 : 0;
			}
			UE_LOG(LogLooter, Display, TEXT("Looter.World.Bounds: %s: %d corners, %d open edges, %d walls built, %.0f square meters%s."),
				*It->GetName(), Outline.NumEdges(), OpenCount, It->GetWalls().Num(), Outline.SurfaceArea() / 10000.0,
				Outline.IsValid() ? TEXT("") : TEXT(", not a usable boundary"));
			++Areas;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.World.Bounds: on, %d playable areas in %s; the %s collision profile %s."), Areas,
			*World->GetName(), *APlayableArea::WallProfile.ToString(), bProfile ? TEXT("is there") : TEXT("is missing from DefaultEngine.ini"));
	}

	FAutoConsoleCommandWithWorldAndArgs BoundsCommandRegistration(
		TEXT("Looter.World.Bounds"),
		TEXT("Draws every playable area's boundary (closed edges orange, open edges cyan) and its walls; again, or 0, clears it: Looter.World.Bounds [1|0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BoundsCommand));

	/**
	 * Looter.Perf.HideTag <tag> [1|0]: hides (1, the default) or shows again (0) every actor and component carrying the
	 * tag, so a tour or perf run can measure what a group of things costs by the difference (the area build scripts tag
	 * everything past the boundary Beyond, and each zone Zone_<id>). Hidden in game, they leave the scene: no draws and
	 * no shadows. Showing again shows everything with the tag, even what was hidden for another reason.
	 */
	void HideTagCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FindGameWorld(World);
		if (!World || Args.Num() == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.HideTag <tag> [1|0]: %s"), World ? TEXT("which tag?") : TEXT("no world."));
			return;
		}
		const FName Tag(*Args[0]);
		const bool bHide = Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0;
		int32 Actors = 0;
		int32 Tagged = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->ActorHasTag(Tag))
			{
				Actor->SetActorHiddenInGame(bHide);
				++Actors;
			}
			TInlineComponentArray<USceneComponent*> SceneComponents(Actor);
			for (USceneComponent* Component : SceneComponents)
			{
				if (Component && Component->ComponentHasTag(Tag))
				{
					Component->SetHiddenInGame(bHide, /*bPropagateToChildren*/ true);
					++Tagged;
				}
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.HideTag: %s %d actors and %d components tagged %s in %s."),
			bHide ? TEXT("hid") : TEXT("showed"), Actors, Tagged, *Tag.ToString(), *World->GetName());
	}

	FAutoConsoleCommandWithWorldAndArgs HideTagCommandRegistration(
		TEXT("Looter.Perf.HideTag"),
		TEXT("Hides (1, the default) or shows again (0) every actor and component with a tag, to measure its cost by the difference: Looter.Perf.HideTag <tag> [1|0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HideTagCommand));
}

#endif
