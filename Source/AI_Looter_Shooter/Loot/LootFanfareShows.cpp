#include "Loot/LootFanfareSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/BulletSubsystem.h"
#include "Loot/LootDropMoments.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "World/LightBeam.h"
#include "World/WorldQueries.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// ULootFanfareSubsystem's landing shows (Phase 12.3, LootDropMoments): an Epic's or Legendary's spin and ring of light
// along the ground, a Legendary's column of light, name popping up over it and rising motes (its burst: LootFanfareBurst.cpp).
// Rings and motes draw through the bullets' pooled effects; the column is the loot beam's own glow on a second, wider
// cylinder; nothing lights. The column and the name pop are pooled (the user's hitch, 2026-10-27: both were built on the
// frame the Legendary landed): made as play begins on the fanfare's own actor, hidden, then attached to a landed gun.

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
	/** A pooled column's stand-in numbers until it's taken (its colour, glow and size are set then). */
	constexpr float PooledGlow = 1.f;
	constexpr float PooledHeight = 100.f;
	constexpr float PooledRadius = 10.f;

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

// ---------------------------------------------------------------------------
// The pools
// ---------------------------------------------------------------------------

USceneComponent* ULootFanfareSubsystem::PoolRoot()
{
	if (PoolHolder && IsValid(PoolHolder) && PoolHolder->GetRootComponent())
	{
		return PoolHolder->GetRootComponent();
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.Name = MakeUniqueObjectName(World->PersistentLevel, AActor::StaticClass(), TEXT("LootFanfarePool"));
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Holder)
	{
		return nullptr;
	}
#if WITH_EDITOR
	Holder->SetActorLabel(TEXT("LootFanfarePool"));
#endif
	// Movable: what it holds is lent to guns that move.
	USceneComponent* Root = NewObject<USceneComponent>(Holder, TEXT("PoolRoot"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Holder->SetRootComponent(Root);
	Root->RegisterComponent();
	PoolHolder = Holder;
	return Root;
}

UStaticMeshComponent* ULootFanfareSubsystem::MakeColumn()
{
	USceneComponent* Root = PoolRoot();
	if (!Root)
	{
		return nullptr;
	}
	UStaticMeshComponent* Beam = NewObject<UStaticMeshComponent>(Root->GetOwner(), NAME_None, RF_Transient);
	Beam->SetupAttachment(Root);
	Beam->SetUsingAbsoluteRotation(true);
	Beam->SetVisibility(false);
	Beam->RegisterComponent();
	// Its mesh and its own glow material instance, once: a landing only colours and sizes it.
	LightBeams::Setup(Beam, FLinearColor::White, PooledGlow, PooledHeight, PooledRadius);
	return Beam;
}

UWidgetComponent* ULootFanfareSubsystem::MakeNamePop()
{
	USceneComponent* Root = PoolRoot();
	if (!Root)
	{
		return nullptr;
	}
	UWidgetComponent* Widget = NewObject<UWidgetComponent>(Root->GetOwner(), NAME_None, RF_Transient);
	Widget->SetupAttachment(Root);
	Widget->SetUsingAbsoluteRotation(true);
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetPivot(FVector2D(0.5f, 1.f));
	Widget->SetWidgetClass(UWeaponLabelWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetVisibility(false);
	Widget->RegisterComponent();
	Widget->InitWidget();
	// Its Slate built now as well (the label's panels and text), not as it first shows.
	if (UUserWidget* Label = Widget->GetUserWidgetObject())
	{
		Label->TakeWidget();
		Label->SetRenderOpacity(0.f);
	}
	return Widget;
}

void ULootFanfareSubsystem::WarmPools()
{
	// The code-drawn effects the burst, rings and motes draw with are made on their first use (their meshes and materials
	// loaded then): a chest's Legendary before the first shot of the session would make them on its landing frame.
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		Bullets->GetEffects().Initialize(World);
	}
	while (FreeColumns.Num() < PoolSize)
	{
		UStaticMeshComponent* Beam = MakeColumn();
		if (!Beam)
		{
			break;
		}
		FreeColumns.Add(Beam);
	}
	while (FreeNamePops.Num() < PoolSize)
	{
		UWidgetComponent* Widget = MakeNamePop();
		if (!Widget)
		{
			break;
		}
		FreeNamePops.Add(Widget);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Loot fanfare: %d columns and %d name pops ready"), FreeColumns.Num(), FreeNamePops.Num());
}

UStaticMeshComponent* ULootFanfareSubsystem::TakeColumn(AWeaponBase& Weapon, const FLinearColor& Color)
{
	USceneComponent* GunRoot = Weapon.GetRootComponent();
	if (!GunRoot)
	{
		return nullptr;
	}
	UStaticMeshComponent* Beam = nullptr;
	while (!Beam && !FreeColumns.IsEmpty())
	{
		Beam = FreeColumns.Pop(EAllowShrinking::No);
		Beam = IsValid(Beam) ? Beam : nullptr;
	}
	// More Legendaries lying at once than the pool holds (a boss's shower): one more, kept in the pool after.
	if (!Beam)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Loot fanfare: the column pool ran dry; making one more"));
		Beam = MakeColumn();
	}
	if (!Beam)
	{
		return nullptr;
	}
	Beam->AttachToComponent(GunRoot, FAttachmentTransformRules::KeepRelativeTransform);
	if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Beam->GetMaterial(0)))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
	Beam->SetVisibility(true);
	return Beam;
}

UWidgetComponent* ULootFanfareSubsystem::TakeNamePop(AWeaponBase& Weapon)
{
	USceneComponent* GunRoot = Weapon.GetRootComponent();
	if (!GunRoot)
	{
		return nullptr;
	}
	UWidgetComponent* Widget = nullptr;
	while (!Widget && !FreeNamePops.IsEmpty())
	{
		Widget = FreeNamePops.Pop(EAllowShrinking::No);
		Widget = IsValid(Widget) ? Widget : nullptr;
	}
	if (!Widget)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Loot fanfare: the name pop pool ran dry; making one more"));
		Widget = MakeNamePop();
	}
	if (!Widget)
	{
		return nullptr;
	}
	Widget->AttachToComponent(GunRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Widget->SetRelativeLocation(FVector(0.f, 0.f, NamePopHeight));
	if (UWeaponLabelWidget* Label = Cast<UWeaponLabelWidget>(Widget->GetUserWidgetObject()))
	{
		Label->SetWeapon(&Weapon);
	}
	Widget->SetVisibility(true);
	return Widget;
}

void ULootFanfareSubsystem::GiveBack(UStaticMeshComponent* Beam)
{
	if (!IsValid(Beam))
	{
		return;
	}
	Beam->SetVisibility(false);
	USceneComponent* Root = PoolRoot();
	if (!Root || Beam->GetOwner() != Root->GetOwner())
	{
		// The world is going (or it isn't the pool's): it goes with it.
		Beam->DestroyComponent();
		return;
	}
	Beam->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	FreeColumns.AddUnique(Beam);
}

void ULootFanfareSubsystem::GiveBack(UWidgetComponent* Widget)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Widget->SetVisibility(false);
	if (UUserWidget* Label = Widget->GetUserWidgetObject())
	{
		Label->SetRenderOpacity(0.f);
	}
	USceneComponent* Root = PoolRoot();
	if (!Root || Widget->GetOwner() != Root->GetOwner())
	{
		Widget->DestroyComponent();
		return;
	}
	Widget->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	FreeNamePops.AddUnique(Widget);
}

// ---------------------------------------------------------------------------
// The shows
// ---------------------------------------------------------------------------

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

	// A Legendary's burst: the moment itself (LootFanfareBurst.cpp).
	if (Moment.bBurst)
	{
		StartBurst(Ground, Color, LootDropMoments::Burst());
	}

	// A Legendary's column: a second, wider beam of its colour from the pool, shooting up as the first flares, standing
	// while it lies there.
	if (Moment.ColumnHeight > 0.f && !Columns.ContainsByPredicate([&Weapon](const FColumn& Column) { return Column.Weapon.Get() == &Weapon; }))
	{
		if (UStaticMeshComponent* Beam = TakeColumn(Weapon, Color))
		{
			FColumn& Column = Columns.AddDefaulted_GetRef();
			Column.Weapon = &Weapon;
			Column.Beam = Beam;
			Column.Glow = Moment.ColumnGlow;
			Column.Height = Moment.ColumnHeight;
			Column.Radius = Moment.ColumnRadius;
			LightBeams::Flare(Beam, 0.f, true, Column.Glow, Column.Height, Column.Radius);
		}
	}

	// Its name pops up over it (a pooled label of its own, apart from the one that shows when it's looked at).
	if (Moment.NamePopSeconds > 0.f && !NamePops.ContainsByPredicate([&Weapon](const FNamePop& Pop) { return Pop.Weapon.Get() == &Weapon; }))
	{
		if (UWidgetComponent* Widget = TakeNamePop(Weapon))
		{
			if (UWeaponLabelWidget* Label = Cast<UWeaponLabelWidget>(Widget->GetUserWidgetObject()))
			{
				Label->SetPop(0.f, Moment.NamePopSeconds);
			}
			FNamePop& Pop = NamePops.AddDefaulted_GetRef();
			Pop.Weapon = &Weapon;
			Pop.Widget = Widget;
			Pop.Seconds = Moment.NamePopSeconds;
		}
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

	AdvanceBurst(DeltaTime);

	for (int32 Index = Columns.Num() - 1; Index >= 0; --Index)
	{
		FColumn& Column = Columns[Index];
		const AWeaponBase* Weapon = Column.Weapon.Get();
		UStaticMeshComponent* Beam = Column.Beam.Get();
		// Taken up (or gone): its column goes back to the pool.
		if (!Weapon || !Weapon->IsPickup() || !Beam)
		{
			GiveBack(Beam);
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
			GiveBack(Widget);
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
	ArmSparks.Reset();
	FountainSparks.Reset();
	// The pooled columns and name pops live on the fanfare's own actor, which goes with the world.
	Columns.Reset();
	NamePops.Reset();
	FreeColumns.Reset();
	FreeNamePops.Reset();
	PoolHolder = nullptr;
}
