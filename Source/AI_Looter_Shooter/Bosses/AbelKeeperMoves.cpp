// AAbelKeeper's moments frame by frame (where each one puts him, which way he faces, what comes at its end) and his ending:
// the kneel, the scene after (Scenes/SitWithPa), his board. AbelKeeperFight.cpp starts the moments.

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SitWithPa.h"
#include "Story/AbelOnBoard.h"
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
		FaceYaw(SunsetYaw(), DeltaSeconds, TurnRate);
		if (MoveTime >= GrieveSeconds)
		{
			EndMove();
			Rejoin();
		}
		break;

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
		FaceYaw(SunsetYaw(), DeltaSeconds, TurnRate);
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
		FaceYaw(SunsetYaw(), DeltaSeconds, TurnRate * 0.5f);
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
	if (SitWithPa::Play(*this))
	{
		return;
	}
	if (USceneSubsystem::AreScenesOn(GetWorld()))
	{
		// Another scene is playing: ask again shortly.
		bEndingAsked = false;
		return;
	}
	// Scenes off (a tour, a perf run): the missions heard it as played; the world as the scene would leave it.
	FinishEndingAtOnce();
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

void AAbelKeeper::SitOnBoard()
{
	if (bEndingDone)
	{
		return;
	}
	bEndingDone = true;
	if (AKeeperLanternPost* Post = GetKeepersPost())
	{
		Post->LightKeepersLantern();
	}
	if (OnBoard)
	{
		OnBoard->ShowNow();
	}
	else
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no AAbelOnBoard in the level to sit him on his board (build_area_deck.py places it)."),
			*GetActorNameOrLabel());
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
	Move = EAbelMove::None;
	MoveTime = 0.f;
	bScriptedMove = false;
	ResetToHome();
	SetPose(EAbelPose::Idle, 0.f);
	if (OnBoard)
	{
		OnBoard->ForgetScene();
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
	SitOnBoard();
	// The missions' last step waits for the scene: counted as played (the story is past it).
	USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (Scenes && !Scenes->HasPlayed(SitWithPa::SceneName()))
	{
		Scenes->MarkPlayed(SitWithPa::SceneName(), true);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the ending as the scene leaves it: the Keeper's Lantern lit, Abel on his board."), *GetActorNameOrLabel());
}
