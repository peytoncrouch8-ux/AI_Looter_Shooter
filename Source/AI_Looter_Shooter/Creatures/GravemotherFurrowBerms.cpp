// AGravemotherFurrow's berms (GravemotherFurrow.h): each rising out of the ground beside the line once her body is past it,
// the earth she throws out both sides as she ploughs, the heave's crest riding the split's front, and the berms crumbling
// back into the ground when their time is up.

#include "Creatures/GravemotherFurrow.h"
#include "Audio/LooterSound.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GravemotherFight.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** The stand-in berm: the engine cube made 230 long, 120 deep and 74 high (the sheet's segment, a little slimmer). */
	const FVector StandInBermScale(2.3, 1.2, 0.74);
	/** A berm starts this deep under the ground (a share of its height) and rises out of it. */
	constexpr float BuriedShare = 0.85f;
	/** The heave's crest: the berm low and short, riding just behind the split's front. */
	const FVector CrestScale(0.5, 0.65, 0.32);
	constexpr float CrestBehind = 40.f;
	/** As they crumble, the far end goes up to this long after the near (s). */
	constexpr float CrumbleStagger = 0.3f;
	/** Earth is thrown out this far either side of her line (cm) as she ploughs. */
	constexpr float SprayOut = 110.f;
}

void AGravemotherFurrow::MakeBermLooks()
{
	UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::Berm);
	bStandInBerm = Mesh == nullptr;
	if (Mesh)
	{
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		BermLength = static_cast<float>(Bounds.BoxExtent.X * 2.0);
		BermHeight = static_cast<float>(Bounds.BoxExtent.Z * 2.0);
		BermSet->SetStaticMesh(Mesh);
		Crest->SetStaticMesh(Mesh);
		return;
	}
	UStaticMesh* Cube = GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Cube);
	BermLength = 100.f * static_cast<float>(StandInBermScale.X);
	BermHeight = 100.f * static_cast<float>(StandInBermScale.Z);
	BermSet->SetStaticMesh(Cube);
	Crest->SetStaticMesh(Cube);
	BermSet->SetMaterial(0, GravemotherAssets::StandInLook(this, GravemotherAssets::BermColor()));
	Crest->SetMaterial(0, GravemotherAssets::StandInLook(this, GravemotherAssets::BermColor()));
}

FTransform AGravemotherFurrow::BermTransform(const FBerm& Berm, float Share, float Sink) const
{
	// Out of the ground as it rises (a little squat at first), sunk Sink as it crumbles; the stand-in cube's pivot is in
	// its middle, so it's lifted half its height.
	const float Tall = Berm.Height * (0.4f + 0.6f * Share);
	const float Height = BermHeight * Tall;
	FVector Where = Berm.Ground - FVector(0.0, 0.0, (1.f - Share) * BermHeight * Berm.Height * BuriedShare + Sink);
	FVector Scale(1.0, 1.0, Tall);
	if (bStandInBerm)
	{
		Scale = FVector(StandInBermScale.X, StandInBermScale.Y, StandInBermScale.Z * Tall);
		Where.Z += Height * 0.5f;
	}
	return FTransform(FRotator(0.f, Berm.Yaw, 0.f), Where, Scale);
}

void AGravemotherFurrow::RaiseBerms()
{
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	for (FBerm& Berm : Berms)
	{
		if (Berm.Rise >= 0.f || Covered - Settings.RearLag < Berm.Along)
		{
			continue;
		}
		Berm.Rise = 0.f;
		Berm.Instance = BermSet->AddInstance(BermTransform(Berm, 0.f, 0.f), /*bWorldSpace*/ true);
		if (Effects)
		{
			// Earth spilling off it as it heaves up.
			FGraveDustBurst Spill;
			Spill.Count = 2;
			Spill.Spread = BermLength * 0.3f;
			Spill.SpreadUp = 20.f;
			Spill.StartSize = 50.f;
			Spill.EndSize = 130.f;
			Spill.Life = 1.f;
			Spill.Opacity = 0.28f;
			Spill.Drift = FVector(0.0, 0.0, 50.0);
			Spill.Outward = 60.f;
			Effects->Puffs(Berm.Ground + FVector(0.0, 0.0, 30.0), Spill);
		}
	}
}

void AGravemotherFurrow::TickBerms(float DeltaSeconds)
{
	const float RiseSeconds = FMath::Max(Settings.BermRiseSeconds, 0.05f);
	for (FBerm& Berm : Berms)
	{
		if (Berm.Rise < 0.f || Berm.Instance == INDEX_NONE || Berm.Rise > RiseSeconds)
		{
			continue;
		}
		Berm.Rise += DeltaSeconds;
		const float Share = GravemotherFight::BermRise(Berm.Rise, RiseSeconds);
		BermSet->UpdateInstanceTransform(Berm.Instance, BermTransform(Berm, Share, 0.f), /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true,
			/*bTeleport*/ true);
	}
}

void AGravemotherFurrow::ThrowEarthAt(float Along)
{
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	if (!Effects)
	{
		return;
	}
	// Earth thrown out both sides of her head as she ploughs.
	const FVector Middle = Start + Direction * Along;
	const bool bClods = LastSprayAlong < 0.f || Along - LastSprayAlong > 90.f;
	for (const float Side : { -1.f, 1.f })
	{
		const FVector Out = Across * Side;
		const FVector At = Middle + Out * SprayOut + FVector(0.0, 0.0, 20.0);
		FGraveDustBurst Spray;
		Spray.Count = 2;
		Spray.Spread = 40.f;
		Spray.SpreadUp = 25.f;
		Spray.StartSize = 55.f;
		Spray.EndSize = 150.f;
		Spray.Life = 1.2f;
		Spray.Opacity = 0.32f;
		Spray.Drift = Out * 220.f + FVector(0.0, 0.0, 90.0);
		Spray.Outward = 50.f;
		Effects->Puffs(At, Spray);
		Effects->Chunks(EGraveChunk::Debris, At, (Out + FVector::UpVector).GetSafeNormal(), 1, 520.f);
		if (bClods)
		{
			Effects->Chunks(EGraveChunk::Clod, At, (Out + FVector::UpVector).GetSafeNormal(), 1, 420.f);
		}
	}
	if (bClods)
	{
		LastSprayAlong = Along;
	}
}

void AGravemotherFurrow::PlaceCrest()
{
	if (Phase != EPhase::Heave || Front <= 0.f || Front >= Length - 1.f)
	{
		if (Crest->GetInstanceCount() > 0)
		{
			Crest->ClearInstances();
		}
		return;
	}
	// The ground bulging just behind the split's front as it crawls (it pulses a little).
	const FVector At = FrontPoint->GetComponentLocation() - Direction * CrestBehind;
	const float Pulse = 1.f + 0.15f * FMath::Sin(Front * 0.05f);
	FVector Scale(CrestScale.X, CrestScale.Y, CrestScale.Z * Pulse);
	FVector Where = At - FVector(0.0, 0.0, BermHeight * 0.1f);
	if (bStandInBerm)
	{
		Scale = FVector(StandInBermScale.X * CrestScale.X, StandInBermScale.Y * CrestScale.Y, StandInBermScale.Z * CrestScale.Z * Pulse);
		Where.Z += BermHeight * CrestScale.Z * 0.5f;
	}
	const FTransform Shape(Direction.Rotation(), Where, Scale);
	if (Crest->GetInstanceCount() == 0)
	{
		Crest->AddInstance(Shape, /*bWorldSpace*/ true);
	}
	else
	{
		Crest->UpdateInstanceTransform(0, Shape, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
}

void AGravemotherFurrow::BeginCrumble()
{
	Phase = EPhase::Crumbling;
	PhaseTime = 0.f;
	// Cover no more as they start to go.
	BermSet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	for (const float Side : { -1.f, 1.f })
	{
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;
		for (const FBerm& Berm : Berms)
		{
			if (Berm.Side == Side && Berm.Instance != INDEX_NONE)
			{
				Sum += Berm.Ground;
				++Count;
			}
		}
		if (Count > 0)
		{
			LooterSound::PlayAt(this, LooterSoundCue::GravemotherBermCrumble, Sum / Count);
		}
	}
}

void AGravemotherFurrow::TickCrumble(float DeltaSeconds)
{
	const float Seconds = FMath::Max(Settings.BermCrumbleSeconds, 0.1f);
	const float Each = FMath::Max(Seconds - CrumbleStagger, 0.05f);
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	const float Before = PhaseTime - DeltaSeconds;
	for (const FBerm& Berm : Berms)
	{
		if (Berm.Instance == INDEX_NONE)
		{
			continue;
		}
		// The near end first, the far end CrumbleStagger later: slumping and sinking back into the ground.
		const float Delay = CrumbleStagger * Berm.Along / FMath::Max(Length, 1.f);
		const float Share = FMath::Clamp((PhaseTime - Delay) / Each, 0.f, 1.f);
		BermSet->UpdateInstanceTransform(Berm.Instance, BermTransform(Berm, 1.f - 0.5f * Share, Share * BermHeight * Berm.Height * 0.7f),
			/*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		// Each puffs its dust as it starts to go.
		if (Effects && Before < Delay && PhaseTime >= Delay)
		{
			FGraveDustBurst Slump;
			Slump.Count = 2;
			Slump.Spread = BermLength * 0.35f;
			Slump.SpreadUp = 15.f;
			Slump.StartSize = 60.f;
			Slump.EndSize = 150.f;
			Slump.Life = 1.3f;
			Slump.Opacity = 0.26f;
			Slump.Outward = 50.f;
			Effects->Puffs(Berm.Ground + FVector(0.0, 0.0, 25.0), Slump);
			Effects->Chunks(EGraveChunk::Debris, Berm.Ground + FVector(0.0, 0.0, 40.0), FVector::UpVector, 1, 200.f);
		}
	}
	FadeDecals(1.f - PhaseTime / Seconds);
	if (PhaseTime >= Seconds)
	{
		Destroy();
	}
}

int32 AGravemotherFurrow::NumBermsRisen() const
{
	int32 Risen = 0;
	for (const FBerm& Berm : Berms)
	{
		Risen += Berm.Instance != INDEX_NONE ? 1 : 0;
	}
	return Risen;
}

TArray<FVector> AGravemotherFurrow::GetRisenBermSpots() const
{
	TArray<FVector> Spots;
	for (const FBerm& Berm : Berms)
	{
		if (Berm.Instance != INDEX_NONE)
		{
			Spots.Add(Berm.Ground);
		}
	}
	return Spots;
}
