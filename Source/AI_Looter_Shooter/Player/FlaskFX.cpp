#include "Player/FlaskFX.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Player/FlaskFXView.h"
#include "Player/PlayerFlaskCues.h"
#include "Player/PlayerFlaskRules.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Math/RandomStream.h"
#include "Templates/SharedPointer.h"
#include "TimerManager.h"

// The soul flask's sounds and light (FlaskFX.h). The light is a short run per look, stepped once a frame on the world's
// timer (as the ember powers pump theirs): it finds the flask in the hand each frame (FlaskFXView), so the light follows
// the hand's motion, and draws its motes and glow as FWeaponFX streaks and soft discs (M_FX_Glow / M_FX_Shape: unlit,
// additive, not Nanite, no lights). Sizes are set against the distance from the camera, so a mote reads the same few
// pixels whether the flask is at the lips in first person or across the screen in third.

using namespace FlaskFXView;

namespace
{
	// --- The look (tune here) ---

	/** The uncork: the flask's light swelling as the cork comes out, then easing back (s; strength), and the glints let out. */
	constexpr float UncorkSeconds = 0.45f;
	constexpr float UncorkRise = 0.08f;
	constexpr float UncorkGlow = 0.8f;
	constexpr int32 UncorkGlints = 3;
	/** The flask's glow disc: a share of its distance from the camera, never wider than this many flask lengths. */
	constexpr float GlowSizeShare = 0.32f;
	constexpr float GlowSizeMaxLengths = 1.4f;

	/** The draught: motes drawn out of the flask's belly, through its mouth and into the drinker (per second, at the flow's mean). */
	constexpr float DraughtRate = 34.f;
	/** Motes peeling off round the mouth, sweeping out to the sides (the light spilling round the face). */
	constexpr float WashRate = 8.f;
	/** A draught mote's life (s), its speed from start to end (cm/s), how hard it turns onto its way, and its twist about it. */
	constexpr float DraughtLifeMin = 0.32f;
	constexpr float DraughtLifeMax = 0.5f;
	constexpr float DraughtSpeedStart = 30.f;
	constexpr float DraughtSpeedEnd = 85.f;
	constexpr float DraughtSteer = 14.f;
	constexpr float SwirlSpeed = 14.f;
	constexpr float SwirlRate = 17.f;
	/** A wash mote's life (s), speed (cm/s) and slowing (1/s). */
	constexpr float WashLifeMin = 0.4f;
	constexpr float WashLifeMax = 0.6f;
	constexpr float WashSpeed = 38.f;
	constexpr float WashDrag = 2.2f;
	/** A mote's width against its distance from the camera (about 0.7 degrees) within these (cm), and its streak behind it (s of travel). */
	constexpr float MoteWidthShare = 0.012f;
	constexpr float MoteWidthMin = 0.12f;
	constexpr float MoteWidthMax = 1.4f;
	constexpr float MoteTrail = 0.035f;
	/** A mote's strength (M_FX_Glow) and its heart's whiteness; a wash mote is fainter. */
	constexpr float MoteGlow = 7.f;
	constexpr float MoteWhite = 0.25f;
	constexpr float WashGlowShare = 0.7f;
	/** In first person a mote fades out before it comes this near the eye (cm), over this much more: nothing fills the view. */
	constexpr float NearestToEye = 7.f;
	constexpr float NearFade = 5.f;
	/** The flask's soft glow while it pours, over the heal and a breath after it. */
	constexpr float HaloGlow = 0.6f;
	constexpr float HaloAfter = 0.2f;
	/**
	 * Third person (the 2026-10-30 play-check): the light comes only from the body's own flask (HandGrip_L) or the head, a few
	 * small motes: this share of the draught, no wash, motes no wider than this (cm), the halo at this share, and a few motes
	 * drawn in to the chest from a shell this far round it.
	 */
	constexpr float ThirdPersonDraughtShare = 0.3f;
	constexpr float ThirdPersonMoteWidthMax = 0.7f;
	constexpr float ThirdPersonHaloShare = 0.5f;
	constexpr int32 ChestMotes = 5;
	constexpr float ChestShell = 22.f;
	/** Third person, the safety: a mote is dropped once it's this near the camera, or this far from the body's middle (cm). */
	constexpr float CameraClearance = 60.f;
	constexpr float BodyReach = 90.f;
	/** First person, the same: a mote this far from the eye is a stray (cm). */
	constexpr float EyeReach = 90.f;

	/** A charge filled: a small star at the flask's mouth (or the hip, the flask stowed), its light pulsing, glints out. */
	constexpr float FlareSeconds = 0.35f;
	constexpr float FlareGlow = 0.7f;
	constexpr float FlareStarShare = 0.15f;
	constexpr float FlareStarMin = 3.f;
	constexpr float FlareStarMax = 16.f;
	constexpr float FlareStarGlow = 3.f;
	constexpr float FlareStarLife = 0.22f;
	constexpr int32 FlareGlints = 4;
	/** Seen in third person, the flare's star no wider than this (cm). */
	constexpr float ThirdPersonStarMax = 7.f;

	enum class ELook : uint8 { Uncork, Draught, Flare };

	/** One mote of the light. A draught mote heads for the mouth, then on into the drinker; a wash mote flies and slows. */
	struct FMote
	{
		FVector At = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Age = 0.f;
		float Life = 0.4f;
		float Phase = 0.f;
		bool bWash = false;
		bool bPastMouth = false;
	};

	/** One look running over a few frames. */
	struct FFlaskRun
	{
		TWeakObjectPtr<APawn> Drinker;
		ELook Look = ELook::Draught;
		float Age = 0.f;
		bool bStepped = false;
		bool bSeenFlask = false;
		/** A flare at the hip (the flask stowed out of sight), not at the flask. */
		bool bAtHip = false;
		/** The mouth given at the start, held in the eye's frame, for frames with no model to take it from (first person only). */
		TOptional<FVector> MouthInEye;
		float DraughtOwed = 0.f;
		float WashOwed = 0.f;
		TArray<FMote> Motes;
		FRandomStream Random;
	};

	FLinearColor SoulLight()
	{
		return LooterUI::Color::Heal();
	}

	void PlayCue(APawn& Drinker, const TCHAR* Cue)
	{
		// The player's own hands: in their head. Anyone else's flask is heard where they stand.
		if (Drinker.IsLocallyControlled())
		{
			LooterSound::Play2D(&Drinker, Cue);
		}
		else
		{
			LooterSound::PlayAt(&Drinker, Cue, Drinker.GetActorLocation());
		}
	}

	FWeaponFX* EffectsFor(UWorld* World)
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

	/** A rise over Rise, then an ease back to nothing by Seconds (0..1). */
	float Pulse(float Age, float Rise, float Seconds)
	{
		if (Age < Rise)
		{
			return FMath::SmoothStep(0.f, 1.f, Age / FMath::Max(Rise, UE_KINDA_SMALL_NUMBER));
		}
		return 1.f - FMath::SmoothStep(0.f, 1.f, (Age - Rise) / FMath::Max(Seconds - Rise, UE_KINDA_SMALL_NUMBER));
	}

	/** The flask's soft glow over its belly, Strength strong. */
	void GlowOver(FWeaponFX& Effects, const FEye& Eye, const FPlace& Place, float Strength)
	{
		const FVector Belly = Place.Along(0.35f);
		// Never a disc near a third-person camera (it would fill the screen's foot).
		const bool bByCamera = !Eye.bFirstPerson && FVector::DistSquared(Belly, Eye.Camera) < FMath::Square(CameraClearance);
		if (Strength > 0.f && !bByCamera)
		{
			Effects.AddGlow(Belly, SizeAt(Eye, Belly, GlowSizeShare, 1.f, Place.Length() * GlowSizeMaxLengths), SoulLight(), Strength);
		}
	}

	void AddWash(FFlaskRun& Run, const FPlace& Place, const FEye& Eye, const FVector& Carried, float Speed)
	{
		FMote& Mote = Run.Motes.AddDefaulted_GetRef();
		Mote.bWash = true;
		Mote.At = Place.Mouth + Run.Random.GetUnitVector() * 0.5f;
		const float Side = Run.Random.FRandRange(0.6f, 1.f) * (Run.Random.FRand() < 0.5f ? -1.f : 1.f);
		const FVector Way = (Place.Out * 0.5f + Eye.Right * Side + Eye.Up * Run.Random.FRandRange(-0.1f, 0.4f)).GetSafeNormal();
		Mote.Velocity = Carried + Way * Speed;
		Mote.Life = Run.Random.FRandRange(WashLifeMin, WashLifeMax);
		Mote.Phase = Run.Random.FRandRange(0.f, UE_TWO_PI);
	}

	void AddDraught(FFlaskRun& Run, const FPlace& Place, const FVector& Carried)
	{
		// From somewhere in the belly (between near the foot and halfway up, a little off the axis), so the light is seen
		// draining out of the flask toward its mouth.
		const FVector Axis = Place.Mouth - Place.Foot;
		const FVector Along = Axis.GetSafeNormal(UE_SMALL_NUMBER, Place.Out);
		const FVector Across = FVector::CrossProduct(Along, Run.Random.GetUnitVector()).GetSafeNormal();
		FMote& Mote = Run.Motes.AddDefaulted_GetRef();
		Mote.At = Place.Foot + Axis * Run.Random.FRandRange(0.12f, 0.55f) + Across * Place.Length() * Run.Random.FRandRange(0.f, 0.22f);
		Mote.Velocity = Carried + Along * DraughtSpeedStart;
		Mote.Life = Run.Random.FRandRange(DraughtLifeMin, DraughtLifeMax);
		Mote.Phase = Run.Random.FRandRange(0.f, UE_TWO_PI);
	}

	/** A mote that has strayed where it must never be drawn: in third person near the camera or away from the body, in first
	 *  person too near the eye or far from it. */
	bool IsStray(const FEye& Eye, const FVector& At, const FVector& BodyMiddle)
	{
		if (Eye.bFirstPerson)
		{
			const double EyeDistance = FVector::Dist(At, Eye.Location);
			return EyeDistance < NearestToEye || EyeDistance > EyeReach;
		}
		return FVector::DistSquared(At, Eye.Camera) < FMath::Square(CameraClearance)
			|| FVector::DistSquared(At, BodyMiddle) > FMath::Square(BodyReach);
	}

	/** A few motes drawn in to the chest from a shell round it (third person's soul going in). */
	void AddChestMotes(FFlaskRun& Run, const FVector& Sink, const FVector& Carried)
	{
		for (int32 Index = 0; Index < ChestMotes; ++Index)
		{
			FMote& Mote = Run.Motes.AddDefaulted_GetRef();
			Mote.bPastMouth = true;
			Mote.At = Sink + Run.Random.GetUnitVector() * ChestShell * Run.Random.FRandRange(0.8f, 1.2f);
			Mote.Velocity = Carried + (Sink - Mote.At).GetSafeNormal() * DraughtSpeedStart;
			Mote.Life = Run.Random.FRandRange(DraughtLifeMin, DraughtLifeMax) * 1.4f;
			Mote.Phase = Run.Random.FRandRange(0.f, UE_TWO_PI);
		}
	}

	/** Moves the motes on DeltaSeconds and draws them; spent ones go. */
	void StepMotes(FFlaskRun& Run, const FPlace& Place, const FEye& Eye, const FVector& Sink, const FVector& Carried, float DeltaSeconds,
		FWeaponFX& Effects, float Glow, const FVector& BodyMiddle)
	{
		const FLinearColor Heart = FMath::Lerp(SoulLight(), FLinearColor::White, MoteWhite);
		const float WidthMax = Eye.bFirstPerson ? MoteWidthMax : ThirdPersonMoteWidthMax;
		for (int32 Index = Run.Motes.Num() - 1; Index >= 0; --Index)
		{
			FMote& Mote = Run.Motes[Index];
			Mote.Age += DeltaSeconds;
			const float T = Mote.Age / Mote.Life;
			if (Mote.bWash)
			{
				// Flung out and slowing, carried along with the drinker.
				Mote.Velocity = Carried + (Mote.Velocity - Carried) * FMath::Exp(-WashDrag * DeltaSeconds);
			}
			else
			{
				// To the mouth first, then on into the drinker; faster as it goes, twisting a little about its way.
				Mote.bPastMouth = Mote.bPastMouth || FVector::DistSquared(Mote.At, Place.Mouth) < FMath::Square(1.5f);
				const FVector Goal = Mote.bPastMouth ? Sink : Place.Mouth;
				const FVector Way = (Goal - Mote.At).GetSafeNormal(UE_SMALL_NUMBER, Place.Out);
				const FVector Twist = FVector::CrossProduct(Way, Eye.Forward).GetSafeNormal() * FMath::Sin(Mote.Age * SwirlRate + Mote.Phase) * SwirlSpeed;
				const FVector Wanted = Carried + Way * FMath::Lerp(DraughtSpeedStart, DraughtSpeedEnd, FMath::Clamp(T, 0.f, 1.f)) + Twist;
				Mote.Velocity = FMath::Lerp(Mote.Velocity, Wanted, 1.f - FMath::Exp(-DraughtSteer * DeltaSeconds));
				if (Mote.bPastMouth && FVector::DistSquared(Mote.At, Sink) < FMath::Square(2.f))
				{
					Mote.Age = Mote.Life;
				}
			}
			Mote.At += Mote.Velocity * DeltaSeconds;
			const FVector Tail = Mote.At - (Mote.Velocity - Carried) * MoteTrail;
			if (Mote.Age >= Mote.Life || IsStray(Eye, Mote.At, BodyMiddle) || IsStray(Eye, Tail, BodyMiddle))
			{
				Run.Motes.RemoveAtSwap(Index, 1, EAllowShrinking::No);
				continue;
			}
			const float EyeDistance = static_cast<float>(FVector::Dist(Mote.At, Eye.Location));
			const float Near = Eye.bFirstPerson ? FMath::Clamp((EyeDistance - NearestToEye) / NearFade, 0.f, 1.f) : 1.f;
			const float Fade = FMath::SmoothStep(0.f, 1.f, Mote.Age / 0.06f) * (1.f - FMath::SmoothStep(0.65f, 1.f, T)) * Near;
			Effects.AddStreak(Tail, Mote.At, SizeAt(Eye, Mote.At, MoteWidthShare, MoteWidthMin, WidthMax), Heart,
				MoteGlow * Fade * Glow * (Mote.bWash ? WashGlowShare : 1.f));
		}
	}

	/** The draught's frame: motes let out with the flow (most at the heal's middle), and the flask's glow swelling and fading. */
	bool StepDraught(FFlaskRun& Run, const FPlace& Place, bool bPlaced, const FEye& Eye, const FVector& Carried, float DeltaSeconds,
		FWeaponFX& Effects, float Glow)
	{
		const float Pour = FFlaskRules::HealSeconds;
		// Third person: a few small motes off the body's own flask, no wash (it sweeps along the eye's sides), a fainter halo.
		const float DraughtShare = Eye.bFirstPerson ? 1.f : ThirdPersonDraughtShare;
		const float WashShare = Eye.bFirstPerson ? 1.f : 0.f;
		if (bPlaced && Run.Age < Pour)
		{
			const float Flow = 0.35f + 1.3f * FMath::Sin(UE_PI * FMath::Clamp(Run.Age / Pour, 0.f, 1.f));
			Run.DraughtOwed += DraughtRate * DraughtShare * Flow * DeltaSeconds;
			Run.WashOwed += WashRate * WashShare * Flow * DeltaSeconds;
			if (!Run.bSeenFlask)
			{
				// The first frame lets a few go at once, so the light starts with the gulp.
				Run.DraughtOwed += 4.f * DraughtShare;
				Run.WashOwed += WashShare;
				Run.bSeenFlask = true;
			}
			for (; Run.DraughtOwed >= 1.f; Run.DraughtOwed -= 1.f)
			{
				AddDraught(Run, Place, Carried);
			}
			for (; Run.WashOwed >= 1.f; Run.WashOwed -= 1.f)
			{
				AddWash(Run, Place, Eye, Carried, WashSpeed);
			}
		}
		if (bPlaced && Run.Age < Pour + HaloAfter)
		{
			GlowOver(Effects, Eye, Place, HaloGlow * (Eye.bFirstPerson ? 1.f : ThirdPersonHaloShare) * Glow
				* FMath::Sin(UE_PI * FMath::Clamp(Run.Age / (Pour + HaloAfter), 0.f, 1.f)));
		}
		return Run.Age < Pour + HaloAfter;
	}

	/** One frame of a run: true while it has more to show. */
	bool StepRun(FFlaskRun& Run)
	{
		APawn* Drinker = Run.Drinker.Get();
		UWorld* World = Drinker ? Drinker->GetWorld() : nullptr;
		FWeaponFX* Effects = EffectsFor(World);
		if (!Effects)
		{
			return false;
		}
		// The drinker's own clock (Sundown slows the world, never the player).
		const float DeltaSeconds = Run.bStepped ? World->GetDeltaSeconds() * FMath::Max(Drinker->CustomTimeDilation, 0.f) : 0.f;
		Run.bStepped = true;
		Run.Age += DeltaSeconds;
		const float Glow = FVfxTuning::Live().Glow;
		const FEye Eye = EyeOf(*Drinker);
		const FVector Carried = Drinker->GetVelocity();
		FPlace Place;
		bool bPlaced = true;
		if (Run.bAtHip)
		{
			Place = HipPlaceOf(*Drinker);
		}
		else
		{
			// The given mouth is a first-person fallback only: in third person PlaceOf takes the body's flask or the head.
			bPlaced = PlaceOf(*Drinker, Eye, Eye.bFirstPerson ? Run.MouthInEye.GetPtrOrNull() : nullptr, Place);
		}

		bool bMore = false;
		switch (Run.Look)
		{
		case ELook::Uncork:
			if (bPlaced && Place.bModel)
			{
				if (!Run.bSeenFlask)
				{
					// The first frame it's in sight: the glints the cork lets out.
					for (int32 Index = 0; Index < UncorkGlints; ++Index)
					{
						AddWash(Run, Place, Eye, Carried, WashSpeed * 0.5f);
					}
					Run.bSeenFlask = true;
				}
				GlowOver(*Effects, Eye, Place, UncorkGlow * Glow * Pulse(Run.Age, UncorkRise, UncorkSeconds));
			}
			bMore = Run.Age < UncorkSeconds;
			break;
		case ELook::Flare:
			if (bPlaced)
			{
				GlowOver(*Effects, Eye, Place, FlareGlow * Glow * Pulse(Run.Age, 0.05f, FlareSeconds));
			}
			bMore = Run.Age < FlareSeconds;
			break;
		case ELook::Draught:
		default:
			bMore = StepDraught(Run, Place, bPlaced, Eye, Carried, DeltaSeconds, *Effects, Glow);
			break;
		}
		StepMotes(Run, Place, Eye, SinkOf(*Drinker, Eye), Carried, DeltaSeconds, *Effects, Glow, BodyMiddleOf(*Drinker));
		return bMore || !Run.Motes.IsEmpty();
	}

	void Schedule(const TSharedRef<FFlaskRun>& Run)
	{
		APawn* Drinker = Run->Drinker.Get();
		UWorld* World = Drinker ? Drinker->GetWorld() : nullptr;
		if (!World)
		{
			return;
		}
		// Weak to the drinker: a body gone (a level change, a test's end) simply ends the run.
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Drinker, [Run]()
		{
			if (StepRun(*Run))
			{
				Schedule(Run);
			}
		}));
	}

	TSharedRef<FFlaskRun> MakeRun(APawn& Drinker, ELook Look)
	{
		TSharedRef<FFlaskRun> Run = MakeShared<FFlaskRun>();
		Run->Drinker = &Drinker;
		Run->Look = Look;
		Run->Random.Initialize(static_cast<int32>(GFrameCounter & 0x7fffffff) ^ (static_cast<int32>(Look) << 12));
		return Run;
	}

	/** Shows Run's first frame now and the rest a frame at a time (nothing on a server with no one to see it). */
	void Begin(APawn& Drinker, const TSharedRef<FFlaskRun>& Run)
	{
		const UWorld* World = Drinker.GetWorld();
		if (World && World->GetNetMode() != NM_DedicatedServer && StepRun(*Run))
		{
			Schedule(Run);
		}
	}
}

void FlaskFX::Uncork(APawn& Drinker)
{
	PlayCue(Drinker, FlaskCue::Uncork);
	Begin(Drinker, MakeRun(Drinker, ELook::Uncork));
}

void FlaskFX::Drink(APawn& Drinker, const FVector& Mouth)
{
	PlayCue(Drinker, FlaskCue::Drink);
	PlayCue(Drinker, FlaskCue::Heal);
	// Mouth is the socket's own place; a first-person model shows it elsewhere (its own field of view), so each frame the run
	// takes the model's mouth as it shows, and keeps this one (held still in the view) for frames with no model.
	// In third person the mouth given isn't used at all (PlaceOf takes the body's flask in HandGrip_L, or the head).
	const FEye Eye = EyeOf(Drinker);
	TSharedRef<FFlaskRun> Run = MakeRun(Drinker, ELook::Draught);
	if (Eye.bFirstPerson)
	{
		Run->MouthInEye = ToEye(Eye, Mouth);
	}
	else
	{
		// Seen from behind: a few small motes drawn in to the chest as the soul goes in.
		AddChestMotes(*Run, SinkOf(Drinker, Eye), Drinker.GetVelocity());
	}
	Begin(Drinker, Run);
}

void FlaskFX::ChargeFilled(APawn& Drinker)
{
	PlayCue(Drinker, FlaskCue::ChargeFilled);
	FWeaponFX* Effects = EffectsFor(Drinker.GetWorld());
	if (!Effects)
	{
		return;
	}
	const FEye Eye = EyeOf(Drinker);
	const float Glow = FVfxTuning::Live().Glow;
	// The flask in sight (a kill mid-drink: the model this eye draws, never the other view's), else stowed at the hip (seen
	// looking down, or from behind). A small star at its mouth, a few glints out of it, its light pulsing once.
	TSharedRef<FFlaskRun> Run = MakeRun(Drinker, ELook::Flare);
	FPlace Place;
	if (!FindFlask(Drinker, Eye.bFirstPerson) || !PlaceOf(Drinker, Eye, nullptr, Place))
	{
		Place = HipPlaceOf(Drinker);
		Run->bAtHip = true;
	}
	const float StarMax = Eye.bFirstPerson ? FlareStarMax : ThirdPersonStarMax;
	Effects->SpawnStarburst(Place.Mouth, SizeAt(Eye, Place.Mouth, FlareStarShare, FlareStarMin, StarMax), SoulLight(), FlareStarGlow * Glow,
		FlareStarLife, 6);
	for (int32 Index = 0; Index < FlareGlints; ++Index)
	{
		AddWash(*Run, Place, Eye, Drinker.GetVelocity(), WashSpeed * 0.6f);
	}
	Begin(Drinker, Run);
}

void FlaskFX::Denied(APawn& Drinker, bool bFullHealth)
{
	// Sound only: the HUD's pips say the rest (a red shake, or "Health full").
	PlayCue(Drinker, bFullHealth ? FlaskCue::Full : FlaskCue::Empty);
}

void FlaskFX::RefilledAll(APawn& Drinker)
{
	// Quiet on purpose: it comes with the respawn, which has its own moment.
	PlayCue(Drinker, FlaskCue::Refilled);
}
