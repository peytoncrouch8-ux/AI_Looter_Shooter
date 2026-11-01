// UAbelCorruptionComponent: the call-backs (his Smog bar locked at 66% and 33%, his kneel, the hold at his keeper's post,
// the gold pips under his bar), the last call at zero, the black lantern falling into the canyon, and the optional's word
// that the six dead were saved.

#include "Bosses/AbelCorruption.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/DredgerLook.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Weapons/WeaponFX.h"
#include "World/KeeperLanternPost.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

namespace
{
	/** The keeper's lantern on the post beckons while a call is open (its glow's low and high, how fast it throbs a second). */
	constexpr float BeckonLow = 0.12f;
	constexpr float BeckonHigh = 0.45f;
	constexpr float BeckonRate = 3.2f;

	/** A pip's burn: the gold (linear), the starburst and ring sizes (cm), the shake. */
	const FLinearColor PipGold(1.f, 0.8f, 0.42f);
	constexpr float PipStarburst = 170.f;
	constexpr float PipRing = 560.f;
	constexpr float PipShake = 0.5f;
	/** The smog torn off him at a pip: soot-violet tatters (count, size). */
	const FLinearColor TornSmog(0.05f, 0.035f, 0.07f);
	constexpr int32 TornCount = 14;
	constexpr float TornSize = 70.f;

	/** The black lantern's fall: thrown out over the canyon (cm/s along the sunset, up), turning, gone after this long. */
	constexpr float FallOut = 380.f;
	constexpr float FallUp = 260.f;
	constexpr float FallGravity = 980.f;
	constexpr float FallSpin = 260.f;
	constexpr float FallSeconds = 3.5f;

	/** The last call's tendril is thrown once the player comes this near the keeper's post or him (cm). */
	constexpr float FinalReach = 900.f;

	UBulletSubsystem* BulletsOf(const UWorld* World)
	{
		return World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	}
}

AKeeperLanternPost* UAbelCorruptionComponent::BindKeepersPost()
{
	AKeeperLanternPost* Post = BoundPost.Get();
	if (Post)
	{
		return Post;
	}
	const AAbelKeeper* Abel = GetAbel();
	Post = Abel ? Abel->GetKeepersPost() : nullptr;
	if (Post)
	{
		BoundPost = Post;
		Post->OnCalledBack.RemoveAll(this);
		Post->OnCalledBack.AddUObject(this, &UAbelCorruptionComponent::HandleCalledBack);
	}
	return Post;
}

// ---------------------------------------------------------------------------
// The lock and the kneel
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::BeginCallBack()
{
	// The smog's echo of him (a vigil stone's rematch) has no Abel in it to call back: the phase goes straight on.
	if (IsRematch() || bFreed)
	{
		FinishCallBack(Pips);
		return;
	}
	++LocksOwed;
	if (!bLocked)
	{
		StartLock();
	}
}

void UAbelCorruptionComponent::StartLock()
{
	UBossComponent* Boss = GetBoss();
	AAbelKeeper* Abel = GetAbel();
	if (LocksOwed <= 0 || !Boss || !Abel || Abel->IsDead())
	{
		return;
	}
	--LocksOwed;
	bLocked = true;
	// His bar locks: he can't be hurt, but stays in the fight (hits only spark off the smog: HandleDamaged), and the line under
	// the bar says what to do. Not the fog's spell: he doesn't withdraw.
	FBossUntargetable Lock;
	Lock.Seconds = 0.f;
	Lock.bUntilAddsDie = false;
	Lock.bWithdraw = false;
	Lock.Hint = LOCTEXT("CallBackHint", "Call him back at the keeper's post");
	Boss->BeginUntargetable(Lock);
	BreakTendrils(AbelCorruptionRules::EBreak::Staggered, /*bQuiet*/ false);
	StopDrink(false);
	if (!bHintSaid)
	{
		bHintSaid = true;
		static const FText Hob = LOCTEXT("Hob", "Hob");
		Say(TEXT("DA_Lines_AbelCallHint"), { FStoryLine::Make(Hob, LOCTEXT("CallHint", "He's down! Get to the keeper's post. Call him back!"), 3.f) });
	}
	BeginKneel();
	UE_LOG(LogLooter, Log, TEXT("%s: his Smog bar locks; the keeper's post can call him back (pip %d of %d)."), *Abel->GetActorNameOrLabel(),
		Pips + 1, AbelCorruptionRules::Pips);
}

void UAbelCorruptionComponent::BeginKneel()
{
	AAbelKeeper* Abel = GetAbel();
	if (!Abel || Abel->IsDead())
	{
		return;
	}
	bKneeling = true;
	KneelLeft = AbelCorruptionRules::KneelSeconds;
	RetryIn = 0.f;
	Abel->BeginCallKneel();
	if (AKeeperLanternPost* Post = BindKeepersPost())
	{
		Post->OpenCallBack(AbelCorruptionRules::CallBackHold, LOCTEXT("CallBack", "Call him back"));
	}
	LooterSound::PlayAt(Abel, LooterSoundCue::AbelStagger, Abel->GetActorLocation());
}

void UAbelCorruptionComponent::EndKneel(bool bCalled)
{
	bKneeling = false;
	KneelLeft = 0.f;
	if (AKeeperLanternPost* Post = BoundPost.Get())
	{
		Post->CloseCallBack();
		Post->SetCallGlow(static_cast<float>(Pips) / static_cast<float>(AbelCorruptionRules::Pips));
	}
	if (AAbelKeeper* Abel = GetAbel())
	{
		Abel->EndCallKneel();
	}
	// Missed: he rises with the bar still locked, and the next kneel comes later (it can't be skipped, it can't be failed).
	if (!bCalled && bLocked)
	{
		RetryIn = AbelCorruptionRules::RetryAfter;
	}
}

void UAbelCorruptionComponent::TickCallBack(float DeltaSeconds)
{
	AKeeperLanternPost* Post = BoundPost.Get();
	if (bKneeling)
	{
		KneelLeft -= DeltaSeconds;
		// The keeper's lantern on its post beckons while he kneels: it throbs, a third brighter for each pip already won.
		if (Post)
		{
			const float Won = static_cast<float>(Pips) / static_cast<float>(AbelCorruptionRules::Pips);
			const float Throb = 0.5f + 0.5f * FMath::Sin(LineClock * BeckonRate * UE_TWO_PI);
			Post->SetCallGlow(Won + FMath::Lerp(BeckonLow, BeckonHigh, Throb));
		}
		if (KneelLeft <= 0.f)
		{
			EndKneel(false);
		}
		return;
	}
	if (bLocked && RetryIn > 0.f)
	{
		RetryIn -= DeltaSeconds;
		const AAbelKeeper* Abel = GetAbel();
		// Back down on his knees once he's free of whatever he started meanwhile (a lunge lands first).
		if (RetryIn <= 0.f && Abel && !Abel->IsDead() && !Abel->IsLunging())
		{
			BeginKneel();
		}
		else if (RetryIn <= 0.f)
		{
			RetryIn = 0.2f;
		}
	}
}

void UAbelCorruptionComponent::HandleCalledBack(AKeeperLanternPost& Post)
{
	if (bFinalCall)
	{
		// The last call held to its end: the third pip, the last of the smog off him, the black lantern into the canyon.
		bFinalCall = false;
		BreakTendrils(AbelCorruptionRules::EBreak::OwnerDown, /*bQuiet*/ false);
		Post.CloseCallBack();
		Post.SetCallGlow(1.f);
		Pips = AbelCorruptionRules::Pips;
		PlayPipBurst(Pips - 1);
		DropBlackLantern();
		bFreed = true;
		UE_LOG(LogLooter, Log, TEXT("%s: called home at the keeper's post; the black lantern falls into the canyon."), *GetNameSafe(GetOwner()));
		return;
	}
	if (!bKneeling)
	{
		return;
	}
	// A pip: a third of the smog burned off him for good, a little of himself back in his words.
	Pips = FMath::Min(Pips + 1, AbelCorruptionRules::Pips - 1);
	PlayPipBurst(Pips - 1);
	static const FText Abel = LOCTEXT("Abel", "Abel");
	if (Pips == 1)
	{
		Say(TEXT("DA_Lines_AbelCallBack1"), { FStoryLine::Make(Abel, LOCTEXT("CallBack1", "...Ma? Is that my lantern?"), 2.8f) }, INDEX_NONE, true);
	}
	else
	{
		Say(TEXT("DA_Lines_AbelCallBack2"), { FStoryLine::Make(Abel, LOCTEXT("CallBack2", "Who are you? You're not from here."), 2.8f) }, INDEX_NONE, true);
	}
	EndKneel(true);
	FinishCallBack(Pips);
}

void UAbelCorruptionComponent::FinishCallBack(int32 Index)
{
	UBossComponent* Boss = GetBoss();
	AAbelKeeper* Abel = GetAbel();
	if (!Boss || !Abel)
	{
		return;
	}
	if (bLocked)
	{
		bLocked = false;
		Boss->EndUntargetable();
	}
	// A hit that went past both lines owes a second call first; the first phase's drift is passed over for it.
	if (LocksOwed > 0)
	{
		StartLock();
		return;
	}
	if (!Boss->IsFighting() || Abel->IsDead())
	{
		return;
	}
	const int32 Phase = Boss->GetPhase();
	if (Phase == 1)
	{
		// "The Hose": out over the canyon into the fog to drink, the lanterns going dark behind him.
		Abel->DriftOut();
		Boss->BeginUntargetable(AbelRules::FogSpell());
	}
	else if (Phase >= 2)
	{
		// "Keeper": the drinking is over (the optional's word), the smog from the wreck up over the deck, the Gravewind.
		SendSixSaved();
		SetSurge(true);
		Abel->StartWind();
	}
}

// ---------------------------------------------------------------------------
// The last call
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::BeginFinalCall()
{
	if (bFinalCall || bFreed || IsRematch())
	{
		return;
	}
	AKeeperLanternPost* Post = BindKeepersPost();
	if (!Post)
	{
		// No keeper's post (a test level without one): nothing to call him home with, so he's freed as the scene would free him.
		bFreed = true;
		Pips = AbelCorruptionRules::Pips;
		return;
	}
	bFinalCall = true;
	Post->OpenCallBack(AbelCorruptionRules::FinalHold, LOCTEXT("CallHome", "Call him home"));
	static const FText Finch = LOCTEXT("Finch", "Brother Finch");
	Say(TEXT("DA_Lines_AbelFinal"), { FStoryLine::Make(Finch, LOCTEXT("Final", "Now, Warden! Hold the post! Call him home!"), 3.f) }, INDEX_NONE, true);
	UE_LOG(LogLooter, Log, TEXT("%s: knelt at zero; the last call is open at the keeper's post."), *GetNameSafe(GetOwner()));
}

void UAbelCorruptionComponent::TickFinalCall(float DeltaSeconds)
{
	if (!bFinalCall)
	{
		return;
	}
	AKeeperLanternPost* Post = BoundPost.Get();
	if (Post)
	{
		const float Throb = 0.5f + 0.5f * FMath::Sin(LineClock * BeckonRate * UE_TWO_PI);
		Post->SetCallGlow(0.66f + FMath::Lerp(BeckonLow, BeckonHigh, Throb));
	}
	// The last of the smog fights back: a tendril on the player as they come to the post, drained through the hold.
	const bool bTendrilOut = Tendrils.ContainsByPredicate([](const FTendril& Each) { return Each.bFinal && Each.State != FTendril::EState::Off; });
	const APawn* Player = GetPlayer();
	const AAbelKeeper* Abel = GetAbel();
	if (!bTendrilOut && Player && Abel)
	{
		const bool bNearPost = Post && FVector::Dist(Player->GetActorLocation(), Post->GetActorLocation()) <= FinalReach;
		const bool bNearHim = FVector::Dist(Player->GetActorLocation(), Abel->GetActorLocation()) <= FinalReach;
		if (bNearPost || bNearHim)
		{
			ThrowFinalTendril();
		}
	}
}

void UAbelCorruptionComponent::SetFreed()
{
	bFreed = true;
	bFinalCall = false;
	Pips = AbelCorruptionRules::Pips;
	bSceneHoldsLook = false;
	CorruptionShown = 0.f;
	ApplyLook(true);
}

void UAbelCorruptionComponent::SetPips(int32 InPips)
{
	Pips = FMath::Clamp(InPips, 0, AbelCorruptionRules::Pips);
	CorruptionShown = AbelCorruptionRules::CorruptionFor(Pips);
	if (AKeeperLanternPost* Post = BindKeepersPost())
	{
		Post->SetCallGlow(static_cast<float>(Pips) / static_cast<float>(AbelCorruptionRules::Pips));
	}
	ApplyLook(true);
}

void UAbelCorruptionComponent::SetCorruptionShown(float Share)
{
	bSceneHoldsLook = true;
	CorruptionShown = FMath::Clamp(Share, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// The show: the pips, a pip's burn, the black lantern's fall
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::UpdatePips()
{
	UBossComponent* Boss = GetBoss();
	UHudBossBarWidget* Bar = Boss ? Boss->GetBar() : nullptr;
	if (!Bar)
	{
		return;
	}
	// The real Abel's: three gold pips "ABEL" under a bar that reads SMOG. The echo's bar is a plain boss's.
	if (IsRematch())
	{
		Bar->SetPips(0, 0, FText::GetEmpty(), FText::GetEmpty());
		return;
	}
	Bar->SetPips(AbelCorruptionRules::Pips, Pips, LOCTEXT("PipLabel", "Abel"), LOCTEXT("BarLabel", "Smog"));
}

void UAbelCorruptionComponent::PlayPipBurst(int32 Pip) const
{
	AAbelKeeper* Abel = GetAbel();
	UWorld* World = GetWorld();
	if (!Abel || !World)
	{
		return;
	}
	const FVector Chest = Abel->GetCoalLocation();
	const FVector Feet = Abel->GetActorLocation() - FVector(0.0, 0.0, 110.0 * Abel->GetSizeScale());
	if (UBulletSubsystem* Bullets = BulletsOf(World))
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		// The smog torn off him in black tatters, and what's under it showing gold: a starburst at his coal, a ring on the boards.
		Effects.SpawnSoulTatters(Chest, FVector::UpVector, TornSmog, TornSize, TornCount);
		Effects.SpawnStarburst(Chest, PipStarburst, PipGold, 6.f, 0.6f, 10);
		Effects.SpawnShockRing(Feet, FVector::UpVector, 40.f, PipRing, 30.f, PipGold, 3.f, 0.7f);
		if (const AKeeperLanternPost* Post = BoundPost.Get())
		{
			Effects.SpawnStarburst(Post->GetKeepersLanternGlobe(), 90.f, PipGold, 5.f, 0.5f, 8);
		}
	}
	LooterSound::Play2D(Abel, LooterSoundCue::StingPhase, 0.9f);
	LooterSound::PlayAt(Abel, LooterSoundCue::Element::SoulLightApply, Chest);
	BossCameraShake::Kick(Abel, Chest, PipShake, 0.6f);
	UE_LOG(LogLooter, Log, TEXT("%s: pip %d of %d: a third of the smog burned off him."), *Abel->GetActorNameOrLabel(), Pip + 1, AbelCorruptionRules::Pips);
}

void UAbelCorruptionComponent::DropBlackLantern()
{
	AAbelKeeper* Abel = GetAbel();
	UStaticMeshComponent* Held = Abel ? Abel->Lantern.Get() : nullptr;
	if (!Held || !Held->GetStaticMesh())
	{
		return;
	}
	// The black lantern cracks and falls out of his hand into the canyon: a copy of it, thrown out past the open end and
	// tumbling down; his own goes (he's in his own lantern on the post from here).
	UStaticMeshComponent* Falling = NewObject<UStaticMeshComponent>(Abel, TEXT("FallingBlackLantern"), RF_Transient);
	Falling->SetStaticMesh(Held->GetStaticMesh());
	for (int32 Slot = 0; Slot < Held->GetNumMaterials(); ++Slot)
	{
		Falling->SetMaterial(Slot, Held->GetMaterial(Slot));
	}
	Falling->SetUsingAbsoluteLocation(true);
	Falling->SetUsingAbsoluteRotation(true);
	Falling->SetUsingAbsoluteScale(true);
	Falling->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Falling->SetCastShadow(false);
	Falling->SetupAttachment(Abel->GetRootComponent());
	Falling->RegisterComponent();
	Falling->SetWorldTransform(Held->GetComponentTransform());
	Held->SetVisibility(false);
	FallingLantern = Falling;
	FallVelocity = Abel->GetSunsetDirection() * FallOut + FVector(0.0, 0.0, FallUp);
	FallClock = 0.f;
	const FVector At = Falling->GetComponentLocation();
	LooterSound::PlayAt(Abel, LooterSoundCue::Impact::Glass, At, 1.f, 0.8f);
	if (UBulletSubsystem* Bullets = BulletsOf(GetWorld()))
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		Effects.SpawnSoulTatters(At, FVector::UpVector, DredgerLook::SoulCyan, 30.f, 6);
		Effects.SpawnFlash(At, FVector::UpVector, 0.5f);
	}
}

void UAbelCorruptionComponent::TickFallingLantern(float DeltaSeconds)
{
	UStaticMeshComponent* Falling = FallingLantern.Get();
	if (!Falling)
	{
		return;
	}
	FallClock += DeltaSeconds;
	FallVelocity.Z -= FallGravity * DeltaSeconds;
	Falling->SetWorldLocation(Falling->GetComponentLocation() + FallVelocity * DeltaSeconds);
	Falling->AddWorldRotation(FRotator(FallSpin * DeltaSeconds, FallSpin * 0.6f * DeltaSeconds, 0.f));
	if (FallClock >= FallSeconds)
	{
		Falling->DestroyComponent();
		FallingLantern.Reset();
	}
}

void UAbelCorruptionComponent::SendSixSaved()
{
	if (bSixSent || IsRematch() || !AbelCorruptionRules::AreSixSaved(BoardsDead.Num(), NumDrunk()))
	{
		return;
	}
	bSixSent = true;
	// Main 7's optional ("Save all six of the dead on the boards"): the event for its objective, the flag for later words.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(AbelCorruptionRules::SixSavedEvent(), GetOwner()));
	}
	if (FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(GetWorld()))
	{
		Campaign->SetFlag(AbelCorruptionRules::SixSavedEvent());
	}
	UE_LOG(LogLooter, Log, TEXT("%s: none of the six dead on the boards was drunk (%s)."), *GetNameSafe(GetOwner()),
		*AbelCorruptionRules::SixSavedEvent().ToString());
}

#undef LOCTEXT_NAMESPACE
