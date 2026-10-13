#include "Creatures/CreaturePursuit.h"

const TCHAR* LexToString(EPursuitEnd End)
{
	switch (End)
	{
	case EPursuitEnd::Gone: return TEXT("gone");
	case EPursuitEnd::Sheltered: return TEXT("in a safe zone");
	case EPursuitEnd::Scene: return TEXT("in a scene");
	case EPursuitEnd::Lost: return TEXT("lost");
	case EPursuitEnd::OffGround: return TEXT("off its ground");
	case EPursuitEnd::OutOfReach: return TEXT("out of its reach");
	default: return TEXT("still hunting");
	}
}

void FCreaturePursuit::Begin(const FVector& Prey)
{
	SetStep(EPursuitStep::Hunt);
	Walk = FWalk();
	End = EPursuitEnd::None;
	LastKnown = Prey;
	Unseen = 0.f;
	Patience = 0.f;
	bProwlOffGround = false;
	Overhead = 0.f;
	BestReach = TNumericLimits<float>::Max();
	ProgressPrey = Prey;
	Stalled = 0.f;
}

EPursuitStep FCreaturePursuit::Look(const FLook& In, const FRules& Rules)
{
	StepTime += In.Interval;
	if (Step == EPursuitStep::GiveUp)
	{
		return Step;
	}
	// The real reasons that end a hunt at once.
	if (In.bGone)
	{
		return Finish(EPursuitEnd::Gone);
	}
	if (In.bInScene)
	{
		return Finish(EPursuitEnd::Scene);
	}
	if (In.bSheltered)
	{
		return Finish(EPursuitEnd::Sheltered);
	}

	const float Scale = FMath::Max(In.SizeScale, 0.1f);
	const float Reach = static_cast<float>(FVector::Dist(In.Here, In.Prey));

	// Off its ground the chase can't follow (Docs/Areas/RansomsRest.md, "Encounters stay on one level of ground"): it waits at
	// its edge watching them, while its patience lasts, and comes again the moment they're back on its ground.
	if (In.bOffGround)
	{
		if (Step != EPursuitStep::Prowl || !bProwlOffGround)
		{
			SetStep(EPursuitStep::Prowl);
			// From under a target out of its reach, it walks up to its edge afresh.
			Walk = FWalk();
			bProwlOffGround = true;
		}
		return Wait(In.Interval, Rules, EPursuitEnd::OffGround);
	}
	if (Step == EPursuitStep::Prowl && bProwlOffGround)
	{
		// It was watching them: it knows where they stepped back on.
		SetStep(EPursuitStep::Hunt);
		ResetProgress(Reach, In.Prey);
		Overhead = 0.f;
		Unseen = 0.f;
		LastKnown = In.Prey;
	}

	if (In.bSeen)
	{
		Unseen = 0.f;
		LastKnown = In.Prey;
	}
	else
	{
		Unseen += In.Interval;
	}

	// Under a target out of its reach: it tries again once they come down to it or move off, else it waits on.
	if (Step == EPursuitStep::Prowl)
	{
		const bool bCameNear = !In.bFarOff && !In.bOverhead;
		const bool bMovedOff = FVector::Dist(In.Prey, ProwlFrom) > Rules.ResumeShift * Scale;
		if (!bCameNear && !bMovedOff)
		{
			return Wait(In.Interval, Rules, EPursuitEnd::OutOfReach);
		}
		SetStep(EPursuitStep::Hunt);
		ResetProgress(Reach, In.Prey);
		Overhead = 0.f;
	}

	if (Step == EPursuitStep::Hunt)
	{
		// Out of sight a moment, it still follows where they went (it heard them).
		if (!In.bSeen && Unseen < Rules.UnseenGrace)
		{
			LastKnown = In.Prey;
		}
		// Out of its reach: near but above or below its strike, or making its way in without getting nearer while they stand.
		Overhead = In.bOverhead ? Overhead + In.Interval : 0.f;
		if (In.bFarOff && !In.bBreakingOff)
		{
			const bool bNearer = Reach < BestReach - Rules.ProgressStep * Scale;
			const bool bTheyMoved = FVector::Dist(In.Prey, ProgressPrey) > Rules.TargetShift * Scale;
			if (bNearer || bTheyMoved)
			{
				ResetProgress(Reach, In.Prey);
			}
			else
			{
				Stalled += In.Interval;
			}
		}
		else
		{
			ResetProgress(Reach, In.Prey);
		}
		if (Overhead >= Rules.OverheadSeconds || Stalled >= Rules.StuckSeconds)
		{
			SetStep(EPursuitStep::Prowl);
			bProwlOffGround = false;
			ProwlFrom = In.Prey;
			return Wait(0.f, Rules, EPursuitEnd::OutOfReach);
		}
		// Breaking off it runs from them, and doesn't go looking until it turns again.
		if (In.bSeen || In.bBreakingOff || Unseen < Rules.UnseenGrace)
		{
			return Step;
		}
		SetStep(EPursuitStep::Track);
	}

	// Searching: seen again, the hunt is on.
	if (In.bSeen)
	{
		SetStep(EPursuitStep::Hunt);
		ResetProgress(Reach, In.Prey);
		Overhead = 0.f;
		return Step;
	}
	if (Unseen >= Rules.MaxUnseenSeconds)
	{
		return Finish(EPursuitEnd::Lost);
	}
	const float Arrive = FMath::Max(Rules.ArriveRadius * Scale, In.HoldDistance);
	if (Step == EPursuitStep::Track && (FVector::DistSquared2D(In.Here, LastKnown) <= FMath::Square(Arrive) || StepTime >= Rules.TrackSeconds))
	{
		SetStep(EPursuitStep::Sniff);
	}
	if (Step == EPursuitStep::Sniff && StepTime >= Rules.SniffSeconds)
	{
		if (!In.bBeyondSenses)
		{
			// Still near: it catches their scent and goes after it.
			LastKnown = In.Prey;
			SetStep(EPursuitStep::Track);
		}
		else if (Unseen >= Rules.LostSeconds)
		{
			return Finish(EPursuitEnd::Lost);
		}
		// Otherwise it sniffs on until it has been long enough without them.
	}
	return Step;
}

void FCreaturePursuit::Reacquire(const FVector& Prey)
{
	if (Step == EPursuitStep::GiveUp)
	{
		Begin(Prey);
		return;
	}
	Unseen = 0.f;
	LastKnown = Prey;
	if (Step == EPursuitStep::Track || Step == EPursuitStep::Sniff)
	{
		SetStep(EPursuitStep::Hunt);
		BestReach = TNumericLimits<float>::Max();
		ProgressPrey = Prey;
		Stalled = 0.f;
	}
}

void FCreaturePursuit::NoteHit(const FVector& From)
{
	Patience = 0.f;
	Reacquire(From);
}

void FCreaturePursuit::NoteReached()
{
	Patience = 0.f;
	Stalled = 0.f;
	Overhead = 0.f;
	BestReach = TNumericLimits<float>::Max();
}

void FCreaturePursuit::Stop(EPursuitEnd Why)
{
	Finish(Why);
}

void FCreaturePursuit::Spurn(uint32 PawnId, const FVector& Where, double Until)
{
	SpurnedId = PawnId;
	SpurnedAt = Where;
	SpurnedUntil = Until;
}

bool FCreaturePursuit::IsSpurned(uint32 PawnId, const FVector& Where, double Now, float ResumeShift) const
{
	return PawnId == SpurnedId && Now < SpurnedUntil && FVector::DistSquared(Where, SpurnedAt) <= FMath::Square(static_cast<double>(ResumeShift));
}

void FCreaturePursuit::SetStep(EPursuitStep NewStep)
{
	if (Step != NewStep)
	{
		Walk = FWalk();
	}
	Step = NewStep;
	StepTime = 0.f;
}

EPursuitStep FCreaturePursuit::Finish(EPursuitEnd Why)
{
	End = Why;
	SetStep(EPursuitStep::GiveUp);
	return Step;
}

EPursuitStep FCreaturePursuit::Wait(float Interval, const FRules& Rules, EPursuitEnd Why)
{
	Patience += Interval;
	return Patience >= Rules.ProwlSeconds ? Finish(Why) : Step;
}

void FCreaturePursuit::ResetProgress(float Reach, const FVector& Prey)
{
	BestReach = Reach;
	ProgressPrey = Prey;
	Stalled = 0.f;
}
