// AAbelKeeper's moments frame by frame (where each one puts him, which way he faces, what comes at its end), his rise out of
// the canyon fog, the call-backs' kneel, and his ending: the kneel at zero, the last call, the scene after (Scenes/FreeAbel),
// and him gone into his own lantern. AbelKeeperFight.cpp starts the moments.

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelCorruption.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Scenes/FreeAbel.h"
#include "Scenes/SceneSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/KeeperLanternPost.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** His kneel settles this long before the scene (s). */
	constexpr float KneelSettleSeconds = 2.f;

	/** A pull yanks him back this quickly before its stun (s). */
	constexpr float PullYankSeconds = 0.4f;

	/** In the fog he bobs this much (cm), this often (Hz). */
	constexpr float FogBob = 14.f;
	constexpr float FogBobHz = 0.45f;

	/** With no lanterns to relight (a level without them), the fog lets him go after this long (s). */
	constexpr float FogWithoutLanterns = 20.f;

	/** He turns this fast in his own moments (degrees a second). */
	constexpr float TurnRate = 160.f;

	/** His entrance: the lantern's light peaks this share of the way in, and the view shakes this hard then. */
	constexpr float IntroPeakShare = 0.4f;
	constexpr float IntroShake = 0.4f;

	/** Staggered, his lantern burns low (toward an ember). */
	constexpr float StaggerLantern = -0.5f;

	/** His rise out of the canyon fog: this far below his spot, out past the open end, and how long it takes (s). */
	constexpr float AscendDrop = 1600.f;
	constexpr float AscendOut = 900.f;
	constexpr float AscendSeconds = 4.5f;
}

// ---------------------------------------------------------------------------
// A moment, frame by frame
// ---------------------------------------------------------------------------

void AAbelKeeper::TickMove(float DeltaSeconds)
{
	MoveTime += DeltaSeconds;
	if (Move != EAbelMove::None && Move != EAbelMove::Kneel && Move != EAbelMove::Scene)
	{
		// Held by his moment: the brain's steering goes unused.
		ConsumeMovementInputVector();
		if (!bScriptedMove)
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
	}
	const APawn* Player = Boss->GetFightPlayer();
	auto FacePlayer = [this, Player, DeltaSeconds]()
	{
		if (Player)
		{
			FaceYaw(static_cast<float>((Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw), DeltaSeconds, TurnRate);
		}
	};
	const float Scale = GetSizeScale();

	switch (Move)
	{
	case EAbelMove::None:
		TryWantedMoments(DeltaSeconds);
		break;

	case EAbelMove::Intro:
	{
		// His lantern raised to the player: its light swells, peaks (the view shakes) and dies back, then he fights.
		FacePlayer();
		const float Peak = ShowRules.IntroSeconds * IntroPeakShare;
		SetLanternFlare(MoveTime < Peak ? MoveTime / Peak : 1.f - FMath::Clamp((MoveTime - Peak) / (ShowRules.IntroSeconds - Peak), 0.f, 1.f));
		if (MoveTime - DeltaSeconds < Peak && MoveTime >= Peak)
		{
			BossCameraShake::Kick(this, GetLanternGlobe(), IntroShake, 0.6f);
		}
		if (MoveTime >= ShowRules.IntroSeconds)
		{
			EndMove();
			Rejoin();
		}
		// Phase one's moments may start now (they cut the entrance short).
		TryWantedMoments(DeltaSeconds);
		break;
	}

	case EAbelMove::Staggered:
		// Down on a knee facing the player, his coal open, his light low; his boss ends it (HandleStaggered).
		FacePlayer();
		SetLanternFlare(StaggerLantern);
		break;

	case EAbelMove::Grieve:
	{
		// Toward the six dead on the boards (the keeper's job), or the sunset where there are none.
		FVector Dead;
		const float Yaw = Corruption->GetDeadToWalk(GetActorLocation(), Dead) ? static_cast<float>((Dead - GetActorLocation()).Rotation().Yaw)
			: SunsetYaw();
		FaceYaw(Yaw, DeltaSeconds, TurnRate);
		if (MoveTime >= GrieveSeconds)
		{
			EndMove();
			Rejoin();
		}
		break;
	}

	case EAbelMove::CallKneel:
		// Down on his knees facing the keeper's post (his own lantern calling him), his bar locked; the corruption component
		// lets him up (called back, or the kneel run out).
		if (const AKeeperLanternPost* Post = GetKeepersPost())
		{
			FaceYaw(static_cast<float>((Post->GetActorLocation() - GetActorLocation()).Rotation().Yaw), DeltaSeconds, TurnRate * 0.5f);
		}
		SetLanternFlare(StaggerLantern);
		break;

	case EAbelMove::Ascend:
	{
		// Up out of the canyon fog to his spot on the boards, smog pouring off him, the view jolting as he lands.
		const float Alpha = FMath::Clamp(MoveTime / AscendSeconds, 0.f, 1.f);
		SetActorLocation(AbelRules::GlideAt(MoveFrom, GetHome().GetLocation(), Alpha, 120.f * Scale), false, nullptr, ETeleportType::TeleportPhysics);
		FaceYaw(static_cast<float>((-GetSunsetDirection()).Rotation().Yaw), DeltaSeconds, TurnRate);
		SetLanternFlare(Alpha);
		if (MoveTime >= AscendSeconds)
		{
			BossCameraShake::Kick(this, GetActorLocation(), IntroShake, 0.6f);
			LooterSound::PlayAt(this, LooterSoundCue::AbelIntro, GetActorLocation());
			EndMove();
			// On the boards: a player on the deck starts his fight now.
			RefreshStory();
		}
		break;
	}

	case EAbelMove::Flare:
		FacePlayer();
		SetLanternFlare(FMath::Clamp(MoveTime / FlareSeconds, 0.f, 1.f));
		if (MoveTime >= FlareSeconds)
		{
			// In the wind it's a barrage; before it, one shot.
			const bool bBarrage = bWindBlowing && ShowRules.BarrageShots > 1;
			BeginMove(EAbelMove::Fire);
			if (bBarrage)
			{
				FireBarrageShot();
			}
			else
			{
				ReleaseBuckshot();
			}
		}
		break;

	case EAbelMove::Fire:
	{
		// A barrage's next shots, each a gap after the last; the shot's pose held until the last is away.
		const bool bBarrage = BarrageFired > 0;
		if (bBarrage)
		{
			FacePlayer();
		}
		if (bBarrage && BarrageFired < ShowRules.BarrageShots && MoveTime >= BarrageFired * ShowRules.BarrageGap)
		{
			FireBarrageShot();
		}
		const float Held = bBarrage ? ShowRules.BarrageGap * (ShowRules.BarrageShots - 1) : 0.f;
		SetLanternFlare(1.f - FMath::Clamp((MoveTime - Held) / FireSeconds, 0.f, 1.f));
		if (MoveTime >= FireSeconds + Held)
		{
			EndMove();
		}
		break;
	}

	case EAbelMove::DriftOut:
		SetActorLocation(AbelRules::GlideAt(MoveFrom, DragTarget(), MoveTime / DriftSeconds, 60.f * Scale), false, nullptr, ETeleportType::TeleportPhysics);
		FacePlayer();
		if (MoveTime >= DriftSeconds)
		{
			BeginMove(EAbelMove::InFog);
		}
		break;

	case EAbelMove::InFog:
	{
		const FVector Goal = DragTarget() + FVector(0.0, 0.0, FogBob * Scale * FMath::Sin(2.f * UE_PI * FogBobHz * MoveTime));
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), Goal, DeltaSeconds, 2.5f), false, nullptr, ETeleportType::TeleportPhysics);
		FacePlayer();
		// A shot from the fog under way: his lantern's flare, then the buckshot.
		TickFogShot(DeltaSeconds);
		if (LanternPosts.IsEmpty() && MoveTime >= FogWithoutLanterns)
		{
			BeginMove(EAbelMove::DragBack);
		}
		break;
	}

	case EAbelMove::DragBack:
		SetActorLocation(AbelRules::GlideAt(MoveFrom, GetHome().GetLocation(), MoveTime / DragBackSeconds, 40.f * Scale), false, nullptr,
			ETeleportType::TeleportPhysics);
		FacePlayer();
		if (MoveTime >= DragBackSeconds)
		{
			// On the deck again, and his spell over; the light that dragged him back leaves him stunned, his coal open.
			LetGo();
			Boss->EndUntargetable();
			BeginMove(EAbelMove::Pulled);
			HoldStill(false);
			UE_LOG(LogLooter, Log, TEXT("%s: dragged back onto the boards by the lanterns."), *GetActorNameOrLabel());
		}
		break;

	case EAbelMove::WalkOff:
	{
		// Facing the post he's making for (his black lantern held out to hang).
		if (!MoveTo.Equals(GetActorLocation(), 5.0))
		{
			FaceYaw(static_cast<float>((MoveTo - GetActorLocation()).Rotation().Yaw), DeltaSeconds, TurnRate);
		}
		const FVector Here = GetActorLocation();
		const FVector Next = FMath::VInterpConstantTo(Here, MoveTo, DeltaSeconds, WalkOffSpeed * Scale);
		SetActorLocation(Next, false, nullptr, ETeleportType::TeleportPhysics);
		if (Next.Equals(MoveTo, 2.0) || MoveTime >= WalkOffSeconds)
		{
			Pull();
		}
		break;
	}

	case EAbelMove::Pulled:
		if (MoveTime < PullYankSeconds && !MoveTo.Equals(MoveFrom, 1.0))
		{
			SetActorLocation(AbelRules::GlideAt(MoveFrom, MoveTo, MoveTime / PullYankSeconds), false, nullptr, ETeleportType::TeleportPhysics);
		}
		else if (bScriptedMove)
		{
			LetGo();
			HoldStill(false);
		}
		FacePlayer();
		if (MoveTime >= PullYankSeconds + StunSeconds)
		{
			EndMove();
			// "Between pulls he fights faster."
			if (bWindBlowing)
			{
				ApplyPace(true);
			}
			Rejoin();
		}
		break;

	case EAbelMove::Kneel:
		FaceYaw(KneelYaw, DeltaSeconds, 90.f);
		if (!bEndingAsked && MoveTime >= KneelSettleSeconds)
		{
			PlayEnding();
		}
		// The last call held at the keeper's post: the scene (asked again while another one plays).
		if (bEndingAsked && !bFreeAsked && Corruption->IsFreed())
		{
			PlayFreeScene();
		}
		break;

	case EAbelMove::Scene:
	default:
		break;
	}

	// The pose his moment asks for (the scene picks its own).
	if (Move != EAbelMove::Scene)
	{
		float Blend = 0.f;
		const EAbelPose Want = WantedPose(Blend);
		if (Want != TargetPose)
		{
			SetPose(Want, Blend);
		}
	}
}

// ---------------------------------------------------------------------------
// His ending
// ---------------------------------------------------------------------------

void AAbelKeeper::PlayEnding()
{
	bEndingAsked = true;
	// The smog's echo (a vigil stone's rematch) is its stone's to end: nothing of Abel's is in it to call home.
	if (Boss->IsRematch())
	{
		return;
	}
	// The last call at the keeper's post: the hold under the last of the smog (UAbelCorruptionComponent), then the scene.
	Corruption->BeginFinalCall();
}

void AAbelKeeper::PlayFreeScene()
{
	bFreeAsked = true;
	if (FreeAbel::Play(*this))
	{
		return;
	}
	if (USceneSubsystem::AreScenesOn(GetWorld()))
	{
		// Another scene is playing: ask again shortly.
		bFreeAsked = false;
		return;
	}
	// Scenes off (a tour, a perf run): the missions hear it as played; the world as the scene would leave it.
	FinishEndingAtOnce();
}

void AAbelKeeper::BeginAscend()
{
	// From down in the canyon fog out past the open end, rising to his spot: his own lantern on the post has called him.
	const FVector HomeAt = GetHome().GetLocation();
	SetActorLocation(HomeAt + GetSunsetDirection() * (OpenEndDistance + AscendOut) - FVector(0.0, 0.0, AscendDrop), false, nullptr,
		ETeleportType::TeleportPhysics);
	EndMove();
	BeginMove(EAbelMove::Ascend);
	HoldStill(true);
	Boss->EngageRadius = 0.f;
	LooterSound::PlayAt(this, LooterSoundCue::AbelWindRise, HomeAt);
	UE_LOG(LogLooter, Log, TEXT("%s: his lantern on the keeper's post calls him; he comes up out of the canyon fog."), *GetActorNameOrLabel());
}

void AAbelKeeper::BeginCallKneel()
{
	if (IsDead())
	{
		return;
	}
	// Whatever he was at, down on his knees: the smog in him fighting the keeper, his bar locked (the corruption's).
	EndMove();
	BeginMove(EAbelMove::CallKneel);
	HoldStill(false);
	SetPose(EAbelPose::Kneel, 0.35f);
	BossCameraShake::Kick(this, GetActorLocation(), IntroShake * 0.6f, 0.5f);
}

void AAbelKeeper::EndCallKneel()
{
	if (Move != EAbelMove::CallKneel)
	{
		return;
	}
	EndMove();
	Rejoin();
}

void AAbelKeeper::BeginScene()
{
	Move = EAbelMove::Scene;
	MoveTime = 0.f;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
}

void AAbelKeeper::SetScenePlace(const FVector& Location, float Yaw)
{
	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
}

void AAbelKeeper::CompleteFreeing(bool bLanternToo)
{
	if (bEndingDone)
	{
		return;
	}
	bEndingDone = true;
	if (bLanternToo)
	{
		// He's in his own lantern, burning like a sunrise, and Sully has it down off the post: Main 7 carries it home.
		Corruption->SetFreed();
		if (AKeeperLanternPost* Post = GetKeepersPost())
		{
			Post->LightKeepersLantern();
			Post->TakeDownLit();
		}
	}
	else if (UWorld* World = GetWorld())
	{
		// The smog's echo boils away into the fog: there's nothing in it to step into a lantern.
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.SpawnSoulTatters(GetActorLocation(), FVector::UpVector, FLinearColor(0.05f, 0.035f, 0.07f), 90.f, 16);
		}
		LooterSound::PlayAt(this, LooterSoundCue::UnpaidDissolve, GetActorLocation());
	}
	SetPresent(false);
}

bool AAbelKeeper::ComeBack()
{
	if (IsDead())
	{
		return false;
	}
	bEndingDone = false;
	bEndingAsked = false;
	bFreeAsked = false;
	Move = EAbelMove::None;
	MoveTime = 0.f;
	bScriptedMove = false;
	ResetToHome();
	SetPose(EAbelPose::Idle, 0.f);
	if (Lantern)
	{
		// The black lantern in his fist again (it fell into the canyon with the real one's freeing).
		Lantern->SetVisibility(true);
	}
	ForcePresent(true);
	return true;
}

void AAbelKeeper::FinishEndingAtOnce()
{
	if (bEndingDone)
	{
		return;
	}
	CompleteFreeing(true);
	// The missions' scene step waits for the scene: counted as played (the story is past it).
	USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (Scenes && !Scenes->HasPlayed(FreeAbel::SceneName()))
	{
		Scenes->MarkPlayed(FreeAbel::SceneName(), true);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the ending as the scene leaves it: Abel in his own lantern, lit, taken down off the post."), *GetActorNameOrLabel());
}
