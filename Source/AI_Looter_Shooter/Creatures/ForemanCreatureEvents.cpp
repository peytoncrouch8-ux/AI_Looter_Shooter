// AForemanCreature's fight as it starts, changes phase, staggers him, starts over and ends: what each of his boss's events
// does to him (the shift, the dark, the roof's first strike, a fuse dropped at his feet), and his hurts, his death (his last
// word) and his coming back with nothing of the last fight left.

#include "Creatures/ForemanCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Creatures/ForemanFuseStick.h"
#include "Creatures/ForemanRockFall.h"
#include "Creatures/ForemanTimberProp.h"
#include "Scenes/BossIntro.h"
#include "Components/SceneComponent.h"

// ---------------------------------------------------------------------------
// Putting a fight away
// ---------------------------------------------------------------------------

void AForemanCreature::ClearFightState(bool bMendProps)
{
	DropHeldStick(/*bPutOut*/ true);
	EndDark(/*bAtOnce*/ true);
	bIntroWanted = false;
	bStaggeredNow = false;
	Move = EForemanMove::Swing;
	SinceRoofStrike = -1.f;
	StruckProp.Reset();
	for (const TWeakObjectPtr<AForemanRockFall>& Each : RockFalls)
	{
		if (AForemanRockFall* Fall = Each.Get(); Fall && !Fall->HasFallen())
		{
			Fall->Destroy();
		}
	}
	RockFalls.Reset();
	ApplyPace(0);
	if (bMendProps)
	{
		for (AForemanTimberProp* Prop : GetProps(false))
		{
			Prop->Mend();
		}
	}
}

// ---------------------------------------------------------------------------
// His boss's events
// ---------------------------------------------------------------------------

void AForemanCreature::HandleFightStarted()
{
	// His bar sweeps in as he turns from the fall (his next attack is his entrance). Fresh props, and his first fuse a
	// moment after.
	bIntroWanted = true;
	SetAct(EAct::None);
	FuseCooldownLeft = Foreman::FirstFuseDelay;
	RoofCooldownLeft = Foreman::FirstRoofDelay;
	SinceRoofStrike = -1.f;
	for (AForemanTimberProp* Prop : GetProps(false))
	{
		Prop->Mend();
	}
	// His first fight in this session: the stope out of the dark before it (Scenes/BossIntro; once per session, never a rematch).
	BossIntro::PlayIfDue(*this);
}

void AForemanCreature::HandleFightReset()
{
	// The player fell: he's back at the fall and healed (his boss's reset; his shift went with it).
	ClearFightState(/*bMendProps*/ true);
}

void AForemanCreature::HandlePhaseChanged(int32 NewPhase, int32 OldPhase)
{
	ApplyPace(NewPhase);
	if (OldPhase == INDEX_NONE || NewPhase <= OldPhase)
	{
		return;
	}
	switch (NewPhase)
	{
	case Foreman::ShiftPhase:
	case Foreman::LastShiftPhase:
		// The shift up out of the rubble; the roof soon after.
		CallShift();
		Say(ForemanFight::ShiftLine());
		RoofCooldownLeft = FMath::Max(RoofCooldownLeft, Foreman::FirstRoofDelay * 0.5f);
		break;
	case Foreman::DarkPhase:
		BeginDark();
		break;
	default:
		break;
	}
}

void AForemanCreature::HandleStaggered(bool bStaggered)
{
	bStaggeredNow = bStaggered;
	if (bStaggered)
	{
		// Knocked to a knee: whatever he was at is broken off (its blow never lands: Strike does nothing while staggered),
		// a fuse in his fist falls at his feet, and he's held still. His coal stays open while he's down.
		if (GetCreatureState() == ECreatureState::Attack)
		{
			AttackWindup = FMath::Min(AttackWindup, GetStateTime() + 0.01f);
			AttackRecovery = 0.05f;
		}
		DropHeldStick(/*bPutOut*/ false);
		UE_LOG(LogLooter, Log, TEXT("%s is knocked to a knee, staggered."), *GetName());
	}
	ApplyPace(PacePhase);
}

// ---------------------------------------------------------------------------
// Hurt, dying, coming back
// ---------------------------------------------------------------------------

void AForemanCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	Super::OnHurt(bCritical, HitLocation);
	if (bCritical && !IsDead())
	{
		// A shot into his open coal: its roar as it flares.
		LooterSound::PlayAttached(LooterSoundCue::Foreman::CoalOpen, GetRootComponent(), NAME_None, 0.6f);
	}
}

void AForemanCreature::OnDied()
{
	Super::OnDied();
	// A fuse in his fist falls lit at his feet; the stope's lanterns catch again; he kneels where he fell (his pose) and
	// says his last word.
	DropHeldStick(/*bPutOut*/ false);
	EndDark(/*bAtOnce*/ false);
	bStaggeredNow = false;
	if (!bLastWordSaid)
	{
		bLastWordSaid = true;
		Say(ForemanFight::LastWord());
	}
	UE_LOG(LogLooter, Log, TEXT("%s kneels at the fall: his shift is over."), *GetName());
}

void AForemanCreature::OnRespawned()
{
	Super::OnRespawned();
	// A fight started over, a stand-down, a call back: nothing of the last one left.
	ClearFightState(/*bMendProps*/ true);
	bLastWordSaid = false;
	LampBrightness = Foreman::OwnLampBrightness;
}
