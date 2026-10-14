#include "Progression/LevelUpBurstSubsystem.h"
#include "Combat/BulletSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The ring starts this far out (cm), not from a point, and sits this far over the ground. */
	constexpr float BurstRingStart = 30.f;
	constexpr double BurstRingOverGround = 4.0;
	/** The ring's bright line narrows as it runs out; its soft halo stays wide and faint. Glow strengths: a tracer's is 30. */
	constexpr float BurstCoreWidthStart = 5.f;
	constexpr float BurstCoreWidthEnd = 2.5f;
	constexpr float BurstHaloWidth = 18.f;
	constexpr float BurstRingGlow = 7.f;
	constexpr float BurstHaloShare = 0.16f;
	/** Each piece runs a little past its neighbours, so the ring has no gaps at the joins. */
	constexpr float BurstSegmentOverlap = 0.08f;
	/** The ring comes up over this long, then fades as it runs out. */
	constexpr float BurstFadeIn = 0.05f;

	/** Embers: their glow, their length (cm), how fast they slow, how far they sway; dimmed this near the eye. */
	constexpr float BurstEmberGlow = 9.f;
	constexpr float BurstEmberLength = 5.f;
	constexpr float BurstEmberDrag = 0.6f;
	constexpr float BurstEmberSway = 18.f;
	constexpr float BurstEmberFadeIn = 0.1f;
	constexpr float BurstNearEye = 30.f;
	constexpr float BurstNearEyeFade = 70.f;

	/** The ground is looked for this far over and under the feet (cm), and taken within these of them. */
	constexpr double BurstGroundUp = 150.0;
	constexpr double BurstGroundDown = 250.0;
	constexpr float BurstMostRise = 120.f;
	constexpr float BurstMostDrop = 120.f;

	float BurstEaseOut(float Alpha)
	{
		const float Left = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}

	FVector2D SpokeWay(int32 Spoke)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Spoke) / static_cast<float>(ULevelUpBurstSubsystem::Segments);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}

	float RingRadiusAt(float Time)
	{
		return FMath::Lerp(BurstRingStart, ULevelUpBurstSubsystem::RingRadius, BurstEaseOut(Time / ULevelUpBurstSubsystem::RingSeconds));
	}
}

ULevelUpBurstSubsystem* ULevelUpBurstSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<ULevelUpBurstSubsystem>() : nullptr;
}

bool ULevelUpBurstSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULevelUpBurstSubsystem::Deinitialize()
{
	Time = -1.f;
	EmberList.Reset();
	Super::Deinitialize();
}

void ULevelUpBurstSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool ULevelUpBurstSubsystem::IsTickable() const
{
	return IsPlaying();
}

TStatId ULevelUpBurstSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULevelUpBurstSubsystem, STATGROUP_Tickables);
}

float ULevelUpBurstSubsystem::GetRingRadius() const
{
	return Time >= 0.f && Time < RingSeconds ? RingRadiusAt(Time) : 0.f;
}

// ---------------------------------------------------------------------------
// Starting one
// ---------------------------------------------------------------------------

bool ULevelUpBurstSubsystem::Play(const APawn* Pawn, int32 NewLevel)
{
	const UWorld* World = GetWorld();
	if (!IsValid(Pawn) || !World)
	{
		return false;
	}
	// The same level-up told twice (its banner shown again) bursts once.
	const double Now = World->GetTimeSeconds();
	if (NewLevel == LastLevel && Now - LastPlayed < RepeatSeconds)
	{
		return false;
	}
	LastLevel = NewLevel;
	LastPlayed = Now;
	++BurstCount;

	// At the feet: the capsule's bottom for a character.
	Center = Pawn->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Center = Capsule->GetComponentLocation() - FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight());
		}
	}
	FindGround(*Pawn);

	// Some embers at the feet at once; the rest left by the ring as it passes, the last of them two thirds of its way out.
	EmberList.SetNum(Embers);
	for (int32 Index = 0; Index < Embers; ++Index)
	{
		FEmber& Ember = EmberList[Index];
		const bool bAtFeet = Index < FeetEmbers;
		Ember = FEmber();
		Ember.Born = bAtFeet ? Random.FRandRange(0.f, 0.12f)
			: FMath::Lerp(0.06f, RingSeconds * 0.65f, static_cast<float>(Index - FeetEmbers) / FMath::Max(Embers - FeetEmbers - 1, 1));
		Ember.Life = Random.FRandRange(1.f, 1.7f);
		Ember.Phase = Random.FRandRange(0.f, UE_TWO_PI);
		Ember.Width = Random.FRandRange(1.6f, 2.6f);
		Ember.Glow = Random.FRandRange(0.7f, 1.f);
		// Where it starts is known once it's born (the ring's reach then); until then it waits, unseen, at the feet.
		Ember.Location = Center;
	}
	Time = 0.f;
	return true;
}

void ULevelUpBurstSubsystem::FindGround(const APawn& Pawn)
{
	HalfwayHeight.Init(0.f, Segments);
	EdgeHeight.Init(0.f, Segments);
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The terrain and what's built on it, not the level's volumes or the pawn itself.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, FName(TEXT("LevelUpBurstGround")), &Pawn, /*bTraceComplex*/ false);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	auto HeightAt = [&](const FVector& Over, float Fallback)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Over + FVector(0.0, 0.0, BurstGroundUp), Over - FVector(0.0, 0.0, BurstGroundDown), Objects, Params))
		{
			return FMath::Clamp(static_cast<float>(Hit.ImpactPoint.Z - Center.Z), -BurstMostDrop, BurstMostRise);
		}
		return Fallback;
	};
	// The ground under the feet first (a level-up mid-jump bursts on the ground below).
	Center.Z += HeightAt(Center, 0.f);
	for (int32 Spoke = 0; Spoke < Segments; ++Spoke)
	{
		const FVector2D Way = SpokeWay(Spoke);
		const FVector Out(Way.X, Way.Y, 0.0);
		HalfwayHeight[Spoke] = HeightAt(Center + Out * (RingRadius * 0.5f), 0.f);
		EdgeHeight[Spoke] = HeightAt(Center + Out * RingRadius, HalfwayHeight[Spoke]);
	}
}

float ULevelUpBurstSubsystem::GroundAt(int32 Spoke, float Radius) const
{
	const int32 Index = ((Spoke % Segments) + Segments) % Segments;
	if (!HalfwayHeight.IsValidIndex(Index) || !EdgeHeight.IsValidIndex(Index))
	{
		return 0.f;
	}
	const float Half = RingRadius * 0.5f;
	return Radius <= Half ? FMath::Lerp(0.f, HalfwayHeight[Index], Radius / Half)
		: FMath::Lerp(HalfwayHeight[Index], EdgeHeight[Index], FMath::Clamp((Radius - Half) / Half, 0.f, 1.f));
}

FVector ULevelUpBurstSubsystem::RingPoint(int32 Spoke, float Radius) const
{
	const FVector2D Way = SpokeWay(Spoke);
	return Center + FVector(Way.X * Radius, Way.Y * Radius, GroundAt(Spoke, Radius) + BurstRingOverGround);
}

// ---------------------------------------------------------------------------
// The show
// ---------------------------------------------------------------------------

void ULevelUpBurstSubsystem::Advance(float DeltaSeconds)
{
	if (Time < 0.f)
	{
		return;
	}
	Time += FMath::Max(DeltaSeconds, 0.f);
	StreaksDrawn = 0;
	if (Time >= Seconds)
	{
		// Over: nothing drawn from here, and nothing ticks.
		Time = -1.f;
		EmbersGlowing = 0;
		return;
	}
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	const APlayerController* Controller = World->GetFirstPlayerController();
	const FVector Camera = Controller && Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetCameraLocation()
		: Center + FVector(0.0, 0.0, 1.0e5);
	FWeaponFX& Effects = Bullets->GetEffects();
	DrawRing(Effects);
	DrawEmbers(Effects, DeltaSeconds, Camera);
}

void ULevelUpBurstSubsystem::DrawRing(FWeaponFX& Effects)
{
	if (Time >= RingSeconds)
	{
		return;
	}
	const float Share = Time / RingSeconds;
	const float Strength = FMath::Clamp(Time / BurstFadeIn, 0.f, 1.f) * FMath::Pow(1.f - Share, 1.4f);
	const float Radius = RingRadiusAt(Time);
	const float CoreWidth = FMath::Lerp(BurstCoreWidthStart, BurstCoreWidthEnd, Share);
	// The level gem's pale cyan, with the banner's cyan for the halo round it.
	const FLinearColor Core = LooterUI::Color::GemLight();
	const FLinearColor Halo = LooterUI::Color::CyanText();
	for (int32 Spoke = 0; Spoke < Segments; ++Spoke)
	{
		const FVector From = RingPoint(Spoke, Radius);
		const FVector To = RingPoint(Spoke + 1, Radius);
		const FVector Past = (To - From) * BurstSegmentOverlap;
		Effects.AddStreak(From - Past, To + Past, CoreWidth, Core, BurstRingGlow * Strength);
		Effects.AddStreak(From - Past, To + Past, BurstHaloWidth, Halo, BurstRingGlow * BurstHaloShare * Strength);
		StreaksDrawn += 2;
	}
}

void ULevelUpBurstSubsystem::DrawEmbers(FWeaponFX& Effects, float DeltaSeconds, const FVector& Camera)
{
	EmbersGlowing = 0;
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	const FLinearColor Warm = LooterUI::Color::Accent();
	const FLinearColor Pale = LooterUI::Color::AccentLight();
	for (int32 Index = 0; Index < EmberList.Num(); ++Index)
	{
		FEmber& Ember = EmberList[Index];
		if (!Ember.bLaunched && Time >= Ember.Born)
		{
			// Born at the feet, or where the ring is as it passes, just over the ground there.
			const bool bAtFeet = Index < FeetEmbers;
			const int32 Spoke = Random.RandRange(0, Segments - 1);
			const float Radius = bAtFeet ? Random.FRandRange(25.f, 70.f) : RingRadiusAt(Ember.Born) * Random.FRandRange(0.85f, 1.f);
			const FVector2D Way = SpokeWay(Spoke).GetRotated(Random.FRandRange(-5.f, 5.f));
			Ember.Location = Center + FVector(Way.X * Radius, Way.Y * Radius, GroundAt(Spoke, Radius) + Random.FRandRange(3.f, 12.f));
			Ember.Velocity = FVector(Way.X, Way.Y, 0.0) * Random.FRandRange(10.f, 35.f) + FVector(0.0, 0.0, Random.FRandRange(65.f, 140.f));
			Ember.bLaunched = true;
		}
		float Glow = 0.f;
		if (Ember.bLaunched)
		{
			const float Age = Time - Ember.Born;
			// Rising and slowing, swaying a little side to side.
			const FVector Sway = FVector(-Ember.Velocity.Y, Ember.Velocity.X, 0.0).GetSafeNormal() * (BurstEmberSway * FMath::Sin(Age * 6.f + Ember.Phase));
			Ember.Velocity *= FMath::Exp(-BurstEmberDrag * Dt);
			Ember.Location += (Ember.Velocity + Sway) * Dt;
			if (Age < Ember.Life)
			{
				const float Flicker = 0.8f + 0.2f * FMath::Sin(Age * 23.f + Ember.Phase);
				const float Near = FMath::Clamp((static_cast<float>(FVector::Dist(Ember.Location, Camera)) - BurstNearEye) / BurstNearEyeFade, 0.f, 1.f);
				Glow = BurstEmberGlow * Ember.Glow * Flicker * Near * FMath::Clamp(Age / BurstEmberFadeIn, 0.f, 1.f)
					* FMath::Pow(1.f - Age / Ember.Life, 1.3f);
			}
		}
		// Every ember is drawn the whole show (unborn and spent ones dark), so the effects' count of glows holds steady.
		const FVector Along = Ember.Velocity.IsNearlyZero() ? FVector::UpVector : Ember.Velocity.GetSafeNormal();
		Effects.AddStreak(Ember.Location - Along * BurstEmberLength, Ember.Location, Ember.Width, Index % 4 == 0 ? Pale : Warm, Glow);
		++StreaksDrawn;
		EmbersGlowing += Glow > 0.01f ? 1 : 0;
	}
}
