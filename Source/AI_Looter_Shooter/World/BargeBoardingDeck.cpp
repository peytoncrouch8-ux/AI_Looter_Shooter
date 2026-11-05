// ABargeBoarding: her tick: the furnace vents breathing along the deck, her hose snaking down toward the chapel's roof, and
// the player kept aboard her (or caught onto the Lamplighter) through the boarding.

#include "World/BargeBoarding.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "World/BargeFurnace.h"
#include "World/BargeRules.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "World/SinkIntakeRules.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"

#define LOCTEXT_NAMESPACE "BargeBoarding"

namespace
{
	/** Past this far from her middle nothing of her deck moves on (the vents' fire, the hose's relay): too far to matter. */
	constexpr float BusyReach = 25000.f;
	/** The vents' sounds are played within this of the player (cm), and Hob warns once a burst comes this near them. */
	constexpr float VentEarshot = 3500.f;
	constexpr float VentWarnReach = 900.f;
	const FName VentLines(TEXT("DA_Lines_Main6Vents"));
	/** Her hose: 1.5 m across (Landfall's), its sections overlapped at the joints; the hose's sag sideways. */
	constexpr float HoseGirth = 1.5f;
	constexpr double SectionOverlap = 1.03;
	constexpr float HoseSag = 600.f;
	/** Without a target: her hose hangs straight down this far. */
	constexpr float HoseFallbackDrop = 2000.f;
	/** As she dies the hose goes slack and is reeled away: this share of its reach a second. */
	constexpr float HoseSlackPerSecond = 0.08f;
	/** Smoke off a burning vent's column this often. */
	constexpr float VentSmokeEvery = 0.15f;
}

void ABargeBoarding::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

APawn* ABargeBoarding::FindPlayer() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

void ABargeBoarding::Advance(float DeltaSeconds)
{
	if (!IsPresent())
	{
		return;
	}
	StepClock += DeltaSeconds;
	const APawn* Player = FindPlayer();
	// A test level has no player: everything moves on. In play, far off she only hangs there.
	const bool bNear = !Player || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(BusyReach);
	if (bNear && Furnace)
	{
		Furnace->Advance(DeltaSeconds);
	}
	if (State == EBargeState::Dying)
	{
		AdvanceDying(DeltaSeconds);
	}
	// Her lights by where the player is (only while she hangs whole: dying puts them out).
	if (Player && State == EBargeState::Hanging)
	{
		const int32 View = LightViewAt(GetActorTransform().InverseTransformPosition(Player->GetActorLocation()) - FVector(0.0, 0.0, KitLift));
		if (View != LightView)
		{
			ShowLightsFor(View);
		}
	}
	if (bNear && State != EBargeState::Falling)
	{
		AdvancePushes(DeltaSeconds);
		AdvanceVents(DeltaSeconds);
		AdvanceHose(DeltaSeconds);
	}
	KeepPlayerAboard(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The vents
// ---------------------------------------------------------------------------

void ABargeBoarding::AdvanceVents(float DeltaSeconds)
{
	// They breathe from the grapples' bite (riflemen at the rail fire through their glare) until her furnace dies.
	const bool bBreathing = State == EBargeState::Hanging && StoryStep >= BargeRules::GrappleStep && StoryStep < BargeRules::EscapeStep;
	uint32 Glow = 0u;
	uint32 Fire = 0u;
	if (bBreathing)
	{
		VentClock += DeltaSeconds;
		for (int32 Index = 0; Index < BargeRules::NumVents; ++Index)
		{
			const BargeRules::EVentPhase Phase = BargeRules::VentPhase(Index, VentClock);
			Glow |= Phase != BargeRules::EVentPhase::Quiet ? 1u << Index : 0u;
			Fire |= Phase == BargeRules::EVentPhase::Burn ? 1u << Index : 0u;
		}
	}
	const APawn* Player = FindPlayer();
	if (Glow != VentGlowMask || Fire != VentFireMask)
	{
		TArray<FTransform> Cold, Hot, Flames, HoodsCold, HoodsHot;
		for (int32 Index = 0; Index < BargeRules::NumVents; ++Index)
		{
			const FVector Spot = BargeRules::VentSpots[Index];
			const bool bGlowing = (Glow & (1u << Index)) != 0u;
			(bGlowing ? Hot : Cold).Add(VentPlate(Spot));
			if (HasHood(Index))
			{
				// Its hood in the hall glows with it (the furnace breathing up through it).
				(bGlowing ? HoodsHot : HoodsCold).Add(VentHood(Spot));
			}
			if (Fire & (1u << Index))
			{
				// The column: a tongue of furnace fire as tall as a man and a half, wide as the grille.
				Flames.Add(VentColumn(Spot));
			}
			const uint32 Bit = 1u << Index;
			const FVector World = ToWorld(Spot);
			const bool bHeard = Player && FVector::DistSquared(Player->GetActorLocation(), World) < FMath::Square(VentEarshot);
			if (bHeard && (Glow & Bit) && !(VentGlowMask & Bit))
			{
				// The tell: the grille's slots glow and it draws breath (a hiss rising).
				LooterSound::PlayAt(this, LooterSoundCue::Hero::GaugeVent, World, 0.8f, 0.6f);
			}
			if (bHeard && (Fire & Bit) && !(VentFireMask & Bit))
			{
				LooterSound::PlayAt(this, LooterSoundCue::Foreman::FuseBurst, World, 0.9f, 0.7f);
				LamplighterEffects::Sparks(this, World + FVector(0.0, 0.0, 30.0), FVector::UpVector, 10, 0.8f);
				if (!bVentLineSaid && Player && FVector::DistSquared(Player->GetActorLocation(), World) < FMath::Square(VentWarnReach))
				{
					bVentLineSaid = true;
					LandfallRules::Say(this, VentLines);
				}
			}
		}
		VentCold->ClearInstances();
		VentGlow->ClearInstances();
		VentFire->ClearInstances();
		VentCold->AddInstances(Cold, false);
		VentGlow->AddInstances(Hot, false);
		VentFire->AddInstances(Flames, false);
		if (HoodCold && HoodGlow && HoodCold->GetStaticMesh())
		{
			HoodCold->ClearInstances();
			HoodGlow->ClearInstances();
			HoodCold->AddInstances(HoodsCold, false);
			HoodGlow->AddInstances(HoodsHot, false);
		}
		VentGlowMask = Glow;
		VentFireMask = Fire;
	}
	if (Fire == 0u)
	{
		VentHurtClock = 0.f;
		return;
	}
	VentHurtClock += DeltaSeconds;
	if (VentHurtClock < BargeRules::HurtEvery)
	{
		return;
	}
	const float Seconds = VentHurtClock;
	VentHurtClock = 0.f;
	for (int32 Index = 0; Index < BargeRules::NumVents; ++Index)
	{
		if (Fire & (1u << Index))
		{
			const FVector Base = ToWorld(BargeRules::VentSpots[Index]);
			HurtColumn(Base, BargeRules::VentColumnRadius, BargeRules::VentColumnHeight, BargeRules::VentBurnShare, Seconds);
			LamplighterEffects::SmokePuff(this, Base + FVector(0.0, 0.0, BargeRules::VentColumnHeight), FVector(0.0, 0.0, 220.0));
		}
	}
}

void ABargeBoarding::HurtColumn(const FVector& Base, float Radius, float Height, float HealthShare, float Seconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	TArray<FOverlapResult> Overlaps;
	const FVector Middle = Base + FVector(0.0, 0.0, Height * 0.5);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BargeVent), false, this);
	World->OverlapMultiByObjectType(Overlaps, Middle, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeCapsule(Radius, Height * 0.5f), Params);
	TSet<AActor*> Hurt;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Pawn = Overlap.GetActor();
		if (!Pawn || Hurt.Contains(Pawn))
		{
			continue;
		}
		Hurt.Add(Pawn);
		const UHealthComponent* Life = Pawn->FindComponentByClass<UHealthComponent>();
		if (!Life || Life->IsDead())
		{
			continue;
		}
		// The player by a share of their health a second; Company men caught in their own vents too (half as hard: their
		// coats are made for it).
		const bool bPlayer = Pawn == FindPlayer();
		const float Amount = Life->GetMaxHealth() * HealthShare * Seconds * (bPlayer ? 1.f : 0.5f);
		UGameplayStatics::ApplyDamage(Pawn, Amount, nullptr, this, UDamageType::StaticClass());
		if (bPlayer)
		{
			LooterSound::PlayAt(this, LooterSoundCue::Spitter::Burn, Pawn->GetActorLocation(), 0.9f);
		}
	}
}

// ---------------------------------------------------------------------------
// The hose
// ---------------------------------------------------------------------------

void ABargeBoarding::AdvanceHose(float DeltaSeconds)
{
	if (!HoseSections)
	{
		return;
	}
	if (State == EBargeState::Hanging && !bHoseFrozen && StoryStep < BargeRules::CageStep)
	{
		// Snaking lower toward Saint Ada's roof: never back up (a load mid-step starts it at the step's own reach).
		HoseShare = FMath::Max(HoseShare, BargeRules::HoseReach(StoryStep, StepClock));
	}
	else if (State == EBargeState::Dying || State == EBargeState::Burst)
	{
		// Her pilot light gone, the drum lets go: the hose slackens and drops away.
		HoseShare = FMath::Max(HoseShare - HoseSlackPerSecond * DeltaSeconds, 0.f);
	}
	const FVector Top = GetHoseTop();
	const FVector End = HoseTarget.IsNearlyZero() ? Top - FVector(0.0, 0.0, HoseFallbackDrop) : HoseTarget;
	const FVector Drop = End - Top;
	FVector Across = FVector::CrossProduct(Drop.GetSafeNormal(), FVector::UpVector);
	if (Across.IsNearlyZero())
	{
		Across = GetActorRightVector();
	}
	// From the drum down and out in a lazy curve, hanging to one side as a heavy hose does.
	FIntakeHoseShape Shape;
	Shape.P0 = Top;
	Shape.P1 = Top + FVector(0.0, 0.0, Drop.Z * 0.35) + Across * HoseSag;
	Shape.P2 = End - Drop * 0.25 + FVector(0.0, 0.0, -Drop.Z * 0.15) + Across * HoseSag * 0.6;
	Shape.P3 = End;

	TArray<FTransform> Transforms;
	const float Upto = FMath::Clamp(HoseShare, 0.f, 1.f);
	if (Upto > 0.01f)
	{
		double Rough = 0.0;
		FVector Last = Shape.At(0.f);
		for (int32 Step = 1; Step <= 16; ++Step)
		{
			const FVector Next = Shape.At(Upto * Step / 16.f);
			Rough += FVector::Dist(Last, Next);
			Last = Next;
		}
		const double SectionLength = FMath::Max(static_cast<double>(HoseNativeLength) * HoseGirth, 60.0);
		const int32 Count = FMath::Clamp(FMath::CeilToInt32(Rough / SectionLength), 6, 48);
		FVector From = Shape.At(0.f);
		for (int32 Index = 1; Index <= Count; ++Index)
		{
			const FVector To = Shape.At(Upto * Index / Count);
			const FVector Span = To - From;
			const double Length = Span.Size();
			if (Length > 1.0)
			{
				Transforms.Add(FTransform(FRotationMatrix::MakeFromZ(Span / Length).ToQuat(), (From + To) * 0.5,
					FVector(HoseGirth, HoseGirth, Length * SectionOverlap / FMath::Max(static_cast<double>(HoseNativeLength), 1.0))));
			}
			From = To;
		}
	}
	if (HoseSections->GetInstanceCount() != Transforms.Num())
	{
		HoseSections->ClearInstances();
		if (!Transforms.IsEmpty())
		{
			HoseSections->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
		}
	}
	else if (!Transforms.IsEmpty())
	{
		HoseSections->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
}

// ---------------------------------------------------------------------------
// Keeping the player aboard
// ---------------------------------------------------------------------------

void ABargeBoarding::KeepPlayerAboard(float DeltaSeconds)
{
	const bool bBoarding = StoryStep >= BargeRules::DeckStep && StoryStep <= BargeRules::EscapeStep
		&& (State == EBargeState::Hanging || State == EBargeState::Burst || State == EBargeState::Dying);
	APawn* Player = FindPlayer();
	const UHealthComponent* Life = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	ALamplighterShip* Ship = ALamplighterShip::Find(GetWorld());
	if (!bBoarding || !Player || (Life && Life->IsDead()) || (Scenes && Scenes->IsPlaying()))
	{
		OffFor = 0.f;
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Player->GetActorLocation()) - FVector(0.0, 0.0, KitLift);
	if (BargeRules::IsAboard(Local) || (Ship && Ship->IsPlayerAboard()))
	{
		OffFor = 0.f;
		return;
	}
	OffFor += DeltaSeconds;
	const bool bEscape = StoryStep == BargeRules::EscapeStep;
	if (OffFor < (bEscape ? BargeRules::OffGraceEscape : BargeRules::OffGrace))
	{
		return;
	}
	OffFor = 0.f;
	APlayerController* Controller = Cast<APlayerController>(Player->GetController());
	if (bEscape && Ship && Ship->PutPlayerOnDeck())
	{
		// The jump missed: Hob's catch, onto her deck (as everywhere a fall is his).
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Play({ FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("HobCatch", "Caught you, sunshine. She's the one under you."), 3.f) },
				ECaptionPlay::Queue);
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the jump missed; the player is caught onto the Lamplighter's deck."), *GetActorNameOrLabel());
	}
	else
	{
		// Off her mid-boarding (over the side, or woken at a lantern post far below): back aboard where the fight stands.
		const FTransform Start = GetPlayerStart();
		Player->TeleportTo(Start.GetLocation(), Start.Rotator());
		if (Controller)
		{
			Controller->SetControlRotation(Start.Rotator());
		}
		if (ACharacter* Character = Cast<ACharacter>(Player))
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the player was off her while boarding; back aboard."), *GetActorNameOrLabel());
	}
	if (Controller && Controller->PlayerCameraManager)
	{
		Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor::Black, false, false);
	}
}

#undef LOCTEXT_NAMESPACE
