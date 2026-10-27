#include "Embers/Powers/EmberFX.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

// EmberFX's real cards (the user's note 6, 2026-10-27: Odessa's Fifty-Two Pickup "shoots tiny particles, needs adjusted
// with better animations and bigger card like projectiles"): each card two planes back to back with a pip, on three
// instanced components of a deck's own, turned to flutter and spin as they fly.

namespace
{
	TAutoConsoleVariable<float> CVarCardSize(TEXT("Looter.Embers.Cards.Size"), 2.2f,
		TEXT("How much bigger than a real card (8.9 x 6.4 cm) a thrown card is drawn, so a fan of them reads at range."));
	TAutoConsoleVariable<float> CVarCardTilt(TEXT("Looter.Embers.Cards.Tilt"), 50.f,
		TEXT("Degrees a flying card's face is tipped back toward the thrower (0: flat, seen edge on from behind)."));
	TAutoConsoleVariable<float> CVarCardFlutter(TEXT("Looter.Embers.Cards.Flutter"), 22.f,
		TEXT("Degrees a flying card's tilt wobbles either way as it flutters."));
	TAutoConsoleVariable<float> CVarCardGlow(TEXT("Looter.Embers.Cards.Glow"), 0.6f,
		TEXT("How much the cards glow on top of the world's light (the back in its colour, the face ivory), so they read in shade."));

	const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* const SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	/** The face: old ivory pasteboard, as Odessa's idle card (BodyFigureIdleProps.cpp); the pip her red ink. */
	const FLinearColor Paper(0.78f, 0.72f, 0.6f);
	const FLinearColor Ink(0.55f, 0.04f, 0.07f);
	/** The pip's size against the card's short side, and the gap between the faces (a share of the card's size, cm). */
	constexpr float PipShare = 0.3f;
	constexpr float FaceGap = 0.05f;
	/** Instances are kept in blocks of this many (as FWeaponFX's): a card born or gone rarely rebuilds the components. */
	constexpr int32 Block = 32;

	UInstancedStaticMeshComponent* MakeSet(AActor& Owner, const TCHAR* Name, UStaticMesh* Plane, UMaterialInterface* Surface,
		const FLinearColor& Colour, float Glow)
	{
		UInstancedStaticMeshComponent* Set = NewObject<UInstancedStaticMeshComponent>(&Owner, Name, RF_Transient);
		Set->SetupAttachment(Owner.GetRootComponent());
		Set->SetMobility(EComponentMobility::Movable);
		Set->SetStaticMesh(Plane);
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		Set->SetCanEverAffectNavigation(false);
		Set->bReceivesDecals = false;
		Set->bAffectDistanceFieldLighting = false;
		Set->bAffectDynamicIndirectLighting = false;
		Set->bVisibleInRayTracing = false;
		if (Surface)
		{
			UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Surface, Set);
			Look->SetVectorParameterValue(TEXT("Color"), Colour);
			Look->SetScalarParameterValue(TEXT("Glow"), Glow);
			Set->SetMaterial(0, Look);
		}
		Set->RegisterComponent();
		return Set;
	}

	void DrawSet(UInstancedStaticMeshComponent* Set, const TArray<FTransform>& Transforms)
	{
		if (!Set)
		{
			return;
		}
		const int32 Wanted = Transforms.Num();
		const int32 Current = Set->GetInstanceCount();
		int32 Count = Current;
		if (Wanted == 0)
		{
			Count = 0;
		}
		else if (Wanted > Current || Wanted < Current - 2 * Block)
		{
			Count = FMath::DivideAndRoundUp(Wanted, Block) * Block;
		}
		TArray<FTransform> Padded = Transforms;
		const FTransform Hidden(FQuat::Identity, Wanted > 0 ? Transforms[0].GetLocation() : FVector::ZeroVector, FVector::ZeroVector);
		while (Padded.Num() < Count)
		{
			Padded.Add(Hidden);
		}
		if (Count != Current)
		{
			Set->ClearInstances();
			if (Count > 0)
			{
				Set->AddInstances(Padded, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
			Set->MarkRenderStateDirty();
		}
		else if (Count > 0)
		{
			Set->BatchUpdateInstancesTransforms(0, Padded, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
	}
}

namespace EmberFX
{
	float CardSize()
	{
		return FMath::Clamp(CVarCardSize.GetValueOnAnyThread(), 0.5f, 6.f);
	}

	FQuat FlyingCardTurn(const FVector& Velocity, float Spin, float Flutter, float Roll)
	{
		const FVector Way = Velocity.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		// The flight's own up (square to its way), rolled about the way: a fan's cards lie at a slant of their own.
		FVector Up = FVector::UpVector - Way * FVector::DotProduct(Way, FVector::UpVector);
		Up = Up.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		Up = FQuat(Way, Roll).RotateVector(Up);
		// The face tipped back toward the thrower, wobbling as it flutters.
		const float Tilt = FMath::DegreesToRadians(CVarCardTilt.GetValueOnAnyThread() + CVarCardFlutter.GetValueOnAnyThread() * FMath::Sin(Flutter));
		const FVector Face = (Up * FMath::Cos(Tilt) - Way * FMath::Sin(Tilt)).GetSafeNormal();
		// Spun about its face: the long side turning round in the card's own plane.
		FVector Along = Way - Face * FVector::DotProduct(Face, Way);
		Along = Along.GetSafeNormal(UE_SMALL_NUMBER, FVector::CrossProduct(Face, FVector::RightVector).GetSafeNormal());
		Along = FQuat(Face, Spin).RotateVector(Along);
		return FRotationMatrix::MakeFromZX(Face, Along).ToQuat();
	}

	void FCardDeck::Add(const FVector& Center, const FQuat& Turn, float Size)
	{
		if (Size > KINDA_SMALL_NUMBER)
		{
			Pending.Add({ Center, Turn, Size });
		}
	}

	bool FCardDeck::Make(UWorld& World, const FLinearColor& Back)
	{
		if (Owner.IsValid() && Owner->GetWorld() == &World && Faces.IsValid() && Backs.IsValid() && Pips.IsValid())
		{
			return true;
		}
		UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, PlanePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Plane)
		{
			return false;
		}
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (!Actor)
		{
			return false;
		}
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("EmberCards"));
#endif
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
		Root->SetMobility(EComponentMobility::Movable);
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, SurfacePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		const float Glow = FMath::Max(CVarCardGlow.GetValueOnGameThread(), 0.f);
		Owner = Actor;
		Faces = MakeSet(*Actor, TEXT("CardFaces"), Plane, Surface, Paper, Glow * 0.5f);
		Backs = MakeSet(*Actor, TEXT("CardBacks"), Plane, Surface, Back, Glow);
		Pips = MakeSet(*Actor, TEXT("CardPips"), Plane, Surface, Ink, Glow * 0.5f);
		return true;
	}

	void FCardDeck::Flush(UWorld* World, const FLinearColor& Back)
	{
		Drawn = Pending.Num();
		if (!World || (Drawn == 0 && !Owner.IsValid()) || !Make(*World, Back))
		{
			Pending.Reset();
			return;
		}
		TArray<FTransform> FaceTransforms;
		TArray<FTransform> BackTransforms;
		TArray<FTransform> PipTransforms;
		FaceTransforms.Reserve(Drawn);
		BackTransforms.Reserve(Drawn);
		PipTransforms.Reserve(Drawn);
		// The engine's plane is 100 cm square facing +Z and one-sided: the face up, the back turned over beneath it.
		const FQuat Over(FVector::ForwardVector, UE_PI);
		const FQuat Diamond(FVector::UpVector, UE_PI * 0.25f);
		for (const FCardDraw& Card : Pending)
		{
			const FVector Normal = Card.Turn.GetAxisZ();
			const FVector Scale(CardLong * Card.Size * 0.01f, CardShort * Card.Size * 0.01f, 1.f);
			const float Gap = FaceGap * Card.Size;
			FaceTransforms.Add(FTransform(Card.Turn, Card.Center + Normal * Gap, Scale));
			BackTransforms.Add(FTransform(Card.Turn * Over, Card.Center - Normal * Gap, Scale));
			const float Pip = CardShort * PipShare * Card.Size * 0.01f;
			PipTransforms.Add(FTransform(Card.Turn * Diamond, Card.Center + Normal * (Gap * 2.f), FVector(Pip, Pip, 1.f)));
		}
		DrawSet(Faces.Get(), FaceTransforms);
		DrawSet(Backs.Get(), BackTransforms);
		DrawSet(Pips.Get(), PipTransforms);
		Pending.Reset();
	}
}
