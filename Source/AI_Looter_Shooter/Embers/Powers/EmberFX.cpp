#include "Embers/Powers/EmberFX.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/CreatureTierLookComponent.h"
#include "Player/SlideDust.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

// EmberFX: the new powers' shared looks on FWeaponFX (sparks, ground rings, cards) and the body tint on the hit flash's overlay.

namespace
{
	/** M_FX_HitFlash's parameters (the creatures' hit flash, UCreatureReactComponent). */
	const FName TintColorParam(TEXT("Color"));
	const FName TintIntensityParam(TEXT("Intensity"));

	UMaterialInterface* TintBase()
	{
		static TWeakObjectPtr<UMaterialInterface> Found;
		const FString Path(UCreatureReactComponent::FlashMaterialPath);
		if (!Found.IsValid() && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			Found = LoadObject<UMaterialInterface>(nullptr, *Path);
		}
		return Found.Get();
	}
}

namespace EmberFX
{
	FLinearColor DustColor(const UObject* WorldContext)
	{
		UCreatureReactSubsystem* Reacts = WorldContext ? UCreatureReactSubsystem::Get(WorldContext) : nullptr;
		return FSlideDust::DustAlbedo(ESlideGround::Dirt) * (Reacts ? Reacts->GetDustLight() : FLinearColor(0.8f, 0.8f, 0.8f));
	}

	TArray<FVector> GroundRing(const UWorld& World, const FVector& Center, float Radius, int32 Segments, float Reach, float Lift)
	{
		TArray<FVector> Points;
		const int32 Count = FMath::Max(Segments, 3);
		Points.Reserve(Count);
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("EmberRing"));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = 2.f * UE_PI * Index / Count;
			FVector Point = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
			FHitResult Ground;
			if (World.LineTraceSingleByObjectType(Ground, Point + FVector(0.0, 0.0, Reach), Point - FVector(0.0, 0.0, Reach),
					FCollisionObjectQueryParams(ECC_WorldStatic), Params)
				&& !Ground.bStartPenetrating)
			{
				Point.Z = Ground.ImpactPoint.Z;
			}
			Points.Add(Point + FVector(0.0, 0.0, Lift));
		}
		return Points;
	}

	void DrawLine(FWeaponFX& Effects, const TArray<FVector>& Points, bool bClosed, float Width, const FLinearColor& Color, float Glow)
	{
		if (Points.Num() < 2 || Glow <= 0.f)
		{
			return;
		}
		const int32 Last = bClosed ? Points.Num() : Points.Num() - 1;
		for (int32 Index = 0; Index < Last; ++Index)
		{
			Effects.AddStreak(Points[Index], Points[(Index + 1) % Points.Num()], Width, Color, Glow);
		}
	}

	void DrawRing(FWeaponFX& Effects, const FVector& Center, const FVector& Axis, float Radius, int32 Segments, float Width,
		const FLinearColor& Color, float Glow)
	{
		const FVector Normal = Axis.GetSafeNormal();
		if (Normal.IsNearlyZero() || Radius <= 0.f || Glow <= 0.f)
		{
			return;
		}
		FVector U, V;
		Normal.FindBestAxisVectors(U, V);
		const int32 Count = FMath::Max(Segments, 3);
		// Each piece reaches a third of a piece past its ends: a streak's ends are soft, and end to end they read as a
		// dashed ring (2026-10-20, Rail Spike's shock).
		const float Step = UE_TWO_PI / static_cast<float>(Count);
		auto At = [&](float Angle) { return Center + (U * FMath::Cos(Angle) + V * FMath::Sin(Angle)) * Radius; };
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float From = Step * (static_cast<float>(Index) - 0.33f);
			Effects.AddStreak(At(From), At(From + Step * 1.66f), Width, Color, Glow);
		}
	}

	void DrawCard(FWeaponFX& Effects, const FVector& Center, const FVector& Along, float Spin, const FLinearColor& Edge, float Glow, float Size)
	{
		// A card is 9 cm by 6: seen as a short, wide streak along its tumble, its long side turned by the spin (a card
		// end-on is a sliver, face-on a whole card), with a hairline of its colour round the face.
		const FVector Way = Along.GetSafeNormal();
		if (Way.IsNearlyZero())
		{
			return;
		}
		const float Face = FMath::Abs(FMath::Cos(Spin));
		const float Length = (4.5f + 4.5f * Face) * Size;
		const float Width = (1.2f + 4.8f * FMath::Abs(FMath::Sin(Spin + 0.6f))) * Size;
		const FLinearColor Paper(1.f, 0.96f, 0.9f);
		Effects.AddStreak(Center - Way * (Length * 0.5f), Center + Way * (Length * 0.5f), Width, Paper, Glow * 0.35f);
		Effects.AddStreak(Center - Way * (Length * 0.55f), Center + Way * (Length * 0.55f), Width * 1.25f, Edge, Glow * 0.25f);
	}

	// ---------------------------------------------------------------------------
	// Sparks
	// ---------------------------------------------------------------------------

	void FEmberSparks::Spawn(const FVector& Location, const FVector& Velocity, float Life, float Width, const FLinearColor& Color, float Glow,
		float Gravity, float Drag)
	{
		if (Sparks.Num() >= MaxSparks)
		{
			Sparks.RemoveAt(0);
		}
		FSpark& Spark = Sparks.AddDefaulted_GetRef();
		Spark.Location = Location;
		Spark.Velocity = Velocity;
		Spark.Life = FMath::Max(Life, 0.05f);
		Spark.Width = Width;
		Spark.Color = Color;
		Spark.Glow = Glow;
		Spark.Gravity = Gravity;
		Spark.Drag = Drag;
	}

	void FEmberSparks::Tick(float DeltaSeconds, FWeaponFX* Effects)
	{
		const float Dt = FMath::Max(DeltaSeconds, 0.f);
		for (int32 Index = Sparks.Num() - 1; Index >= 0; --Index)
		{
			FSpark& Spark = Sparks[Index];
			Spark.Age += Dt;
			if (Spark.Age >= Spark.Life)
			{
				Sparks.RemoveAtSwap(Index);
				continue;
			}
			Spark.Velocity *= FMath::Max(1.f - Spark.Drag * Dt, 0.f);
			Spark.Velocity.Z -= Spark.Gravity * Dt;
			Spark.Location += Spark.Velocity * Dt;
		}
		if (!Effects)
		{
			return;
		}
		for (const FSpark& Spark : Sparks)
		{
			// A streak along its motion, as long as it goes in a thirtieth of a second (at least its own width), cooling as it dies.
			const float Left = 1.f - Spark.Age / Spark.Life;
			const FVector Tail = Spark.Location - Spark.Velocity * (1.f / 30.f);
			const FVector Head = (Spark.Location - Tail).SizeSquared() < FMath::Square(Spark.Width) ? Spark.Location + FVector(0.0, 0.0, Spark.Width) : Spark.Location;
			Effects->AddStreak(Tail, Head, Spark.Width * (0.6f + 0.4f * Left), Spark.Color, Spark.Glow * Left * Left);
		}
	}

	// ---------------------------------------------------------------------------
	// The body tint
	// ---------------------------------------------------------------------------

	UMaterialInstanceDynamic* FEmberTint::MakeMaterial(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = TintBase();
		if (!Base)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Base, Outer);
		if (Look)
		{
			Look->SetVectorParameterValue(TintColorParam, Color);
			Look->SetScalarParameterValue(TintIntensityParam, 0.5f);
		}
		return Look;
	}

	void FEmberTint::SetMaterial(UMaterialInstanceDynamic* InMaterial)
	{
		Material = InMaterial;
	}

	void FEmberTint::SetIntensity(float Intensity)
	{
		if (UMaterialInstanceDynamic* Look = Material.Get())
		{
			Look->SetScalarParameterValue(TintIntensityParam, FMath::Max(Intensity, 0.f));
		}
	}

	void FEmberTint::Wear(AActor& Actor)
	{
		UMaterialInstanceDynamic* Look = Material.Get();
		if (!Look)
		{
			return;
		}
		bool bWorn = false;
		TInlineComponentArray<USkinnedMeshComponent*> Meshes(&Actor);
		for (USkinnedMeshComponent* Mesh : Meshes)
		{
			// Never a creature's tier shell (its body drawn again in its tier's glow): the body's tint shows through it.
			if (!Mesh || !Mesh->IsVisible() || UCreatureTierLookComponent::IsTierShell(Mesh))
			{
				continue;
			}
			const UMaterialInterface* Worn = Mesh->GetOverlayMaterial();
			if (!Worn)
			{
				// Never over another overlay (the hit flash, an objective's highlight): it waits for that one to go.
				Mesh->SetOverlayMaterial(Look);
				bWorn = true;
			}
			else if (Worn == Look)
			{
				bWorn = true;
			}
		}
		if (bWorn && !WornOn.Contains(&Actor))
		{
			WornOn.Add(&Actor);
		}
	}

	void FEmberTint::Remove(AActor& Actor)
	{
		const UMaterialInstanceDynamic* Look = Material.Get();
		TInlineComponentArray<USkinnedMeshComponent*> Meshes(&Actor);
		for (USkinnedMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Look && Mesh->GetOverlayMaterial() == Look)
			{
				Mesh->SetOverlayMaterial(nullptr);
			}
		}
		WornOn.Remove(&Actor);
	}

	void FEmberTint::RemoveAll()
	{
		const TArray<TWeakObjectPtr<AActor>> Worn = WornOn;
		for (const TWeakObjectPtr<AActor>& Each : Worn)
		{
			if (AActor* Actor = Each.Get())
			{
				Remove(*Actor);
			}
		}
		WornOn.Reset();
	}

	bool FEmberTint::IsWornOn(const AActor& Actor) const
	{
		return WornOn.Contains(&Actor);
	}
}
