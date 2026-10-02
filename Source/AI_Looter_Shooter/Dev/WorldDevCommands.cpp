// Developer console commands for the level's geometry (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
}

#endif
