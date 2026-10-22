#include "Loot/LootFanfareSubsystem.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/BulletSubsystem.h"
#include "Loot/LootDropMoments.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "World/LightBeam.h"
#include "World/WorldQueries.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/RotatingMovementComponent.h"

// ULootFanfareSubsystem's landing shows (Phase 12.3, LootDropMoments): an Epic's or Legendary's spin and ring of light
// along the ground, a Legendary's column of light, name popping up over it and rising motes. Rings and motes draw
// through the bullets' pooled effects; the column is the loot beam's own glow on a second, wider cylinder; nothing lights.

namespace
{
	/** The name pops up this far over the gun (cm), clear of its own label. */
	constexpr float NamePopHeight = 150.f;
	/** The ring: its start width and end width (cm), and its glow at the start. */
	constexpr float RingStartWidth = 7.f;
	constexpr float RingEndWidth = 2.f;
	constexpr float RingGlow = 18.f;
	/** A fainter ring following the first in, at this share of its radius. */
	constexpr float RingInnerShare = 0.6f;
	constexpr float RingInnerGlow = 0.45f;
	/** The motes rise round the gun within this (cm). */
	constexpr float MoteRadius = 120.f;
	/** How far down the ground under a landed gun is looked for (cm). */
	constexpr float GroundLook = 300.f;

	FVector GroundUnder(const AWeaponBase& Weapon)
	{
		const FVector At = Weapon.GetActorLocation();
		const UWorld* World = Weapon.GetWorld();
		FHitResult Hit;
		if (World && World->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 20.f), At - FVector(0.f, 0.f, GroundLook), ECC_Visibility,
			LooterWorld::StaticGeometryParams(World, TEXT("DropRing"), &Weapon)))
		{
			return Hit.ImpactPoint + FVector(0.f, 0.f, 2.f);
		}
		return At - FVector(0.f, 0.f, 10.f);
	}
}

void ULootFanfareSubsystem::StartShows(AWeaponBase& Weapon)
{
	const EWeaponRarity Rarity = Weapon.GetRarity();
	const FDropMoment Moment = LootDropMoments::For(Rarity);
	const FLinearColor Color = UWeaponRollLibrary::GetRarityColor(Weapon.GetInstance().Definition, Rarity);
	UWorld* World = GetWorld();

	// A spin as it lands, easing back to the pickup's own turn.
	URotatingMovementComponent* Spinner = Weapon.FindComponentByClass<URotatingMovementComponent>();
	if (Spinner && Moment.SpinRate > 0.f && Moment.SpinSeconds > 0.f
		&& !Spins.ContainsByPredicate([&Weapon](const FSpin& Spin) { return Spin.Weapon.Get() == &Weapon; }))
	{
		FSpin& Spin = Spins.AddDefaulted_GetRef();
		Spin.Weapon = &Weapon;
		Spin.Seconds = Moment.SpinSeconds;
		Spin.Rate = Moment.SpinRate;
		Spin.OwnRate = Spinner->RotationRate;
	}

	const FVector Ground = GroundUnder(Weapon);
	if (Moment.RingRadius > 0.f && Moment.RingSeconds > 0.f)
	{
		FRing& Ring = Rings.AddDefaulted_GetRef();
		Ring.Center = Ground;
		Ring.Color = Color;
		Ring.Seconds = Moment.RingSeconds;
		Ring.Radius = Moment.RingRadius;
	}

	// A Legendary's column: a second, wider beam of its colour, shooting up as the first flares, standing while it lies there.
	USceneComponent* Root = Weapon.GetRootComponent();
	if (Root && Moment.ColumnHeight > 0.f && !Columns.ContainsByPredicate([&Weapon](const FColumn& Column) { return Column.Weapon.Get() == &Weapon; }))
	{
		UStaticMeshComponent* Beam = NewObject<UStaticMeshComponent>(&Weapon, NAME_None, RF_Transient);
		Beam->SetupAttachment(Root);
		Beam->SetUsingAbsoluteRotation(true);
		Beam->RegisterComponent();
		LightBeams::Setup(Beam, Color, Moment.ColumnGlow, Moment.ColumnHeight, Moment.ColumnRadius);
		FColumn& Column = Columns.AddDefaulted_GetRef();
		Column.Weapon = &Weapon;
		Column.Beam = Beam;
		Column.Glow = Moment.ColumnGlow;
		Column.Height = Moment.ColumnHeight;
		Column.Radius = Moment.ColumnRadius;
		LightBeams::Flare(Beam, 0.f, true, Column.Glow, Column.Height, Column.Radius);
	}

	// Its name pops up over it (a label of its own, apart from the one that shows when it's looked at).
	if (Root && Moment.NamePopSeconds > 0.f && !NamePops.ContainsByPredicate([&Weapon](const FNamePop& Pop) { return Pop.Weapon.Get() == &Weapon; }))
	{
		UWidgetComponent* Widget = NewObject<UWidgetComponent>(&Weapon, NAME_None, RF_Transient);
		Widget->SetupAttachment(Root);
		Widget->SetUsingAbsoluteRotation(true);
		Widget->SetRelativeLocation(FVector(0.f, 0.f, NamePopHeight));
		Widget->SetWidgetSpace(EWidgetSpace::Screen);
		Widget->SetDrawAtDesiredSize(true);
		Widget->SetPivot(FVector2D(0.5f, 1.f));
		Widget->SetWidgetClass(UWeaponLabelWidget::StaticClass());
		Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Widget->RegisterComponent();
		Widget->InitWidget();
		if (UWeaponLabelWidget* Label = Cast<UWeaponLabelWidget>(Widget->GetUserWidgetObject()))
		{
			Label->SetWeapon(&Weapon);
			Label->SetPop(0.f, Moment.NamePopSeconds);
		}
		FNamePop& Pop = NamePops.AddDefaulted_GetRef();
		Pop.Weapon = &Weapon;
		Pop.Widget = Widget;
		Pop.Seconds = Moment.NamePopSeconds;
	}

	if (Moment.Motes > 0 && World)
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			Bullets->GetEffects().Initialize(World);
			Bullets->GetEffects().SpawnLootMotes(Ground, MoteRadius, Color, Moment.Motes);
		}
	}
}

void ULootFanfareSubsystem::AdvanceShows(float DeltaTime)
{
	for (int32 Index = Spins.Num() - 1; Index >= 0; --Index)
	{
		FSpin& Spin = Spins[Index];
		AWeaponBase* Weapon = Spin.Weapon.Get();
		URotatingMovementComponent* Spinner = Weapon ? Weapon->FindComponentByClass<URotatingMovementComponent>() : nullptr;
		if (!Spinner)
		{
			Spins.RemoveAtSwap(Index);
			continue;
		}
		Spin.Time += DeltaTime;
		const float Along = FMath::Clamp(Spin.Time / FMath::Max(Spin.Seconds, 0.01f), 0.f, 1.f);
		if (Along >= 1.f || !Weapon->IsPickup())
		{
			Spinner->RotationRate = Spin.OwnRate;
			Spins.RemoveAtSwap(Index);
			continue;
		}
		// Fast at first, settling smoothly into its own slow turn.
		const float Ease = FMath::SmoothStep(0.f, 1.f, Along);
		Spinner->RotationRate = FRotator(Spin.OwnRate.Pitch, FMath::Lerp(Spin.Rate, static_cast<float>(Spin.OwnRate.Yaw), Ease), Spin.OwnRate.Roll);
	}

	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (Bullets && !Rings.IsEmpty())
	{
		Bullets->GetEffects().Initialize(World);
	}
	for (int32 Index = Rings.Num() - 1; Index >= 0; --Index)
	{
		FRing& Ring = Rings[Index];
		Ring.Time += DeltaTime;
		const float Along = FMath::Clamp(Ring.Time / FMath::Max(Ring.Seconds, 0.01f), 0.f, 1.f);
		if (Along >= 1.f || !Bullets)
		{
			Rings.RemoveAtSwap(Index);
			continue;
		}
		// Racing out and slowing, thinning and fading as it goes; a fainter one following inside it.
		const float Out = 1.f - FMath::Pow(1.f - Along, 3.f);
		const float Fade = FMath::Pow(1.f - Along, 1.5f);
		const float Width = FMath::Lerp(RingStartWidth, RingEndWidth, Along);
		FWeaponFX& FX = Bullets->GetEffects();
		FX.AddGroundRing(Ring.Center, Ring.Radius * Out, Width, Ring.Color, RingGlow * Fade, 40);
		FX.AddGroundRing(Ring.Center, Ring.Radius * Out * RingInnerShare, Width * 0.6f, Ring.Color, RingGlow * RingInnerGlow * Fade, 28);
	}

	for (int32 Index = Columns.Num() - 1; Index >= 0; --Index)
	{
		FColumn& Column = Columns[Index];
		const AWeaponBase* Weapon = Column.Weapon.Get();
		UStaticMeshComponent* Beam = Column.Beam.Get();
		// Taken up (or gone): its column goes with it.
		if (!Weapon || !Weapon->IsPickup() || !Beam)
		{
			if (Beam)
			{
				Beam->DestroyComponent();
			}
			Columns.RemoveAtSwap(Index);
			continue;
		}
		if (Column.Time <= LightBeams::FlareSeconds)
		{
			Column.Time += DeltaTime;
			LightBeams::Flare(Beam, Column.Time, true, Column.Glow, Column.Height, Column.Radius);
		}
	}

	for (int32 Index = NamePops.Num() - 1; Index >= 0; --Index)
	{
		FNamePop& Pop = NamePops[Index];
		const AWeaponBase* Weapon = Pop.Weapon.Get();
		UWidgetComponent* Widget = Pop.Widget.Get();
		Pop.Time += DeltaTime;
		if (!Weapon || !Weapon->IsPickup() || !Widget || Pop.Time >= Pop.Seconds)
		{
			if (Widget)
			{
				Widget->DestroyComponent();
			}
			NamePops.RemoveAtSwap(Index);
			continue;
		}
		if (UWeaponLabelWidget* Label = Cast<UWeaponLabelWidget>(Widget->GetUserWidgetObject()))
		{
			Label->SetPop(Pop.Time, Pop.Seconds);
		}
	}
}

void ULootFanfareSubsystem::ClearShows()
{
	for (const FSpin& Spin : Spins)
	{
		const AWeaponBase* Weapon = Spin.Weapon.Get();
		if (URotatingMovementComponent* Spinner = Weapon ? Weapon->FindComponentByClass<URotatingMovementComponent>() : nullptr)
		{
			Spinner->RotationRate = Spin.OwnRate;
		}
	}
	Spins.Reset();
	Rings.Reset();
	// The column and name components belong to their guns, which go with the world.
	Columns.Reset();
	NamePops.Reset();
}
