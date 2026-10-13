// ADeadMinerCreature's head lamp: where it sits (the hat's SOCKET_Lamp, the hat bone, or where the model carries it
// without one), its glow and its light (the wind-up's flare, its fade with the body), the light budget (only the few
// nearest the player light up round them), and the lamp as a weak spot: shot out for this life, and a broken swing when
// it's shot in the wind-up.

#include "Creatures/DeadMinerCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/EncounterSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "AnimationRuntime.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "ReferenceSkeleton.h"

namespace
{
	/** A hit counts as the lamp's within this of its ball's surface (cm): the impact point sits on it, give or take. */
	constexpr float LampHitSlack = 4.f;

	/** What has mostly faded away takes no shots and gives no light (as the Unpaid's body: GoneEnoughToMiss). */
	constexpr float GoneEnoughToMiss = 0.6f;

	/** The budget is looked at this often (s): a light coming on or going off a moment late is never seen. */
	constexpr float BudgetEvery = 0.4f;

	/** The glow and the light are written again only when they change by this share. */
	constexpr float WriteShare = 0.02f;

	/** The player in WorldContext's level: the encounters' (a test's stand-in too), else the first local player's pawn. */
	const APawn* FindPlayer(const AActor& WorldContext)
	{
		if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(&WorldContext))
		{
			if (const APawn* Player = Encounters->GetPlayer())
			{
				return Player;
			}
		}
		return UGameplayStatics::GetPlayerPawn(&WorldContext, 0);
	}
}

// ---------------------------------------------------------------------------
// Where it sits, and its glow
// ---------------------------------------------------------------------------

void ADeadMinerCreature::PlaceLamp()
{
	UStaticMeshComponent* HatPart = GetHat();
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	const int32 HatBone = Model && !Rig.Hat.IsNone() ? Model->GetRefSkeleton().FindBoneIndex(Rig.Hat) : INDEX_NONE;
	if (HatPart && HatPart->IsVisible() && HatPart->GetStaticMesh() && HatPart->DoesSocketExist(LampSocket))
	{
		// The hat's own mount: the lamp sits where the art put it, and rides the hat.
		Lamp->AttachToComponent(HatPart, FAttachmentTransformRules::SnapToTargetNotIncludingScale, LampSocket);
	}
	else if (HatBone != INDEX_NONE)
	{
		// On the hat bone where the felt hat carries it (the parts table's place and turn), riding the head.
		Lamp->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.Hat);
		Lamp->SetRelativeTransform(PartOnBone(Model, Rig.Hat, LampOnHat, LampTurnOnHat));
	}
	else
	{
		// No model: where a man's brow would be.
		Lamp->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
		Lamp->SetRelativeLocationAndRotation(LampPointWithoutRig, FQuat::Identity);
	}
	// The lens: the lamp's own socket, or a hand's width in front of its pivot.
	const FVector Lens = Lamp->GetStaticMesh() && Lamp->DoesSocketExist(LightSocket)
		? Lamp->GetSocketTransform(LightSocket, RTS_Component).GetLocation() : LensOnLamp;
	LampLight->SetRelativeLocation(Lens);
	LampGlow->SetRelativeLocation(Lens);
	LampHitZone->SetRelativeLocation(Lens);
	LampHitZone->SetSphereRadius(Tuning.LampHitRadius);
	LampLight->SetAttenuationRadius(Tuning.LampRadius);
	LampLight->SetMaxDrawDistance(Tuning.LampLightRange);
}

void ADeadMinerCreature::BuildLampGlow()
{
	LampGlow->ClearInstances();
	LampGlow->SetNumCustomDataFloats(5);
	// The engine's plane lies flat in XY: one card flat, one standing along X, one along Y, so it reads from every side.
	const FRotator Cards[] = { FRotator(0.0, 0.0, 0.0), FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) };
	const FLinearColor Color = DeadMinerRules::LampColor();
	const float Data[] = { Color.R, Color.G, Color.B, Tuning.LampGlowStrength, 1.f };
	for (const FRotator& Card : Cards)
	{
		LampGlow->SetCustomData(LampGlow->AddInstance(FTransform(Card, FVector::ZeroVector, FVector(Tuning.LampGlowSize / 100.f))), Data);
	}
	ShownGlow = -1.f;
}

FVector ADeadMinerCreature::GetLampCenter() const
{
	return LampHitZone->GetComponentLocation();
}

bool ADeadMinerCreature::IsLampHit(const FVector& Point) const
{
	return FVector::DistSquared(GetLampCenter(), Point) <= FMath::Square(static_cast<double>(LampHitZone->GetScaledSphereRadius() + LampHitSlack));
}

float ADeadMinerCreature::WantedLampFlare() const
{
	// Through the swing's wind-up, swelling to the blow; nothing in a broken swing or a reel.
	if (GetCreatureState() != ECreatureState::Attack || bSwingBroken || Act == EAct::Reel || GetStateTime() >= AttackWindup)
	{
		return 0.f;
	}
	return DeadMinerRules::WindupFlare(GetStateTime(), AttackWindup);
}

// ---------------------------------------------------------------------------
// Its glow and light, frame by frame
// ---------------------------------------------------------------------------

void ADeadMinerCreature::ApplyLamp(float DeltaSeconds)
{
	// The lamp fades with the body (a phase-step, its death), flares with the wind-up, and is dark once shot out. The lamp
	// itself takes only the body's dissolve, never its rank's ember: its reflector is plain silver, and the glow cards are
	// its flame.
	if (!FMath::IsNearlyEqual(GetPhase(), ShownLampPhase, 0.002f))
	{
		Lamp->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, GetPhase());
		ShownLampPhase = GetPhase();
	}
	const float Fade = FMath::Clamp(1.f - GetPhase(), 0.f, 1.f);
	const float Flare = FMath::Clamp(LampFlare, 0.f, 1.f);
	const float Glow = bLampLit ? Tuning.LampGlowStrength * LampBrightness * (1.f + 2.f * Flare) * Fade : 0.f;
	if (ShownGlow < 0.f || FMath::Abs(Glow - ShownGlow) > WriteShare * FMath::Max(Tuning.LampGlowStrength, 1.f))
	{
		const int32 Count = LampGlow->GetInstanceCount();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			LampGlow->SetCustomDataValue(Index, 3, Glow, Index == Count - 1);
		}
		LampGlow->SetRelativeScale3D(FVector(1.f + Tuning.FlareSwell * Flare));
		LampGlow->SetVisibility(Glow > 0.01f && LampGlow->GetStaticMesh() != nullptr);
		ShownGlow = Glow;
	}
	// A miner's light goes out as it falls; the Foreman's gutters while he kneels (HoldsPoseInDeath).
	const bool bLightOn = bLampLightWanted && bLampLit && (!IsDead() || HoldsPoseInDeath()) && GetPhase() < GoneEnoughToMiss;
	const float Candela = bLightOn ? FMath::Lerp(Tuning.LampCandela, Tuning.FlareCandela, Flare) * LampBrightness * Fade : 0.f;
	if (LampLight->IsVisible() != bLightOn)
	{
		LampLight->SetVisibility(bLightOn);
	}
	if (bLightOn && (ShownLight < 0.f || FMath::Abs(Candela - ShownLight) > WriteShare * FMath::Max(Tuning.LampCandela, 1.f)))
	{
		LampLight->SetIntensity(Candela);
		ShownLight = Candela;
	}
}

void ADeadMinerCreature::TickLampBudget(float DeltaSeconds)
{
	LampBudgetClock -= DeltaSeconds;
	if (LampBudgetClock > 0.f)
	{
		return;
	}
	LampBudgetClock = BudgetEvery;
	RefreshLampLight();
}

void ADeadMinerCreature::RefreshLampLight()
{
	bool bWanted = false;
	const APawn* Player = FindPlayer(*this);
	if (Player && bLampLit && (!IsDead() || HoldsPoseInDeath()) && GetPhase() < GoneEnoughToMiss)
	{
		const FVector At = Player->GetActorLocation();
		const float Mine = static_cast<float>(FVector::Dist(GetActorLocation(), At));
		if (!UsesLampBudget())
		{
			// A lamp of its own outside the miners' budget (the Foreman's): lit round him wherever the player can see him.
			bWanted = Mine <= Tuning.LampLightRange * 1.5f;
		}
		else
		{
			// Lit round it only if fewer than the budget's dead miners with lit lamps stand nearer the player.
			int32 NearerLit = 0;
			auto Count = [&](const ADeadMinerCreature& Other)
			{
				if (&Other != this && !Other.IsDead() && Other.bLampLit && Other.UsesLampBudget() && Other.GetPhase() < GoneEnoughToMiss
					&& FVector::Dist(Other.GetActorLocation(), At) < Mine)
				{
					++NearerLit;
				}
			};
			if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
			{
				for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
				{
					if (const ADeadMinerCreature* Other = Cast<ADeadMinerCreature>(Each.Get()))
					{
						Count(*Other);
					}
				}
			}
			else if (UWorld* World = GetWorld())
			{
				for (TActorIterator<ADeadMinerCreature> It(World); It; ++It)
				{
					Count(**It);
				}
			}
			bWanted = DeadMinerRules::ShouldLightLamp(Mine, NearerLit, Tuning.MaxLitLamps, Tuning.LampLightRange);
		}
	}
	bLampLightWanted = bWanted;
	ApplyLamp(0.f);
}

bool ADeadMinerCreature::IsLampLightOn() const
{
	return LampLight && LampLight->IsVisible();
}

// ---------------------------------------------------------------------------
// The lamp as a weak spot
// ---------------------------------------------------------------------------

void ADeadMinerCreature::ApplyLampHitZone()
{
	// As the body's: a shot passes through what has mostly faded away; the Foreman's carbide lamp is no weak spot at all.
	const bool bOn = bLampIsWeakSpot && bLampZoneWanted && !IsDead() && GetPhase() < GoneEnoughToMiss;
	if (bOn != bLampZoneOn)
	{
		LampHitZone->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		bLampZoneOn = bOn;
	}
}

void ADeadMinerCreature::LampStruck(const FVector& HitLocation)
{
	if (bLampLit)
	{
		// The glass pops back toward the shot: from its target if it has one, else from in front of it.
		const APawn* Shooter = GetTarget();
		PutOutLamp(Shooter ? Shooter->GetActorLocation() : GetLampCenter() + GetActorForwardVector() * 500.f);
	}
	else
	{
		// Out already: a bullet ringing off its brass.
		LooterSound::PlayAt(this, LooterSoundCue::DeadMiner::LampStruck, HitLocation, 0.8f);
	}
	// Shot in the wind-up, the swing is lost: it reels, the pick flung wide.
	if (IsWindingUp())
	{
		BeginReel();
	}
}

void ADeadMinerCreature::PutOutLamp(const FVector& From)
{
	if (!bLampLit)
	{
		return;
	}
	bLampLit = false;
	LampFlare = 0.f;
	const FVector Center = GetLampCenter();
	const FVector Back = (From - Center).GetSafeNormal();
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		// A pop of glass and sparks, and the flame gone: it reads at once that the lamp is out.
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnImpact(Center, Back.IsNearlyZero() ? GetActorForwardVector() : Back, -Back, EImpactSurface::Target, true);
	}
	LooterSound::PlayAt(this, LooterSoundCue::DeadMiner::LampBreak, Center);
	RefreshLampLight();
	UE_LOG(LogLooter, Verbose, TEXT("%s's lamp is shot out."), *GetName());
}
