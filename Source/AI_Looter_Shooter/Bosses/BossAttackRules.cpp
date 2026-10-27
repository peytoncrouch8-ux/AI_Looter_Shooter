// BossAttackRules (BossAttackRules.h): the bosses' shared attack rules, their cvars, a boss's attack log and its hurt zones.

#include "Bosses/BossAttackRules.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarRules(TEXT("Looter.Boss.Attack.Rules"), 1,
		TEXT("1: the bosses (the Gravemother, the Foreman, Abel) choose their attacks under the shared rules: a gap between attacks, ")
		TEXT("no long idle stretch, never the same attack three times in a row, reach and facing. 0: each chooses as before."));

	TAutoConsoleVariable<float> CVarMinGap(TEXT("Looter.Boss.Attack.MinGap"), BossAttackRules::DefaultMinGap,
		TEXT("Seconds at least between the end of a boss's attack and the start of its next (any move or shot)."));

	TAutoConsoleVariable<float> CVarMaxIdle(TEXT("Looter.Boss.Attack.MaxIdle"), BossAttackRules::DefaultMaxIdle,
		TEXT("Seconds a boss may stand free to attack without attacking before it readies a move it has (0: no cap)."));

	TAutoConsoleVariable<int32> CVarMaxRepeats(TEXT("Looter.Boss.Attack.MaxRepeats"), BossAttackRules::DefaultMaxRepeats,
		TEXT("The same boss attack at most this many times in a row (2: never three; 0: no limit)."));

	TAutoConsoleVariable<float> CVarFrontDegrees(TEXT("Looter.Boss.Attack.FrontDegrees"), BossAttackRules::DefaultFrontDegrees,
		TEXT("A boss's frontal attack (a bite, a swing, a lunge, a stomp) starts only with its target within this many degrees of its front, each side."));

	TAutoConsoleVariable<float> CVarAimedDegrees(TEXT("Looter.Boss.Attack.AimedDegrees"), BossAttackRules::DefaultAimedDegrees,
		TEXT("A boss's aimed attack (a lob, a shot it turns to aim through its tell) starts only with its target within this many degrees of its front."));

	TAutoConsoleVariable<float> CVarActiveSeconds(TEXT("Looter.Boss.Attack.ActiveSeconds"), BossAttackRules::DefaultActiveSeconds,
		TEXT("Seconds a boss's one-frame blow (a bite, a slam) is drawn live after it lands (Looter.Boss.HurtZones)."));
}

// ---------------------------------------------------------------------------
// Hurt zones
// ---------------------------------------------------------------------------

FBossHurtZone FBossHurtZone::Disc(FName InAttack, EBossAttackWindow InWindow, const FVector& InCenter, float InRadius, float InHeight)
{
	FBossHurtZone Zone;
	Zone.Attack = InAttack;
	Zone.Shape = EBossHurtShape::Disc;
	Zone.Window = InWindow;
	Zone.Center = InCenter;
	Zone.End = InCenter;
	Zone.Radius = InRadius;
	Zone.Height = InHeight;
	return Zone;
}

FBossHurtZone FBossHurtZone::Wedge(FName InAttack, EBossAttackWindow InWindow, const FVector& InApex, const FVector& InDirection,
	float InRadius, float InHalfAngle, float InHeight)
{
	FBossHurtZone Zone = Disc(InAttack, InWindow, InApex, InRadius, InHeight);
	Zone.Shape = EBossHurtShape::Wedge;
	const FVector Flat = InDirection.GetSafeNormal2D();
	Zone.Direction = Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	Zone.HalfAngle = InHalfAngle;
	return Zone;
}

FBossHurtZone FBossHurtZone::Lane(FName InAttack, EBossAttackWindow InWindow, const FVector& InStart, const FVector& InEnd, float InHalfWidth,
	float InHeight)
{
	FBossHurtZone Zone = Disc(InAttack, InWindow, InStart, InHalfWidth, InHeight);
	Zone.Shape = EBossHurtShape::Lane;
	Zone.End = InEnd;
	const FVector Flat = (InEnd - InStart).GetSafeNormal2D();
	Zone.Direction = Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	return Zone;
}

bool FBossHurtZone::Contains(const FVector& Point) const
{
	// Its ground is Center's height: a little under it still counts (a step down), its height over it.
	const double Rise = Point.Z - Center.Z;
	if (Rise < -Height * 0.5 || Rise > Height)
	{
		return false;
	}
	switch (Shape)
	{
	case EBossHurtShape::Disc:
		return FVector::Dist2D(Point, Center) <= Radius;
	case EBossHurtShape::Wedge:
		return FVector::Dist2D(Point, Center) <= Radius && BossAttackRules::IsFacing(HalfAngle, Direction, Point - Center);
	case EBossHurtShape::Lane:
	{
		const FVector2D Flat(Point.X, Point.Y);
		const FVector2D Nearest = FMath::ClosestPointOnSegment2D(Flat, FVector2D(Center.X, Center.Y), FVector2D(End.X, End.Y));
		return FVector2D::Distance(Flat, Nearest) <= Radius;
	}
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// A boss's log
// ---------------------------------------------------------------------------

void FBossAttackLog::Reset()
{
	*this = FBossAttackLog();
}

void FBossAttackLog::Advance(float DeltaSeconds, bool bIdle)
{
	const float Step = FMath::Max(DeltaSeconds, 0.f);
	if (!bAttacking)
	{
		SinceEnd += Step;
	}
	// The idle clock counts only while it could attack and doesn't; anything else (an attack, a stagger, a moment of its own,
	// nobody to fight) starts it again.
	Idle = bIdle && !bAttacking ? Idle + Step : 0.f;
}

void FBossAttackLog::NoteStarted(FName Id)
{
	Streak = Id == Last ? Streak + 1 : 1;
	Last = Id;
	++Started;
	bAttacking = true;
	Idle = 0.f;
}

void FBossAttackLog::NoteEnded()
{
	if (bAttacking)
	{
		bAttacking = false;
		SinceEnd = 0.f;
		Idle = 0.f;
	}
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

namespace BossAttackRules
{
	bool IsOn()
	{
		return CVarRules.GetValueOnGameThread() != 0;
	}

	float MinGap()
	{
		return FMath::Max(CVarMinGap.GetValueOnGameThread(), 0.f);
	}

	float MaxIdle()
	{
		return FMath::Max(CVarMaxIdle.GetValueOnGameThread(), 0.f);
	}

	int32 MaxRepeats()
	{
		return FMath::Max(CVarMaxRepeats.GetValueOnGameThread(), 0);
	}

	float FrontDegrees()
	{
		return FMath::Clamp(CVarFrontDegrees.GetValueOnGameThread(), 5.f, 180.f);
	}

	float AimedDegrees()
	{
		return FMath::Clamp(CVarAimedDegrees.GetValueOnGameThread(), 5.f, 180.f);
	}

	float ActiveSeconds()
	{
		return FMath::Max(CVarActiveSeconds.GetValueOnGameThread(), 0.02f);
	}

	bool IsInReach(const FBossAttackSpec& Spec, float Distance)
	{
		return Distance >= Spec.MinReach && (Spec.MaxReach <= 0.f || Distance <= Spec.MaxReach);
	}

	bool IsFacing(float FacingDegrees, const FVector& Forward, const FVector& ToTarget)
	{
		if (FacingDegrees >= 180.f)
		{
			return true;
		}
		const FVector Flat = ToTarget.GetSafeNormal2D();
		const FVector Front = Forward.GetSafeNormal2D();
		if (Flat.IsNearlyZero() || Front.IsNearlyZero())
		{
			return true;
		}
		// A hair of slack so a target dead on the cone's edge (the tests' 70 degrees) counts.
		return FVector::DotProduct(Front, Flat) >= FMath::Cos(FMath::DegreesToRadians(FacingDegrees)) - 1.0e-4;
	}

	bool WouldRepeat(FName Id, FName Last, int32 Streak, int32 InMaxRepeats)
	{
		return InMaxRepeats > 0 && !Id.IsNone() && Id == Last && Streak >= InMaxRepeats;
	}

	bool IsGapOpen(float SinceEnd, bool bAttacking, float Gap)
	{
		return !bAttacking && SinceEnd >= Gap;
	}

	bool IsOverdue(float Idle, float Cap)
	{
		return Cap > 0.f && Idle >= Cap;
	}

	EBossAttackWindow WindowAt(float Time, float Windup, float Active, float Recovery)
	{
		if (Time < 0.f)
		{
			return EBossAttackWindow::None;
		}
		if (Time < Windup)
		{
			return EBossAttackWindow::Windup;
		}
		// Its active moment never runs past its recovery's end (a blow with a short recovery is drawn live only that long).
		const float End = Windup + FMath::Max(Recovery, 0.f);
		if (Time < FMath::Min(Windup + FMath::Max(Active, 0.f), End))
		{
			return EBossAttackWindow::Active;
		}
		return Time < End ? EBossAttackWindow::Recovery : EBossAttackWindow::None;
	}

	float DegreesForDot(float Dot)
	{
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	}

	bool IsBlocked(FName Id, const FBossAttackLog& Log)
	{
		return IsOn() && WouldRepeat(Id, Log.GetLast(), Log.GetStreak(), MaxRepeats());
	}

	bool IsGapOpen(const FBossAttackLog& Log)
	{
		return !IsOn() || IsGapOpen(Log.GetSinceEnd(), Log.IsAttacking(), MinGap());
	}

	bool IsOverdue(const FBossAttackLog& Log)
	{
		return IsOn() && IsOverdue(Log.GetIdle(), MaxIdle());
	}

	bool IsPlaced(const FBossAttackSpec& Spec, const FVector& Forward, const FVector& ToTarget)
	{
		return !IsOn() || (IsInReach(Spec, static_cast<float>(ToTarget.Size2D())) && IsFacing(Spec.FacingDegrees, Forward, ToTarget));
	}

	bool MayStart(const FBossAttackSpec& Spec, const FBossAttackLog& Log, const FVector& Forward, const FVector& ToTarget)
	{
		return !IsBlocked(Spec.Id, Log) && IsGapOpen(Log) && IsPlaced(Spec, Forward, ToTarget);
	}
}
