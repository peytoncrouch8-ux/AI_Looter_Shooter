// ARespawnMarker's lantern: the post beside the grave, dark until the grave is marked, lit after, with a flare of light and
// flame when it catches (a first mark) and a lifting flame when the grave becomes the save point again.

#include "World/RespawnMarker.h"
#include "World/GraveMarkRules.h"
#include "World/RespawnMarkerLanternKit.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The glass slot of a lantern mesh: by its name, or the slot whose own material is named for it; INDEX_NONE without one. */
	int32 FindGlassSlot(const UStaticMeshComponent* Mesh, FName Slot)
	{
		const UStaticMesh* Model = Mesh ? Mesh->GetStaticMesh() : nullptr;
		if (!Model)
		{
			return INDEX_NONE;
		}
		const int32 Named = Mesh->GetMaterialIndex(Slot);
		if (Named != INDEX_NONE)
		{
			return Named;
		}
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			const UMaterialInterface* Own = Model->GetMaterial(Index);
			if (Own && Own->GetName().Contains(Slot.ToString()))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

void ARespawnMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FitLantern();
	ApplyLantern();
	ApplyFlare();
}

void ARespawnMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceLantern(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Where its pieces are
// ---------------------------------------------------------------------------

void ARespawnMarker::FitLantern()
{
	if (Lantern->GetStaticMesh() != LanternMesh)
	{
		// Another model (the build script's): its own glass is the lit one.
		Lantern->SetStaticMesh(LanternMesh);
		LitGlass = nullptr;
	}
	// Beside the spot, so the player who wakes here never stands in the post; its front toward the spot.
	Lantern->SetRelativeLocationAndRotation(LanternOffset, FRotator(0.0, LanternYaw, 0.0));
	Lantern->SetVisibility(LanternMesh != nullptr);
	const UStaticMesh* Model = Lantern->GetStaticMesh();
	const FName Socket = RespawnLantern::LightSocket();
	const FVector FlameAt = Model && Lantern->DoesSocketExist(Socket) ? Lantern->GetSocketTransform(Socket, RTS_Component).GetLocation()
		: RespawnLantern::FallbackFlame();
	LanternLight->SetRelativeLocation(FlameAt);
	Flame->SetRelativeLocation(FlameAt);
	BuildFlame();
}

void ARespawnMarker::BuildFlame()
{
	Flame->ClearInstances();
	Flame->SetNumCustomDataFloats(RespawnLantern::GlowFloats);
	// The engine's plane lies flat in XY: one card flat, one standing along X, one along Y, so the glow reads from every side.
	const FRotator Cards[] = { FRotator(0.0, 0.0, 0.0), FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) };
	const FLinearColor Colour = RespawnLantern::FlameColor();
	const float Data[] = { Colour.R, Colour.G, Colour.B, FlameStrength, RespawnLantern::RoundShape };
	for (const FRotator& Card : Cards)
	{
		Flame->SetCustomData(Flame->AddInstance(FTransform(Card, FVector::ZeroVector, FVector(FlameSize / 100.f))), Data);
	}
}

FVector ARespawnMarker::GetLanternLocation() const
{
	return Flame->GetComponentLocation();
}

// ---------------------------------------------------------------------------
// Dark, lit and the flare
// ---------------------------------------------------------------------------

void ARespawnMarker::ApplyLantern()
{
	const int32 Slot = FindGlassSlot(Lantern, GlassSlot);
	if (Slot != INDEX_NONE)
	{
		if (!LitGlass)
		{
			LitGlass = Lantern->GetStaticMesh()->GetMaterial(Slot);
		}
		UMaterialInterface* Wanted = bLit ? LitGlass.Get() : DarkGlass.Get();
		if (Wanted && Lantern->GetMaterial(Slot) != Wanted)
		{
			Lantern->SetMaterial(Slot, Wanted);
		}
	}
	LanternLight->SetVisibility(bLit);
	Flame->SetVisibility(bLit && LanternMesh != nullptr);
}

void ARespawnMarker::ApplyFlare()
{
	const bool bFirst = FlareKind == EGraveMark::First;
	const float Pulse = GraveMarkRules::FlarePulse(FlareAge, FlareKind);
	// A first mark's light catches from nothing in the rise; every other moment, at rest or lifting, the light is already on.
	const float Catch = bFirst && FlareAge >= 0.f ? FMath::Clamp(FlareAge / GraveMarkRules::FlareRiseSeconds, 0.f, 1.f) : 1.f;
	const float Peak = bFirst ? RespawnLantern::FirstPeakFactor : RespawnLantern::AgainPeakFactor;
	LanternLight->SetIntensity(bLit ? LanternCandela * Catch * (1.f + (Peak - 1.f) * Pulse) : 0.f);
	Flame->SetRelativeScale3D(FVector(1.f + (bFirst ? RespawnLantern::FirstSwell : RespawnLantern::AgainSwell) * Pulse));
}

void ARespawnMarker::SetLit(bool bInLit)
{
	bLit = bInLit;
	FlareAge = -1.f;
	SetActorTickEnabled(false);
	ApplyLantern();
	ApplyFlare();
}

void ARespawnMarker::StartFlare(EGraveMark Kind)
{
	if (Kind == EGraveMark::None)
	{
		return;
	}
	bLit = true;
	FlareKind = Kind;
	FlareLength = GraveMarkRules::FlareSeconds(Kind);
	FlareAge = 0.f;
	ApplyLantern();
	ApplyFlare();
	// Only for the second it lasts.
	SetActorTickEnabled(true);
}

void ARespawnMarker::AdvanceLantern(float DeltaSeconds)
{
	if (FlareAge < 0.f)
	{
		return;
	}
	FlareAge += DeltaSeconds;
	if (FlareAge >= FlareLength)
	{
		FlareAge = -1.f;
		SetActorTickEnabled(false);
	}
	ApplyFlare();
}

float ARespawnMarker::GetLanternCandela() const
{
	return bLit && LanternLight ? LanternLight->Intensity : 0.f;
}

float ARespawnMarker::GetFlameScale() const
{
	return Flame ? static_cast<float>(Flame->GetRelativeScale3D().X) : 1.f;
}
