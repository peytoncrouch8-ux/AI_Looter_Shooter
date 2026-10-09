// AAbelKeeper's fight: his own moments as his phases' events start them (the grief, the buckshot's flare, the bell and the
// drift into the fog, the lanterns dragging him back, the walk into the wind and the pull), and the fight starting, starting
// over and won. AbelKeeperMoves.cpp moves his moments on frame by frame and has his ending (the scene, his board);
// AbelKeeper.cpp has his place in the story.

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Story/CaptionSubsystem.h"
#include "World/ChapelBell.h"
#include "World/KeeperLanternPost.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A moment his phase asks for while he's busy waits this long for him to be free, then it's dropped (s). */
	constexpr float MomentPatience = 3.f;

	/** The flare's glow at the lantern's globe (cm), pale as his ghost light. */
	constexpr float FlareGlow = 16.f;
	const FLinearColor FlareColor(1.f, 0.94f, 0.82f);

	/** Where the pump's muzzle socket is (SM_AbelPump). */
	const FName MuzzleSocket(TEXT("Muzzle"));

	/** The model stands on the capsule's foot: its middle is this far over the ground at size 1. */
	constexpr float ModelOverMiddle = 90.f;
}

// ---------------------------------------------------------------------------
// The fight starting, starting over, won
// ---------------------------------------------------------------------------

void AAbelKeeper::HandleFightStarted()
{
	// Now he can be hurt (the boss's spells take it from here) and the numbers show it.
	Health->bInvulnerable = false;
	Health->bShowDamageNumbers = true;
	LastFighter = Boss->GetFightPlayer();
	KnownAdds.Reset();
	bHobSpokeOfFall = false;
	bBuckshotWanted = bGrieveWanted = bWalkOffWanted = false;
	LightAllLanterns();
	ApplyPace(false);
	EndMove();
	// He turns to the player and raises his lantern as his bar sweeps in.
	BeginIntro();
}

void AAbelKeeper::HandleFightReset()
{
	// The player fell: he's home and healed already (the boss's reset); everything of his fight goes with it.
	EndMove();
	StopWind();
	LightAllLanterns();
	ApplyPace(false);
	bBuckshotWanted = bGrieveWanted = bWalkOffWanted = false;
	Health->bInvulnerable = true;
	Health->bShowDamageNumbers = false;
	SetLanternFlare(0.f);
	SetPose(EAbelPose::Idle, 0.3f);
	KnownAdds.Reset();
	UE_LOG(LogLooter, Log, TEXT("%s: the fight starts over; the lanterns burn, he walks the boards again."), *GetActorNameOrLabel());
}

void AAbelKeeper::HandleFightWon()
{
	// His adds and the wall went with the fight (the boss's); his kneel and the scene come with his death (OnDied).
	StopWind();
	LightAllLanterns();
	KnownAdds.Reset();
	UE_LOG(LogLooter, Log, TEXT("%s: beaten. He kneels."), *GetActorNameOrLabel());
}

void AAbelKeeper::HandlePhaseChanged(int32 NewPhase, int32 OldPhase)
{
	Say(AbelRules::PhaseLine(NewPhase));
}

void AAbelKeeper::HandleCustomEvent(FName EventName)
{
	if (!Boss->IsFighting() || IsDead())
	{
		return;
	}
	if (EventName == AbelRules::BuckshotEvent())
	{
		if (!StartFlare())
		{
			bBuckshotWanted = true;
			BuckshotWaited = 0.f;
		}
	}
	else if (EventName == AbelRules::GrieveEvent())
	{
		if (!Grieve())
		{
			bGrieveWanted = true;
			GrieveWaited = 0.f;
		}
	}
	else if (EventName == AbelRules::BellEvent())
	{
		RingTheBell();
		Say(AbelRules::BellLine());
		DriftOut();
	}
	else if (EventName == AbelRules::GravewindEvent())
	{
		StartWind();
	}
	else if (EventName == AbelRules::WalkOffEvent())
	{
		if (!WalkOff())
		{
			bWalkOffWanted = true;
			WalkOffWaited = 0.f;
		}
	}
	else if (EventName == AbelRules::FogShotEvent())
	{
		// Only from the fog itself (not on his way out, nor dragged back).
		StartFogShot();
	}
}

void AAbelKeeper::OnDied()
{
	Super::OnDied();
	// At zero he kneels where he is, facing whoever brought him down; his coal sinks to an ember, his light dims.
	EndMove();
	StopWind();
	BeginMove(EAbelMove::Kneel);
	// Heading for the kneel from the moment he falls, not a frame later when his pose is next chosen.
	SetPose(EAbelPose::Kneel, 1.2f);
	bEndingAsked = false;
	const APawn* Who = LastFighter.Get();
	if (!Who)
	{
		const APlayerController* First = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		Who = First ? First->GetPawn() : nullptr;
	}
	const FVector To = Who ? Who->GetActorLocation() - GetActorLocation() : GetActorForwardVector();
	KneelYaw = static_cast<float>(To.IsNearlyZero() ? GetActorRotation().Yaw : To.Rotation().Yaw);
	SetLanternFlare(-1.f);
	// His coal flares once as he falls (his boss's slow beat and shake come with it), then sinks to an ember.
	FlareCoalAtDeath();
}

void AAbelKeeper::OnRespawned()
{
	Super::OnRespawned();
	// Put back at his spot (his fight starting over): nothing of his own moments is left.
	Move = EAbelMove::None;
	MoveTime = 0.f;
	bScriptedMove = false;
	Heat = 0.f;
}

// ---------------------------------------------------------------------------
// His moments
// ---------------------------------------------------------------------------

void AAbelKeeper::BeginMove(EAbelMove NewMove)
{
	Move = NewMove;
	MoveTime = 0.f;
	MoveFrom = GetActorLocation();
	MoveTo = MoveFrom;
	// A fog shot's flare or a barrage belongs to the moment it began in.
	FogFlareLeft = 0.f;
	BarrageFired = 0;
}

void AAbelKeeper::EndMove()
{
	LetGo();
	Move = EAbelMove::None;
	MoveTime = 0.f;
	FogFlareLeft = 0.f;
	BarrageFired = 0;
	if (!IsDead())
	{
		SetLanternFlare(0.f);
	}
}

void AAbelKeeper::HoldStill(bool bScripted)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	if (bScripted && !bScriptedMove)
	{
		// Placed by his moment each frame, off the ground's rules: out over the canyon there's nothing to stand on.
		Movement->DisableMovement();
		bScriptedMove = true;
	}
	ConsumeMovementInputVector();
}

void AAbelKeeper::LetGo()
{
	if (bScriptedMove && !IsDead())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	bScriptedMove = false;
}

void AAbelKeeper::Rejoin()
{
	if (APawn* Player = Boss->GetFightPlayer())
	{
		AlertTo(Player);
	}
}

bool AAbelKeeper::IsFreeForMoment() const
{
	// His entrance is cut short by any moment his phase asks for.
	return !IsDead() && Boss->IsFighting() && (Move == EAbelMove::None || Move == EAbelMove::Intro) && !IsLunging() && !IsPhasing()
		&& GetCreatureState() != ECreatureState::Attack && !Boss->IsUntargetable();
}

float AAbelKeeper::SunsetYaw() const
{
	return static_cast<float>(GetSunsetDirection().Rotation().Yaw);
}

void AAbelKeeper::FaceYaw(float Yaw, float DeltaSeconds, float DegreesPerSecond)
{
	const FRotator Now = GetActorRotation();
	SetActorRotation(FMath::RInterpConstantTo(Now, FRotator(0.f, Yaw, 0.f), DeltaSeconds, DegreesPerSecond));
}

bool AAbelKeeper::Grieve()
{
	if (!IsFreeForMoment())
	{
		return false;
	}
	// He stops and turns to the sunset, his lantern lowered: his coal open for its seconds.
	BeginMove(EAbelMove::Grieve);
	HoldStill(false);
	UE_LOG(LogLooter, Verbose, TEXT("%s grieves: his coal is open."), *GetActorNameOrLabel());
	return true;
}

bool AAbelKeeper::StartFlare()
{
	if (!IsFreeForMoment() || !Boss->GetFightPlayer())
	{
		return false;
	}
	// The tell: his ghost light flares where he raises it, a full second before the shot.
	BeginMove(EAbelMove::Flare);
	HoldStill(false);
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ShowCharge(this, AbelPoses::FlareGlobe() - FVector(0.0, 0.0, ModelOverMiddle), FlareSeconds, FlareGlow, FlareColor);
	}
	return true;
}

void AAbelKeeper::ReleaseBuckshot()
{
	// From the pump's muzzle where it is in the shot, at the player: slow, pale pellets in a cone.
	FBossVolley Volley = Buckshot;
	Volley.WindupSeconds = 0.f;
	if (Pump && Pump->GetStaticMesh() && Pump->DoesSocketExist(MuzzleSocket))
	{
		Volley.Muzzle = GetActorTransform().InverseTransformPosition(Pump->GetSocketLocation(MuzzleSocket));
	}
	Boss->FireVolley(Volley);
	// The spectral pump's report: a shotgun's, slower and deeper, as a ghost's would be.
	LooterSound::PlayAt(this, LooterSoundCue::ShotgunFire, GetActorTransform().TransformPosition(Volley.Muzzle), 1.f, 0.75f);
}

bool AAbelKeeper::DriftOut()
{
	if (IsDead())
	{
		return false;
	}
	// Out over the canyon, into the fog, whatever he was doing; the deck's lanterns go out behind him.
	EndMove();
	BeginMove(EAbelMove::DriftOut);
	HoldStill(true);
	for (AKeeperLanternPost* Post : LanternPosts)
	{
		if (Post)
		{
			Post->SetDark(true);
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::AbelLanternsOut, GetHome().GetLocation());
	UE_LOG(LogLooter, Log, TEXT("%s drifts out into the fog over the canyon; the lanterns go dark."), *GetActorNameOrLabel());
	return true;
}

bool AAbelKeeper::WalkOff()
{
	if (!IsFreeForMoment())
	{
		return false;
	}
	// Toward the deck's open end, into the wind, stopping short of the edge.
	const FVector Sunset = GetSunsetDirection();
	const FVector Here = GetActorLocation();
	const double Along = FVector::DotProduct(Here - GetHome().GetLocation(), Sunset);
	const double Left = FMath::Max(0.0, (OpenEndDistance - 120.0) - Along);
	if (Left < 30.0)
	{
		Pull();
		return true;
	}
	BeginMove(EAbelMove::WalkOff);
	HoldStill(true);
	MoveTo = Here + Sunset * Left;
	UE_LOG(LogLooter, Log, TEXT("%s walks off into the wind."), *GetActorNameOrLabel());
	return true;
}

void AAbelKeeper::Pull()
{
	if (IsDead())
	{
		return;
	}
	// The dark saint pulls him back from the edge: a yank toward the deck's middle, then a stun with his coal open.
	EndMove();
	BeginMove(EAbelMove::Pulled);
	HoldStill(true);
	const FVector Sunset = GetSunsetDirection();
	const double Along = FVector::DotProduct(GetActorLocation() - GetHome().GetLocation(), Sunset);
	const double Back = FMath::Max(Along - PullDistance * GetSizeScale(), -OpenEndDistance * 0.5);
	MoveTo = GetActorLocation() + Sunset * (Back - Along);
	UE_LOG(LogLooter, Log, TEXT("%s: the dark saint pulls him back; stunned, his coal open."), *GetActorNameOrLabel());
}

FVector AAbelKeeper::DragTarget() const
{
	return FMath::Lerp(GetFogSpot(), GetHome().GetLocation(), GetDragShare());
}

int32 AAbelKeeper::NumLitLanterns() const
{
	int32 Lit = 0;
	for (const AKeeperLanternPost* Post : LanternPosts)
	{
		Lit += Post && !Post->IsDark() ? 1 : 0;
	}
	return Lit;
}

float AAbelKeeper::GetDragShare() const
{
	int32 Total = 0;
	for (const AKeeperLanternPost* Post : LanternPosts)
	{
		Total += Post ? 1 : 0;
	}
	return AbelRules::DragShare(NumLitLanterns(), Total);
}

void AAbelKeeper::HandleLanternRelit(AKeeperLanternPost& Post)
{
	if (!IsOutInFog())
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %d of %d lanterns relit; the light drags him back."), *GetActorNameOrLabel(), NumLitLanterns(), LanternPosts.Num());
	if (GetDragShare() >= 1.f && Move != EAbelMove::DragBack)
	{
		BeginMove(EAbelMove::DragBack);
		HoldStill(true);
	}
}

void AAbelKeeper::LightAllLanterns()
{
	for (AKeeperLanternPost* Post : LanternPosts)
	{
		if (Post)
		{
			Post->SetDark(false);
		}
	}
}

void AAbelKeeper::ApplyPace(bool bFaster)
{
	if (!bPaceCaptured)
	{
		return;
	}
	bHastened = bFaster;
	AttackCooldown = OwnCooldown * (bFaster ? Pace.Cooldown : 1.f);
	AttackWindup = OwnWindup * (bFaster ? Pace.Windup : 1.f);
	ChaseSpeed = OwnChase * (bFaster ? Pace.Chase : 1.f);
	LungeSpeed = OwnLunge * (bFaster ? Pace.Lunge : 1.f);
}

void AAbelKeeper::RingTheBell()
{
	// The chapel's bell, across the valley; the missions don't hear it (nobody rang it).
	for (TActorIterator<AChapelBell> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(AChapelBell::BellTag))
		{
			It->Ring(nullptr);
		}
	}
}

void AAbelKeeper::RiseNewAdds()
{
	if (!Boss->IsFighting())
	{
		return;
	}
	KnownAdds.RemoveAll([](const TWeakObjectPtr<ACreatureBase>& Each) { return !Each.IsValid(); });
	for (ACreatureBase* Add : Boss->GetAliveAdds())
	{
		if (KnownAdds.Contains(Add))
		{
			continue;
		}
		KnownAdds.Add(Add);
		// They rise through the boards: faded in where they stand, and lit by his lantern as the deck is.
		if (AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Add))
		{
			Unpaid->RiseIn();
		}
		AbelRules::LetGhostLightReach(*Add);
	}
}

void AAbelKeeper::Say(const FStoryLine& Line) const
{
	if (Line.Text.IsEmpty())
	{
		return;
	}
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->Play({ Line }, ECaptionPlay::Queue);
	}
}

void AAbelKeeper::TryWantedMoments(float DeltaSeconds)
{
	auto Try = [DeltaSeconds](bool& bWanted, float& Waited, TFunctionRef<bool()> Start)
	{
		if (!bWanted)
		{
			return false;
		}
		Waited += DeltaSeconds;
		if (Start())
		{
			bWanted = false;
			return true;
		}
		bWanted = Waited <= MomentPatience;
		return false;
	};
	if (Try(bWalkOffWanted, WalkOffWaited, [this]() { return WalkOff(); }))
	{
		return;
	}
	if (Try(bGrieveWanted, GrieveWaited, [this]() { return Grieve(); }))
	{
		return;
	}
	Try(bBuckshotWanted, BuckshotWaited, [this]() { return StartFlare(); });
}
