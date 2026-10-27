// AGravemotherCreature's hide shrugging hits off (the user's calls, 2026-10-27): "you need to break the tombstone on her back
// before you can damage her", and "I can just sit above the sink and shoot her": shots from off her hunting ground (the
// Sink's rim) do nothing. A shrugged hit never reaches her health (no number, no flinch, no brood call): it glances off
// with a look and sound of its own, her bar calls out why, the headstone glows as the target, and after a few Hob says
// what to do. She still turns on the shooter as a hit would turn her (off her ground: she goes to its edge and watches).
// Also the playtest's health and hit scales (GravemotherTuning).

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherFight.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GravemotherStone.h"
#include "Creatures/GravemotherTuning.h"
#include "Story/CaptionSubsystem.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A shrugged hit's glint (cm across), its glancing sparks, and the mud flaking off her hide (cm, s). */
	constexpr float ShrugFlashSize = 30.f;
	constexpr int32 ShrugSparksMin = 1;
	constexpr int32 ShrugSparksMax = 3;
	constexpr int32 ShrugPuffs = 3;
	/** Its thud isn't played twice within this long (s) and this near (cm): a shotgun's pellets are one thud. */
	constexpr double ShrugSoundGap = 0.06;
	constexpr float ShrugSoundMerge = 250.f;
	/** The ricochet's whine over it no oftener than this (s): a rifle's stream whines now and then, not every round. */
	constexpr double ShrugRicochetGap = 0.35;
	/** Her bar calls a shrug out for this long, and not again within CalloutGap (s): the line under the bar, not a strobe. */
	constexpr float CalloutSeconds = 1.4f;
	constexpr double CalloutGap = 3.0;
	/**
	 * The look grows with the shooter's distance past LookNear (cm) so it keeps about its size on screen, up to LookMax
	 * times (the rim, 20-40 m off: about 1.5-2.5x), and from the rim it throws more sparks.
	 */
	constexpr float LookNear = 1500.f;
	constexpr float LookMax = 3.f;
	/** A rim shot's glance heard by the shooter wherever they are (2D, this loud), and her screech up at them at most this
	 *  often (s): an answer, not a drone under a rifle's stream. */
	constexpr float RimGlanceVolume = 0.55f;
	constexpr double RimScreechGap = 4.0;

	int32 KindIndex(EGravemotherShrug Why)
	{
		return FMath::Clamp(static_cast<int32>(Why), 0, static_cast<int32>(EGravemotherShrug::Count) - 1);
	}
}

void AGravemotherCreature::ApplyTuning()
{
	// The class carries the defaults (the Ledger reads them); a cvar moved from its default moves this one.
	const float HealthRatio = GravemotherTuning::HealthScale() / GravemotherTuning::DefaultHealthScale;
	const float DamageRatio = GravemotherTuning::DamageScale() / GravemotherTuning::DefaultDamageScale;
	if (!FMath::IsNearlyEqual(HealthRatio, 1.f))
	{
		Health->MaxHealth = FMath::Max(1.f, Health->MaxHealth * HealthRatio);
	}
	if (!FMath::IsNearlyEqual(DamageRatio, 1.f))
	{
		AttackDamage *= DamageRatio;
	}
}

// ---------------------------------------------------------------------------
// Which hits
// ---------------------------------------------------------------------------

EGravemotherShrug AGravemotherCreature::ShrugFor(const AController* By) const
{
	if (IsDead())
	{
		return EGravemotherShrug::None;
	}
	// A shooter off her ground (a player on the rim): she can't come for them, so they can't hurt her. Only someone who isn't
	// one of the creatures (her brood never hurts her anyway), and only where she has a ground (her lair gives her one).
	const APawn* Shooter = By ? By->GetPawn() : nullptr;
	if (GravemotherTuning::RimShrug() && Shooter && !Shooter->IsA<ACreatureBase>() && HuntingGround.IsSet())
	{
		// Her ground grown by a slack all round, so a player jumping on a berm or a step up the ramp still counts: only one
		// well off it (the rim, 12 m up) is shrugged.
		FHuntingGround Wider = HuntingGround;
		const float Slack = GravemotherTuning::RimSlack();
		if (Wider.MaxRise > 0.f)
		{
			Wider.MaxRise += Slack;
		}
		if (Wider.Corners.Num() >= 3)
		{
			Wider.Margin += Slack;
		}
		else
		{
			Wider.Radius += Slack;
		}
		if (!Wider.Contains(Shooter->GetActorLocation(), GetHome().GetLocation()))
		{
			return EGravemotherShrug::OffGround;
		}
	}
	if (GravemotherTuning::StoneGuards() && IsStoneOn())
	{
		return EGravemotherShrug::Stone;
	}
	return EGravemotherShrug::None;
}

float AGravemotherCreature::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const EGravemotherShrug Why = DamageAmount > 0.f ? ShrugFor(EventInstigator) : EGravemotherShrug::None;
	if (Why == EGravemotherShrug::None)
	{
		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}
	// Where it landed: a bullet's or a blow's point; a blast's (a grave-salt jar) nearest her middle.
	FVector Where = GetActorLocation();
	FVector Normal = FVector::ZeroVector;
	FVector Along = FVector::ZeroVector;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& Point = static_cast<const FPointDamageEvent&>(DamageEvent);
		Where = !FVector(Point.HitInfo.ImpactPoint).IsZero() ? FVector(Point.HitInfo.ImpactPoint) : FVector(Point.HitInfo.Location);
		Normal = Point.HitInfo.ImpactNormal;
		Along = Point.ShotDirection;
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		const FRadialDamageEvent& Blast = static_cast<const FRadialDamageEvent&>(DamageEvent);
		Along = (GetActorLocation() - Blast.Origin).GetSafeNormal();
		Where = GetActorLocation() - Along * GetSimpleCollisionRadius() * 0.6f;
	}
	if (Where.IsZero())
	{
		Where = GetActorLocation();
	}
	PlayShrugLook(Where, Normal, Along, ShrugLookScale(EventInstigator, Where));
	PlayShrugSound(Where, Why);
	if (Why == EGravemotherShrug::OffGround)
	{
		AnswerRimShot(EventInstigator);
	}
	NoteShrug(Why, EventInstigator);
	UE_LOG(LogLooter, Verbose, TEXT("%s shrugs off %.0f (%s)."), *GetName(), DamageAmount,
		Why == EGravemotherShrug::Stone ? TEXT("her headstone stands") : TEXT("from off her ground"));
	return 0.f;
}

// ---------------------------------------------------------------------------
// What a shrug does
// ---------------------------------------------------------------------------

void AGravemotherCreature::NoteShrug(EGravemotherShrug Why, AController* By)
{
	if (Why == EGravemotherShrug::None || IsDead())
	{
		return;
	}
	++ShrugCounts[KindIndex(Why)];
	// It still gets her attention, as any hit would: on her ground she comes for them (her fight starts); off it, she goes to
	// her ground's edge and watches them up there.
	if (APawn* Shooter = By ? By->GetPawn() : nullptr)
	{
		AlertTo(Shooter);
	}
	// The stone is the target: it glows a moment (Stone only: a rim shooter's answer is to come down, not to aim elsewhere).
	if (Why == EGravemotherShrug::Stone)
	{
		if (AGravemotherStone* Slab = Stone.Get())
		{
			Slab->Mark(GravemotherTuning::MarkSeconds());
		}
	}
	// Her bar (in her fight) says why, now and then, never over a stagger's own callout.
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	UHudBossBarWidget* Bar = Boss && Boss->IsFighting() && !Boss->IsStaggered() ? Boss->GetBar() : nullptr;
	if (Bar && (LastShrugCallout < 0.0 || Now - LastShrugCallout >= CalloutGap))
	{
		LastShrugCallout = Now;
		Bar->ShowCallout(GravemotherFight::ShrugCallout(Why), CalloutSeconds);
	}
	MaybeHint(Why, By);
}

void AGravemotherCreature::MaybeHint(EGravemotherShrug Why, const AController* By)
{
	const int32 After = GravemotherTuning::HintAfter();
	const uint8 Bit = static_cast<uint8>(1u << KindIndex(Why));
	const APawn* Shooter = By ? By->GetPawn() : nullptr;
	// Once a Gravemother, to someone who isn't a creature, after a few: a stray round isn't a lesson yet.
	if (After <= 0 || (HintedKinds & Bit) != 0 || !Shooter || Shooter->IsA<ACreatureBase>() || ShrugCounts[KindIndex(Why)] < After)
	{
		return;
	}
	HintedKinds |= Bit;
	const FStoryLine Line = GravemotherFight::ShrugLine(Why);
	if (UCaptionSubsystem* Captions = !Line.Text.IsEmpty() ? UCaptionSubsystem::Get(this) : nullptr)
	{
		Captions->Play({ Line }, ECaptionPlay::Queue);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: Hob's hint (%s)."), *GetName(), Why == EGravemotherShrug::Stone ? TEXT("break the headstone") : TEXT("come down"));
}

int32 AGravemotherCreature::GetShrugCount(EGravemotherShrug Why) const
{
	return ShrugCounts[KindIndex(Why)];
}

bool AGravemotherCreature::WasHinted(EGravemotherShrug Why) const
{
	return (HintedKinds & static_cast<uint8>(1u << KindIndex(Why))) != 0;
}

float AGravemotherCreature::ShrugLookScale(const AController* By, const FVector& Where) const
{
	const APawn* Shooter = By ? By->GetPawn() : nullptr;
	if (!Shooter)
	{
		return 1.f;
	}
	return FMath::Clamp(static_cast<float>(FVector::Dist(Shooter->GetActorLocation(), Where)) / LookNear, 1.f, LookMax);
}

void AGravemotherCreature::PlayShrugLook(const FVector& Where, const FVector& Normal, const FVector& ShotDirection, float Scale)
{
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	if (!Effects)
	{
		return;
	}
	const float Size = FMath::Max(Scale, 1.f);
	const FVector Along = ShotDirection.GetSafeNormal();
	const FVector Out = !Normal.IsNearlyZero() ? Normal.GetSafeNormal() : (!Along.IsNearlyZero() ? -Along : FVector::UpVector);
	// A glancing spark or two off the shot's bounce: it skipped off, it didn't go in (more of them seen from far off).
	Effects->Sparks(Where, Out, Along.IsNearlyZero() ? -Out : Along,
		FMath::RandRange(ShrugSparksMin, ShrugSparksMax) + FMath::RoundToInt32(Size - 1.f) * 2);
	// A small pale glint, once a frame (a shotgun's pellets glint once).
	if (LastShrugFlashFrame != GFrameCounter)
	{
		LastShrugFlashFrame = GFrameCounter;
		Effects->Flash(Where + Out * 3.f, ShrugFlashSize * Size);
	}
	// Dried grave mud flaking off her hide where it struck, and a crumb of it thrown.
	FGraveDustBurst Mud;
	Mud.Kind = EGraveDust::Earth;
	Mud.Count = ShrugPuffs;
	Mud.Spread = 10.f * Size;
	Mud.SpreadUp = 5.f * Size;
	Mud.StartSize = 20.f * Size;
	Mud.EndSize = 55.f * Size;
	Mud.Life = 0.8f;
	Mud.Opacity = 0.45f;
	Mud.Drift = Out * 50.f + FVector(0.0, 0.0, 15.0);
	Mud.Outward = 50.f;
	Mud.Fall = 0.3f;
	Effects->Puffs(Where + Out * 4.f, Mud);
	Effects->Chunks(EGraveChunk::Clod, Where + Out * 3.f, Out, 1, 300.f, 0.5f);
}

void AGravemotherCreature::AnswerRimShot(const AController* By)
{
	const UWorld* World = GetWorld();
	if (!World || IsDead())
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	// She hears it and rears up at them, screeching: seen and heard from the rim (her voice carries further than a hit's thud).
	if (LastRimScreech < 0.0 || Now - LastRimScreech >= RimScreechGap)
	{
		LastRimScreech = Now;
		Voice->Play(LooterSoundCue::SpiderAlert, 1.3f);
		// Her rear: the spider's own flinch from below her, as a hit on her chest would rock her back.
		ASpiderCreature::OnHurt(false, GetActorLocation() + GetActorForwardVector() * (150.f * GetSizeScale())
			- FVector(0.0, 0.0, 200.0));
	}
}

void AGravemotherCreature::PlayShrugSound(const FVector& Where, EGravemotherShrug Why)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	// A dull thud on caked earth (not a wound's wet hit)...
	if (LastShrugSound < 0.0 || Now - LastShrugSound >= ShrugSoundGap || FVector::DistSquared(Where, LastShrugSoundAt) > FMath::Square(ShrugSoundMerge))
	{
		LastShrugSound = Now;
		LastShrugSoundAt = Where;
		LooterSound::PlayAt(this, LooterSoundCue::Impact::Dirt, Where);
	}
	// ...and the round's whine skipping away: the ear's "that did nothing". From the rim the shooter is too far to hear it
	// out there, so they hear it as their own (2D, softer).
	if (LastShrugRicochet < 0.0 || Now - LastShrugRicochet >= ShrugRicochetGap)
	{
		LastShrugRicochet = Now;
		if (Why == EGravemotherShrug::OffGround)
		{
			LooterSound::Play2D(this, LooterSoundCue::Impact::Ricochet, RimGlanceVolume);
		}
		else
		{
			LooterSound::PlayAt(this, LooterSoundCue::Impact::Ricochet, Where);
		}
	}
}

// ---------------------------------------------------------------------------
// Bullets: her own look for a shot she shrugs off
// ---------------------------------------------------------------------------

void AGravemotherCreature::BindShrugLook(bool bOn)
{
	if (UBulletSubsystem* Old = Bullets.Get())
	{
		Old->OnBulletLanding.Remove(LandingHandle);
	}
	LandingHandle.Reset();
	Bullets.Reset();
	UWorld* World = bOn ? GetWorld() : nullptr;
	if (UBulletSubsystem* Subsystem = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		Bullets = Subsystem;
		LandingHandle = Subsystem->OnBulletLanding.AddUObject(this, &AGravemotherCreature::NoteBulletLanding);
	}
}

void AGravemotherCreature::NoteBulletLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing)
{
	// The bullets draw a creature's wound unless the part struck shows its own hits (UBulletSubsystem::OwnLookTag), which they
	// read right after this: a shot her hide will shrug off wears the tag (her TakeDamage plays its look), any other doesn't.
	if (Hit.GetActor() != this)
	{
		return;
	}
	UPrimitiveComponent* Part = Hit.GetComponent();
	if (!Part)
	{
		return;
	}
	const FName OwnLook(UBulletSubsystem::OwnLookTag);
	if (!Shot.bBlank && ShrugFor(Shot.Instigator.Get()) != EGravemotherShrug::None)
	{
		Part->ComponentTags.AddUnique(OwnLook);
	}
	else
	{
		Part->ComponentTags.Remove(OwnLook);
	}
}
