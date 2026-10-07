#include "Bosses/AbelRules.h"
#include "Bosses/AbelPoses.h"
#include "Creatures/UnpaidCreature.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

namespace
{
	/** The model stands on the capsule's foot: its middle is this far over the ground at size 1 (AUnpaidCreature's capsule). */
	constexpr float ModelOverMiddle = 90.f;
}

namespace AbelRules
{
	FName BuckshotEvent()
	{
		return FName(TEXT("Buckshot"));
	}

	FName GrieveEvent()
	{
		return FName(TEXT("Grieve"));
	}

	FName BellEvent()
	{
		return FName(TEXT("Bell"));
	}

	FName GravewindEvent()
	{
		return FName(TEXT("Gravewind"));
	}

	FName WalkOffEvent()
	{
		return FName(TEXT("WalkOff"));
	}

	FBossVolley Buckshot()
	{
		FBossVolley Volley;
		Volley.Pellets = 7;
		Volley.SpreadDegrees = 18.f;
		// Slow and pale: seen leaving the pump and stepped out of.
		Volley.Speed = 950.f;
		Volley.DamageShare = 0.35f;
		Volley.Radius = 11.f;
		Volley.Range = 3500.f;
		// The tell is his: the lantern's flare (AAbelKeeper), so the pellets leave as the shot comes.
		Volley.WindupSeconds = 0.f;
		// Where the pump's muzzle is in the shot, from his middle (he aims from the pump's own socket when it has one).
		Volley.Muzzle = AbelPoses::FireMuzzle() - FVector(0.0, 0.0, ModelOverMiddle);
		Volley.Color = FLinearColor(0.72f, 0.86f, 1.f);
		return Volley;
	}

	FBossAddWave RisingUnpaid(int32 Count, int32 MaxAlive)
	{
		FBossAddWave Wave;
		Wave.CreatureClass = AUnpaidCreature::StaticClass();
		Wave.Rank = ECreatureRank::Basic;
		Wave.Count = Count;
		Wave.MaxAlive = MaxAlive;
		// Round the deck's middle, between the biers, on its boards: wherever he is (out over the canyon in the second phase).
		Wave.bAroundSpot = true;
		Wave.Radius = 650.f;
		return Wave;
	}

	TArray<FBossPhase> MakePhases()
	{
		// "You brought them here" (100-60%): the buckshot after the flare, two Unpaid every 25 s (at most 4), and every 12 s
		// he turns to the sunset for 3 s with his coal open.
		FBossPhase Brought;
		Brought.Name = LOCTEXT("PhaseBrought", "You brought them here");
		Brought.HealthShare = 1.f;
		Brought.Events.Add(FBossPhaseEvent::MakeCustom(BuckshotEvent(), 4.f, BuckshotEvery));
		Brought.Events.Add(FBossPhaseEvent::MakeCustom(GrieveEvent(), GrieveEvery, GrieveEvery));
		Brought.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(RiseCount, RiseMaxAlive), RiseEvery, RiseEvery));

		// "The bell" (60-25%): the bell tolls, he drifts out into the fog over the canyon where he can't be hurt, the lanterns
		// go dark, and 8 Unpaid rise in two waves. Relit, the lanterns drag him back (his own code ends the spell).
		FBossPhase Bell;
		Bell.Name = LOCTEXT("PhaseBell", "The bell");
		Bell.HealthShare = BellShare;
		Bell.Events.Add(FBossPhaseEvent::MakeCustom(BellEvent()));
		FBossUntargetable InTheFog;
		InTheFog.Seconds = 0.f;
		InTheFog.bUntilAddsDie = false;
		InTheFog.bWithdraw = true;
		InTheFog.Hint = LOCTEXT("RelightHint", "Relight the keeper's lanterns");
		Bell.Events.Add(FBossPhaseEvent::MakeUntargetable(InTheFog));
		Bell.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(BellWave, 0), 2.5f));
		Bell.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(BellWave, 0), 2.5f + SecondWaveAfter));
		// Back on the deck he fights on as before (out in the fog these wait for him).
		Bell.Events.Add(FBossPhaseEvent::MakeCustom(BuckshotEvent(), 8.f, BuckshotEvery));
		Bell.Events.Add(FBossPhaseEvent::MakeCustom(GrieveEvent(), 14.f, GrieveEvery));

		// "Let me go" (25-0%): the Gravewind pours off the point in gusts; he tries to walk off into it and the dark saint
		// pulls him back (a stun, his coal open); between pulls he fights faster.
		FBossPhase Wind;
		Wind.Name = LOCTEXT("PhaseWind", "Let me go");
		Wind.HealthShare = WindShare;
		Wind.Events.Add(FBossPhaseEvent::MakeCustom(GravewindEvent()));
		Wind.Events.Add(FBossPhaseEvent::MakeCustom(WalkOffEvent(), 4.f, WalkOffEvery));
		Wind.Events.Add(FBossPhaseEvent::MakeCustom(BuckshotEvent(), 2.f, WindBuckshotEvery));

		return { Brought, Bell, Wind };
	}

	float DragShare(int32 LitLanterns, int32 TotalLanterns)
	{
		return TotalLanterns > 0 ? FMath::Clamp(static_cast<float>(LitLanterns) / static_cast<float>(TotalLanterns), 0.f, 1.f) : 1.f;
	}

	FVector GlideAt(const FVector& From, const FVector& To, float Alpha, float Lift)
	{
		const float Eased = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
		return FMath::Lerp(From, To, Eased) + FVector(0.0, 0.0, Lift * FMath::Sin(UE_PI * Eased));
	}

	float GustStrength(const FAbelGustRules& Rules, float WindTime, int32* OutGust)
	{
		const float Since = WindTime - Rules.FirstAfter;
		if (OutGust)
		{
			*OutGust = Since < 0.f ? 0 : 1 + FMath::FloorToInt32(Since / FMath::Max(Rules.Every, 0.1f));
		}
		if (Since < 0.f)
		{
			return 0.f;
		}
		const float Into = FMath::Fmod(Since, FMath::Max(Rules.Every, 0.1f));
		if (Into > Rules.Seconds)
		{
			return 0.f;
		}
		// Up over its ramp, held, down over its ramp.
		const float Ramp = FMath::Max(Rules.Ramp, KINDA_SMALL_NUMBER);
		const float Rise = FMath::Min3(Into / Ramp, (Rules.Seconds - Into) / Ramp, 1.f);
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Rise, 0.f, 1.f));
	}

	double FallSpeedPastEnd(const FAbelGustRules& Rules, double VelocityZ)
	{
		return FMath::Min(VelocityZ, -static_cast<double>(Rules.Downdraft));
	}

	FStoryLine PhaseLine(int32 Phase)
	{
		const FText Abel = LOCTEXT("Abel", "Abel");
		switch (Phase)
		{
		case 0:
			return FStoryLine::Make(Abel, LOCTEXT("LineBrought", "You brought them here, El. Right to our door."), 3.4f);
		case 1:
			return FStoryLine::Make(Abel, LOCTEXT("LineBell", "Hear her ring? Ringing for a saint that isn't there."), 3.6f);
		case 2:
			return FStoryLine::Make(Abel, LOCTEXT("LineLetGo", "Let me go, El. The wind's come for me."), 3.4f);
		default:
			return FStoryLine();
		}
	}

	FStoryLine BellLine()
	{
		// No sound is made yet, so the bell is read, as the cold open's is.
		return FStoryLine::Make(FText::GetEmpty(), LOCTEXT("BellTolls", "[The chapel bell tolls across the valley.]"), 2.8f);
	}

	FStoryLine HobOnFall()
	{
		return FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("HobFall", "Caught you. Keep off the end, sunshine. The wind's his, not yours."), 3.6f);
	}

	void ShineOnGhostChannel(ULightComponent& Light)
	{
		static_assert(GhostLightChannel == 2, "The channels below are written out for channel 2");
		// Written straight in (as the stands' lights are), so it holds in a constructor too; drawn again if it's showing.
		Light.LightingChannels.bChannel0 = false;
		Light.LightingChannels.bChannel1 = false;
		Light.LightingChannels.bChannel2 = true;
		Light.MarkRenderStateDirty();
	}

	bool IsOnGhostChannel(const ULightComponent& Light)
	{
		return !Light.LightingChannels.bChannel0 && !Light.LightingChannels.bChannel1 && Light.LightingChannels.bChannel2;
	}

	void LetGhostLightReach(UPrimitiveComponent& Part)
	{
		Part.SetLightingChannels(Part.LightingChannels.bChannel0, Part.LightingChannels.bChannel1, /*bChannel2*/ true);
	}

	void LetGhostLightReach(AActor& Actor)
	{
		TInlineComponentArray<UPrimitiveComponent*> Parts(&Actor);
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part)
			{
				LetGhostLightReach(*Part);
			}
		}
	}

	bool DoesGhostLightReach(const UPrimitiveComponent& Part)
	{
		return Part.LightingChannels.bChannel2;
	}
}

#undef LOCTEXT_NAMESPACE
