#include "Embers/Powers/EmberIronHide.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"

const FName UEmberIronHide::ScaleSource(TEXT("IronHide"));

namespace
{
	const FName PaceName(TEXT("Pace"));
	const FName BiteBurnName(TEXT("BiteBurn"));

	/** The slow is renewed every frame for this long, so it ends a moment after the plates open. */
	constexpr float PaceHold = 0.15f;
	/** Steam breathes off the shoulders this often while it lasts (seconds). */
	constexpr float VentEvery = 0.45f;
	/** Hot iron: the sparks off the plates and the biter's teeth. */
	const FLinearColor HotIron(1.f, 0.55f, 0.18f);
	/** The body's forge glow while locked: a low, breathing base (it shouldn't hide the body), a flare on the lock and each blow. */
	const FLinearColor ForgeGlow(1.f, 0.32f, 0.06f);
	constexpr float HeatBase = 0.12f;
	constexpr float HeatBreath = 0.04f;
	constexpr float HeatFlareLock = 0.6f;
	constexpr float HeatFlareBlow = 0.45f;
	/** The flare dies away at this much a second. */
	constexpr float HeatFlareFall = 2.5f;
}

void UEmberIronHide::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	const float Strength = Def && Def->Strength > 0.f ? FMath::Clamp(Def->Strength, 0.f, 0.95f) : DefaultStrength;
	TakenShare = 1.f - Strength;
	Pace = FMath::Clamp(Def ? Def->Extra(PaceName, DefaultPace) : DefaultPace, 0.1f, 1.f);
	BiteBurn = (Def ? Def->Extra(BiteBurnName, DefaultBiteBurn) : DefaultBiteBurn) * LevelScale;
	Health = Character->FindComponentByClass<UHealthComponent>();
	Burned = 0;
	Time = 0.f;
	VentClock = VentEvery;
	LastBurned.Reset();
	bLocked = true;
	if (UHealthComponent* Life = Health.Get())
	{
		Life->SetDamageTakenScale(ScaleSource, TakenShare);
	}
	UMovementSlowComponent::Apply(Character, Pace, PaceHold);
	Bind(true);
	if (!HeatLook)
	{
		HeatLook = EmberFX::FEmberTint::MakeMaterial(this, ForgeGlow);
	}
	Heat.SetMaterial(HeatLook);
	HeatFlare = HeatFlareLock;
	Heat.SetIntensity(HeatBase + HeatFlare);
	Heat.Wear(*Character);

	LooterSound::Play2D(Character, LooterSoundCue::Ember::IronHideLock);
	Vent(12, 1.f);
	// (The VFX pass) the plates slam shut: a forge-orange ring of heat thrown out over the ground from his feet.
	if (FWeaponFX* FX = Effects())
	{
		EmberVfx::Activation(*FX, FeetOf(*Character), ForgeGlow, 1.f);
	}
	// The plates slammed shut: a heavy dip, the view drawing in a little.
	FViewKick Lock;
	Lock.Pitch = -0.8f;
	Lock.Roll = 0.4f;
	Lock.FieldOfView = -0.025f;
	Lock.Frequency = 9.f;
	Lock.Damping = 0.55f;
	Kick(Lock);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Iron Hide: %.0f%% of each blow gets through, %.0f%% pace, biters burned for %.1f"), TakenShare * 100.f, Pace * 100.f,
		BiteBurn);
}

void UEmberIronHide::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	ACharacter* Character = Body.Get();
	if (!bLocked || !Character)
	{
		return;
	}
	Time += FMath::Max(DeltaSeconds, 0.f);
	// Heavy: the pace held down a moment at a time.
	UMovementSlowComponent::Apply(Character, Pace, PaceHold);
	VentClock -= DeltaSeconds;
	if (VentClock <= 0.f)
	{
		VentClock = VentEvery;
		Vent(2, 0.4f);
	}
	// The plates' glow breathing, the last flare dying; worn again each frame so a mesh the view switched on (F5) glows too.
	HeatFlare = FMath::Max(HeatFlare - HeatFlareFall * FMath::Max(DeltaSeconds, 0.f), 0.f);
	Heat.SetIntensity(HeatBase + HeatBreath * FMath::Sin(Time * 4.f) + HeatFlare);
	Heat.Wear(*Character);
}

void UEmberIronHide::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bLocked)
	{
		return;
	}
	bLocked = false;
	Bind(false);
	Heat.RemoveAll();
	HeatFlare = 0.f;
	if (UHealthComponent* Life = Health.Get())
	{
		Life->SetDamageTakenScale(ScaleSource, 1.f);
	}
	const UWorld* World = GetBodyWorld();
	if (World && !World->bIsTearingDown)
	{
		LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::IronHideUnlock);
		Vent(10, 0.9f);
		FViewKick Open;
		Open.Pitch = 0.4f;
		Open.FieldOfView = 0.015f;
		Open.Frequency = 5.f;
		Open.Damping = 0.7f;
		Kick(Open);
	}
	UE_LOG(LogLooter, Log, TEXT("Iron Hide: the plates open%s, %d biter(s) burned"), bInterrupted ? TEXT(" (cut short)") : TEXT(""), Burned);
}

void UEmberIronHide::Bind(bool bOn)
{
	UHealthComponent* Life = Health.Get();
	if (!Life)
	{
		return;
	}
	if (bOn)
	{
		Life->OnDamaged.AddUniqueDynamic(this, &UEmberIronHide::HandleDamaged);
	}
	else
	{
		Life->OnDamaged.RemoveDynamic(this, &UEmberIronHide::HandleDamaged);
	}
}

void UEmberIronHide::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!bLocked)
	{
		return;
	}
	const ACharacter* Character = Body.Get();
	// Every blow rings off the plates: a clang and sparks where it landed.
	LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::IronHideClang, 0.9f, Random.FRandRange(0.95f, 1.05f));
	HeatFlare = FMath::Max(HeatFlare, HeatFlareBlow);
	const FVector Toward = Character && DamageCauser ? (DamageCauser->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal() : FVector::UpVector;
	for (int32 Index = 0; Index < 9; ++Index)
	{
		const FVector Out = (Toward + Random.GetUnitVector() * 0.8).GetSafeNormal();
		Sparks.Spawn(HitLocation, Out * Random.FRandRange(250.f, 550.f), Random.FRandRange(0.15f, 0.3f), 1.2f, HotIron, 24.f, 900.f, 2.f);
	}

	// A creature's own blow is a bite: it burns its teeth on the hot plates (once a moment per biter).
	ACreatureBase* Biter = Cast<ACreatureBase>(DamageCauser);
	if (!Biter || Biter->IsDead())
	{
		return;
	}
	if (const float* Last = LastBurned.Find(Biter); Last && Time - *Last < BiteEvery)
	{
		return;
	}
	LastBurned.Add(Biter, Time);
	++Burned;
	FVector Middle;
	float BodyRadius = 0.f;
	float HalfHeight = 0.f;
	BodyOf(*Biter, Middle, BodyRadius, HalfHeight);
	const FVector Mouth = Middle - Toward * BodyRadius;
	Hurt(*Biter, BiteBurn * Random.FRandRange(0.9f, 1.1f), Mouth, Toward);
	LooterSound::PlayAt(Biter, LooterSoundCue::Ember::IronHideBurn, Mouth);
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnWisp(Mouth, FVector(0.0, 0.0, 50.0), 0.3f, 8.f, 34.f, 0.8f);
	}
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const FVector Out = (Toward + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(Mouth, Out * Random.FRandRange(150.f, 400.f), Random.FRandRange(0.2f, 0.4f), 1.4f, HotIron, 22.f, 500.f, 2.f);
	}
}

void UEmberIronHide::Vent(int32 Puffs, float Strength)
{
	const ACharacter* Character = Body.Get();
	FWeaponFX* FX = Effects();
	if (!Character || !FX)
	{
		return;
	}
	// Steam off its shoulders and its stacks: two jets either side, rising and spreading.
	const float HalfHeight = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Shoulders = Character->GetActorLocation() + FVector(0.0, 0.0, HalfHeight * 0.7f);
	const FVector Side = Character->GetActorRightVector();
	const FVector Back = -Character->GetActorForwardVector();
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		const float Sign = Index % 2 ? 1.f : -1.f;
		const FVector Out = (Side * Sign + Back * 0.5f + FVector(0.0, 0.0, 0.8) + Random.GetUnitVector() * 0.3).GetSafeNormal();
		FX->SpawnWisp(Shoulders + Side * (Sign * 30.f) + Back * 15.f, Out * Random.FRandRange(90.f, 180.f) * Strength, 0.28f * Strength + 0.1f, 8.f,
			Random.FRandRange(40.f, 70.f), Random.FRandRange(0.7f, 1.1f));
	}
}

bool UEmberIronHide::Linger(float DeltaSeconds)
{
	Sparks.Tick(DeltaSeconds, Effects());
	return bLocked || !Sparks.IsEmpty();
}
