#include "Player/SlideDust.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include <initializer_list>

namespace
{
	/** Puffs and grains a second at full speed on dirt; fewer as the slide slows and on other ground. */
	constexpr float PuffsPerSecond = 38.f;
	constexpr float GritPerSecond = 34.f;
	/** On top, as the body drops into the slide. */
	constexpr float BurstPuffs = 7.f;
	constexpr float BurstGrit = 12.f;
	/** The share of puffs pushed out ahead of the heel rather than rising off it. */
	constexpr float AheadShare = 0.4f;
	/** How far ahead of the hips the leading heel is in the slide pose (full-size cm; the character's scale shrinks it). */
	constexpr float HeelReach = 50.f;
	constexpr float BaseOpacity = 0.4f;

	/**
	 * How bright the dust shows against its own color: about twice it in full sun (a sunlit surface of albedo A shows
	 * about 2.5 A in the game's light, and dust is thinner), about 0.7 in shade under the sky.
	 */
	constexpr float SunLevel = 2.f;
	constexpr float ShadeLevel = 0.7f;
	const FLinearColor SkyColor(0.86f, 0.93f, 1.05f);
	constexpr float SunCheckSeconds = 0.15f;
	constexpr float SunCheckDistance = 6000.f;

	/** A light's color with its brightest channel at 1: the hue (its strength is the exposure's business). */
	FLinearColor Hue(const FLinearColor& Color)
	{
		const float Brightest = FMath::Max3(Color.R, Color.G, Color.B);
		return Brightest > 0.01f ? Color / Brightest : FLinearColor::White;
	}

	bool Says(const FString& Name, std::initializer_list<const TCHAR*> Words)
	{
		for (const TCHAR* Word : Words)
		{
			if (Name.Contains(Word, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	/** How thick each ground's dust is (wood's is a thin haze). */
	float DustOpacity(ESlideGround Ground)
	{
		switch (Ground)
		{
		case ESlideGround::Wood: return 0.55f;
		case ESlideGround::Grass: return 0.75f;
		case ESlideGround::Stone: return 0.8f;
		case ESlideGround::Dirt: return 1.f;
		default: return 0.f;
		}
	}
}

// ---------------------------------------------------------------------------
// The ground
// ---------------------------------------------------------------------------

ESlideGround FSlideDust::GroundFromName(const FString& Name)
{
	// Water first: a waterfall's or a creek's name says water before anything else it says.
	if (Says(Name, { TEXT("Water") }))
	{
		return ESlideGround::Water;
	}
	if (Says(Name, { TEXT("Wood"), TEXT("Plank"), TEXT("Walnut"), TEXT("Timber"), TEXT("Deck"), TEXT("Board") }))
	{
		return ESlideGround::Wood;
	}
	if (Says(Name, { TEXT("Grass"), TEXT("Meadow"), TEXT("Hay"), TEXT("Leaves"), TEXT("Needles"), TEXT("Reed"), TEXT("Moss"), TEXT("Foliage") }))
	{
		return ESlideGround::Grass;
	}
	if (Says(Name, { TEXT("Rock"), TEXT("Stone"), TEXT("Granite"), TEXT("Cliff"), TEXT("Ballast"), TEXT("Gravel"), TEXT("Brick"),
		TEXT("Cobble"), TEXT("Concrete"), TEXT("Metal"), TEXT("Iron"), TEXT("Steel") }))
	{
		return ESlideGround::Stone;
	}
	if (Says(Name, { TEXT("Dirt"), TEXT("Soil"), TEXT("Sand"), TEXT("Mud"), TEXT("Terrain"), TEXT("Macro"), TEXT("Ground"), TEXT("Road") }))
	{
		return ESlideGround::Dirt;
	}
	return ESlideGround::None;
}

ESlideGround FSlideDust::GroundOf(const FHitResult& Hit)
{
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component)
	{
		return ESlideGround::Dirt;
	}
	// The material at the spot (a complex trace says which face it hit), else the mesh's first.
	int32 Section = 0;
	const UMaterialInterface* Material = Hit.FaceIndex != INDEX_NONE ? Component->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex, Section) : nullptr;
	if (!Material)
	{
		Material = Component->GetMaterial(0);
	}
	// An instance's own name is the most telling (MI_WoodPlanks, made from M_World), then what it's made from.
	for (int32 Depth = 0; Material && Depth < 8; ++Depth)
	{
		const ESlideGround Said = GroundFromName(Material->GetName());
		if (Said != ESlideGround::None)
		{
			return Said;
		}
		const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
		Material = Instance ? Instance->Parent.Get() : nullptr;
	}
	// Then the mesh (an area's SM_<Area>_Water piece).
	const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
	const ESlideGround ByMesh = Mesh && Mesh->GetStaticMesh() ? GroundFromName(Mesh->GetStaticMesh()->GetName()) : ESlideGround::None;
	return ByMesh != ESlideGround::None ? ByMesh : ESlideGround::Dirt;
}

float FSlideDust::DustAmount(ESlideGround Ground)
{
	switch (Ground)
	{
	case ESlideGround::Wood: return 0.35f;
	case ESlideGround::Grass: return 0.55f;
	case ESlideGround::Stone: return 0.6f;
	case ESlideGround::Dirt: return 1.f;
	default: return 0.f;
	}
}

float FSlideDust::GritAmount(ESlideGround Ground)
{
	switch (Ground)
	{
	case ESlideGround::Grass: return 0.25f;
	case ESlideGround::Stone: return 1.f;
	case ESlideGround::Dirt: return 0.7f;
	default: return 0.f;
	}
}

FLinearColor FSlideDust::DustAlbedo(ESlideGround Ground)
{
	switch (Ground)
	{
	case ESlideGround::Wood: return FLinearColor(0.42f, 0.37f, 0.30f);
	case ESlideGround::Grass: return FLinearColor(0.38f, 0.35f, 0.25f);
	case ESlideGround::Stone: return FLinearColor(0.46f, 0.44f, 0.40f);
	default: return FLinearColor(0.40f, 0.33f, 0.25f);
	}
}

// ---------------------------------------------------------------------------
// Throwing it
// ---------------------------------------------------------------------------

void FSlideDust::Update(const ACharacter& Character, bool bSliding, bool bStarted, const FVector& Direction, float Speed, float TopSpeed,
	float DeltaTime)
{
	if (bStarted)
	{
		PuffsThrown = 0;
		GritThrown = 0;
		PuffsOwed = 0.f;
		GritOwed = 0.f;
		bLightFound = false;
		SunShare = -1.f;
		SunCheckIn = 0.f;
	}
	UWorld* World = Character.GetWorld();
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const FVector Along = FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	if (!bSliding || !World || !Capsule || Along.IsNearlyZero())
	{
		PuffsOwed = 0.f;
		GritOwed = 0.f;
		return;
	}
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!Bullets)
	{
		return;
	}

	// The ground under the leading heel, and what it is.
	const float Scale = FMath::Max(static_cast<float>(Character.GetActorScale3D().Z), 0.1f);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Along);
	const FVector Heel = Character.GetActorLocation() + Along * (HeelReach * Scale) - FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SlideDustGround), /*bTraceComplex*/ true, &Character);
	Params.bReturnFaceIndex = true;
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Heel + FVector(0.0, 0.0, 40.0), Heel - FVector(0.0, 0.0, 60.0), ECC_Visibility, Params))
	{
		Ground = ESlideGround::None;
		return;
	}
	Ground = GroundOf(Hit);
	const float DustShare = DustAmount(Ground);
	const float GritShare = GritAmount(Ground);
	if (DustShare <= 0.f && GritShare <= 0.f)
	{
		return;
	}
	const FVector Spot = Hit.ImpactPoint;
	const FLinearColor Light = LightAt(*World, Spot + FVector(0.0, 0.0, 40.0), Character, DeltaTime);

	// It thins faster than the slide slows, so its last moments are a wisp.
	const float Pace = FMath::Clamp(Speed / FMath::Max(TopSpeed, 1.f), 0.f, 1.f);
	const float Strength = Pace * FMath::Sqrt(Pace);
	PuffsOwed += PuffsPerSecond * DustShare * Strength * DeltaTime + (bStarted ? BurstPuffs * DustShare : 0.f);
	GritOwed += GritPerSecond * GritShare * Strength * DeltaTime + (bStarted ? BurstGrit * GritShare : 0.f);
	const int32 Puffs = FMath::FloorToInt32(PuffsOwed);
	const int32 Grains = FMath::FloorToInt32(GritOwed);
	PuffsOwed -= Puffs;
	GritOwed -= Grains;
	if (Puffs <= 0 && Grains <= 0)
	{
		return;
	}

	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FLinearColor Color = DustAlbedo(Ground) * Light;
	const float Opacity = BaseOpacity * DustOpacity(Ground) * FMath::Lerp(0.5f, 1.f, Strength);
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		ThrowPuff(Effects, Spot, Along, Side, Speed, Scale, Color, Opacity, Random.FRand() < AheadShare);
	}
	for (int32 Index = 0; Index < Grains; ++Index)
	{
		ThrowGrit(Effects, Spot, Along, Side, Speed, Scale);
	}
	PuffsThrown += Puffs;
	GritThrown += Grains;
}

void FSlideDust::ThrowPuff(FWeaponFX& Effects, const FVector& Spot, const FVector& Along, const FVector& Side, float Speed, float Scale,
	const FLinearColor& Color, float Opacity, bool bAhead)
{
	// Most rise off the heel and are left behind as the slide carries on past them; some are pushed out ahead of it, so
	// the first-person view sees them churn at the bottom of the screen before it passes over them.
	const float Forward = bAhead ? Random.FRandRange(45.f, 90.f) : Random.FRandRange(-25.f, 10.f);
	const FVector Where = Spot + (Along * Forward + Side * Random.FRandRange(-22.f, 22.f) + FVector(0.0, 0.0, Random.FRandRange(5.f, 12.f))) * Scale;
	const float Carried = bAhead ? Random.FRandRange(0.7f, 1.f) : Random.FRandRange(0.45f, 0.85f);
	const FVector Velocity = Along * (Speed * Carried) + Side * Random.FRandRange(-130.f, 130.f) + FVector(0.0, 0.0, Random.FRandRange(45.f, 130.f));
	const float Size = Random.FRandRange(13.f, 21.f) * Scale;
	Effects.SpawnDustPuff(Where, Velocity, Color, Opacity * Random.FRandRange(0.7f, 1.1f), Size, Size * Random.FRandRange(3.2f, 4.5f),
		Random.FRandRange(0.7f, 1.25f));
}

void FSlideDust::ThrowGrit(FWeaponFX& Effects, const FVector& Spot, const FVector& Along, const FVector& Side, float Speed, float Scale)
{
	// Flicked forward off the heel faster than the slide, so the grains arc out ahead and fall: in view from first person
	// too, where the heel itself is under the bottom of the screen.
	const FVector Where = Spot + (Along * Random.FRandRange(0.f, 20.f) + Side * Random.FRandRange(-14.f, 14.f)) * Scale + FVector(0.0, 0.0, 3.0);
	const FVector Velocity = Along * (Speed * Random.FRandRange(1.2f, 1.6f)) + Side * Random.FRandRange(-110.f, 110.f)
		+ FVector(0.0, 0.0, Random.FRandRange(140.f, 340.f));
	Effects.SpawnGrit(Where, Velocity, Random.FRandRange(0.7f, 1.8f), Random.FRandRange(0.45f, 0.8f));
}

// ---------------------------------------------------------------------------
// The light it's in
// ---------------------------------------------------------------------------

void FSlideDust::FindLight(const UWorld& World)
{
	bLightFound = true;
	if (!Sun.IsValid())
	{
		// The sun is the light the sky atmosphere sets by; any directional light will do in a level without one.
		const UDirectionalLightComponent* Fallback = nullptr;
		for (TActorIterator<ADirectionalLight> It(&World); It; ++It)
		{
			// GetLightComponent, not GetComponent: that one is editor-only (a packaged game can't compile it).
			const UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
			if (!Light || !Light->IsVisible())
			{
				continue;
			}
			if (Light->bAtmosphereSunLight)
			{
				Sun = Light;
				break;
			}
			Fallback = Fallback ? Fallback : Light;
		}
		if (!Sun.IsValid())
		{
			Sun = Fallback;
		}
	}

	// What the lighting state does to things that can't see the light themselves: white by day, darker and warmer at dusk.
	StateTint = FLinearColor::White;
	const UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, ALightingStates::DefaultCollectionPath);
	const UMaterialParameterCollectionInstance* Instance = Collection ? World.GetParameterCollectionInstance(Collection) : nullptr;
	FLinearColor Tint;
	if (Instance && Instance->GetVectorParameterValue(ULightingStateSubsystem::BackdropTintParameter, Tint))
	{
		StateTint = Tint;
	}
}

FLinearColor FSlideDust::LightAt(const UWorld& World, const FVector& Where, const AActor& Ignore, float DeltaTime)
{
	if (!bLightFound)
	{
		FindLight(World);
	}
	const UDirectionalLightComponent* Light = Sun.Get();
	const FVector ToSun = Light ? -Light->GetDirection() : FVector::UpVector;

	// In the sun or not: a trace toward it now and then, eased, so crossing a shadow's edge fades rather than flicks.
	SunCheckIn -= DeltaTime;
	if (SunCheckIn <= 0.f)
	{
		SunCheckIn = SunCheckSeconds;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SlideDustSun), false, &Ignore);
		const bool bLit = Light && Light->Intensity > 0.f && ToSun.Z > 0.0
			&& !World.LineTraceTestByChannel(Where, Where + ToSun * SunCheckDistance, ECC_Visibility, Params);
		SunTarget = bLit ? 1.f : 0.f;
		if (SunShare < 0.f)
		{
			SunShare = SunTarget;
		}
	}
	SunShare = FMath::FInterpTo(SunShare, SunTarget, DeltaTime, 8.f);

	// A sun low over the horizon lights it less.
	const float Height = FMath::Clamp(static_cast<float>(ToSun.Z) * 4.f, 0.f, 1.f);
	const FLinearColor SunColor = Light ? Hue(Light->GetLightColor()) : FLinearColor::White;
	return (SkyColor * ShadeLevel + SunColor * ((SunLevel - ShadeLevel) * SunShare * Height)) * StateTint;
}
