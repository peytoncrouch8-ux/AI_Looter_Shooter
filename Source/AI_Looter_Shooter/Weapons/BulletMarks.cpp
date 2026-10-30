#include "Weapons/BulletMarks.h"
#include "AI_Looter_Shooter.h"
#include "Weapons/ElementVfxTuning.h"
#include "World/BreakableKinds.h"
#include "Components/DecalComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

const TCHAR* FBulletMarks::MaterialPath = TEXT("/Game/Art/Materials/Masters/M_BulletMark.M_BulletMark");

namespace
{
	TAutoConsoleVariable<bool> CVarBulletMarks(TEXT("Looter.BulletMarks"), true,
		TEXT("Bullets leave marks on the world (1) or don't (0)."));
	TAutoConsoleVariable<int32> CVarBulletMarksMax(TEXT("Looter.BulletMarks.Max"), 64,
		TEXT("The most bullet marks showing at once (0-256); past it the oldest makes room."));
	TAutoConsoleVariable<float> CVarBulletMarksHold(TEXT("Looter.BulletMarks.Hold"), 20.f,
		TEXT("Seconds a bullet mark shows in full before it starts to fade."));
	TAutoConsoleVariable<float> CVarBulletMarksFade(TEXT("Looter.BulletMarks.Fade"), 15.f,
		TEXT("Seconds a bullet mark takes to fade away."));
	TAutoConsoleVariable<bool> CVarBulletMarksHeavyCracks(TEXT("Looter.BulletMarks.HeavyCracks"), true,
		TEXT("A heavy round's mark on stone or wood is ringed or split by cracks (needs the atlas with styles 13-14)."));
	TAutoConsoleVariable<float> CVarBulletMarksHeavyFrom(TEXT("Looter.BulletMarks.HeavyFrom"), 1.2f,
		TEXT("A round's mark scale from which it leaves the cracked marks (revolver 1.25, sniper 1.5)."));

	/** A mark hurried out of the way under a long burst fades over this long. */
	constexpr float MarkHurrySeconds = 1.2f;
	/** Hurrying starts this many marks short of the pool's cap, so a burst never runs it dry. */
	constexpr int32 MarkHurryMargin = 12;
	/**
	 * Marks fade out once they're this small on screen (the decal's own FadeScreenSize: a 6 cm mark still shows some 40 m
	 * away), so far ones cost nothing.
	 */
	constexpr float MarkFadeScreenSize = 0.0025f;

	/**
	 * Each surface's mark: its style (the atlas's order, make_bullet_marks.py STYLES: 0 a plain hole, 1 dirt, 2 grass,
	 * 3 gravel, 4 sand, 5 stone, 6 wood, 7 metal, 8 sheet metal, 9 glass, 10 cloth, 11 hay, 12 bone; 13-14 a heavy round's
	 * stone and wood, HeavyLookOf) and its radius in cm
	 * for a rifle round: soft ground throws a wider puff than a hole punched in timber, glass cracks wide.
	 */
	FBulletMarkLook MarkLook(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Dirt: return { 1, 9.f };
		case EImpactMaterial::Grass: return { 2, 8.f };
		case EImpactMaterial::Gravel: return { 3, 8.f };
		case EImpactMaterial::Sand: return { 4, 9.5f };
		case EImpactMaterial::Stone: return { 5, 7.f };
		case EImpactMaterial::Wood: return { 6, 6.f };
		case EImpactMaterial::Metal: return { 7, 5.f };
		case EImpactMaterial::SheetMetal: return { 8, 5.5f };
		case EImpactMaterial::Glass: return { 9, 11.f };
		case EImpactMaterial::Cloth: return { 10, 4.5f };
		case EImpactMaterial::Hay: return { 11, 6.f };
		case EImpactMaterial::Bone: return { 12, 5.f };
		case EImpactMaterial::Water: return { INDEX_NONE, 0.f };
		case EImpactMaterial::Unknown:
		default:
			return { 0, 5.f };
		}
	}

	/** Any unit vector at right angles to Direction. */
	FVector MarkPerpendicular(const FVector& Direction)
	{
		const FVector Other = FMath::Abs(Direction.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Direction, Other).GetSafeNormal();
	}
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

bool FBulletMarks::CanMark(const FHitResult& Hit, EImpactMaterial Material)
{
	if (LookOf(Material).Style < 0)
	{
		return false;
	}
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component || !Component->bReceivesDecals)
	{
		return false;
	}
	// Never on anything that moves: a decal stays where it's put, and would slide off a moving thing.
	if (Component->Mobility == EComponentMobility::Movable || Component->IsSimulatingPhysics())
	{
		return false;
	}
	const AActor* Actor = Hit.GetActor();
	return !Actor || (!Actor->IsA<APawn>() && !Actor->ActorHasTag(FName(LooterBreakables::Tag)));
}

FBulletMarkLook FBulletMarks::LookOf(EImpactMaterial Material)
{
	return MarkLook(Material);
}

FBulletMarkLook FBulletMarks::HeavyLookOf(EImpactMaterial Material)
{
	// The atlas's styles 13 and 14 (make_bullet_marks.py StoneHeavy, WoodHeavy), each a little wider than the plain one: the
	// cracks run out past the chip.
	switch (Material)
	{
	case EImpactMaterial::Stone: return { 13, 8.5f };
	case EImpactMaterial::Wood: return { 14, 7.5f };
	default: return { INDEX_NONE, 0.f };
	}
}

FBulletMarkLook FBulletMarks::LookFor(EImpactMaterial Material, float SizeScale)
{
	if (CVarBulletMarksHeavyCracks.GetValueOnGameThread() && SizeScale >= CVarBulletMarksHeavyFrom.GetValueOnGameThread())
	{
		const FBulletMarkLook Heavy = HeavyLookOf(Material);
		if (Heavy.Style >= 0)
		{
			return Heavy;
		}
	}
	return LookOf(Material);
}

float FBulletMarks::GunScale(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		return 0.6f;
	case EWeaponKind::Revolver:
		return 1.25f;
	// The SMG's small rounds leave small holes, the sniper's big round the biggest; the LMG's are the rifle's round. A
	// rocket leaves no bullet's mark (its burst is its own).
	case EWeaponKind::SMG:
		return 0.8f;
	case EWeaponKind::Sniper:
		return 1.5f;
	case EWeaponKind::LMG:
		return 1.05f;
	case EWeaponKind::Launcher:
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		return 1.f;
	}
}

FBulletMarkPlacement FBulletMarks::Place(const FVector& Point, const FVector& Normal, const FVector& ShotDirection, float Radius, float Turn)
{
	FBulletMarkPlacement Placement;
	const FVector Out = Normal.IsNearlyZero() ? -ShotDirection.GetSafeNormal() : Normal.GetSafeNormal();
	const FVector Shot = ShotDirection.IsNearlyZero() ? -Out : ShotDirection.GetSafeNormal();
	// How square the shot met the surface: a round that only grazed it gouges a long streak down range.
	const float Square = FMath::Clamp(static_cast<float>(FVector::DotProduct(-Shot, Out)), 0.f, 1.f);
	Placement.Grazing = 1.f - FMath::SmoothStep(0.3f, 0.75f, Square);
	FVector Along = Shot - Out * FVector::DotProduct(Shot, Out);
	if (!Along.Normalize())
	{
		Along = MarkPerpendicular(Out);
	}
	// Square shots turn their mark at random, so no two look alike; grazing ones keep it along their way.
	const float Angle = Turn * 2.f * UE_PI * (1.f - Placement.Grazing);
	const FVector Turned = Along.RotateAngleAxisRad(Angle, Out);
	Placement.Rotation = FRotationMatrix::MakeFromXZ(-Out, Turned).Rotator();
	const float Stretch = 1.f + Placement.Grazing;
	Placement.Size = FVector(FMath::Max(3.f, 0.6f * Radius), Radius, Radius * Stretch);
	Placement.Location = Point + Turned * (Radius * 0.35f * Placement.Grazing);
	return Placement;
}

FLinearColor FBulletMarks::Code(int32 Style, float Seed)
{
	return FLinearColor((FMath::Clamp(Style, 0, StyleSlots - 1) + 0.5f) / static_cast<float>(StyleSlots), FMath::Clamp(Seed, 0.f, 0.999f), 0.f, 1.f);
}

int32 FBulletMarks::PickSlot(TConstArrayView<double> Ends, TConstArrayView<double> Born, double Now, int32 Max)
{
	for (int32 Index = 0; Index < Ends.Num(); ++Index)
	{
		if (Ends[Index] <= Now)
		{
			return Index;
		}
	}
	if (Ends.Num() < Max)
	{
		return Ends.Num();
	}
	int32 Oldest = INDEX_NONE;
	for (int32 Index = 0; Index < Born.Num(); ++Index)
	{
		if (Oldest == INDEX_NONE || Born[Index] < Born[Oldest])
		{
			Oldest = Index;
		}
	}
	return Oldest;
}

int32 FBulletMarks::PickHurry(TConstArrayView<double> Born, TConstArrayView<double> FadeAt, TConstArrayView<double> Ends,
	TConstArrayView<bool> Hurried, double Now, int32 SoftCap)
{
	int32 Live = 0;
	int32 Oldest = INDEX_NONE;
	for (int32 Index = 0; Index < Ends.Num(); ++Index)
	{
		if (Ends[Index] <= Now)
		{
			continue;
		}
		++Live;
		const bool bHolding = FadeAt[Index] > Now && !Hurried[Index];
		if (bHolding && (Oldest == INDEX_NONE || Born[Index] < Born[Oldest]))
		{
			Oldest = Index;
		}
	}
	return Live > SoftCap ? Oldest : INDEX_NONE;
}

// ---------------------------------------------------------------------------
// The pool
// ---------------------------------------------------------------------------

bool FBulletMarks::Initialize(UWorld& World)
{
	if (!DecalMaterial.IsValid())
	{
		if (bMaterialMissing)
		{
			return false;
		}
		DecalMaterial = LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!DecalMaterial.IsValid())
		{
			bMaterialMissing = true;
			UE_LOG(LogLooter, Warning, TEXT("No bullet marks: %s isn't made yet (Tools/Unreal/build_world_materials.py M_BulletMark)."), MaterialPath);
			return false;
		}
	}
	if (Owner.IsValid())
	{
		return true;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Actor)
	{
		return false;
	}
#if WITH_EDITOR
	Actor->SetActorLabel(TEXT("BulletMarks"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Owner = Actor;
	Marks.Reset();
	return true;
}

UDecalComponent* FBulletMarks::MakeDecal()
{
	AActor* Actor = Owner.Get();
	if (!Actor)
	{
		return nullptr;
	}
	UDecalComponent* Decal = NewObject<UDecalComponent>(Actor, NAME_None, RF_Transient);
	Decal->SetupAttachment(Actor->GetRootComponent());
	Decal->SetMobility(EComponentMobility::Movable);
	Decal->SetUsingAbsoluteLocation(true);
	Decal->SetUsingAbsoluteRotation(true);
	Decal->SetUsingAbsoluteScale(true);
	Decal->SetDecalMaterial(DecalMaterial.Get());
	Decal->FadeScreenSize = MarkFadeScreenSize;
	// The pool keeps its decals: never let the engine's fade-out timer destroy one (or the pool's actor).
	Decal->bDestroyOwnerAfterFade = false;
	Decal->SetVisibility(false);
	// Registered with no fade set, so the component's own life span (set as it begins play) is none.
	Decal->RegisterComponent();
	return Decal;
}

bool FBulletMarks::Add(UWorld& World, const FHitResult& Hit, const FVector& ShotDirection, EImpactMaterial Material, float SizeScale,
	float Pick, ELooterElement Element)
{
	const int32 Max = FMath::Clamp(CVarBulletMarksMax.GetValueOnGameThread(), 0, 256);
	// An element round leaves its element's mark (BulletMarksElements.cpp), else the surface's own.
	const FBulletMarkLook ElementLook = Element != ELooterElement::None && FElementVfxTuning::Live().bMarks ? ElementLookOf(Element, Material) : FBulletMarkLook();
	const FBulletMarkLook Look = ElementLook.Style >= 0 ? ElementLook : LookFor(Material, SizeScale);
	if (!CVarBulletMarks.GetValueOnGameThread() || Max == 0 || !CanMark(Hit, Material) || !Initialize(World))
	{
		return false;
	}
	const double Now = World.GetTimeSeconds();
	TArray<double, TInlineAllocator<256>> Ends;
	TArray<double, TInlineAllocator<256>> Born;
	for (const FMark& Mark : Marks)
	{
		// A decal lost with an old actor counts as ended: its slot is made again.
		Ends.Add(Mark.Decal.IsValid() ? Mark.Ends : -1.0);
		Born.Add(Mark.Born);
	}
	const int32 Slot = PickSlot(Ends, Born, Now, Max);
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	if (Slot >= Marks.Num())
	{
		Marks.AddDefaulted();
	}
	FMark& Mark = Marks[Slot];
	UDecalComponent* Decal = Mark.Decal.Get();
	if (!Decal)
	{
		Decal = MakeDecal();
		Mark.Decal = Decal;
	}
	if (!Decal)
	{
		return false;
	}

	// A little variety in size, the turn and the atlas variant from the one random number.
	const float Radius = Look.Radius * SizeScale * FMath::Lerp(0.85f, 1.15f, FMath::Frac(Pick * 7.31f));
	const FBulletMarkPlacement Placement = Place(Hit.ImpactPoint, Hit.ImpactNormal, ShotDirection, Radius, Pick);
	const float Hold = FMath::Max(CVarBulletMarksHold.GetValueOnGameThread(), 0.f);
	const float Fade = FMath::Max(CVarBulletMarksFade.GetValueOnGameThread(), 0.1f);
	Decal->SetWorldLocationAndRotation(Placement.Location, Placement.Rotation);
	Decal->DecalSize = Placement.Size;
	Decal->DecalColor = Code(Look.Style, FMath::Frac(Pick * 13.7f));
	// Set straight on the decal, never through SetFadeOut (which would destroy it when it ends): the proxy made as its
	// render state is rebuilt below starts the fade's clock now, and M_BulletMark fades by DecalLifetimeOpacity.
	Decal->FadeStartDelay = Hold;
	Decal->FadeDuration = Fade;
	Decal->FadeInStartDelay = 0.f;
	Decal->FadeInDuration = 0.f;
	Decal->SetVisibility(true);
	Decal->MarkRenderStateDirty();
	Mark.Born = Now;
	Mark.FadeAt = Now + Hold;
	Mark.Ends = Now + Hold + Fade;
	Mark.bHurried = false;
	Mark.bShown = true;

	// Under a long burst the oldest marks still holding fade out fast, so the pool has room before it's full.
	TArray<double, TInlineAllocator<256>> FadeAt;
	TArray<bool, TInlineAllocator<256>> Hurried;
	Ends.Reset();
	Born.Reset();
	for (const FMark& Each : Marks)
	{
		Ends.Add(Each.bShown ? Each.Ends : -1.0);
		Born.Add(Each.Born);
		FadeAt.Add(Each.FadeAt);
		Hurried.Add(Each.bHurried);
	}
	// Never the one just made.
	Hurried[Slot] = true;
	const int32 Hurry = PickHurry(Born, FadeAt, Ends, Hurried, Now, FMath::Max(Max - MarkHurryMargin, Max / 2));
	if (Hurry != INDEX_NONE)
	{
		FMark& Old = Marks[Hurry];
		if (UDecalComponent* OldDecal = Old.Decal.Get())
		{
			OldDecal->FadeStartDelay = 0.f;
			OldDecal->FadeDuration = MarkHurrySeconds;
			OldDecal->MarkRenderStateDirty();
		}
		Old.bHurried = true;
		Old.FadeAt = Now;
		Old.Ends = Now + MarkHurrySeconds;
	}
	ScheduleExpiry(World);
	return true;
}

void FBulletMarks::ScheduleExpiry(UWorld& World)
{
	AActor* Actor = Owner.Get();
	if (!Actor)
	{
		return;
	}
	double Next = TNumericLimits<double>::Max();
	for (const FMark& Mark : Marks)
	{
		if (Mark.bShown)
		{
			Next = FMath::Min(Next, Mark.Ends);
		}
	}
	FTimerManager& Timers = World.GetTimerManager();
	if (Next == TNumericLimits<double>::Max())
	{
		Timers.ClearTimer(ExpiryTimer);
		return;
	}
	// A little past the end, so the decal has faded to nothing on the GPU before it's hidden.
	const float Delay = FMath::Max(static_cast<float>(Next - World.GetTimeSeconds()) + 0.1f, 0.05f);
	if (Timers.IsTimerActive(ExpiryTimer) && Timers.GetTimerRemaining(ExpiryTimer) <= Delay)
	{
		return;
	}
	Timers.SetTimer(ExpiryTimer, FTimerDelegate::CreateWeakLambda(Actor, [this]() { HideExpired(); }), Delay, false);
}

void FBulletMarks::HideExpired()
{
	AActor* Actor = Owner.Get();
	UWorld* World = Actor ? Actor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	for (FMark& Mark : Marks)
	{
		if (Mark.bShown && Mark.Ends <= Now)
		{
			// Faded to nothing already; hidden, it stops being drawn at all.
			if (UDecalComponent* Decal = Mark.Decal.Get())
			{
				Decal->SetVisibility(false);
			}
			Mark.bShown = false;
		}
	}
	ScheduleExpiry(*World);
}

void FBulletMarks::Clear()
{
	for (FMark& Mark : Marks)
	{
		if (UDecalComponent* Decal = Mark.Decal.Get())
		{
			Decal->SetVisibility(false);
		}
		Mark.bShown = false;
		Mark.Ends = -1.0;
	}
	if (AActor* Actor = Owner.Get())
	{
		if (UWorld* World = Actor->GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpiryTimer);
		}
	}
}

void FBulletMarks::Shutdown()
{
	if (AActor* Actor = Owner.Get())
	{
		if (UWorld* World = Actor->GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpiryTimer);
		}
		Actor->Destroy();
	}
	Owner.Reset();
	Marks.Reset();
}

int32 FBulletMarks::NumLive(double Now) const
{
	int32 Live = 0;
	for (const FMark& Mark : Marks)
	{
		Live += Mark.bShown && Mark.Ends > Now && Mark.Decal.IsValid() ? 1 : 0;
	}
	return Live;
}
