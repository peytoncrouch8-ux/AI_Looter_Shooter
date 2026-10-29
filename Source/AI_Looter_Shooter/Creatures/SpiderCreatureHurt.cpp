// ASpiderCreature's hit feel (HITS, 2026-10-28: "enemy hit reactions you can feel", Docs/Work/2026-10-28-Core/HITS.md), on top
// of the jolt and tilt SpiderCreatureReact.cpp gives every hit: each hit twists the body by where it landed off its middle;
// a leg shot buckles that leg (SpiderCreatureLegs.cpp poses it) and rolls the body onto it; a head crit snaps the nose up and
// splays the fangs; an abdomen hit kicks and wobbles the abdomen and dribbles goo; a Big hit skids the far feet along the
// shot; a stagger crouches it (body down, nose up, feet splayed, fangs spread) with one hiss-shriek; badly hurt it limps on
// its most-shot leg and, worst hurt, drips; and its death goes by the killing hit (SpiderCreatureDeath.cpp plays it).
//
// Looks only: nothing here changes its speed, damage, attack timing or brain (Main's call for the user, HITS Q1, and the
// user's 10-28 "leave them" for the spiders' fight). Every layer has its switch (Looter.Hits.Zones, .StaggerPose, .Wounds,
// .KillStyles) and Looter.Hits.SpiderScale sizes them all (0: today's spider). The Gravemother (a boss) keeps the zone
// looks only: no stagger pose, limp, drip or kill style (her death is her show).

#include "Creatures/SpiderCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitPose.h"
#include "Combat/HitReaction.h"
#include "Creatures/CreatureHitCues.h"
#include "Creatures/CreatureHitZones.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/WeaponFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// --- Every hit (strength 1 = a full flinch; cm and degrees at size 1) ---

	/** A hit twists the body this far (degrees) per TwistLever cm its push passes off the body's middle, at most TwistMost. */
	constexpr float TwistPerLever = 8.f;
	constexpr float TwistLever = 40.f;
	constexpr float TwistMost = 20.f;
	/** The twist (and a leg's roll and dip) holds this long before it fades on the jolt's spring. */
	constexpr float TwistHold = 0.05f;

	// --- By zone ---

	/** A leg shot rolls the body onto that leg this far (at most LegRollMost) and dips it this far; chitin chips fly off it. */
	constexpr float LegRoll = 5.f;
	constexpr float LegRollMost = 12.f;
	constexpr float LegDip = 4.f;
	constexpr int32 LegChips = 3;
	/** FWeaponFX's sizes (cm at size 1): a leg's chips are a leg's (SpawnChitinChips reads 100 as the whole body), a goo bit's own. */
	constexpr float LegChipSize = 40.f;
	constexpr float GooBitSize = 4.f;
	/** Its dry snap plays 6 dB under the hit. */
	constexpr float LegCrackVolume = 0.5f;
	/** A head crit snaps the nose up this far (at most HeadSnapMost), held as long as a hit's kick; the fangs splay this far, in and back. */
	constexpr float HeadSnap = 14.f;
	constexpr float HeadSnapMost = 30.f;
	constexpr float HeadSnapHold = 0.06f;
	constexpr float FangSplay = 35.f;
	constexpr float FangSplayIn = 0.08f;
	constexpr float FangSplayOut = 0.3f;
	/** An abdomen hit kicks it this far (at most AbdomenHitMost), then it wobbles this fast this long, this share of the kick; goo dribbles out. */
	constexpr float AbdomenHit = 10.f;
	constexpr float AbdomenHitMost = 26.f;
	constexpr float AbdomenWobbleHz = 7.f;
	constexpr float AbdomenWobbleSeconds = 0.4f;
	constexpr float AbdomenWobbleShare = 0.35f;
	constexpr int32 AbdomenGoo = 2;
	/** From behind, every hit kicks the abdomen this many times as hard as an abdomen hit's (at strength 1, 6 degrees). */
	constexpr float BehindAbdomen = 1.5f;
	constexpr float BehindAbdomenBase = 6.f;
	/** A Big hit skids the feet on the far side this far along the shot. */
	constexpr float BigSkid = 15.f;
	/** A shotgun's pellets land together: within this long of the first, at most two parts kick (each again only harder). */
	constexpr double ZoneBurstWindow = 0.08;
	/** The parts a burst counts (a leg is its index into Legs, 0-7). */
	constexpr int32 PartHead = 8;
	constexpr int32 PartAbdomen = 9;
	constexpr int32 PartWeak = 10;

	// --- The stagger pose, at its full ---
	constexpr float StaggerDrop = 10.f;
	constexpr float StaggerNose = 12.f;
	constexpr float StaggerFang = 25.f;

	// --- Wounds (by its health: under half stage 1, under a quarter stage 2) ---
	/** Worst hurt, a drip of goo from its most-shot part this often, only within DripRange of a player, at most DripBudget a beat world-wide. */
	constexpr float DripEvery = 0.6f;
	constexpr float DripRange = 2500.f;
	constexpr int32 DripBudget = 6;

	// --- Kill styles ---
	/** An overkill throws this many big chitin plates off the wound, sized as a body twice its own (SpawnChitinChips' cm). */
	constexpr int32 OverkillPlates = 4;
	constexpr float OverkillPlateSize = 200.f;

	/** Every age stops counting past here (each look is long over). */
	constexpr float AgeDone = 1.f;

	/** The world's drips this beat (DripBudget a DripEvery): the "at most six creatures dripping at once" rule. */
	double DripBeatStart = -1.0;
	int32 DripsThisBeat = 0;

	bool TakeDrip(const UWorld& World)
	{
		const double Now = World.GetTimeSeconds();
		if (Now < DripBeatStart || Now - DripBeatStart >= DripEvery)
		{
			DripBeatStart = Now;
			DripsThisBeat = 0;
		}
		if (DripsThisBeat >= DripBudget)
		{
			return false;
		}
		++DripsThisBeat;
		return true;
	}

	bool NearAPlayer(const UWorld& World, const FVector& Where, float Range)
	{
		for (FConstPlayerControllerIterator It = World.GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Player = It->Get();
			const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
			if (Pawn && FVector::DistSquared(Pawn->GetActorLocation(), Where) <= FMath::Square(static_cast<double>(Range)))
			{
				return true;
			}
		}
		return false;
	}

	/** The game's effects (FWeaponFX) in a game world, ready; none in a test world or the editor. */
	FWeaponFX* EffectsIn(UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			return nullptr;
		}
		UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** Advances an age that's running (0 or more), and stops it once its look is over. */
	void Age(float& Seconds, float DeltaSeconds)
	{
		if (Seconds >= 0.f)
		{
			Seconds += DeltaSeconds;
			if (Seconds > AgeDone)
			{
				Seconds = -1.f;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// A hit
// ---------------------------------------------------------------------------

int32 ASpiderCreature::LegOfBone(FName Bone) const
{
	if (Bone.IsNone())
	{
		return INDEX_NONE;
	}
	const int32 Found = Legs.IndexOfByPredicate([Bone](const FLeg& Leg) { return Leg.Femur == Bone || Leg.Tibia == Bone; });
	if (Found != INDEX_NONE)
	{
		return Found;
	}
	// A foot's bone (its hull is the tibia's tip): read off its name, in the rig's order.
	const int32 ByName = CreatureHitZones::SpiderLegOf(Bone);
	return Legs.IsValidIndex(ByName) ? ByName : INDEX_NONE;
}

void ASpiderCreature::ReactToHitFeel(const FCreatureHit& Hit, const FVector& HitLocation, float Strength)
{
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Amount = FMath::Max(static_cast<float>(Feel.SpiderScale), 0.f);
	// The killing hit's look is its death's (OnDied picks the style); a flinch the body plays itself isn't a hit.
	if (!bRigReady || !Hit.bFresh || Hit.bKilled || Amount <= 0.f)
	{
		return;
	}
	// Its wounds: what each part has taken, for the limp's leg and the drip's place.
	if (!Hit.Bone.IsNone())
	{
		HurtByBone.FindOrAdd(Hit.Bone) += Hit.Share;
	}
	const float Hard = Strength * Amount;
	const float Scale = GetSizeScale();
	const FQuat Facing = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Center = BodyFrame.GetLocation();
	FVector Shot = Hit.Direction.GetSafeNormal2D();
	if (Shot.IsNearlyZero())
	{
		Shot = (Center - HitLocation).GetSafeNormal2D();
	}

	// The twist: the push's turn about the body's upright through its middle (its lever off the middle, in its own frame at
	// size 1: a shot into its right flank from ahead turns it right, into its left from behind turns it right too).
	const FVector Arm = Facing.UnrotateVector(HitLocation - Center) / Scale;
	const FVector Push = Facing.UnrotateVector(Shot);
	const float Lever = static_cast<float>(Arm.X * Push.Y - Arm.Y * Push.X);
	const float Twist = FMath::Clamp(TwistPerLever * Hard * Lever / TwistLever, -TwistMost, TwistMost);
	if (FMath::Abs(Twist) >= FMath::Abs(static_cast<float>(HitTwist.Kick.X)))
	{
		HitTwist.Kick.X = Twist;
		HitTwist.Hold = FMath::Max(HitTwist.Hold, TwistHold);
	}

	const bool bZones = Feel.Zones;
	const ECreatureHitZone Zone = bZones ? Hit.Zone : ECreatureHitZone::Body;
	// From behind, the abdomen takes the shove (the body's own lurch forward is OnHurt's).
	if (bZones && Hit.Side == EHitSide::Back)
	{
		AbdomenKick = FMath::Max(AbdomenKick, FMath::Min(BehindAbdomenBase * BehindAbdomen * Hard, AbdomenHitMost));
	}

	// The part this hit kicks, and whether its burst still lets it: within one blast at most two parts, each again only harder.
	int32 Part = INDEX_NONE;
	switch (Zone)
	{
	case ECreatureHitZone::Leg: Part = LegOfBone(Hit.Bone); break;
	case ECreatureHitZone::Head: Part = Hit.bCritical ? PartHead : INDEX_NONE; break;
	case ECreatureHitZone::Abdomen: Part = PartAbdomen; break;
	case ECreatureHitZone::Weak: Part = PartWeak; break;
	default: break;
	}
	bool bNewPart = false;
	if (Part != INDEX_NONE)
	{
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		if (Now - ZoneBurstStart > ZoneBurstWindow || Now < ZoneBurstStart)
		{
			ZoneBurstStart = Now;
			ZoneBurstTargets[0] = INDEX_NONE;
			ZoneBurstTargets[1] = INDEX_NONE;
		}
		if (ZoneBurstTargets[0] != Part && ZoneBurstTargets[1] != Part)
		{
			const int32 Free = ZoneBurstTargets[0] == INDEX_NONE ? 0 : (ZoneBurstTargets[1] == INDEX_NONE ? 1 : INDEX_NONE);
			if (Free == INDEX_NONE)
			{
				Part = INDEX_NONE;
			}
			else
			{
				ZoneBurstTargets[Free] = Part;
				bNewPart = true;
			}
		}
	}
	FWeaponFX* Effects = bNewPart ? EffectsIn(GetWorld()) : nullptr;

	if (Part != INDEX_NONE && Part < Legs.Num() && Zone == ECreatureHitZone::Leg)
	{
		// That leg buckles (the stronger buckle stands), the body rolls onto it and dips; chips fly and the chitin snaps.
		FLeg& Leg = Legs[Part];
		if (Hard >= BuckleNow(Leg))
		{
			Leg.BuckleAge = 0.f;
			Leg.BuckleStrength = Hard;
		}
		const float Roll = FMath::Min(LegRoll * Hard, LegRollMost) * Leg.Side;
		if (FMath::Abs(Roll) >= FMath::Abs(static_cast<float>(HitTwist.Kick.Y)))
		{
			HitTwist.Kick.Y = Roll;
			HitTwist.Kick.Z = FMath::Max(static_cast<float>(HitTwist.Kick.Z), LegDip * Hard * Scale);
			HitTwist.Hold = FMath::Max(HitTwist.Hold, TwistHold);
		}
		if (bNewPart)
		{
			if (Effects)
			{
				Effects->SpawnChitinChips(HitLocation, -Shot, LegChipSize * Scale, LegChips);
			}
			if (Voice)
			{
				Voice->Play(FName(CreatureHitCue::SpiderLegCrack), LegCrackVolume);
			}
		}
	}
	else if (Part == PartHead)
	{
		// The nose snaps up (on the jolt's tilt) and the fangs splay; the crit's own pop and Crit.Shell play as ever.
		HurtTiltKick.X = FMath::Max(static_cast<float>(HurtTiltKick.X), FMath::Min(HeadSnap * Hard, HeadSnapMost));
		HurtHold = FMath::Max(HurtHold, HeadSnapHold);
		FangSnapAge = 0.f;
	}
	else if (Part == PartAbdomen)
	{
		const float Kick = FMath::Min(AbdomenHit * Hard * (Hit.Side == EHitSide::Back ? BehindAbdomen : 1.f), AbdomenHitMost);
		AbdomenKick = FMath::Max(AbdomenKick, Kick);
		AbdomenWobbleAge = 0.f;
		AbdomenWobbleSize = Kick;
		if (Effects)
		{
			const FLinearColor Goo = CreatureBlood::TintOf(this);
			for (int32 Bit = 0; Bit < AbdomenGoo; ++Bit)
			{
				Effects->SpawnGooDrip(HitLocation, Goo, GooBitSize * Scale);
			}
		}
	}
	else if (Part == PartWeak)
	{
		// The Spitter's sac: it wobbles the sac and drips venom by this (SpitterSpider.cpp).
		WeakHitAge = 0.f;
	}

	// A Big hit (it shoves the body back: UCreatureHitReactionComponent) skids the feet on the far side along the shot.
	if (Hit.Weight == EHitWeight::Big && !IsBossCreature())
	{
		const bool bFrontHeld = GetCreatureState() == ECreatureState::Attack || (bPoseOverride && PoseOverride.bFrontFeet);
		for (FLeg& Leg : Legs)
		{
			if (!Leg.bStepping && !(bFrontHeld && Leg.Pair == 0) && FVector::DotProduct(Leg.Foot - Center, Shot) > 0.0)
			{
				Leg.Skid = Shot * (BigSkid * Scale);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Its clocks, the pose it adds
// ---------------------------------------------------------------------------

void ASpiderCreature::TickHitFeel(float DeltaSeconds)
{
	for (FLeg& Leg : Legs)
	{
		Age(Leg.BuckleAge, DeltaSeconds);
	}
	Age(FangSnapAge, DeltaSeconds);
	Age(AbdomenWobbleAge, DeltaSeconds);
	Age(WeakHitAge, DeltaSeconds);

	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Amount = FMath::Clamp(static_cast<float>(Feel.SpiderScale), 0.f, 2.f);
	const bool bAlive = !IsDead();
	// The stagger: its pose follows the stagger's own curve (rise, hold, ease back), its cry once as it begins.
	const bool bStaggerPose = Feel.StaggerPose && bAlive && !IsBossCreature() && Amount > 0.f;
	const float Stagger = bStaggerPose ? FMath::Clamp(GetStaggerAlpha(), 0.f, 1.f) : 0.f;
	StaggerShown = Stagger * Amount;
	if (Stagger > 0.f && !bStaggerCried)
	{
		bStaggerCried = true;
		if (Voice)
		{
			Voice->Play(FName(CreatureHitCue::SpiderStagger));
		}
	}
	else if (Stagger <= 0.f)
	{
		bStaggerCried = false;
	}

	// Its wounds by its health, its limp on its most-shot leg.
	const bool bWounds = Feel.Wounds && bAlive && !IsBossCreature() && Health && Health->GetMaxHealth() > 0.f && Amount > 0.f;
	WoundStageNow = bWounds ? WoundStage(Health->GetHealth() / Health->GetMaxHealth()) : 0;
	const int32 Limp = GetLimpLeg();
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		Legs[Index].bLimp = Index == Limp;
	}
	if (WoundStageNow < 2 || DeltaSeconds <= 0.f)
	{
		DripClock = 0.f;
		return;
	}
	// Worst hurt, goo drips from its most-shot part (near a player only, a few creatures at a time world-wide).
	DripClock += DeltaSeconds;
	if (DripClock < DripEvery)
	{
		return;
	}
	DripClock -= DripEvery;
	UWorld* World = GetWorld();
	FName Worst = TEXT("abdomen");
	float Most = 0.f;
	for (const TPair<FName, float>& Part : HurtByBone)
	{
		if (Part.Value > Most)
		{
			Most = Part.Value;
			Worst = Part.Key;
		}
	}
	USkeletalMeshComponent* Body = GetMesh();
	const FVector Where = Body && Body->GetBoneIndex(Worst) != INDEX_NONE ? Body->GetBoneLocation(Worst) : GetActorLocation();
	if (World && NearAPlayer(*World, Where, DripRange) && TakeDrip(*World))
	{
		if (FWeaponFX* Effects = EffectsIn(World))
		{
			Effects->SpawnGooDrip(Where, CreatureBlood::TintOf(this), GooBitSize * GetSizeScale());
		}
	}
}

ASpiderCreature::FHitFeelPose ASpiderCreature::HitFeelPose() const
{
	FHitFeelPose Pose;
	// The twist, a shot leg's roll and dip, on their spring.
	Pose.Yaw = static_cast<float>(HitTwist.Offset.X);
	Pose.Roll = static_cast<float>(HitTwist.Offset.Y);
	Pose.Drop = static_cast<float>(HitTwist.Offset.Z);
	// The stagger's crouch: body down, nose up, fangs spread (its feet splay: PoseGaitLeg). Dead, its death took it over.
	Pose.Drop += StaggerDrop * GetSizeScale() * StaggerShown;
	Pose.Pitch = StaggerNose * StaggerShown;
	Pose.FangAtLeast = StaggerFang * StaggerShown;
	if (FangSnapAge >= 0.f)
	{
		Pose.FangAdd = FangSplay * HitEnvelope(FangSnapAge, FangSplayIn, FangSplayOut);
	}
	if (AbdomenWobbleAge >= 0.f && AbdomenWobbleAge < AbdomenWobbleSeconds)
	{
		Pose.Abdomen = AbdomenWobbleSize * AbdomenWobbleShare * FMath::Sin(2.f * UE_PI * AbdomenWobbleHz * AbdomenWobbleAge)
			* (1.f - AbdomenWobbleAge / AbdomenWobbleSeconds);
	}
	return Pose;
}

// ---------------------------------------------------------------------------
// Its death's style
// ---------------------------------------------------------------------------

ASpiderCreature::EKillStyle ASpiderCreature::PickKillStyle(const FCreatureHit& Hit) const
{
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	if (!Feel.KillStyles || Feel.SpiderScale <= 0.f || IsBossCreature() || !Hit.bKilled)
	{
		return EKillStyle::Body;
	}
	// A gun's kill only throws it (a burn's, a power's, a test's keeps today's death); a head crit's puts its lights out
	// unless it was far more than it took (then it's thrown).
	const bool bGun = Hit.Kind != EWeaponKind::None && Hit.Element == ELooterElement::None;
	const bool bFarOver = Hit.Overkill >= Feel.OverkillShare;
	if (Hit.bCritical && Hit.Zone == ECreatureHitZone::Head && (!bGun || !bFarOver))
	{
		return EKillStyle::LightsOut;
	}
	if (bGun && Hit.Weight == EHitWeight::Overkill)
	{
		return EKillStyle::Overkill;
	}
	if (bGun && (Hit.Kind == EWeaponKind::Shotgun || Hit.Kind == EWeaponKind::Launcher))
	{
		return EKillStyle::Blast;
	}
	return EKillStyle::Body;
}

void ASpiderCreature::PlayKillStyle(const FCreatureHit& Hit)
{
	// The blast's and overkill's sound (Creature.Hit.Overkill), splats and burst are CreatureKillLook::Play's; the spider's own
	// part is the overkill's plates.
	if (KillStyle != EKillStyle::Overkill)
	{
		return;
	}
	const FVector Center = BodyFrame.GetLocation();
	const FVector Wound = Hit.Location.IsNearlyZero() ? Center : Hit.Location;
	{
		// Big chitin plates blown off the wound, out the far side along the shot.
		if (FWeaponFX* Effects = EffectsIn(GetWorld()))
		{
			FVector Out = Hit.Direction.GetSafeNormal();
			if (Out.IsNearlyZero())
			{
				Out = (Wound - Center).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
			}
			Effects->SpawnChitinChips(Wound, (Out + FVector::UpVector * 0.3).GetSafeNormal(), OverkillPlateSize * GetSizeScale(), OverkillPlates);
		}
	}
}

void ASpiderCreature::ResetHitFeel()
{
	HitTwist = FHitSpring();
	FangSnapAge = -1.f;
	AbdomenWobbleAge = -1.f;
	AbdomenWobbleSize = 0.f;
	WeakHitAge = -1.f;
	ZoneBurstStart = -1000.0;
	ZoneBurstTargets[0] = INDEX_NONE;
	ZoneBurstTargets[1] = INDEX_NONE;
	StaggerShown = 0.f;
	bStaggerCried = false;
	HurtByBone.Reset();
	WoundStageNow = 0;
	DripClock = 0.f;
	KillStyle = EKillStyle::Body;
	for (FLeg& Leg : Legs)
	{
		Leg.BuckleAge = -1.f;
		Leg.BuckleStrength = 0.f;
		Leg.Skid = FVector::ZeroVector;
		Leg.bLimp = false;
	}
}
