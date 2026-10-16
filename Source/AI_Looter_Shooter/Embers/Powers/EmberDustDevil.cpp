#include "Embers/Powers/EmberDustDevil.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Embers/EmberDamageType.h"
#include "Embers/EmberDefinition.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerViewComponent.h"
#include "Player/SlideDust.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	const FName FlingMinName(TEXT("FlingMin"));
	const FName FlingMaxName(TEXT("FlingMax"));
	const FName DizzyName(TEXT("Dizzy"));
	const FName FovKickName(TEXT("FovKick"));

	/** How high over or under the path a creature may stand and still be caught in it (cm). */
	constexpr float ReachHeight = 220.f;
	/** Speed lines a second while it dashes, how bright, and how far round the eye they rush past (cm). */
	constexpr float LinesPerSecond = 80.f;
	constexpr float LineGlow = 7.f;
	constexpr float LineNear = 70.f;
	constexpr float LineFar = 190.f;
	/** A puff of the trail every so far along the ground (cm). */
	constexpr float TrailEvery = 110.f;

	float Ease(float U)
	{
		const float X = FMath::Clamp(U, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** Dust in the light where it's thrown (the puffs are unlit), as the creatures' and the slide's is. */
	FLinearColor DustColor(const UObject* WorldContext)
	{
		UCreatureReactSubsystem* Reacts = WorldContext ? UCreatureReactSubsystem::Get(WorldContext) : nullptr;
		return FSlideDust::DustAlbedo(ESlideGround::Dirt) * (Reacts ? Reacts->GetDustLight() : FLinearColor(0.8f, 0.8f, 0.8f));
	}

	APlayerController* PlayerOf(const ACharacter* Character)
	{
		return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	}
}

// ---------------------------------------------------------------------------
// The path
// ---------------------------------------------------------------------------

FVector FDustDevilPath::At(float Distance) const
{
	if (Points.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	float Left = FMath::Max(Distance, 0.f);
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const float Piece = static_cast<float>(FVector::Dist(Points[Index - 1], Points[Index]));
		if (Left <= Piece && Piece > 0.f)
		{
			return FMath::Lerp(Points[Index - 1], Points[Index], Left / Piece);
		}
		Left -= Piece;
	}
	return Points.Last();
}

FVector UEmberDustDevil::DashDirection(const ACharacter& Character)
{
	// The keys held, else the way the body is already going, else the way the player looks.
	FVector Way = Character.GetLastMovementInputVector();
	Way.Z = 0.0;
	if (Way.SizeSquared() < 0.01)
	{
		const UCharacterMovementComponent* Movement = Character.GetCharacterMovement();
		FVector Going = Movement ? Movement->Velocity : FVector::ZeroVector;
		Going.Z = 0.0;
		if (Going.SizeSquared() > FMath::Square(150.0))
		{
			Way = Going;
		}
	}
	if (Way.SizeSquared() < 0.01)
	{
		const AController* Controller = Character.GetController();
		const FRotator Look = Controller ? Controller->GetControlRotation() : Character.GetActorRotation();
		Way = FRotator(0.f, Look.Yaw, 0.f).Vector();
	}
	return Way.GetSafeNormal2D();
}

FDustDevilPath UEmberDustDevil::PlanDash(const ACharacter& Character, const FVector& Direction, float Range)
{
	FDustDevilPath Plan;
	const FVector Start = Character.GetActorLocation();
	Plan.Points.Add(Start);
	Plan.Direction = FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	if (Plan.Direction.IsNearlyZero())
	{
		Plan.Direction = FRotator(0.f, Character.GetActorRotation().Yaw, 0.f).Vector();
	}
	const UWorld* World = Character.GetWorld();
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Character.GetCharacterMovement();
	if (!World || !Capsule || Range <= 0.f)
	{
		return Plan;
	}
	const float BodyRadius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float StepUp = Movement ? Movement->MaxStepHeight : 45.f;
	const float WalkableZ = Movement ? Movement->GetWalkableFloorZ() : 0.7f;
	const APlayableArea* Area = APlayableArea::Find(World);

	// The ground: world-static (terrain and props), never a volume or the playable area's walls.
	const FCollisionQueryParams GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("DustDevilGround"), &Character);
	// The way: whatever stops the player walking stops the dash (walls, rocks, trees, the playable area's walls), but not
	// creatures (the dash goes through them, flinging them aside) or loose bodies.
	FCollisionQueryParams SweepParams(SCENE_QUERY_STAT(DustDevilSweep), false, &Character);
	TArray<AActor*> Carried;
	Character.GetAttachedActors(Carried, true, true);
	SweepParams.AddIgnoredActors(Carried);
	FCollisionResponseParams Response(Capsule->GetCollisionResponseToChannels());
	Response.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	Response.CollisionResponse.SetResponse(ECC_PhysicsBody, ECR_Ignore);
	Response.CollisionResponse.SetResponse(ECC_Destructible, ECR_Ignore);
	// Lifted by half a step, so the ground's small bumps don't stop it; a step's rise is the walk's to climb.
	const FVector Lift(0.0, 0.0, StepUp * 0.5f);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(BodyRadius * 0.9f, FMath::Max(HalfHeight - StepUp * 0.5f, BodyRadius));

	// On the ground (or just over it): it follows the ground. High in the air: it keeps its height.
	const float StartFeet = static_cast<float>(Start.Z) - HalfHeight;
	FHitResult Under;
	Plan.bOnGround = (Movement && Movement->IsMovingOnGround())
		|| World->LineTraceSingleByObjectType(Under, FVector(Start.X, Start.Y, StartFeet + StepUp), FVector(Start.X, Start.Y, StartFeet - 60.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams);

	FVector Last = Start;
	float Covered = 0.f;
	const int32 Steps = FMath::CeilToInt(Range / StepLength);
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Along = FMath::Min(Step * StepLength, Range);
		FVector Next = Start + Plan.Direction * Along;
		Next.Z = Last.Z;
		if (Area && !Area->Contains(Next))
		{
			break;
		}
		if (Plan.bOnGround)
		{
			// Up or down what a walk could: a step's rise, or a walkable slope over this step's length. Past it (a ledge,
			// a drop, a cliff) the dash stops at the edge.
			const float Rise = StepUp + (Along - Covered);
			const float LastFeet = static_cast<float>(Last.Z) - HalfHeight;
			FHitResult Ground;
			if (!World->LineTraceSingleByObjectType(Ground, FVector(Next.X, Next.Y, LastFeet + Rise), FVector(Next.X, Next.Y, LastFeet - Rise),
					FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams)
				|| Ground.bStartPenetrating || Ground.ImpactNormal.Z < WalkableZ)
			{
				break;
			}
			Next.Z = Ground.ImpactPoint.Z + HalfHeight + 2.0;
		}
		FHitResult Block;
		if (World->SweepSingleByChannel(Block, Last + Lift, Next + Lift, FQuat::Identity, Capsule->GetCollisionObjectType(), Shape, SweepParams,
				Response)
			&& !Block.bStartPenetrating)
		{
			// Stopped short of a wall: as far as the sweep got, if that's anywhere.
			const FVector Reached = FMath::Lerp(Last, Next, FMath::Max(Block.Time - 0.05f, 0.f));
			if (FVector::Dist(Reached, Last) > 5.0)
			{
				Plan.Points.Add(Reached);
			}
			break;
		}
		Plan.Points.Add(Next);
		Covered = Along;
		Last = Next;
	}
	for (int32 Index = 1; Index < Plan.Points.Num(); ++Index)
	{
		Plan.Length += static_cast<float>(FVector::Dist(Plan.Points[Index - 1], Plan.Points[Index]));
	}
	return Plan;
}

// ---------------------------------------------------------------------------
// Using it
// ---------------------------------------------------------------------------

EEmberDenied UEmberDustDevil::CanActivate(const FEmberContext& Context) const
{
	const ACharacter* Character = Context.Character;
	if (!Character)
	{
		return EEmberDenied::Refused;
	}
	if (bDashing)
	{
		return EEmberDenied::Active;
	}
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	// A mantle or vault has the movement in its own mode: the body is spoken for.
	if (!Movement || Movement->MovementMode == MOVE_None || Movement->MovementMode == MOVE_Custom)
	{
		return EEmberDenied::Busy;
	}
	const float Range = Context.Definition && Context.Definition->Range > 0.f ? Context.Definition->Range : 900.f;
	// Facing a wall or the edge of a drop: no room to dash, so the press does nothing (the socket shakes).
	return PlanDash(*Character, DashDirection(*Character), Range).Length >= MinDash ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberDustDevil::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Character || !World)
	{
		return;
	}
	// The definition's numbers (the asset holds them), with the design's as a fallback.
	const UEmberDefinition* Def = Context.Definition;
	Damage = (Def && Def->Damage > 0.f ? Def->Damage : 15.f) * FMath::Max(Context.LevelScale, 0.f);
	const float Range = Def && Def->Range > 0.f ? Def->Range : 900.f;
	Radius = Def && Def->Radius > 0.f ? Def->Radius : 260.f;
	FlingMin = Def ? Def->Extra(FlingMinName, 400.f) : 400.f;
	FlingMax = FMath::Max(FlingMin, Def ? Def->Extra(FlingMaxName, 500.f) : 500.f);
	Dizzy = Def ? Def->Extra(DizzyName, 1.f) : 1.f;
	FovKick = Def ? Def->Extra(FovKickName, 14.f) : 14.f;
	bComponentTicks = Def && Def->DurationSeconds > 0.f;
	DashSeconds = bComponentTicks ? Def->DurationSeconds : DefaultSeconds;
	if (Def)
	{
		Color = Def->Color;
	}

	Body = Character;
	Instigator = Character->GetController();
	Path = PlanDash(*Character, DashDirection(*Character), Range);
	Struck.Reset();
	SpeedLines.Reset();
	LinesOwed = 0.f;
	DashTime = 0.f;
	Travelled = 0.f;
	TrailLaid = 0.f;
	SettleTime = -1.f;
	bDashing = true;

	// Who may be in its way: every living creature near enough to the path to be reached.
	Candidates.Reset();
	const FVector Start = Character->GetActorLocation();
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), Start) <= Range + Radius + 400.f)
		{
			Candidates.Add(*It);
		}
	}

	// Nothing hurts the player through it (unless something else already made them so: theirs to undo).
	UHealthComponent* Life = Character->FindComponentByClass<UHealthComponent>();
	Health = Life;
	bMadeInvulnerable = Life && !Life->bInvulnerable;
	if (bMadeInvulnerable)
	{
		Life->bInvulnerable = true;
	}
	// The movement stands aside while the body rides the path.
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);
	}

	// Felt at once: the whoosh, the dust thrown up where it leaves, a lean into the way it goes.
	LooterSound::Play2D(Character, LooterSoundCue::Ember::DustDevilUse);
	if (Path.bOnGround)
	{
		SpawnDustRing(FeetAt(Start), 1.f, 16);
	}
	if (APlayerController* Player = PlayerOf(Character))
	{
		const FVector ViewRight = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
		FViewKick Lean;
		Lean.Pitch = -0.5f;
		Lean.Roll = 1.4f * static_cast<float>(FVector::DotProduct(Path.Direction, ViewRight));
		Lean.Frequency = 5.f;
		Lean.Damping = 0.7f;
		UCameraShakeModifier::Kick(Player, Lean);
	}
	UE_LOG(LogLooter, Log, TEXT("Dust Devil: %.0f cm dash (%s), %d creatures near"), Path.Length, Path.bOnGround ? TEXT("on the ground") : TEXT("in the air"),
		Candidates.Num());

	// An instant definition isn't ticked by the component: the dash runs on its own timer.
	if (!bComponentTicks)
	{
		SchedulePump();
	}
}

void UEmberDustDevil::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	if (bComponentTicks)
	{
		StepDash(DeltaSeconds);
	}
}

void UEmberDustDevil::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bDashing)
	{
		return;
	}
	if (bInterrupted)
	{
		// Death, a scene or the level ending: it stops where it is, and the world is left as it was.
		FinishDash(true);
	}
	else if (bComponentTicks)
	{
		// Its time is up: whatever's left of the path, now.
		StepDash(DashSeconds);
	}
	// An instant definition's End comes the moment it's used: its timer carries the dash on.
}

// ---------------------------------------------------------------------------
// The dash, frame by frame
// ---------------------------------------------------------------------------

void UEmberDustDevil::StepDash(float DeltaSeconds)
{
	if (!bDashing)
	{
		return;
	}
	ACharacter* Character = Body.Get();
	if (!Character)
	{
		FinishDash(true);
		return;
	}
	DashTime += FMath::Max(DeltaSeconds, 0.f);
	const float U = DashSeconds > 0.f ? FMath::Clamp(DashTime / DashSeconds, 0.f, 1.f) : 1.f;
	// Fastest as it leaves (a burst), still quick as it arrives: an even pace blended with an ease-out.
	const float Share = 0.6f * U + 0.4f * (1.f - FMath::Square(1.f - U));
	const float Distance = Path.Length * Share;
	const FVector Was = Character->GetActorLocation();
	const FVector Middle = Path.At(Distance);
	Character->SetActorLocation(Middle, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		// For what reads the body's speed (its animation, its footfalls): the dash's own.
		Movement->Velocity = DeltaSeconds > 0.f ? (Middle - Was) / DeltaSeconds : FVector::ZeroVector;
	}
	StrikeAlong(Travelled, Distance);
	TrailDust(Travelled, Distance);
	Travelled = Distance;
	SetViewWidening(FovKick * Ease(DashTime / WidenSeconds));
	TickSpeedLines(DeltaSeconds);
	if (U >= 1.f)
	{
		FinishDash(false);
	}
}

void UEmberDustDevil::FinishDash(bool bAborted)
{
	if (!bDashing)
	{
		return;
	}
	bDashing = false;
	ACharacter* Character = Body.Get();
	if (UHealthComponent* Life = Health.Get(); Life && bMadeInvulnerable)
	{
		Life->bInvulnerable = false;
	}
	bMadeInvulnerable = false;
	if (Character)
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			// Walking on with a little of its speed (or falling, from a dash in the air).
			Movement->SetMovementMode(Path.bOnGround ? MOVE_Walking : MOVE_Falling);
			Movement->Velocity = bAborted ? FVector::ZeroVector : Path.Direction * ExitSpeed;
		}
		if (!bAborted)
		{
			if (Path.bOnGround)
			{
				SpawnDustRing(FeetAt(Character->GetActorLocation()), 0.65f, 9);
			}
			if (APlayerController* Player = PlayerOf(Character))
			{
				FViewKick Arrive;
				Arrive.Pitch = -0.35f;
				Arrive.FieldOfView = -0.01f;
				Arrive.Frequency = 9.f;
				Arrive.Damping = 0.6f;
				UCameraShakeModifier::Kick(Player, Arrive);
			}
		}
	}
	// The view settles back and the lines fly on, on the dash's own timer.
	SettleTime = 0.f;
	SchedulePump();
	UE_LOG(LogLooter, Verbose, TEXT("Dust Devil: dash %s after %.0f cm, %d struck"), bAborted ? TEXT("cut short") : TEXT("done"), Travelled, Struck.Num());
}

void UEmberDustDevil::SchedulePump()
{
	UWorld* World = Body.IsValid() ? Body->GetWorld() : nullptr;
	if (bPumping || !World)
	{
		return;
	}
	bPumping = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEmberDustDevil::Pump));
}

void UEmberDustDevil::Pump()
{
	bPumping = false;
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	// On the player's own clock (Sundown's world crawl doesn't slow the player's dash).
	const float DeltaSeconds = World->GetDeltaSeconds() * FMath::Max(Character->CustomTimeDilation, 0.f);
	if (bDashing)
	{
		if (!bComponentTicks)
		{
			StepDash(DeltaSeconds);
		}
	}
	else
	{
		if (SettleTime >= 0.f)
		{
			SettleTime += DeltaSeconds;
			SetViewWidening(FovKick * (1.f - Ease(SettleTime / SettleSeconds)));
			if (SettleTime >= SettleSeconds)
			{
				SetViewWidening(0.f);
				SettleTime = -1.f;
			}
		}
		TickSpeedLines(DeltaSeconds);
	}
	if (bDashing || SettleTime >= 0.f || !SpeedLines.IsEmpty())
	{
		SchedulePump();
	}
}

// ---------------------------------------------------------------------------
// The creatures in its way
// ---------------------------------------------------------------------------

void UEmberDustDevil::StrikeAlong(float From, float To)
{
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World || To < From)
	{
		return;
	}
	const FVector A = Path.At(From);
	const FVector B = Path.At(To);
	const FCollisionQueryParams SightParams = LooterWorld::StaticGeometryParams(World, TEXT("DustDevilSight"), Character);
	for (const TWeakObjectPtr<ACreatureBase>& Each : Candidates)
	{
		ACreatureBase* Creature = Each.Get();
		if (!Creature || Creature->IsDead() || Struck.Contains(Each))
		{
			continue;
		}
		const FVector Where = Creature->GetActorLocation();
		const FVector Nearest = FMath::ClosestPointOnSegment(Where, A, B);
		if (FVector::Dist2D(Where, Nearest) > Radius || FMath::Abs(Where.Z - Nearest.Z) > ReachHeight)
		{
			continue;
		}
		// Not through a wall: the wind reaches only what the path can see.
		FHitResult Wall;
		if (World->LineTraceSingleByObjectType(Wall, Nearest, Where, FCollisionObjectQueryParams(ECC_WorldStatic), SightParams))
		{
			continue;
		}
		Strike(*Creature, Nearest);
	}
}

void UEmberDustDevil::Strike(ACreatureBase& Creature, const FVector& Nearest)
{
	Struck.Add(&Creature);
	ACharacter* Character = Body.Get();
	UWorld* World = Creature.GetWorld();

	// Flung out to the side of the path it was on (straight in the way: a side of its own), carried a little along.
	FVector Side = Creature.GetActorLocation() - Nearest;
	Side.Z = 0.0;
	// Square to the dash: the nearest path point lies behind a creature struck as the dash passes it, and measured from
	// there the way out was a diagonal (the test saw a fling more along the path than off it).
	Side -= Path.Direction * FVector::DotProduct(Side, Path.Direction);
	if (Side.SizeSquared() < 100.0)
	{
		const float Sign = (GetTypeHash(Creature.GetFName()) & 1u) ? 1.f : -1.f;
		Side = FVector::CrossProduct(FVector::UpVector, Path.Direction) * Sign;
	}
	const FVector Away = (Side.GetSafeNormal() + Path.Direction * 0.2).GetSafeNormal();

	// The damage, the player's (the hit marker, the number, the kill's experience), no gun's.
	UGameplayStatics::ApplyDamage(&Creature, Damage, Instigator.Get(), nullptr, UEmberDamageType::StaticClass());
	if (!Creature.IsDead())
	{
		if (UCreatureKnockComponent* Knock = Creature.GetKnock())
		{
			Knock->Knock(Away, Random.FRandRange(FlingMin, FlingMax), FlingHeight, Dizzy);
		}
	}

	// A burst of dust off it the way it goes, grit, and the blow's weight.
	const FVector Middle = Creature.GetActorLocation();
	LooterSound::PlayAt(&Creature, LooterSoundCue::Melee::Hit, Middle, 0.8f);
	if (FWeaponFX* Effects = EffectsIn(World))
	{
		const FLinearColor Dust = DustColor(&Creature);
		const float Size = FMath::Max(Creature.GetSizeScale(), 0.4f);
		for (int32 Index = 0; Index < 7; ++Index)
		{
			const FVector Out = (Away + Random.GetUnitVector() * 0.6).GetSafeNormal();
			Effects->SpawnDustPuff(Middle + Out * (25.f * Size), Out * Random.FRandRange(220.f, 420.f) + FVector(0.0, 0.0, 60.0), Dust, 0.45f,
				22.f * Size, Random.FRandRange(80.f, 120.f) * Size, Random.FRandRange(0.7f, 1.1f));
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Out = (Away + FVector(0.0, 0.0, 0.8) + Random.GetUnitVector() * 0.5).GetSafeNormal();
			Effects->SpawnGrit(Middle, Out * Random.FRandRange(300.f, 600.f), Random.FRandRange(0.8f, 2.f), Random.FRandRange(0.5f, 0.9f));
		}
	}
	// Each one struck is felt: a small jolt toward it.
	if (APlayerController* Player = PlayerOf(Character))
	{
		const FVector ViewRight = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
		FViewKick Jolt;
		Jolt.Pitch = 0.3f;
		Jolt.Yaw = 0.4f * FMath::Sign(static_cast<float>(FVector::DotProduct(Side, ViewRight)));
		Jolt.Frequency = 12.f;
		Jolt.Damping = 0.6f;
		UCameraShakeModifier::Kick(Player, Jolt);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Dust Devil: struck %s for %.1f"), *Creature.GetName(), Damage);
}

// ---------------------------------------------------------------------------
// The show
// ---------------------------------------------------------------------------

void UEmberDustDevil::SetViewWidening(float Degrees)
{
	Widening = Degrees;
	if (UPlayerViewComponent* View = Body.IsValid() ? Body->FindComponentByClass<UPlayerViewComponent>() : nullptr)
	{
		View->SetPowerFieldOfView(Degrees);
	}
}

FVector UEmberDustDevil::EyeLocation() const
{
	const ACharacter* Character = Body.Get();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const APlayerController* Player = PlayerOf(Character);
	if (Player && Player->PlayerCameraManager)
	{
		return Player->PlayerCameraManager->GetCameraLocation();
	}
	return Character->GetActorLocation() + FVector(0.0, 0.0, Character->BaseEyeHeight);
}

FVector UEmberDustDevil::FeetAt(const FVector& Middle) const
{
	const ACharacter* Character = Body.Get();
	const float HalfHeight = Character && Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return Middle - FVector(0.0, 0.0, HalfHeight);
}

void UEmberDustDevil::SpawnSpeedLine()
{
	// A line of rushing air somewhere ahead and round the eye (never across the crosshair, never into the ground), lying
	// along the dash: the body runs through them, so they streak past.
	const FVector Along = Path.Direction;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal();
	float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
	if (FMath::Sin(Angle) < -0.55f)
	{
		Angle = -Angle;
	}
	const float Out = Random.FRandRange(LineNear, LineFar);
	const FVector Offset = (Right * FMath::Cos(Angle) + FVector::UpVector * FMath::Sin(Angle)) * Out;
	const float Length = Random.FRandRange(140.f, 300.f);
	const FVector Center = EyeLocation() + Along * Random.FRandRange(200.f, 650.f) + Offset;
	FSpeedLine& Line = SpeedLines.AddDefaulted_GetRef();
	Line.Tail = Center - Along * (Length * 0.5f);
	Line.Head = Center + Along * (Length * 0.5f);
	Line.Life = Random.FRandRange(0.16f, 0.24f);
	Line.Width = Random.FRandRange(1.1f, 1.8f);
}

void UEmberDustDevil::TickSpeedLines(float DeltaSeconds)
{
	if (bDashing)
	{
		LinesOwed += DeltaSeconds * LinesPerSecond;
		while (LinesOwed >= 1.f)
		{
			LinesOwed -= 1.f;
			SpawnSpeedLine();
		}
	}
	for (int32 Index = SpeedLines.Num() - 1; Index >= 0; --Index)
	{
		SpeedLines[Index].Age += DeltaSeconds;
		if (SpeedLines[Index].Age >= SpeedLines[Index].Life)
		{
			SpeedLines.RemoveAtSwap(Index);
		}
	}
	FWeaponFX* Effects = SpeedLines.IsEmpty() ? nullptr : EffectsIn(Body.IsValid() ? Body->GetWorld() : nullptr);
	if (!Effects)
	{
		return;
	}
	for (const FSpeedLine& Line : SpeedLines)
	{
		const float Fade = FMath::Clamp(Line.Age / 0.04f, 0.f, 1.f) * (1.f - Line.Age / Line.Life);
		Effects->AddStreak(Line.Tail, Line.Head, Line.Width, Color, LineGlow * Fade);
	}
}

void UEmberDustDevil::TrailDust(float From, float To)
{
	if (!Path.bOnGround || To <= TrailLaid)
	{
		return;
	}
	FWeaponFX* Effects = EffectsIn(Body.IsValid() ? Body->GetWorld() : nullptr);
	if (!Effects)
	{
		TrailLaid = To;
		return;
	}
	const FLinearColor Dust = DustColor(Body.Get());
	// A low, thin trail kicked up where the feet skimmed, curling up behind.
	while (TrailLaid + TrailEvery <= To)
	{
		TrailLaid += TrailEvery;
		const FVector Feet = FeetAt(Path.At(TrailLaid));
		const FVector Drift = -Path.Direction * Random.FRandRange(40.f, 120.f) + FVector(0.0, 0.0, Random.FRandRange(30.f, 70.f));
		Effects->SpawnDustPuff(Feet + FVector(0.0, 0.0, 10.0), Drift, Dust, 0.22f, 20.f, Random.FRandRange(70.f, 95.f), Random.FRandRange(0.7f, 1.0f));
	}
}

void UEmberDustDevil::SpawnDustRing(const FVector& Feet, float Scale, int32 Puffs)
{
	FWeaponFX* Effects = EffectsIn(Body.IsValid() ? Body->GetWorld() : nullptr);
	if (!Effects || Puffs <= 0)
	{
		return;
	}
	const FLinearColor Dust = DustColor(Body.Get());
	// The ring rolling out over the ground...
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		const float Angle = 2.f * UE_PI * (Index + Random.FRandRange(-0.3f, 0.3f)) / Puffs;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const float Start = Random.FRandRange(22.f, 32.f) * Scale;
		Effects->SpawnDustPuff(Feet + Out * (30.f * Scale) + FVector(0.0, 0.0, 10.0 * Scale),
			Out * Random.FRandRange(280.f, 460.f) * Scale + FVector(0.0, 0.0, Random.FRandRange(30.f, 80.f)), Dust, 0.5f, Start,
			Start * Random.FRandRange(3.2f, 4.2f), Random.FRandRange(0.9f, 1.4f));
	}
	// ...grit flicked up off it...
	for (int32 Index = 0; Index < Puffs / 2; ++Index)
	{
		const FVector Out = (Random.GetUnitVector() * FVector(1.0, 1.0, 0.0) + FVector(0.0, 0.0, 1.0)).GetSafeNormal();
		Effects->SpawnGrit(Feet + FVector(0.0, 0.0, 5.0), Out * Random.FRandRange(250.f, 480.f) * Scale, Random.FRandRange(0.8f, 1.8f),
			Random.FRandRange(0.45f, 0.8f));
	}
	// ...and a little whirl of air spinning up off the middle: the dust devil itself.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Angle = 2.f * UE_PI * Index / 5.f;
		const FVector Round(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Spin(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		Effects->SpawnWisp(Feet + Round * (35.f * Scale) + FVector(0.0, 0.0, 20.0 * Scale),
			Spin * (160.f * Scale) + FVector(0.0, 0.0, Random.FRandRange(90.f, 150.f) * Scale), 0.18f, 18.f * Scale, 70.f * Scale,
			Random.FRandRange(0.6f, 0.9f));
	}
}
