// AUnpaidCreature's rise out of the ground, its look in the world (2026-10-31; the flow is UnpaidCreatureRise.cpp): the ground
// stirring over one of the buried (dust breathing up through it, grit hopping, its coal's light seeping through a crack: the
// tell, a moment before the hand), the dirt bursting as its hand breaks the ground (the grave dirt's clods and dust ring, more
// grit thrown high, a flash of its coal's light in the hole), and a hand slammed down on the ground (a puff and grit where it
// lands). All through the bullets' pooled effects (FWeaponFX): no actors, no lights; near a viewer only. Cheap: a stir is two
// puffs and two chips every 0.13 s for 0.7 s, a burst one dirt throw and a dozen pieces, a plant three.

#include "Creatures/UnpaidLifeWork.h"
#include "Creatures/UnpaidCreature.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Player/SlideDust.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

namespace
{
	/** The rise is seen from this near (cm); farther, nobody sees the dirt. */
	constexpr float SeenWithin = 4500.f;

	/** The world's effects drawer (the bullets' pooled FWeaponFX), made ready, when Point is near a viewer; else null. */
	FWeaponFX* RiseEffects(const AUnpaidCreature& Unpaid, const FVector& Point)
	{
		UWorld* World = Unpaid.GetWorld();
		UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Unpaid);
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets || !Reacts || !Reacts->IsNearAViewer(Point, SeenWithin))
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** Grave dirt's colour as the light where it is shows it (the dust's material is unlit). */
	FLinearColor DirtColor(const AUnpaidCreature& Unpaid)
	{
		UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Unpaid);
		return FSlideDust::DustAlbedo(ESlideGround::Dirt) * (Reacts ? Reacts->GetDustLight() : FLinearColor::White);
	}

	/** A random way out along the ground (unit, flat). */
	FVector Outward(FRandomStream& Dice)
	{
		const float Angle = Dice.FRandRange(0.f, 2.f * UE_PI);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}
}

FVector FUnpaidRiseWork::FeetOf(const AUnpaidCreature& Unpaid)
{
	return Unpaid.GetActorLocation() - FVector(0.f, 0.f, Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

void FUnpaidRiseWork::Stir(const AUnpaidCreature& Unpaid)
{
	const FVector Feet = FeetOf(Unpaid);
	FWeaponFX* Effects = RiseEffects(Unpaid, Feet);
	UCreatureReactComponent* Reacts = Unpaid.GetReact();
	if (!Effects || !Reacts)
	{
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();
	const float Scale = Unpaid.GetSizeScale();
	const FLinearColor Dust = DirtColor(Unpaid);
	// Dust breathing up through the ground over it, grit hopping as something shoves under it.
	for (int32 Puff = 0; Puff < 2; ++Puff)
	{
		const FVector At = Feet + Outward(Dice) * (Dice.FRandRange(10.f, 45.f) * Scale) + FVector(0.f, 0.f, 4.f);
		const float Start = Dice.FRandRange(10.f, 16.f) * Scale;
		Effects->SpawnDustPuff(At, FVector(0.f, 0.f, Dice.FRandRange(30.f, 60.f)) + Outward(Dice) * 20.f, Dust, 0.22f, Start,
			Start * Dice.FRandRange(2.5f, 3.5f), Dice.FRandRange(0.6f, 0.9f));
	}
	for (int32 Chip = 0; Chip < 2; ++Chip)
	{
		const FVector At = Feet + Outward(Dice) * (Dice.FRandRange(5.f, 35.f) * Scale) + FVector(0.f, 0.f, 3.f);
		Effects->SpawnGrit(At, Outward(Dice) * Dice.FRandRange(30.f, 80.f) + FVector(0.f, 0.f, Dice.FRandRange(120.f, 220.f)),
			Dice.FRandRange(1.f, 2.2f), Dice.FRandRange(0.5f, 0.8f));
	}
	// Its coal's light seeping up through a crack in the dirt: a faint ring of it, opening as the moment comes.
	const float Glow = FVfxTuning::Live().Glow;
	Effects->SpawnShockRing(Feet + FVector(0.f, 0.f, 3.f), FVector::UpVector, 12.f * Scale, 55.f * Scale, 7.f * Scale, Unpaid.GetCoalColor(),
		3.5f * Glow, 0.35f);
}

void FUnpaidRiseWork::Burst(const AUnpaidCreature& Unpaid, float Strength)
{
	const FVector Feet = FeetOf(Unpaid);
	const float Scale = Unpaid.GetSizeScale();
	// The grave dirt's own throw (the clods and the dust ring rolling out), where there's earth to throw.
	if (UCreatureReactComponent* Reacts = Unpaid.GetReact())
	{
		Reacts->ThrowGraveDirt(Feet, Scale, Strength);
	}
	FWeaponFX* Effects = RiseEffects(Unpaid, Feet);
	UCreatureReactComponent* Reacts = Unpaid.GetReact();
	if (!Effects || !Reacts)
	{
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();
	// Grit thrown high out of the hole, and a puff rising up the arm.
	const int32 Chips = FMath::RoundToInt32(10.f * Strength);
	for (int32 Chip = 0; Chip < Chips; ++Chip)
	{
		Effects->SpawnGrit(Feet + FVector(0.f, 0.f, 6.f), Outward(Dice) * Dice.FRandRange(90.f, 260.f) * Scale
			+ FVector(0.f, 0.f, Dice.FRandRange(320.f, 560.f)), Dice.FRandRange(1.5f, 3.5f), Dice.FRandRange(0.7f, 1.2f));
	}
	const FLinearColor Dust = DirtColor(Unpaid);
	Effects->SpawnDustPuff(Feet + FVector(0.f, 0.f, 20.f * Scale), FVector(0.f, 0.f, 160.f), Dust, 0.4f * Strength, 22.f * Scale,
		85.f * Scale, 1.3f);
	// A breath of its coal's light out of the hole as the ground opens.
	const float Glow = FVfxTuning::Live().Glow;
	Effects->SpawnShockRing(Feet + FVector(0.f, 0.f, 4.f), FVector::UpVector, 20.f * Scale, 120.f * Scale, 14.f * Scale, Unpaid.GetCoalColor(),
		6.f * Glow * Strength, 0.3f);
}

void FUnpaidRiseWork::Plant(const AUnpaidCreature& Unpaid, int32 Side)
{
	const FVector Feet = FeetOf(Unpaid);
	FWeaponFX* Effects = RiseEffects(Unpaid, Feet);
	UCreatureReactComponent* Reacts = Unpaid.GetReact();
	if (!Effects || !Reacts)
	{
		return;
	}
	// Where the hand comes down: its bone as last posed, on the ground under it.
	FVector Hand = Feet + Unpaid.GetActorForwardVector() * (60.f * Unpaid.GetSizeScale());
	const int32 Bone = Side >= 0 && Side < 2 ? Unpaid.Arms[Side].Hand : INDEX_NONE;
	if (Unpaid.bRigReady && Unpaid.Bones.IsValidIndex(Bone))
	{
		Hand = Unpaid.GetMesh()->GetComponentTransform().TransformPosition(Unpaid.Bones[Bone].Posed.GetLocation());
	}
	Hand.Z = Feet.Z + 4.f;
	FRandomStream& Dice = Reacts->GetRandom();
	const float Scale = Unpaid.GetSizeScale();
	const float Start = 14.f * Scale;
	Effects->SpawnDustPuff(Hand, Outward(Dice) * 40.f + FVector(0.f, 0.f, 50.f), DirtColor(Unpaid), 0.3f, Start, Start * 3.f, 0.8f);
	for (int32 Chip = 0; Chip < 3; ++Chip)
	{
		Effects->SpawnGrit(Hand, Outward(Dice) * Dice.FRandRange(60.f, 140.f) + FVector(0.f, 0.f, Dice.FRandRange(80.f, 180.f)),
			Dice.FRandRange(1.f, 2.f), Dice.FRandRange(0.4f, 0.7f));
	}
}
