// AChapelWindowHose: its line: the curve in through the window, the sections laid along it, her light's pulses running out
// along it, and the whip back out of the window. Its story, its pull and its sounds are ChapelWindowHose.cpp's.

#include "World/ChapelWindowHose.h"
#include "World/ChapelReliquary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"

namespace
{
	/** The hose: 16 cm across (the cart's is 14: this is its last, fattest length). */
	constexpr float HoseGirth = 16.f;
	/** Her light drawn out along it: this share of its length a second, the pulses' size, strength and colour. */
	constexpr float PulseSpeed = 0.32f;
	constexpr float PulseSize = 14.f;
	constexpr float PulseStrength = 10.f;
	const FLinearColor PulseColor(1.f, 0.45f, 0.1f);
	/** The whip: its first share draws the end back through the window; the rest throws it out and down, snaking this far. */
	constexpr float WhipBackShare = 0.5f;
	constexpr float WhipSnake = 45.f;
	/** Its pulses are drawn within this (cm). */
	constexpr double PulseReach = 4500.0;

	FVector Bezier(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T)
	{
		const float U = 1.f - T;
		return A * (U * U * U) + B * (3.f * U * U * T) + C * (3.f * U * T * T) + D * (T * T * T);
	}
}

FVector AChapelWindowHose::InsideCurve(const FVector& End, float FloorDrop, float Alpha)
{
	// From the sill down the inside of the wall to the floor, along it, and up onto the Reliquary.
	const FVector Down(-35.0, 0.0, -FloorDrop * 1.15);
	const FVector Flat(End.X, End.Y, 0.0);
	const FVector Toward = Flat.GetSafeNormal();
	const FVector Under = Flat - Toward * 70.0 + FVector(0.0, 0.0, -FloorDrop * 1.1);
	return Bezier(FVector::ZeroVector, Down, Under, End, FMath::Clamp(Alpha, 0.f, 1.f));
}

FVector AChapelWindowHose::EndLocal() const
{
	if (const AChapelReliquary* Found = Reliquary.Get())
	{
		// Over her ember, its nozzle at the recess.
		return GetActorTransform().InverseTransformPosition(Found->GetEmberStart() + FVector(0.0, 0.0, 6.0));
	}
	return InsideEnd;
}

FVector AChapelWindowHose::WhipEnd(float Alpha) const
{
	const float Back = FMath::Clamp(Alpha / WhipBackShare, 0.f, 1.f);
	if (Alpha < WhipBackShare)
	{
		// Drawn back along its own line, fast at first, toward the sill.
		return InsideCurve(EndLocal(), InsideFloorDrop, 1.f - FMath::Pow(Back, 0.7f));
	}
	// Thrown out over the sill in an arc and down onto the yard.
	const float Out = FMath::Clamp((Alpha - WhipBackShare) / (1.f - WhipBackShare), 0.f, 1.f);
	const FVector Over(80.0, 0.0, 70.0);
	return Bezier(FVector::ZeroVector, Over, PulledRest + FVector(0.0, 0.0, 120.0), PulledRest, Out);
}

void AChapelWindowHose::BuildLine(TArray<FVector>& OutPoints) const
{
	OutPoints.Reset(SectionCount + 1);
	if (!bOut)
	{
		const FVector End = EndLocal();
		for (int32 Index = 0; Index <= SectionCount; ++Index)
		{
			OutPoints.Add(InsideCurve(End, InsideFloorDrop, static_cast<float>(Index) / SectionCount));
		}
		return;
	}
	const float Alpha = FMath::Clamp(WhipClock / WhipSeconds, 0.f, 1.f);
	if (Alpha < WhipBackShare)
	{
		// Still inside: the line up to where its end has been drawn back, snaking as it goes.
		const float Reach = 1.f - FMath::Pow(Alpha / WhipBackShare, 0.7f);
		for (int32 Index = 0; Index <= SectionCount; ++Index)
		{
			const float T = Reach * Index / SectionCount;
			FVector Point = InsideCurve(EndLocal(), InsideFloorDrop, T);
			Point.Y += FMath::Sin(T * 14.f + Alpha * 30.f) * WhipSnake * (1.f - Alpha) * T;
			OutPoints.Add(Point);
		}
		return;
	}
	// Out: from the sill over and down to its end, snaking until it lies still.
	const FVector End = WhipEnd(Alpha);
	const FVector Over(60.0, 0.0, 30.0);
	for (int32 Index = 0; Index <= SectionCount; ++Index)
	{
		const float T = static_cast<float>(Index) / SectionCount;
		FVector Point = Bezier(FVector::ZeroVector, Over, End + FVector(0.0, 0.0, 60.0), End, T);
		Point.Y += FMath::Sin(T * 9.f + Alpha * 24.f) * WhipSnake * (1.f - Alpha) * T;
		OutPoints.Add(Point);
	}
}

void AChapelWindowHose::PlaceSections()
{
	if (!Sections)
	{
		return;
	}
	TArray<FVector> Points;
	BuildLine(Points);
	TArray<FTransform> Transforms;
	Transforms.Reserve(Points.Num());
	for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
	{
		const FVector Span = Points[Index + 1] - Points[Index];
		const double Length = Span.Size();
		if (Length < 1.0)
		{
			continue;
		}
		// The engine's cylinder: 100 cm tall along its Z, its pivot in its middle; a little long so the joints close.
		Transforms.Add(FTransform(FRotationMatrix::MakeFromZ(Span / Length).ToQuat(), (Points[Index] + Points[Index + 1]) * 0.5,
			FVector(HoseGirth / 100.0, HoseGirth / 100.0, Length * 1.08 / 100.0)));
	}
	if (Sections->GetInstanceCount() != Transforms.Num())
	{
		Sections->ClearInstances();
		Sections->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
	}
	else
	{
		Sections->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ false, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
	if (Nozzle && Points.Num() >= 2)
	{
		const FVector Tip = Points.Last();
		const FVector Along = (Tip - Points[Points.Num() - 2]).GetSafeNormal();
		Nozzle->SetRelativeLocationAndRotation(Tip + Along * 12.0, FRotationMatrix::MakeFromZ(Along).Rotator());
	}
}

void AChapelWindowHose::PlacePulses()
{
	if (!Pulses)
	{
		return;
	}
	const bool bDrinking = bShown && !bOut && IsPlayerNear(PulseReach);
	const int32 Count = Pulses->GetInstanceCount();
	Pulses->SetVisibility(bDrinking);
	if (!bDrinking || Count <= 0)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
	const FVector End = EndLocal();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Out of her toward the window: each pulse from the Reliquary (1) to the sill (0), brightest on the way.
		const float U = FMath::Frac(PulseClock * PulseSpeed + static_cast<float>(Index) / Count);
		const FVector At = GetActorTransform().TransformPosition(InsideCurve(End, InsideFloorDrop, 1.f - U));
		const FVector Camera = Viewer && Viewer->PlayerCameraManager ? Viewer->PlayerCameraManager->GetCameraLocation()
			: At + FVector(500.0, 0.0, 0.0);
		FVector ToCamera = Camera - At;
		if (!ToCamera.Normalize())
		{
			ToCamera = FVector::UpVector;
		}
		const float Strength = FMath::Sin(U * static_cast<float>(UE_PI));
		Pulses->UpdateInstanceTransform(Index, FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), At,
			FVector(PulseSize / 100.0, PulseSize / 100.0, 1.0)), /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		const float Data[] = { PulseColor.R, PulseColor.G, PulseColor.B, PulseStrength * Strength, 1.f };
		Pulses->SetCustomData(Index, MakeArrayView(Data), /*bMarkRenderStateDirty*/ false);
	}
	Pulses->MarkRenderStateDirty();
}
