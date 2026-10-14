#include "World/SwingComponent.h"
#include "World/GustClock.h"
#include "World/GustSubsystem.h"
#include "Components/PrimitiveComponent.h"

namespace
{
	/** The longest step of the pendulum (s): longer ones are split so it never blows up. */
	constexpr float MaxStep = 1.f / 60.f;
	/** At rest: swung less than this (degrees) and moving slower than this (degrees a second). */
	constexpr float RestAngle = 0.05f;
	constexpr float RestSpeed = 0.3f;
	/** A gust's first shove (a share of its push, as a speed at the swing's own rate), so it moves at once. */
	constexpr float Shove = 0.6f;
	/** The buffeting on top of a gust's steady push (a share of it), and how fast it beats (radians a second). */
	constexpr float Buffet = 0.25f;
	constexpr float BuffetRate = 7.f;
	/** Swung against its hooks, it comes back off them with this share of its speed. */
	constexpr float HookBounce = 0.3f;
	/** What hangs on it counts as seen if drawn within this long (s). */
	constexpr float SeenSeconds = 0.5f;
}

USwingComponent::USwingComponent()
{
	// Asleep until the player comes near or a gust swings it (UGustSubsystem wakes it).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	Mobility = EComponentMobility::Movable;
}

float USwingComponent::RateFor(float Length)
{
	return FMath::Sqrt(980.f / FMath::Max(Length, 5.f));
}

void USwingComponent::BeginPlay()
{
	Super::BeginPlay();
	RestRotation = GetRelativeRotation().Quaternion();
	// Each its own place in the breeze, so a row of signs never sways in step.
	IdlePhase = static_cast<float>(GetTypeHash(GetPathName()) % 6283) / 1000.f;
	if (UGustSubsystem* Gusts = UGustSubsystem::Get(this))
	{
		GustHandle = Gusts->OnGust.AddUObject(this, &USwingComponent::HandleGust);
		Gusts->RegisterSwing(this);
	}
}

void USwingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGustSubsystem* Gusts = UGustSubsystem::Get(this))
	{
		Gusts->OnGust.Remove(GustHandle);
		Gusts->UnregisterSwing(this);
	}
	GustHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void USwingComponent::HandleGust(const FGustEvent& InGust)
{
	// Nobody near: nobody sees it swing, and it stays asleep.
	if (bNear)
	{
		Kick(InGust);
	}
}

FVector USwingComponent::GetSwingDirection() const
{
	// Turning about an axis leaves the axis where it is, so the swing now doesn't change it. A small turn moves the bottom
	// (straight down from the pivot) along Axis x Down.
	const FVector Axis = GetComponentQuat().RotateVector(HingeAxis.GetSafeNormal());
	return FVector::CrossProduct(Axis, FVector(0.0, 0.0, -1.0)).GetSafeNormal2D();
}

void USwingComponent::Kick(const FGustEvent& InGust)
{
	Gust = InGust;
	GustAge = 0.f;
	bGust = InGust.Duration > 0.f && InGust.Strength > 0.f;
	const float Facing = static_cast<float>(FVector::DotProduct(GetSwingDirection(), InGust.Direction.GetSafeNormal2D()));
	Speed += Facing * InGust.Strength * GustDegrees * RateFor(PendulumLength) * Shove;
	Wake();
}

void USwingComponent::SetNear(bool bInNear)
{
	bNear = bInNear;
	if (bNear)
	{
		Wake();
	}
}

void USwingComponent::Wake()
{
	if (!IsComponentTickEnabled())
	{
		SetComponentTickEnabled(true);
	}
}

bool USwingComponent::IsSettled() const
{
	return !bGust && FMath::Abs(Angle) < RestAngle && FMath::Abs(Speed) < RestSpeed;
}

void USwingComponent::Step(float DeltaSeconds)
{
	const float Rate = RateFor(PendulumLength);
	const float Facing = bGust ? static_cast<float>(FVector::DotProduct(GetSwingDirection(), Gust.Direction.GetSafeNormal2D())) : 0.f;
	float Left = FMath::Max(DeltaSeconds, 0.f);
	while (Left > 0.f)
	{
		const float H = FMath::Min(Left, MaxStep);
		Left -= H;
		IdleTime += H;
		// Where it would hang now: straight down, or swayed by the breeze while someone's near, or pushed downwind in a gust.
		float Target = 0.f;
		if (bNear)
		{
			// Two slow beats, well under its own rate, so the breeze rocks it gently rather than ringing it.
			Target += IdleDegrees * (0.7f * FMath::Sin(1.3f * IdleTime + IdlePhase) + 0.3f * FMath::Sin(2.3f * IdleTime + 2.f * IdlePhase));
		}
		if (bGust)
		{
			GustAge += H;
			const float Push = Gust.Strength * FGustClock::Shape(GustAge, Gust.Duration);
			Target += GustDegrees * Push * Facing * (1.f + Buffet * FMath::Sin(BuffetRate * IdleTime + IdlePhase));
			if (GustAge >= Gust.Duration)
			{
				bGust = false;
			}
		}
		// A damped pendulum about that: pulled toward it at its own rate, its swing dying away by Damping.
		const float Pull = -Rate * Rate * (Angle - Target) - 2.f * Damping * Rate * Speed;
		Speed += Pull * H;
		Angle += Speed * H;
		if (FMath::Abs(Angle) > MaxDegrees)
		{
			Angle = FMath::Sign(Angle) * MaxDegrees;
			Speed = -Speed * HookBounce;
		}
	}
}

bool USwingComponent::IsSeen() const
{
	bool bAnyDrawn = false;
	for (const USceneComponent* Child : GetAttachChildren())
	{
		if (const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Child))
		{
			bAnyDrawn = true;
			if (Primitive->WasRecentlyRendered(SeenSeconds))
			{
				return true;
			}
		}
	}
	// Nothing that draws hangs on it: it can't be seen, nor saved from.
	return !bAnyDrawn;
}

void USwingComponent::Apply()
{
	SetRelativeRotation(RestRotation * FQuat(HingeAxis.GetSafeNormal(), FMath::DegreesToRadians(Angle)));
}

void USwingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Step(DeltaTime);
	if (!bNear && IsSettled())
	{
		// Far off and still: back to rest, and asleep until the player or a gust comes.
		Angle = 0.f;
		Speed = 0.f;
		Apply();
		SetComponentTickEnabled(false);
		return;
	}
	if (IsSeen())
	{
		Apply();
	}
}
