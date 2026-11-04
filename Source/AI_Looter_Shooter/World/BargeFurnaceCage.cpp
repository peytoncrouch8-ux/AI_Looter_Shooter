// ABargeFurnace: its soul cage: opening it (the held Interact, Abel's lantern, the story), its burst (the boiler's
// porthole torn open, the shards flung), the furnace going out, and where the cage, its mouth and the hall's back wall are.

#include "World/BargeFurnace.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Scenes/CageBurst.h"
#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "World/KeepersLantern.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The burst's shards fan out within this of the cage's facing, from this far past the drum to this far short of the
	 *  hall's wall; each this big (shard model units). */
	constexpr float ShardFan = 70.f;
	constexpr float ShardNear = 60.f;
	constexpr float ShardShort = 60.f;
	constexpr float ShardLeast = 0.6f;
	constexpr float ShardMost = 1.3f;
}

// ---------------------------------------------------------------------------
// The cage
// ---------------------------------------------------------------------------

bool ABargeFurnace::CanOpenCage(bool bForce) const
{
	if (bCageBurst || bOut)
	{
		return false;
	}
	if (bForce)
	{
		return true;
	}
	return ValvesShut >= BargeRules::NumValves && StoryHolds(OpenWhen) && AKeepersLantern::IsTakenIn(this);
}

bool ABargeFurnace::BurstCage(AActor* By, bool bForce)
{
	if (!CanOpenCage(bForce))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the soul cage is opened. What comes out isn't Abel."), *GetActorNameOrLabel());
	ABargeBoarding* Boarding = Barge.Get();
	// The scene bursts it and sets the barge dying as it ends; with scenes off it leaves that end state at once.
	if (!Boarding || !CageBurst::Play(*Boarding, *this))
	{
		SetCageBurst(true);
		if (Boarding)
		{
			Boarding->NoteCageBurst();
		}
	}
	return true;
}

void ABargeFurnace::SetCageBurst(bool bBurst)
{
	bCageBurst = bBurst;
	Heart->SetVisibility(!bBurst && !bOut);
	HeartSmog->SetVisibility(!bBurst && !bOut);
	CageFrame->SetVisibility(!WholeMesh);
	CageLight->SetVisibility(!bBurst && !bOut);
	// The boiler burst: its porthole torn open on its hinge, bars snapped and bent (the same pivot, sockets and slots).
	if (WholeMesh && BrokenMesh)
	{
		Body->SetStaticMesh(bBurst ? BrokenMesh.Get() : WholeMesh.Get());
		UpdateGlow();
	}
	Shards->ClearInstances();
	if (bBurst)
	{
		// The porthole's plate and bars flung across the floor, fanned out from the cage (inside the smaller hall's wall).
		FRandomStream Spread(6006);
		TArray<FTransform> Flung;
		const float Near = BargeRules::FurnaceRadius + ShardNear;
		const float Far = FMath::Max(Near, BargeRules::HallRadius - ShardShort);
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const float Turn = FMath::DegreesToRadians(Spread.FRandRange(-ShardFan, ShardFan));
			const float Reach = Spread.FRandRange(Near, Far);
			const FRotator Tumble(Spread.FRandRange(-20.f, 20.f), Spread.FRandRange(0.f, 360.f), Spread.FRandRange(-20.f, 20.f));
			const FVector At(FMath::Cos(Turn) * Reach, FMath::Sin(Turn) * Reach, 0.0);
			if (bShardModel)
			{
				// The torn plates lie on the floor (their pivot at their foot), each its own size.
				Flung.Add(FTransform(Tumble, At, FVector(Spread.FRandRange(ShardLeast, ShardMost))));
			}
			else
			{
				Flung.Add(FTransform(Tumble, At + FVector(0.0, 0.0, 8.0), FVector(Spread.FRandRange(0.2f, 0.5f), Spread.FRandRange(0.06f, 0.3f), 0.04)));
			}
		}
		Shards->AddInstances(Flung, /*bShouldReturnIndices*/ false);
	}
}

void ABargeFurnace::UpdateGlow()
{
	// The boiler's stoke door and ash vents glow (its CinderGlow slot) while it burns; out, or starved by all three valves,
	// they go dark.
	if (WholeMesh && Body)
	{
		const bool bDark = bOut || ValvesShut >= BargeRules::NumValves;
		BargeLooks::SetSlot(*Body, BargeLooks::GlowSlot, bDark ? DarkGlow.Get() : nullptr);
	}
}

FVector ABargeFurnace::GetCageMouth() const
{
	return GetActorTransform().TransformPosition(MouthAt);
}

void ABargeFurnace::PutOut()
{
	bOut = true;
	SetRingRunning(false);
	Heart->SetVisibility(false);
	HeartSmog->SetVisibility(false);
	CageLight->SetVisibility(false);
	UpdateGlow();
}

FVector ABargeFurnace::GetCageLocation() const
{
	return GetActorTransform().TransformPosition(HeartAt);
}

FVector ABargeFurnace::GetCageFacing() const
{
	return GetActorForwardVector().GetSafeNormal2D();
}

FVector ABargeFurnace::GetBackWall() const
{
	return GetActorTransform().TransformPosition(FVector(-BargeRules::HallRadius, 0.0, BargeRules::CageUp + 200.0));
}

FInteractionOptions ABargeFurnace::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (bCageBurst || bOut || ValvesShut < BargeRules::NumValves || !StoryHolds(OpenWhen))
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	if (!AKeepersLantern::IsTakenIn(this))
	{
		// Shown, not usable: the reason the cage won't be opened yet (he can only be carried in his own lantern).
		Options.bUsable = false;
		Options.bTap = true;
		Options.TapPrompt = NeedsLanternPrompt;
		return Options;
	}
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	return Options;
}

bool ABargeFurnace::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && BurstCage(User.GetOwner());
}

TOptional<FVector> ABargeFurnace::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(HeartAt + FVector(80.0, 0.0, 0.0)));
}

TOptional<FInteractionBox> ABargeFurnace::GetInteractionBox() const
{
	// The cage's window and an arm's reach in front of it.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(HeartAt + FVector(40.0, 0.0, 0.0)), FVector(110.0, 140.0, 140.0), GetActorQuat());
}
