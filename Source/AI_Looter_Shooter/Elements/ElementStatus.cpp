#include "Elements/ElementStatus.h"

void FElementStatus::Apply(ELooterElement Element, float HitDamage, EElementAffinity Affinity, bool bBoss, const FElementTuning& Tuning)
{
	if (HitDamage <= 0.f)
	{
		return;
	}
	const float Scale = ElementRules::AffinityScale(Affinity, Tuning);
	// A boss carries it half as long at the same rate: half the damage, never a slow that holds its moves up (the
	// component leaves the slow off).
	const float BossShare = bBoss ? Tuning.BossDuration : 1.f;
	switch (Element)
	{
	case ELooterElement::Ember:
	{
		const float Total = HitDamage * Tuning.EmberShare * Scale * BossShare;
		const float Seconds = Tuning.EmberSeconds * BossShare;
		if (Total <= 0.f || Seconds <= 0.f)
		{
			return;
		}
		const FOverTime Burn{ Total, Total / Seconds, Seconds };
		if (Burns.Num() < Tuning.EmberMaxStacks)
		{
			Burns.Add(Burn);
			return;
		}
		// Every stack burning: the new one takes the weakest's place if it's stronger than what that one has left.
		int32 Weakest = 0;
		for (int32 Index = 1; Index < Burns.Num(); ++Index)
		{
			Weakest = Burns[Index].DamageLeft < Burns[Weakest].DamageLeft ? Index : Weakest;
		}
		if (Total > Burns[Weakest].DamageLeft)
		{
			Burns[Weakest] = Burn;
		}
		return;
	}
	case ELooterElement::SoulLight:
	{
		const float Burst = HitDamage * Tuning.SoulLightShare * Scale * BossShare;
		if (Burst <= 0.f)
		{
			return;
		}
		if (Smites.Num() < Tuning.SoulLightMaxPending)
		{
			Smites.Add({ Burst, 0.f });
		}
		else
		{
			// Full: the light gathers into the latest one rather than being lost.
			Smites.Last().Damage += Burst;
		}
		return;
	}
	case ELooterElement::GraveSalt:
	{
		const float Total = HitDamage * Tuning.GraveSaltShare * Scale * BossShare;
		const float Seconds = Tuning.GraveSaltSeconds * BossShare;
		if (Total <= 0.f || Seconds <= 0.f)
		{
			return;
		}
		// One salting at a time: a new one keeps whichever has more to deal, and the longer time; the rate follows.
		Salt.DamageLeft = FMath::Max(Salt.DamageLeft, Total);
		Salt.TimeLeft = FMath::Max(Salt.TimeLeft, Seconds);
		Salt.PerSecond = Salt.DamageLeft / Salt.TimeLeft;
		return;
	}
	case ELooterElement::Thunder:
	{
		const float Burst = HitDamage * Tuning.ThunderShare * Scale * BossShare;
		if (Burst <= 0.f)
		{
			return;
		}
		if (Jolts.Num() < Tuning.ThunderMaxPending)
		{
			Jolts.Add({ Burst, 0.f });
		}
		else
		{
			Jolts.Last().Damage += Burst;
		}
		return;
	}
	case ELooterElement::Rot:
	{
		const float Total = HitDamage * Tuning.RotShare * Scale * BossShare;
		const float Seconds = Tuning.RotSeconds * BossShare;
		if (Total <= 0.f || Seconds <= 0.f)
		{
			return;
		}
		// One rot: a new hit adds to what is left and starts its time over, so it keeps growing while the hits come.
		Rot.DamageLeft += Total;
		Rot.TimeLeft = Seconds;
		RotLength = Seconds;
		Rot.PerSecond = Rot.DamageLeft / Seconds;
		return;
	}
	default:
		return;
	}
}

float FElementStatus::DrainRot(FOverTime& Status, float Length, float DeltaSeconds)
{
	if (Status.TimeLeft <= 0.f)
	{
		return 0.f;
	}
	if (Status.TimeLeft <= DeltaSeconds)
	{
		const float Last = Status.DamageLeft;
		Status.DamageLeft = 0.f;
		Status.TimeLeft = 0.f;
		return Last;
	}
	const float Growth = Length > 0.f ? FMath::Clamp(1.f - Status.TimeLeft / Length, 0.f, 1.f) : 1.f;
	const float Dealt = FMath::Min(Status.PerSecond * (0.5f + Growth) * DeltaSeconds, Status.DamageLeft);
	Status.DamageLeft -= Dealt;
	Status.TimeLeft -= DeltaSeconds;
	return Dealt;
}

float FElementStatus::Drain(FOverTime& Status, float DeltaSeconds)
{
	if (Status.TimeLeft <= 0.f)
	{
		return 0.f;
	}
	// Its last step deals whatever is left, so the whole share always comes off.
	if (Status.TimeLeft <= DeltaSeconds)
	{
		const float Last = Status.DamageLeft;
		Status.DamageLeft = 0.f;
		Status.TimeLeft = 0.f;
		return Last;
	}
	const float Dealt = FMath::Min(Status.PerSecond * DeltaSeconds, Status.DamageLeft);
	Status.DamageLeft -= Dealt;
	Status.TimeLeft -= DeltaSeconds;
	return Dealt;
}

void FElementStatus::Advance(float DeltaSeconds, const FElementTuning& Tuning, TArray<FElementTick>& Out)
{
	if (DeltaSeconds <= 0.f || !IsActive())
	{
		return;
	}
	SinceTick += DeltaSeconds;

	for (int32 Index = Burns.Num() - 1; Index >= 0; --Index)
	{
		EmberOwed += Drain(Burns[Index], DeltaSeconds);
		if (Burns[Index].TimeLeft <= 0.f)
		{
			Burns.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}
	SaltOwed += Drain(Salt, DeltaSeconds);
	RotOwed += DrainRot(Rot, RotLength, DeltaSeconds);

	for (int32 Index = Jolts.Num() - 1; Index >= 0; --Index)
	{
		FSmite& Jolt = Jolts[Index];
		Jolt.Wait += DeltaSeconds;
		if (Jolt.Wait >= Tuning.ThunderDelay)
		{
			FElementTick Tick;
			Tick.Element = ELooterElement::Thunder;
			Tick.Damage = Jolt.Damage;
			Tick.bJolt = true;
			Out.Add(Tick);
			Jolts.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}

	for (int32 Index = Smites.Num() - 1; Index >= 0; --Index)
	{
		FSmite& Smite = Smites[Index];
		Smite.Wait += DeltaSeconds;
		if (Smite.Wait >= Tuning.SoulLightDelay)
		{
			Out.Add({ ELooterElement::SoulLight, Smite.Damage, true });
			Smites.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}

	// The pieces come off on the beat, and the last as a status runs out (nothing owed is ever kept back).
	const bool bBeat = SinceTick >= Tuning.TickSeconds;
	if (bBeat)
	{
		SinceTick = FMath::Fmod(SinceTick, Tuning.TickSeconds);
	}
	if (EmberOwed > 0.f && (bBeat || Burns.IsEmpty()))
	{
		Out.Add({ ELooterElement::Ember, EmberOwed, false });
		EmberOwed = 0.f;
	}
	if (SaltOwed > 0.f && (bBeat || !IsSalted()))
	{
		Out.Add({ ELooterElement::GraveSalt, SaltOwed, false });
		SaltOwed = 0.f;
	}
	if (RotOwed > 0.f && (bBeat || !IsRotting()))
	{
		Out.Add({ ELooterElement::Rot, RotOwed, false });
		RotOwed = 0.f;
	}
}

void FElementStatus::Clear()
{
	Burns.Reset();
	Smites.Reset();
	Salt = FOverTime();
	Jolts.Reset();
	Rot = FOverTime();
	RotLength = 0.f;
	EmberOwed = 0.f;
	SaltOwed = 0.f;
	RotOwed = 0.f;
	SinceTick = 0.f;
}

bool FElementStatus::IsActive() const
{
	return !Burns.IsEmpty() || !Smites.IsEmpty() || IsSalted() || !Jolts.IsEmpty() || IsRotting() || EmberOwed > 0.f || SaltOwed > 0.f
		|| RotOwed > 0.f;
}

float FElementStatus::GetSmiteCharge(const FElementTuning& Tuning) const
{
	float Charge = 0.f;
	for (const FSmite& Smite : Smites)
	{
		Charge = FMath::Max(Charge, Tuning.SoulLightDelay > 0.f ? Smite.Wait / Tuning.SoulLightDelay : 1.f);
	}
	return FMath::Clamp(Charge, 0.f, 1.f);
}

float FElementStatus::GetDamageLeft(ELooterElement Element) const
{
	float Left = 0.f;
	switch (Element)
	{
	case ELooterElement::Ember:
		Left = EmberOwed;
		for (const FOverTime& Burn : Burns)
		{
			Left += Burn.DamageLeft;
		}
		break;
	case ELooterElement::SoulLight:
		for (const FSmite& Smite : Smites)
		{
			Left += Smite.Damage;
		}
		break;
	case ELooterElement::GraveSalt:
		Left = Salt.DamageLeft + SaltOwed;
		break;
	case ELooterElement::Thunder:
		for (const FSmite& Jolt : Jolts)
		{
			Left += Jolt.Damage;
		}
		break;
	case ELooterElement::Rot:
		Left = Rot.DamageLeft + RotOwed;
		break;
	default:
		break;
	}
	return Left;
}

float FElementStatus::GetJoltCharge(const FElementTuning& Tuning) const
{
	float Charge = 0.f;
	for (const FSmite& Jolt : Jolts)
	{
		Charge = FMath::Max(Charge, Tuning.ThunderDelay > 0.f ? Jolt.Wait / Tuning.ThunderDelay : 1.f);
	}
	return FMath::Clamp(Charge, 0.f, 1.f);
}

float FElementStatus::GetBurnTimeLeft() const
{
	float Longest = 0.f;
	for (const FOverTime& Burn : Burns)
	{
		Longest = FMath::Max(Longest, Burn.TimeLeft);
	}
	return Longest;
}
