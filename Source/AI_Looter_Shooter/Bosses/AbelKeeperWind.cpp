// AAbelKeeper's Gravewind (his last phase, "Let me go"): gusts toward the deck's open end that carry the player a little
// way, the wind pouring down off the point past the end (so fall recovery's outside rule catches a fall within a second),
// Hob's word on the first fall, and the wisps streaming over the deck.

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "World/FallRecoverySubsystem.h"
#include "World/PlayableArea.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The wisps stream over the deck from this far behind his spot to this far past the open end (cm, at size 1)... */
	constexpr float WispBehind = 900.f;
	constexpr float WispPast = 500.f;
	/** ...this far either side, and between these heights over the boards. */
	constexpr float WispAcross = 950.f;
	constexpr float WispLow = 20.f;
	constexpr float WispHigh = 280.f;
	/** How fast they stream between gusts and at a gust's height (cm/s). */
	constexpr float WispCalm = 260.f;
	constexpr float WispGust = 900.f;
	/** Their glow: pale, cold, a little less than the fog wall's. */
	constexpr float WispGlow = 1.6f;
	const FLinearColor WispColor(0.78f, 0.86f, 1.f);
}

void AAbelKeeper::StartWind()
{
	if (bWindBlowing)
	{
		return;
	}
	bWindBlowing = true;
	WindTime = 0.f;
	GustNow = 0.f;
	// The Gravewind rising off the point: heard all over the deck.
	LooterSound::Play2D(this, LooterSoundCue::AbelWindRise);
	ForcedGustLeft = 0.f;
	WispState.Reset();
	WispRandom.Initialize(0x47a7e);
	// Hob catches a fall off the deck, as everywhere in the story: a word the first time.
	UWorld* World = GetWorld();
	if (UFallRecoverySubsystem* Falls = World ? World->GetSubsystem<UFallRecoverySubsystem>() : nullptr)
	{
		FallHandle = Falls->OnRecovered.AddUObject(this, &AAbelKeeper::HandleFallRecovered);
	}
	if (!WispMaterial && WispBase)
	{
		WispMaterial = UMaterialInstanceDynamic::Create(WispBase, this);
		WispMaterial->SetVectorParameterValue(TEXT("Color"), WispColor);
		WispMaterial->SetScalarParameterValue(TEXT("Glow"), WispGlow);
		WispMaterial->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Wisps->SetMaterial(0, WispMaterial);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the Gravewind pours off the point."), *GetActorNameOrLabel());
}

void AAbelKeeper::StopWind()
{
	if (!bWindBlowing)
	{
		return;
	}
	bWindBlowing = false;
	GustNow = 0.f;
	ForcedGustLeft = 0.f;
	UWorld* World = GetWorld();
	if (UFallRecoverySubsystem* Falls = World ? World->GetSubsystem<UFallRecoverySubsystem>() : nullptr)
	{
		Falls->OnRecovered.Remove(FallHandle);
	}
	FallHandle.Reset();
	ClearWisps();
}

void AAbelKeeper::ForceGust()
{
	StartWind();
	ForcedGustLeft = Gust.Seconds;
}

bool AAbelKeeper::IsPastOpenEnd(const FVector& Location) const
{
	if (const APlayableArea* Playable = APlayableArea::Find(GetWorld()))
	{
		return !Playable->Contains(Location);
	}
	return FVector::DotProduct(Location - GetHome().GetLocation(), GetSunsetDirection()) > OpenEndDistance;
}

void AAbelKeeper::TickWind(float DeltaSeconds)
{
	WindTime += DeltaSeconds;
	float Strength = AbelRules::GustStrength(Gust, WindTime);
	if (ForcedGustLeft > 0.f)
	{
		// A gust asked for now: its own rise and fall over its seconds.
		ForcedGustLeft -= DeltaSeconds;
		FAbelGustRules Now = Gust;
		Now.FirstAfter = 0.f;
		Now.Every = Gust.Seconds + 1.f;
		Strength = FMath::Max(Strength, AbelRules::GustStrength(Now, Gust.Seconds - FMath::Max(ForcedGustLeft, 0.f)));
	}
	GustNow = Strength;

	ACharacter* Player = Cast<ACharacter>(Boss->GetFightPlayer());
	if (Player)
	{
		// A gust carries the player toward the open end (swept: a bier or a post stops them), slower than they walk.
		if (GustNow > 0.f)
		{
			Player->AddActorWorldOffset(GetSunsetDirection() * (Gust.Push * GustNow * DeltaSeconds), true);
		}
		// Past the end the wind pours down off the point and takes a falling player with it: fall recovery's outside rule
		// (5 m under the last safe spot) brings them back to the deck within a second.
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
		if (Movement && Movement->IsFalling() && IsPastOpenEnd(Player->GetActorLocation()))
		{
			Movement->Velocity.Z = AbelRules::FallSpeedPastEnd(Gust, Movement->Velocity.Z);
		}
	}
	DrawWisps(DeltaSeconds);
}

void AAbelKeeper::HandleFallRecovered(const FFallRecoveryEvent& Event)
{
	if (bHobSpokeOfFall || !Boss->IsFighting() || Event.Pawn != Boss->GetFightPlayer())
	{
		return;
	}
	bHobSpokeOfFall = true;
	Say(AbelRules::HobOnFall());
}

void AAbelKeeper::DrawWisps(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || Gust.Wisps <= 0)
	{
		return;
	}
	const float Scale = GetSizeScale();
	const FVector Sunset = GetSunsetDirection();
	const FVector Across = FVector::CrossProduct(FVector::UpVector, Sunset);
	const FVector Feet = GetHome().GetLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float Behind = WispBehind * Scale;
	const float Past = OpenEndDistance + WispPast * Scale;
	auto Restart = [this, Behind](FWisp& Wisp, bool bAnywhere)
	{
		Wisp.Start = FVector(0.0, WispRandom.FRandRange(-WispAcross, WispAcross), WispRandom.FRandRange(WispLow, WispHigh));
		Wisp.Along = bAnywhere ? WispRandom.FRandRange(-Behind, OpenEndDistance) : -Behind - WispRandom.FRandRange(0.f, 300.f);
		Wisp.Speed = WispRandom.FRandRange(0.8f, 1.25f);
		Wisp.Length = WispRandom.FRandRange(60.f, 170.f);
		Wisp.Width = WispRandom.FRandRange(1.5f, 3.f);
	};
	if (WispState.Num() != Gust.Wisps)
	{
		WispState.SetNum(Gust.Wisps);
		for (FWisp& Wisp : WispState)
		{
			Restart(Wisp, true);
		}
	}
	// Faint and slow between gusts, long and quick in one; each streams past the open end and starts again behind.
	const float Speed = FMath::Lerp(WispCalm, WispGust, GustNow);
	const float Stretch = FMath::Lerp(0.6f, 1.4f, GustNow);
	const FQuat Along = Sunset.Rotation().Quaternion();
	TArray<FTransform> Transforms;
	Transforms.Reserve(WispState.Num());
	for (FWisp& Wisp : WispState)
	{
		Wisp.Along += Speed * Wisp.Speed * DeltaSeconds;
		if (Wisp.Along > Past)
		{
			Restart(Wisp, false);
		}
		const FVector Where = Feet + Sunset * Wisp.Along + Across * (Wisp.Start.Y * Scale) + FVector(0.0, 0.0, Wisp.Start.Z * Scale);
		// The engine cube is 100 cm a side.
		Transforms.Emplace(Along, Where, FVector(Wisp.Length * Stretch / 100.f, Wisp.Width / 100.f, Wisp.Width / 100.f));
	}
	if (Wisps->GetInstanceCount() != Transforms.Num())
	{
		Wisps->ClearInstances();
		Wisps->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	}
	else
	{
		Wisps->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
}

void AAbelKeeper::ClearWisps()
{
	WispState.Reset();
	if (Wisps && Wisps->GetInstanceCount() > 0)
	{
		Wisps->ClearInstances();
	}
}
