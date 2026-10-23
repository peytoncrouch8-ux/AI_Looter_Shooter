#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudHitMarker.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Weapons/WeaponBase.h"
#include "World/BreakableKinds.h"
#include "Components/Image.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// UPlayerHUDWidget's hit marker (the gun feel pass, GF-IMPACT 2026-10-22; the rules are HudHitMarker's): a hit's mark (a
// crit's orange, a ward's cyan and its break, a kill's red), its size by the hit's share of the target's health, a pellet
// gun's blast as one marker and one sound, and the sound by the mark. NativeTick fades and pops it.

void UPlayerHUDWidget::HandleHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	ShowHitMarker(Hit, Damage, bCritical, nullptr);
}

void UPlayerHUDWidget::HandleGunHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	ShowHitMarker(Hit, Damage, bCritical, BoundWeapon.Get());
}

void UPlayerHUDWidget::ShowHitMarker(const FHitResult& Hit, float Damage, bool bCritical, const AWeaponBase* Gun)
{
	// Only confirm hits on things that can actually be hurt, not walls.
	const AActor* HitActor = Hit.GetActor();
	const UHealthComponent* TargetHealth = HitActor ? HitActor->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!TargetHealth)
	{
		return;
	}
	const HudHitMarker::FTuning Tuning = HudHitMarker::FTuning::Live();
	// The hit lands before the HUD hears of it, so a dead target is a kill (the creature's voice plays its confirm once per
	// death). A crate or a barrel that breaks isn't a kill: a hit like any other, not the red confirm. A Warded creature's
	// ward: what it held before this hit (as last seen) and now, so its hit and its break show apart from a crit or a wound.
	const bool bKill = TargetHealth->IsDead() && !HitActor->ActorHasTag(FName(LooterBreakables::Tag));
	const UCreatureTraitComponent* Traits = UCreatureTraitComponent::Find(HitActor);
	const bool bWarded = Traits && Traits->HasTrait(ECreatureTrait::Warded);
	const float WardAfter = bWarded ? Traits->GetWardFraction() : 0.f;
	const float WardBefore = bWarded ? WardMemory.Before(HitActor) : 0.f;
	if (bWarded)
	{
		WardMemory.Note(HitActor, Traits, WardAfter);
	}
	const EHudHitMark Mark = HudHitMarker::Decide(bKill, bCritical, bWarded, WardBefore, WardAfter);

	// A pellet gun's blast: its pellets' hits are one marker, sized by how many landed, and one sound.
	const int32 Pellets = Gun ? FMath::Max(Gun->GetStats().PelletsPerShot, 1) : 1;
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const bool bSameBlast = Pellets > 1 && BlastGun.Get() == Gun && Now - BlastStarted <= Tuning.BlastWindow;
	if (!bSameBlast)
	{
		BlastGun = Pellets > 1 ? Gun : nullptr;
		BlastStarted = Now;
		BlastLanded = 0;
	}
	++BlastLanded;
	const float Size = Pellets > 1 ? HudHitMarker::BlastSize(BlastLanded, Pellets, Tuning)
		: HudHitMarker::Size(Damage / FMath::Max(TargetHealth->GetMaxHealth(), 1.f), Tuning);

	// Heard as well as seen: once a blast, again only for a stronger mark later in it (a crit pellet, a break).
	if (!bSameBlast || HudHitMarker::SoundsAgain(Mark, BlastSounded))
	{
		const FName Cue = HudHitMarker::Cue(Mark);
		if (!Cue.IsNone() && !bMuteHitSound)
		{
			// A bank made before the gun feel pass has no crit, ward or break cue: the old tick, a crit's pitched up.
			const ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
			if (!Sounds || Sounds->HasSounds(Cue))
			{
				LooterSound::Play2D(this, Cue);
			}
			else
			{
				LooterSound::Play2D(this, LooterSoundCue::HitMarker, 1.f, Mark == EHudHitMark::Crit ? LooterSoundRules::CritPitch : 1.f);
			}
		}
		BlastSounded = bSameBlast ? (HudHitMarker::Outranks(Mark, BlastSounded) ? Mark : BlastSounded) : Mark;
	}

	// The marker: a new hit's mark takes over, but within a blast only a stronger one, and nothing takes a kill's red back.
	const bool bTakeOver = Mark == EHudHitMark::Kill
		|| (ShownMark != EHudHitMark::Kill && (!bSameBlast || !HudHitMarker::Outranks(ShownMark, Mark)))
		|| HitMarkerTime <= 0.f;
	if (bTakeOver)
	{
		ShownMark = Mark;
		HitMarkerShowFor = HudHitMarker::SecondsOf(Mark, Tuning);
		HitMarkerPop = HudHitMarker::PopOf(Mark, Tuning);
		HitMarkerTime = HitMarkerShowFor;
		const FLinearColor Tint = HudHitMarker::Tint(Mark);
		for (UImage* Tick : HitMarkerTicks)
		{
			Tick->SetColorAndOpacity(Tint);
		}
	}
	// A blast's marker grows as its pellets land; a new hit is its own size.
	HitMarkerSize = bSameBlast ? FMath::Max(HitMarkerSize, Size) : (bTakeOver ? Size : FMath::Max(HitMarkerSize, Size));
	HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}

