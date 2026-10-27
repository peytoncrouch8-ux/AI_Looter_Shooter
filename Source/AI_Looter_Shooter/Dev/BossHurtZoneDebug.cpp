// UBossHurtZoneDebug (BossHurtZoneDebug.h): Looter.Boss.HurtZones 1 draws the fighting bosses' hurt zones each frame.

#include "Dev/BossHurtZoneDebug.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarHurtZones(TEXT("Looter.Boss.HurtZones"), 0,
		TEXT("1: draws every fighting boss's hurt zones (grey in an attack's wind-up, red while it can hurt) and its attack log ")
		TEXT("on screen, to tune the fights' fairness. 0: off."));

	/** The colours: a live zone red, a wind-up's grey (the hurt-zone view's own: debug lines, not the game's UI). */
	const FColor LiveColor(235, 40, 30);
	const FColor TellColor(150, 150, 150);

	/** Its lines a little over the ground so they don't sink into it, the circles' pieces, the lines' weight. */
	constexpr float OverGround = 4.f;
	constexpr int32 CircleSegments = 32;
	constexpr float LiveThickness = 3.f;
	constexpr float TellThickness = 1.5f;
	/** A wedge's arc is drawn in pieces this many degrees wide at most. */
	constexpr float ArcStepDegrees = 10.f;
}

bool UBossHurtZoneDebug::IsOn()
{
	return CVarHurtZones.GetValueOnGameThread() != 0;
}

bool UBossHurtZoneDebug::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UBossHurtZoneDebug::IsTickable() const
{
	return IsOn();
}

TStatId UBossHurtZoneDebug::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBossHurtZoneDebug, STATGROUP_Tickables);
}

bool UBossHurtZoneDebug::ColorFor(EBossAttackWindow Window, FColor& OutColor)
{
	switch (Window)
	{
	case EBossAttackWindow::Active:
		OutColor = LiveColor;
		return true;
	case EBossAttackWindow::Windup:
		OutColor = TellColor;
		return true;
	default:
		return false;
	}
}

int32 UBossHurtZoneDebug::Gather(UWorld* World, TArray<FBossHurtZone>& Out)
{
	int32 Bosses = 0;
	if (!World)
	{
		return 0;
	}
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		UBossComponent* Boss = It->FindComponentByClass<UBossComponent>();
		if (Boss && Boss->GatherHurtZones.IsBound())
		{
			Boss->GatherHurtZones.Execute(Out);
			++Bosses;
		}
	}
	return Bosses;
}

void UBossHurtZoneDebug::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!IsOn() || !World)
	{
		return;
	}
#if ENABLE_DRAW_DEBUG
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		UBossComponent* Boss = It->FindComponentByClass<UBossComponent>();
		if (!Boss || !Boss->GatherHurtZones.IsBound() || It->IsDead())
		{
			continue;
		}
		TArray<FBossHurtZone> Zones;
		Boss->GatherHurtZones.Execute(Zones);
		for (const FBossHurtZone& Zone : Zones)
		{
			Draw(World, Zone);
		}
		// Its log on screen, one line a boss (replaced each frame by its key).
		if (GEngine && Boss->IsFighting())
		{
			const FBossAttackLog& Log = Boss->AttackLog;
			const FString Line = FString::Printf(TEXT("%s: %s x%d%s | idle %.1f s | since last %.1f s | rules %s"),
				*It->GetActorNameOrLabel(), Log.GetLast().IsNone() ? TEXT("-") : *Log.GetLast().ToString(), Log.GetStreak(),
				Log.IsAttacking() ? TEXT(" (now)") : TEXT(""), Log.GetIdle(), FMath::Min(Log.GetSinceEnd(), 99.f),
				BossAttackRules::IsOn() ? TEXT("on") : TEXT("off"));
			GEngine->AddOnScreenDebugMessage(static_cast<uint64>(It->GetUniqueID()) + 0x7B055000ull, 0.1f, FColor::White, Line);
		}
	}
#endif
}

void UBossHurtZoneDebug::Draw(UWorld* World, const FBossHurtZone& Zone)
{
#if ENABLE_DRAW_DEBUG
	FColor Color;
	if (!World || !ColorFor(Zone.Window, Color))
	{
		return;
	}
	const float Thickness = Zone.Window == EBossAttackWindow::Active ? LiveThickness : TellThickness;
	const FVector Up(0.0, 0.0, Zone.Height);
	const FVector Lift(0.0, 0.0, OverGround);
	auto Line = [World, Color, Thickness](const FVector& From, const FVector& To)
	{
		DrawDebugLine(World, From, To, Color, false, -1.f, 0, Thickness);
	};
	auto Ring = [World, Color, Thickness](const FVector& Center, float Radius)
	{
		DrawDebugCircle(World, Center, Radius, CircleSegments, Color, false, -1.f, 0, Thickness, FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0), false);
	};
	switch (Zone.Shape)
	{
	case EBossHurtShape::Disc:
	{
		// Its ring on the ground and at its height, joined at four points.
		const FVector Ground = Zone.Center + Lift;
		Ring(Ground, Zone.Radius);
		Ring(Ground + Up, Zone.Radius);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector Out = FVector::ForwardVector.RotateAngleAxis(90.f * Index, FVector::UpVector) * Zone.Radius;
			Line(Ground + Out, Ground + Out + Up);
		}
		break;
	}
	case EBossHurtShape::Wedge:
	{
		// Its two edges from the apex and its arc, on the ground; its far edge's height at the middle.
		const FVector Apex = Zone.Center + Lift;
		const float Half = FMath::Clamp(Zone.HalfAngle, 0.f, 180.f);
		const int32 Steps = FMath::Max(2, FMath::CeilToInt32(2.f * Half / ArcStepDegrees));
		FVector Last = Apex + Zone.Direction.RotateAngleAxis(-Half, FVector::UpVector) * Zone.Radius;
		if (Half < 180.f)
		{
			Line(Apex, Last);
		}
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			const float Angle = -Half + 2.f * Half * static_cast<float>(Step) / static_cast<float>(Steps);
			const FVector Next = Apex + Zone.Direction.RotateAngleAxis(Angle, FVector::UpVector) * Zone.Radius;
			Line(Last, Next);
			Last = Next;
		}
		if (Half < 180.f)
		{
			Line(Last, Apex);
		}
		const FVector Tip = Apex + Zone.Direction * Zone.Radius;
		Line(Tip, Tip + Up);
		break;
	}
	case EBossHurtShape::Lane:
	{
		// Its two sides and its ends, on the ground; its height at both ends.
		const FVector Start = Zone.Center + Lift;
		const FVector End(Zone.End.X, Zone.End.Y, Start.Z);
		const FVector Side = FVector(-Zone.Direction.Y, Zone.Direction.X, 0.0) * Zone.Radius;
		Line(Start + Side, End + Side);
		Line(Start - Side, End - Side);
		Ring(Start, Zone.Radius);
		Ring(End, Zone.Radius);
		Line(Start, Start + Up);
		Line(End, End + Up);
		break;
	}
	default:
		break;
	}
#endif
}
