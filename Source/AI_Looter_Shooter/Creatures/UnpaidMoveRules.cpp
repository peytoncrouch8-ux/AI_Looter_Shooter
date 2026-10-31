#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidOpeningRules.h"

// The Unpaid's varied fight as plain rules (2026-10-29): the weighted pick of a turn's move, the pack's board of who commits
// what at one player, and the failed-lunge shriek's chance, phases, interrupt and reach. AUnpaidCreature plays them.

namespace
{
	/** The order a fallback looks in when no move weighs anything: the plain ones first. */
	constexpr EUnpaidMove FallbackOrder[] = { EUnpaidMove::Lunge, EUnpaidMove::Rake, EUnpaidMove::Feint, EUnpaidMove::LowSwipe,
		EUnpaidMove::PhaseStrike };

	/** In range for Move now (its start, or closing in to it when planned ahead), and no nearer than its least. */
	bool InRange(EUnpaidMove Move, const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning, bool& bOutClosing)
	{
		bOutClosing = false;
		const float Distance = Context.Distance;
		const float Start = UnpaidMoveRules::StartDistance(Move, Context, Tuning);
		float Least = 0.f;
		switch (Move)
		{
		case EUnpaidMove::Lunge: Least = Tuning.LungeMinRange; break;
		case EUnpaidMove::Feint: Least = Tuning.FeintMinRange; break;
		case EUnpaidMove::PhaseStrike: Least = Tuning.PhaseStrikeMinRange; break;
		case EUnpaidMove::LowSwipe: Least = Tuning.LowSwipeMinRange; break;
		default: break;
		}
		if (Distance < Least)
		{
			return false;
		}
		if (Distance <= Start)
		{
			return true;
		}
		// A close move planned farther out walks in to its start (never as an attack begins: then it strikes from where it is).
		if (!Context.bNow && UnpaidMoveRules::ClosesIn(Move) && Distance <= FMath::Max(Tuning.CloseInRange, Start))
		{
			bOutClosing = true;
			return true;
		}
		// A phase-strike blinks: planned a little past the lunge's start (its turn walks it in the rest), it goes as it begins.
		return !Context.bNow && Move == EUnpaidMove::PhaseStrike && Distance <= FMath::Max(Tuning.CloseInRange, Start);
	}

	/** Whether Move may be picked at all here: not a pack-mate's now, not a third in a row (a hang-back may be both). */
	bool Allowed(EUnpaidMove Move, const FUnpaidMoveContext& Context)
	{
		if (Move == EUnpaidMove::HangBack)
		{
			return true;
		}
		return (Context.Taken & UnpaidMoveRules::Bit(Move)) == 0 && !UnpaidMoveRules::WouldRepeat(Move, Context.Last, Context.BeforeLast);
	}
}

// ---------------------------------------------------------------------------
// The moves
// ---------------------------------------------------------------------------

bool UnpaidMoveRules::IsAttack(EUnpaidMove Move)
{
	return Move != EUnpaidMove::None && Move != EUnpaidMove::HangBack;
}

bool UnpaidMoveRules::IsFrontal(EUnpaidMove Move)
{
	return Move == EUnpaidMove::Lunge || Move == EUnpaidMove::Rake || Move == EUnpaidMove::Feint || Move == EUnpaidMove::LowSwipe;
}

bool UnpaidMoveRules::ClosesIn(EUnpaidMove Move)
{
	return Move == EUnpaidMove::Rake || Move == EUnpaidMove::LowSwipe;
}

const TCHAR* UnpaidMoveRules::Name(EUnpaidMove Move)
{
	switch (Move)
	{
	case EUnpaidMove::None: return TEXT("None");
	case EUnpaidMove::Lunge: return TEXT("Lunge");
	case EUnpaidMove::Rake: return TEXT("Rake");
	case EUnpaidMove::Feint: return TEXT("Feint");
	case EUnpaidMove::PhaseStrike: return TEXT("PhaseStrike");
	case EUnpaidMove::LowSwipe: return TEXT("LowSwipe");
	case EUnpaidMove::HangBack: return TEXT("HangBack");
	}
	return TEXT("Unknown");
}

bool UnpaidMoveRules::WouldRepeat(EUnpaidMove Move, EUnpaidMove Last, EUnpaidMove BeforeLast)
{
	static_assert(MaxRun == 2, "WouldRepeat looks back two moves");
	return IsAttack(Move) && Move == Last && Move == BeforeLast;
}

float UnpaidMoveRules::StartDistance(EUnpaidMove Move, const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning)
{
	switch (Move)
	{
	case EUnpaidMove::Rake: return Context.RakeRange;
	case EUnpaidMove::LowSwipe: return Tuning.LowSwipeStart;
	case EUnpaidMove::Lunge:
	case EUnpaidMove::Feint:
	case EUnpaidMove::PhaseStrike: return Context.LungeStart;
	default: return 0.f;
	}
}

void UnpaidMoveRules::Weights(const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning, float Out[NumUnpaidMoves])
{
	for (int32 Slot = 0; Slot < NumUnpaidMoves; ++Slot)
	{
		Out[Slot] = 0.f;
	}
	// One pressures while another flanks: a pack-mate at their side makes the frontal moves likelier, one in front the blink.
	const float Press = Context.bMateSide ? Tuning.PressBoost : 1.f;
	auto Weigh = [&](EUnpaidMove Move, float Weight)
	{
		// A player open (a reload, a swap, an empty gun, the flask) is pressed: the quick commits up, the feint down.
		Weight *= UnpaidOpeningRules::WeightScale(Move, Context.bOpening, Context.bPunisher, Context.bInFront);
		bool bClosing = false;
		if (Weight <= 0.f || !InRange(Move, Context, Tuning, bClosing) || !Allowed(Move, Context) || Context.Cooldowns[Index(Move)] > 0.f)
		{
			return;
		}
		Out[Index(Move)] = Weight * (bClosing ? Tuning.CloseInShare : 1.f);
	};
	Weigh(EUnpaidMove::Lunge, Tuning.LungeWeight * Press);
	Weigh(EUnpaidMove::Rake, Tuning.RakeWeight * Press);
	// A still or hidden player doesn't dodge: a feint baits nobody (a third as likely).
	const float FeintRead = (Context.bLastLungeMissed ? Tuning.MissFeintBoost : 1.f) * (Context.bCamping ? 0.3f : 1.f);
	Weigh(EUnpaidMove::Feint, Tuning.FeintWeight * FeintRead * Press);
	Weigh(EUnpaidMove::LowSwipe, Tuning.LowSwipeWeight * Press);
	// The blink only while the pack's gate allows one; paired with a pack-mate coming from the front, rarer with one already
	// at their side (one flanks at a time), likelier at a camper.
	if (Context.bMayBlink)
	{
		const float Pair = (Context.bMateFront ? Tuning.PairBoost : 1.f) * (Context.bMateSide ? 0.25f : 1.f) * (Context.bCamping ? 1.5f : 1.f);
		Weigh(EUnpaidMove::PhaseStrike, Tuning.PhaseStrikeWeight * Pair);
	}
	// Hanging back keeps its distance and waits for a pack-mate to commit (one pressing already, it waits for the next): only
	// with pack-mates in the fight, never while one is at the player's side (that one needs the front pressed). A first try
	// with "none committed" only (a 4-pack's simulation, 2026-10-29) never hung back: two are nearly always mid-attack.
	if (!Context.bNow && Context.Mates > 0 && !Context.bMateSide && Context.Cooldowns[Index(EUnpaidMove::HangBack)] <= 0.f)
	{
		Out[Index(EUnpaidMove::HangBack)] = Tuning.HangBackWeight
			* UnpaidOpeningRules::WeightScale(EUnpaidMove::HangBack, Context.bOpening, Context.bPunisher, Context.bInFront);
	}
}

EUnpaidMove UnpaidMoveRules::Choose(const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning, float Roll)
{
	float Weight[NumUnpaidMoves];
	Weights(Context, Tuning, Weight);
	float Sum = 0.f;
	for (const float Each : Weight)
	{
		Sum += Each;
	}
	if (Sum > 0.f)
	{
		float Left = FMath::Clamp(Roll, 0.f, 0.99999f) * Sum;
		for (int32 Slot = 0; Slot < NumUnpaidMoves; ++Slot)
		{
			if (Weight[Slot] <= 0.f)
			{
				continue;
			}
			if (Left < Weight[Slot])
			{
				return static_cast<EUnpaidMove>(Slot);
			}
			Left -= Weight[Slot];
		}
	}
	// Nothing open with pack-mates fighting: it waits for one of them to commit (never at a player who is open: it presses).
	if (!Context.bNow && Context.Mates > 0 && !Context.bOpening && !Context.bPunisher)
	{
		return EUnpaidMove::HangBack;
	}
	// Alone (or already into its attack): any move in range that is neither a third in a row nor a pack-mate's, its cooldown
	// waived; the blink still only when the gate allows it.
	for (const EUnpaidMove Move : FallbackOrder)
	{
		bool bClosing = false;
		if ((Move != EUnpaidMove::PhaseStrike || Context.bMayBlink) && InRange(Move, Context, Tuning, bClosing) && Allowed(Move, Context))
		{
			return Move;
		}
	}
	// Right on top of them with every near move spent: the rake, unless that's a third, else the lunge (it never stands idle).
	return WouldRepeat(EUnpaidMove::Rake, Context.Last, Context.BeforeLast) ? EUnpaidMove::Lunge : EUnpaidMove::Rake;
}

// ---------------------------------------------------------------------------
// The failed-lunge shriek
// ---------------------------------------------------------------------------

bool UnpaidMoveRules::ShriekOnMiss(float Roll, float Chance, float SinceOwn, float Cooldown, float SincePack, float PackGap)
{
	return Roll < Chance && SinceOwn >= Cooldown && SincePack >= PackGap;
}

EUnpaidShriekPhase UnpaidMoveRules::ShriekPhaseAt(float Time, float Tell, float Scream, float Recover)
{
	if (Time < 0.f)
	{
		return EUnpaidShriekPhase::None;
	}
	if (Time < Tell)
	{
		return EUnpaidShriekPhase::Tell;
	}
	if (Time < Tell + Scream)
	{
		return EUnpaidShriekPhase::Scream;
	}
	return Time < Tell + Scream + Recover ? EUnpaidShriekPhase::Recover : EUnpaidShriekPhase::None;
}

bool UnpaidMoveRules::InterruptsShriek(EUnpaidShriekPhase Phase, bool bCoalHit)
{
	return bCoalHit && Phase == EUnpaidShriekPhase::Tell;
}

float UnpaidMoveRules::RattleShare(float Distance, float Radius)
{
	if (Radius <= 0.f || Distance > Radius)
	{
		return 0.f;
	}
	const float Full = Radius / 3.f;
	if (Distance <= Full)
	{
		return 1.f;
	}
	// Eased from full at a third of its reach to a third at its edge: the punish is for being near, not for being in the yard.
	const float Alpha = (Distance - Full) / FMath::Max(Radius - Full, 1.f);
	return FMath::Lerp(1.f, 1.f / 3.f, FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f));
}

bool UnpaidMoveRules::ClearsLowSwipe(float FeetOverIts, bool bAirborne, float Clear)
{
	// A jump clears it as soon as the feet are off the ground a little; standing on something as high clears it too.
	return FeetOverIts >= Clear || (bAirborne && FeetOverIts >= 0.35f * Clear);
}

// ---------------------------------------------------------------------------
// The pack's board
// ---------------------------------------------------------------------------

void FUnpaidMoveBoard::Commit(uint32 Id, EUnpaidMove Move, double Now)
{
	if (Move == EUnpaidMove::None)
	{
		Release(Id);
		return;
	}
	if (FEntry* Mine = Entries.FindByPredicate([Id](const FEntry& Each) { return Each.Id == Id; }))
	{
		if (Mine->Move != Move)
		{
			Mine->Move = Move;
			Mine->Since = Now;
		}
		return;
	}
	Entries.Add({ Id, Move, Now });
}

void FUnpaidMoveBoard::Release(uint32 Id)
{
	Entries.RemoveAll([Id](const FEntry& Each) { return Each.Id == Id; });
}

uint32 FUnpaidMoveBoard::TakenBy(uint32 ExceptId) const
{
	uint32 Bits = 0;
	for (const FEntry& Each : Entries)
	{
		if (Each.Id != ExceptId)
		{
			Bits |= UnpaidMoveRules::Bit(Each.Move);
		}
	}
	return Bits;
}

bool FUnpaidMoveBoard::AnyFront(uint32 ExceptId) const
{
	return Entries.ContainsByPredicate([ExceptId](const FEntry& Each) { return Each.Id != ExceptId && UnpaidMoveRules::IsFrontal(Each.Move); });
}

bool FUnpaidMoveBoard::AnySide(uint32 ExceptId) const
{
	return Entries.ContainsByPredicate([ExceptId](const FEntry& Each) { return Each.Id != ExceptId && Each.Move == EUnpaidMove::PhaseStrike; });
}

bool FUnpaidMoveBoard::AnyCommittedSince(uint32 ExceptId, double Time) const
{
	return Entries.ContainsByPredicate([ExceptId, Time](const FEntry& Each)
	{
		return Each.Id != ExceptId && UnpaidMoveRules::IsAttack(Each.Move) && Each.Since >= Time;
	});
}

EUnpaidMove FUnpaidMoveBoard::MoveOf(uint32 Id) const
{
	const FEntry* Mine = Entries.FindByPredicate([Id](const FEntry& Each) { return Each.Id == Id; });
	return Mine ? Mine->Move : EUnpaidMove::None;
}

void FUnpaidMoveBoard::Prune(double Now, float MaxAge)
{
	Entries.RemoveAll([Now, MaxAge](const FEntry& Each) { return Now - Each.Since > MaxAge; });
}

// ---------------------------------------------------------------------------
// The state
// ---------------------------------------------------------------------------

void FUnpaidMoveState::Reset()
{
	*this = FUnpaidMoveState();
}
