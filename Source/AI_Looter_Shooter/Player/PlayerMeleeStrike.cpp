#include "Player/PlayerMeleeComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerMeleeMotion.h"
#include "Weapons/MeleeDamageType.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "World/BreakableKinds.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// UPlayerMeleeComponent's blow: the body it lands on, the damage with its stagger and hit-stop, the knock back, and what
// the player sees, hears and feels of it.

namespace
{
	/** The search ball reaches this far past the reach: a big creature's middle can be well past it while its side is in it. */
	constexpr float SearchMargin = 150.f;
	/** A wall is knocked within this share of the reach along the look; anything facing up more than this is a floor. */
	constexpr float WallReachShare = 0.9f;
	constexpr float WallMaxUp = 0.7f;
	/** A knocked creature must come down on ground: looked for this far over and under where its hop lands (full-size cm). */
	constexpr float LandingAbove = 100.f;
	constexpr float LandingBelow = 250.f;

	/**
	 * A boss: Boss rank, or a creature a boss fight is built round (its UBossComponent: the Gravemother). It takes the
	 * damage only: its fight's show runs its big moments (as UCreatureHitReactionComponent leaves it alone).
	 */
	bool IsBoss(const ACreatureBase& Creature)
	{
		return Creature.GetRank() == ECreatureRank::Boss || Creature.FindComponentByClass<UBossComponent>() != nullptr;
	}

	/** The body a strike aims at: a creature's capsule, or the box round anything else's solid parts (a dummy, an egg sac). */
	FMeleeBody BodyOf(const AActor& Actor)
	{
		FMeleeBody Body;
		const ACharacter* Character = Cast<ACharacter>(&Actor);
		if (const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr)
		{
			Body.Center = Capsule->GetComponentLocation();
			Body.HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			Body.Radius = Capsule->GetScaledCapsuleRadius();
			return Body;
		}
		FVector Origin;
		FVector Extent;
		Actor.GetActorBounds(true, Origin, Extent);
		Body.Center = Origin;
		Body.HalfHeight = static_cast<float>(Extent.Z);
		Body.Radius = static_cast<float>(FMath::Max(Extent.X, Extent.Y));
		return Body;
	}

	/** No wall, rock or ground between the eye and where the blow lands (grass, volumes and creatures never count). */
	bool CanReach(const UWorld& World, const FVector& Eye, const FVector& Point, const AActor* Attacker, const AActor* Target)
	{
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("MeleeStrike"), Attacker, false);
		Params.AddIgnoredActor(Target);
		FHitResult Block;
		return !World.LineTraceSingleByObjectType(Block, Eye, Point, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	}

	/** Ground under Point (terrain or a solid prop) for a body of Size to land on. */
	bool HasGroundAt(const UWorld& World, const FVector& Point, float Size, const AActor* Ignored)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("MeleeKnockLanding"), Ignored);
		FHitResult Ground;
		return World.LineTraceSingleByObjectType(Ground, Point + FVector(0.f, 0.f, LandingAbove * Size),
			Point - FVector(0.f, 0.f, LandingBelow * Size), FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	}

	/** The blow's dust, sparks or splash, drawn as a bullet's impact is. */
	void SpawnBlowImpact(UWorld& World, const FVector& Point, const FVector& Normal, const FVector& Direction, EImpactSurface Surface)
	{
		if (UBulletSubsystem* Bullets = World.GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(&World);
			Effects.SpawnImpact(Point, Normal, Direction, Surface, false);
		}
	}
}

// ---------------------------------------------------------------------------
// Finding the body
// ---------------------------------------------------------------------------

AActor* UPlayerMeleeComponent::FindStrikeTarget(const UWorld* World, const FMeleeAim& Aim, float Reach, const AActor* Attacker, FMeleeContact& OutContact)
{
	if (!World || Reach <= 0.f)
	{
		return nullptr;
	}
	// Everything with a body near enough: pawns (creatures) and dynamic things (dummies, egg sacs).
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeStrike), false, Attacker);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Aim.Eye, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Reach + SearchMargin), Params);

	AActor* Best = nullptr;
	TArray<const AActor*, TInlineAllocator<16>> Weighed;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == Attacker || Weighed.Contains(Actor))
		{
			continue;
		}
		Weighed.Add(Actor);
		// Something that can be hurt and isn't down; never another player.
		const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
		const APawn* Pawn = Cast<APawn>(Actor);
		if (!Health || Health->IsDead() || (Pawn && Pawn->IsPlayerControlled()))
		{
			continue;
		}
		FMeleeContact Contact;
		if (!FMeleeRules::FindContact(Aim, BodyOf(*Actor), Reach, Contact) || (Best && Contact.Score >= OutContact.Score))
		{
			continue;
		}
		if (CanReach(*World, Aim.Eye, Contact.Point, Attacker, Actor))
		{
			Best = Actor;
			OutContact = Contact;
		}
	}
	return Best;
}

// ---------------------------------------------------------------------------
// The blow
// ---------------------------------------------------------------------------

void UPlayerMeleeComponent::Strike()
{
	bStruck = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FMeleeAim Aim = GetAim();
	// The view leans into the swing at the blow, hit or miss.
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, MeleeMotion::Strike(bArmedSwing));
	}
	FMeleeContact Contact;
	AActor* Target = FindStrikeTarget(World, Aim, FMeleeRules::Reach(), GetOwner(), Contact);
	const bool bHit = Target && LandStrike(*Target, Contact, Aim);
	if (!bHit)
	{
		StrikeWorld(Aim);
	}
	OnMelee.Broadcast(bHit, bHit ? Target : nullptr);
}

bool UPlayerMeleeComponent::LandStrike(AActor& Target, const FMeleeContact& Contact, const FMeleeAim& Aim)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	UHealthComponent* Health = Target.FindComponentByClass<UHealthComponent>();
	UWorld* World = GetWorld();
	if (!Owner || !Health || Health->IsDead() || !World)
	{
		return false;
	}

	// Along the blow: from the eye to where it lands, and across the ground for the knock.
	const FVector Direction = (Contact.Point - Aim.Eye).GetSafeNormal();
	FVector Away = (Contact.Point - Aim.Eye).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = Aim.Forward.GetSafeNormal2D();
	}
	const FHitResult Hit(&Target, Cast<UPrimitiveComponent>(Target.GetRootComponent()), Contact.Point, -Away);
	const float Damage = FMeleeRules::StrikeDamage(GetLevelScale(), FMath::FRand());

	// Looks and sound first, as a bullet's are: the damage below may kill it, and its death burst goes over them. The thud,
	// and on it what the body is made of (a spider's shell cracks, a slime squelches, a dummy rings).
	const EImpactSurface Surface = UBulletSubsystem::SurfaceOf(&Target);
	SpawnBlowImpact(*World, Contact.Point, -Away, Direction, Surface);
	const float Pitch = bArmedSwing ? 1.f : MeleeMotion::FistSoundPitch;
	LooterSound::PlayAt(this, MeleeCue::Hit, Contact.Point, 1.f, Pitch);
	const FName CreatureLayer = HitCueOf(&Target);
	const FName Layer = CreatureLayer.IsNone() ? UBulletSubsystem::ImpactCueOf(Hit, Surface) : CreatureLayer;
	if (!Layer.IsNone())
	{
		LooterSound::PlayAt(this, Layer, Contact.Point);
	}

	{
		// The creature's hit reactions hear the damage through its health, which doesn't say what dealt it: the scope tells
		// them it's a strike, so it staggers and stops for an instant whatever its share.
		FHitReaction::FMeleeScope MeleeHit;
		UGameplayStatics::ApplyPointDamage(&Target, Damage, Direction, Hit, Owner->GetController(), Owner, UMeleeDamageType::StaticClass());
	}
	const bool bKilled = Health->IsDead();
	// A corpse goes with the kill's own knock (UCreatureHitReactionComponent); a living one hops back out of reach.
	const FVector Knock = bKilled ? FVector::ZeroVector : KnockBack(Target, Away);

	// The hit marker: with a gun in hand, the gun tells of it as of any of its shots (the HUD listens to the gun in hand).
	if (AWeaponBase* Gun = SwingWeapon.Get())
	{
		Gun->NotifyBulletHit(Hit, Damage, false);
	}
	OnMeleeHit.Broadcast(Hit, Damage, false);

	// A crate or a barrel that breaks under the blow is felt as a hit, not as a creature's kill (the hard shake).
	const bool bFeltAsKill = bKilled && !Target.ActorHasTag(FName(LooterBreakables::Tag));
	FeelBlow(MeleeMotion::Impact(bFeltAsKill, KickRandom.FRandRange(-1.f, 1.f)), bFeltAsKill ? MeleeMotion::KillShake : MeleeMotion::ImpactShake,
		FMeleeRules::AttackerHitStop);
	UE_LOG(LogLooter, Verbose, TEXT("Melee: %s hit %s for %.1f%s (knocked %.0f cm/s)"), bArmedSwing ? TEXT("stock") : TEXT("fist"),
		*Target.GetName(), Damage, bKilled ? TEXT(", killed") : TEXT(""), Knock.Size2D());
	return true;
}

void UPlayerMeleeComponent::StrikeWorld(const FMeleeAim& Aim)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Only a wall, a rock or a post just ahead: looking down at the ground isn't striking it.
	const FVector End = Aim.Eye + Aim.Forward.GetSafeNormal() * (FMeleeRules::Reach() * WallReachShare);
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("MeleeWall"), GetOwner());
	FHitResult Wall;
	if (!World->LineTraceSingleByObjectType(Wall, Aim.Eye, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
		|| Wall.ImpactNormal.Z > WallMaxUp)
	{
		return;
	}
	SpawnBlowImpact(*World, Wall.ImpactPoint, Wall.ImpactNormal, Aim.Forward, EImpactSurface::World);
	const FName Surface = UBulletSubsystem::ImpactCueOf(Wall, EImpactSurface::World);
	if (!Surface.IsNone())
	{
		LooterSound::PlayAt(this, Surface, Wall.ImpactPoint, MeleeMotion::WallVolume);
	}
	LooterSound::PlayAt(this, MeleeCue::Hit, Wall.ImpactPoint, MeleeMotion::WallVolume, bArmedSwing ? 1.f : MeleeMotion::FistSoundPitch);
	FeelBlow(MeleeMotion::WallKnock(), 0.f, FMeleeRules::AttackerHitStop * 0.6f);
}

void UPlayerMeleeComponent::FeelBlow(const FViewKick& Kick, float Shake, float Seconds)
{
	// The swing stops dead where it struck for an instant (its own clock holds), and comes straight back from there.
	HoldTime = FMath::Max(HoldTime, Seconds);
	bLanded = true;
	APlayerController* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	UCameraShakeModifier::Kick(Player, Kick);
	UCameraShakeModifier* Modifier = Shake > 0.f ? UCameraShakeModifier::FindOrAdd(Player) : nullptr;
	if (Modifier)
	{
		Modifier->AddShake(Shake, MeleeMotion::ShakeSeconds);
	}
}

// ---------------------------------------------------------------------------
// The knock and the sound
// ---------------------------------------------------------------------------

FVector UPlayerMeleeComponent::KnockBack(AActor& Target, const FVector& Away)
{
	ACreatureBase* Creature = Cast<ACreatureBase>(&Target);
	UCharacterMovementComponent* Movement = Creature ? Creature->GetCharacterMovement() : nullptr;
	const UWorld* World = Target.GetWorld();
	// A boss takes the damage only; a corpse goes with the kill's knock; one held back by a spell is walking home.
	if (!Creature || !Movement || !World || Creature->IsDead() || Creature->IsPassive() || IsBoss(*Creature))
	{
		return FVector::ZeroVector;
	}
	FVector Launch = FMeleeRules::KnockVelocity(Away, Creature->GetSizeScale(), Creature->GetRank(), Creature->IsA<ASlimeCreature>());
	if (Launch.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	// Where the hop comes down: there must be ground there, else half as far, else it only takes the stagger. Never off an
	// island's edge or into a pit it can't climb out of.
	const float Gravity = FMath::Max(-Movement->GetGravityZ(), 1.f);
	const float AirTime = 2.f * static_cast<float>(Launch.Z) / Gravity;
	const FVector Across(Launch.X * AirTime, Launch.Y * AirTime, 0.f);
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	float Share = 0.f;
	for (const float Try : { 1.f, 0.5f })
	{
		if (HasGroundAt(*World, Creature->GetActorLocation() + Across * Try, Size, Creature))
		{
			Share = Try;
			break;
		}
	}
	if (Share <= 0.f)
	{
		return FVector::ZeroVector;
	}
	Launch.X *= Share;
	Launch.Y *= Share;
	// Its own speed is replaced, not added to: a lunge coming at the player is turned round, which is the point.
	Creature->LaunchCharacter(Launch, true, true);
	return Launch;
}

FName UPlayerMeleeComponent::HitCueOf(const AActor* Target)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(Target);
	if (!Creature)
	{
		return NAME_None;
	}
	// The body as its death shows it: a spider's shell, a slime's gel, and anything else is flesh.
	switch (UCreatureHitReactionComponent::DeathBurstOf(*Creature))
	{
	case EDeathBurst::Shell: return MeleeCue::HitShell;
	case EDeathBurst::Gel: return MeleeCue::HitGel;
	default: return MeleeCue::HitFlesh;
	}
}
