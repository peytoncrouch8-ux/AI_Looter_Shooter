#include "Player/UnderwaterView.h"
#include "Audio/LooterSound.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

// UUnderwaterViewComponent's bubbles: a breath let out every few seconds and a few off the hands at each stroke's push, rising
// and wobbling past the view, gone at the surface. Rings on the effects' shape master (M_FX_Shape: additive, a ring by its
// custom data), one pool of MaxBubbles quads facing the eye.

namespace
{
	TAutoConsoleVariable<float> CVarBreathEvery(TEXT("Looter.Underwater.BreathEvery"), 4.f,
		TEXT("Seconds between breaths let out under water (each a little either way)."));
	TAutoConsoleVariable<float> CVarRise(TEXT("Looter.Underwater.BubbleRise"), 55.f,
		TEXT("How fast a middling bubble rises (cm/s); small ones slower, big ones faster."));
	TAutoConsoleVariable<float> CVarBubbleGlow(TEXT("Looter.Underwater.BubbleGlow"), 0.55f,
		TEXT("The bubbles' ring strength (additive)."));

	/** M_FX_Shape's custom data (build_vfx_materials.py): colour 0-2, strength 3, shape 4 (0 a ring), thickness 5, seed 6. */
	constexpr int32 ShapeFloats = 7;
	constexpr float RingShape = 0.f;
	/** A bubble's ring is this share of its radius thick: a thin bright rim, the light caught on its skin. */
	constexpr float RingThickness = 0.3f;
	/** Pale cyan-white: the daylight from the surface caught on a bubble's skin (linear). */
	const FLinearColor BubbleColor(0.72f, 0.9f, 1.f);
	/** A breath: this many bubbles, this big (cm across), from this far ahead of and under the eye (cm). */
	constexpr int32 BreathCountMin = 5;
	constexpr int32 BreathCountMax = 9;
	constexpr float BreathSpread = 3.f;
	constexpr float MouthAhead = 14.f;
	constexpr float MouthBelow = 7.f;
	/** A stroke's push: a few off each hand (ahead, out to the side and under the eye, cm). */
	constexpr int32 StrokeBubbles = 2;
	constexpr float HandsAhead = 38.f;
	constexpr float HandsOut = 22.f;
	constexpr float HandsBelow = 18.f;
	/** Sizes (cm across), lives (s), and the wobble's reach (cm) and pace (radians a second). */
	constexpr float SizeMin = 0.8f;
	constexpr float SizeMax = 3.2f;
	constexpr float LifeMin = 1.6f;
	constexpr float LifeMax = 3.f;
	constexpr float WobbleReach = 1.8f;
	constexpr float WobblePace = 9.f;
	/** Fade in and out at either end of a bubble's life (s), and how near the surface it pops (cm). */
	constexpr float FadeSeconds = 0.18f;
	constexpr float PopUnder = 1.5f;
}

void UUnderwaterViewComponent::EmitBubbles(const FVector& At, int32 Count, float Spread)
{
	for (int32 Index = 0; Index < Count && Live.Num() < MaxBubbles; ++Index)
	{
		FBubble Bubble;
		Bubble.Location = At + Random.GetUnitVector() * (Spread * Random.FRand());
		Bubble.Size = FMath::Lerp(SizeMin, SizeMax, FMath::Square(Random.FRand()));
		// Bigger bubbles rise faster; each starts with a little push its own way.
		const float Rise = FMath::Max(CVarRise.GetValueOnGameThread(), 1.f) * FMath::Lerp(0.6f, 1.4f, (Bubble.Size - SizeMin) / (SizeMax - SizeMin));
		Bubble.Velocity = FVector(Random.FRandRange(-8.f, 8.f), Random.FRandRange(-8.f, 8.f), Rise);
		Bubble.Life = static_cast<float>(Random.FRandRange(LifeMin, LifeMax));
		Bubble.Seed = Random.FRand();
		Live.Add(Bubble);
	}
}

void UUnderwaterViewComponent::UpdateBubbles(float DeltaTime, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (!bUnder)
	{
		// Above water nothing under it shows (the surface is opaque): the pool empties at once.
		Live.Reset();
		DrawBubbles(ViewLocation);
		return;
	}
	const UPlayerLocomotionComponent* Moves = Locomotion.Get();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const bool bHeadUnder = Moves && Moves->IsHeadUnderwater();
	if (Pawn && bHeadUnder)
	{
		const FRotator Yaw(0.f, ViewRotation.Yaw, 0.f);
		const FVector Forward = Yaw.Vector();
		const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
		const FVector Head = Pawn->GetPawnViewLocation();
		// A breath let out every few seconds, heard bubbling up past the ears.
		NextBreath -= DeltaTime;
		if (NextBreath <= 0.f)
		{
			const float Every = FMath::Max(CVarBreathEvery.GetValueOnGameThread(), 0.5f);
			NextBreath = static_cast<float>(Random.FRandRange(Every * 0.75f, Every * 1.25f));
			EmitBubbles(Head + Forward * MouthAhead - FVector::UpVector * MouthBelow, Random.RandRange(BreathCountMin, BreathCountMax), BreathSpread);
			LooterSound::Play2D(this, SwimCue::Bubbles, static_cast<float>(Random.FRandRange(0.7f, 1.f)));
		}
		// A stroke's push throws a few off each hand.
		const int32 Strokes = Moves->GetSwimStrokeCount();
		if (Strokes != SeenStrokes)
		{
			SeenStrokes = Strokes;
			const FVector Hands = Head + Forward * HandsAhead - FVector::UpVector * HandsBelow;
			EmitBubbles(Hands + Right * HandsOut, StrokeBubbles, 4.f);
			EmitBubbles(Hands - Right * HandsOut, StrokeBubbles, 4.f);
		}
	}

	// Rising, wobbling, popping at the surface.
	for (int32 Index = Live.Num() - 1; Index >= 0; --Index)
	{
		FBubble& Bubble = Live[Index];
		Bubble.Age += DeltaTime;
		const float Phase = Bubble.Age * WobblePace + Bubble.Seed * 6.2832f;
		Bubble.Velocity.X = FMath::FInterpTo(static_cast<float>(Bubble.Velocity.X), 0.f, DeltaTime, 2.f);
		Bubble.Velocity.Y = FMath::FInterpTo(static_cast<float>(Bubble.Velocity.Y), 0.f, DeltaTime, 2.f);
		const FVector Wobble(FMath::Cos(Phase) * WobbleReach * WobblePace, FMath::Sin(Phase * 1.3f) * WobbleReach * WobblePace, 0.f);
		Bubble.Location += (Bubble.Velocity + Wobble * 0.15f) * DeltaTime;
		if (Bubble.Age >= Bubble.Life || Bubble.Location.Z >= SurfaceZ - PopUnder)
		{
			Live.RemoveAtSwap(Index);
		}
	}
	DrawBubbles(ViewLocation);
}

bool UUnderwaterViewComponent::MakeBubbles()
{
	bTriedBubbles = true;
	AActor* Owner = GetOwner();
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, BubbleMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!Owner || !Material || !Plane)
	{
		return false;
	}
	UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("UnderwaterBubbles"), RF_Transient);
	Instances->SetupAttachment(Owner->GetRootComponent());
	// Kept at the world's origin: the bubbles' places are the world's, whatever the body does.
	Instances->SetUsingAbsoluteLocation(true);
	Instances->SetUsingAbsoluteRotation(true);
	Instances->SetUsingAbsoluteScale(true);
	Instances->SetWorldTransform(FTransform::Identity);
	Instances->SetMobility(EComponentMobility::Movable);
	Instances->SetStaticMesh(Plane);
	Instances->SetMaterial(0, Material);
	Instances->SetNumCustomDataFloats(ShapeFloats);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetCastShadow(false);
	Instances->SetCanEverAffectNavigation(false);
	Instances->bReceivesDecals = false;
	Instances->bAffectDistanceFieldLighting = false;
	Instances->bAffectDynamicIndirectLighting = false;
	Instances->bVisibleInRayTracing = false;
	Instances->RegisterComponent();
	// One pool, made once: every quad is moved each frame it shows (an unused one shrunk to nothing), never re-made.
	TArray<FTransform> Hidden;
	Hidden.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), MaxBubbles);
	Instances->AddInstances(Hidden, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	Instances->SetVisibility(false);
	BubbleInstances = Instances;
	return true;
}

void UUnderwaterViewComponent::DrawBubbles(const FVector& ViewLocation)
{
	if (Live.IsEmpty())
	{
		if (BubbleInstances && BubbleInstances->IsVisible())
		{
			BubbleInstances->SetVisibility(false);
		}
		return;
	}
	if (!BubbleInstances && (bTriedBubbles || !MakeBubbles()))
	{
		return;
	}
	TArray<FTransform> Transforms;
	TArray<float> Data;
	Transforms.Reserve(MaxBubbles);
	Data.Reserve(MaxBubbles * ShapeFloats);
	const float Glow = FMath::Max(CVarBubbleGlow.GetValueOnGameThread(), 0.f);
	for (const FBubble& Bubble : Live)
	{
		FVector Facing = ViewLocation - Bubble.Location;
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		// The engine's plane is 100 cm across.
		Transforms.Add(FTransform(FRotationMatrix::MakeFromZ(Facing).ToQuat(), Bubble.Location, FVector(Bubble.Size / 100.f, Bubble.Size / 100.f, 1.f)));
		const float Fade = FMath::Clamp(Bubble.Age / FadeSeconds, 0.f, 1.f) * FMath::Clamp((Bubble.Life - Bubble.Age) / FadeSeconds, 0.f, 1.f);
		Data.Append({ BubbleColor.R, BubbleColor.G, BubbleColor.B, Glow * Fade, RingShape, RingThickness, Bubble.Seed });
	}
	while (Transforms.Num() < MaxBubbles)
	{
		Transforms.Add(FTransform(FQuat::Identity, ViewLocation, FVector::ZeroVector));
		Data.Append({ 0.f, 0.f, 0.f, 0.f, RingShape, RingThickness, 0.f });
	}
	BubbleInstances->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	BubbleInstances->SetCustomData(0, MaxBubbles - 1, Data, false);
	if (!BubbleInstances->IsVisible())
	{
		BubbleInstances->SetVisibility(true);
	}
}
