#include "Player/Animation/BodyFigureComponent.h"
#include "Audio/LooterSound.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

// UBodyFigureComponent's idle props (the user's stances of 2026-10-22): Gauge's stacks venting steam every few breaths as
// he stands, through the bullets' code-drawn wisps (no particle asset), and Odessa's card walking over her knuckles, a small
// card mesh set on this body's knuckle bones each frame (opaque and owned by the body's actor, so the hero select's stand,
// which captures only its own actor and no translucency, shows it; the steam shows in play only).

namespace
{
	const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* const SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	/** A playing card (cm): long, short; two planes back to back (the engine's Plane: 100 cm square, facing +Z, one-sided). */
	constexpr float CardLong = 8.9f;
	constexpr float CardShort = 6.4f;
	constexpr float CardBackGap = 0.05f;
	/** The card's middle over the knuckles (cm), and how much higher it rides standing on edge halfway over. */
	constexpr float CardLift = 1.2f;
	constexpr float CardStand = 2.5f;
	/** The card's face: old ivory pasteboard; its back Odessa's red (#D04A68), as her thrown cards' edge (linear). */
	const FLinearColor CardPaper(0.78f, 0.72f, 0.6f);
	const FLinearColor CardBack(0.63f, 0.07f, 0.14f);

	/** One face of the card: a plane of the body's owner with no collision, shadow or decals, Colour on the surface master. */
	UStaticMeshComponent* MakeCardFace(AActor& Owner, UStaticMesh* Plane, UMaterialInterface* Surface, const TCHAR* Name, const FLinearColor& Colour,
		const FLightingChannels& Channels)
	{
		UStaticMeshComponent* Face = NewObject<UStaticMeshComponent>(&Owner, Name);
		Face->SetStaticMesh(Plane);
		Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Face->SetGenerateOverlapEvents(false);
		Face->SetCanEverAffectNavigation(false);
		Face->SetCastShadow(false);
		Face->bReceivesDecals = false;
		// On the body's light channels (the hero select's stand lights its hero on its own).
		Face->LightingChannels = Channels;
		if (Surface)
		{
			UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Surface, Face);
			Look->SetVectorParameterValue(TEXT("Color"), Colour);
			Face->SetMaterial(0, Look);
		}
		return Face;
	}
	/** Steam only once the idle has settled in (its eased share), so a stop between steps doesn't puff. */
	constexpr float SteamFromIdle = 0.7f;
	/** Each stack's breath out: puffs, their rise and spread (cm/s), sizes (cm), opacity, life (s); 3 faint short puffs were gone
	 * before they read in play (2026-10-22). */
	constexpr int32 SteamPuffs = 5;
	constexpr float SteamRiseMin = 70.f;
	constexpr float SteamRiseMax = 120.f;
	constexpr float SteamSpread = 25.f;
	constexpr float SteamStartSize = 5.f;
	constexpr float SteamEndSize = 55.f;
	constexpr float SteamOpacity = 0.45f;
	constexpr float SteamLifeMin = 1.6f;
	constexpr float SteamLifeMax = 2.4f;
	constexpr float SteamVolume = 0.7f;
}

void UBodyFigureComponent::TickIdleProps(float DeltaTime)
{
	const USkeletalMeshComponent* Body = HiddenBody.Get();
	const ULooterCharacterAnimInstance* Anim = Body ? Cast<ULooterCharacterAnimInstance>(Body->GetAnimInstance()) : nullptr;
	if (!Anim)
	{
		HideCard();
		LastVentBreath = INDEX_NONE;
		return;
	}
	// The idle as the mannequin's stance layer plays it this frame (the same clock and the tuned stance).
	const FLooterStanceInput& Input = Anim->GetStanceInput();
	const FHeroIdle& Style = Input.Idle;
	const float Alpha = Input.IdleAlpha;

	// Steam with every SteamEvery-th breath out (half a breath in), once the idle has settled; the first breath seen only
	// starts the count.
	if (Style.SteamEvery > 0 && Alpha >= SteamFromIdle && !Style.SteamVent.IsNearlyZero())
	{
		const int32 Out = FMath::FloorToInt(Input.IdleTime / FMath::Max(Style.BreathPeriod, 0.5f) - 0.5f);
		if (Out != LastVentBreath)
		{
			const bool bCounting = LastVentBreath != INDEX_NONE;
			LastVentBreath = Out;
			if (bCounting && Out >= 0 && Out % Style.SteamEvery == 0)
			{
				VentSteam(Style, Input.Forward);
			}
		}
	}
	else
	{
		LastVentBreath = INDEX_NONE;
	}

	const FLooterIdleStance Idle = LooterStancePose::DescribeIdle(Input.IdleTime, Style);
	if (Idle.Card >= 0.f && Alpha > 0.05f && IsVisible() && !bHiddenInGame)
	{
		PlaceCard(Style, Idle, Alpha);
	}
	else
	{
		HideCard();
	}
}

void UBodyFigureComponent::VentSteam(const FHeroIdle& Style, const FVector& Forward)
{
	const USkeletalMeshComponent* Body = HiddenBody.Get();
	UWorld* World = GetWorld();
	FWeaponFX* Effects = World ? UEmberPowerBase::EffectsIn(World) : nullptr;
	if (!Body || !Effects)
	{
		return;
	}
	// The vents in the mannequin's component space (the body's facing; its right is up cross ahead), into the world at the
	// body's size.
	const FTransform& Frame = Body->GetComponentTransform();
	const FVector Up = FVector::UpVector;
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	const float Scale = static_cast<float>(Frame.GetScale3D().Z);
	const FVector WorldUp = Frame.TransformVectorNoScale(Up);
	FVector Middle = FVector::ZeroVector;
	for (const float Side : { -1.f, 1.f })
	{
		const FVector Local = Forward * Style.SteamVent.X + Right * (Side * Style.SteamVent.Y) + Up * Style.SteamVent.Z;
		const FVector Mouth = Frame.TransformPosition(Local);
		const FVector Out = Frame.TransformVectorNoScale(Right * Side - Forward * 0.4f).GetSafeNormal();
		Middle += Mouth * 0.5f;
		for (int32 Puff = 0; Puff < SteamPuffs; ++Puff)
		{
			const FVector Velocity = (WorldUp * SteamRandom.FRandRange(SteamRiseMin, SteamRiseMax) + Out * SteamRandom.FRandRange(0.f, SteamSpread)
				+ SteamRandom.GetUnitVector() * 12.f) * Scale;
			Effects->SpawnWisp(Mouth, Velocity, SteamOpacity, SteamStartSize * Scale, SteamEndSize * Scale, SteamRandom.FRandRange(SteamLifeMin, SteamLifeMax));
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::Hero::GaugeVent, Middle, SteamVolume);
}

void UBodyFigureComponent::PlaceCard(const FHeroIdle& Style, const FLooterIdleStance& Idle, float Alpha)
{
	const bool bRight = Style.RightRest == EHeroIdleRest::CardHand;
	const TCHAR* const Side = bRight ? TEXT("_r") : TEXT("_l");
	const FName Hand(*(FString(TEXT("hand")) + Side));
	const FName Knuckles[] = { FName(*(FString(TEXT("index_01")) + Side)), FName(*(FString(TEXT("middle_01")) + Side)),
		FName(*(FString(TEXT("ring_01")) + Side)), FName(*(FString(TEXT("pinky_01")) + Side)) };
	if (GetBoneIndex(Hand) == INDEX_NONE)
	{
		HideCard();
		return;
	}
	// The knuckles across the back of the hand, index to pinky (a hand short of a finger walks the last it has).
	FVector Points[UE_ARRAY_COUNT(Knuckles)];
	int32 Count = 0;
	for (const FName& Knuckle : Knuckles)
	{
		if (GetBoneIndex(Knuckle) != INDEX_NONE)
		{
			Points[Count++] = GetBoneLocation(Knuckle);
		}
	}
	if (Count < 2)
	{
		HideCard();
		return;
	}
	const FVector Wrist = GetBoneLocation(Hand);
	const FVector Fingers = ((Points[0] + Points[Count - 1]) * 0.5f - Wrist).GetSafeNormal();
	const FVector Across = (Points[0] - Points[Count - 1]).GetSafeNormal();
	// The palm's facing, the way the stance layer's hand rig reads it (a right hand's fingers cross the knuckle line).
	const FVector Palm = FVector::VectorPlaneProject(bRight ? FVector::CrossProduct(Fingers, Across) : FVector::CrossProduct(Across, Fingers), Fingers)
		.GetSafeNormal();
	if (Fingers.IsNearlyZero() || Palm.IsNearlyZero())
	{
		HideCard();
		return;
	}

	if (!IdleCard)
	{
		UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, PlanePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		AActor* Owner = GetOwner();
		if (!Plane || !Owner)
		{
			return;
		}
		// A flat card: the face, and the back on it turned over (the plane is one-sided), a hair behind.
		UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, SurfacePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		IdleCard = MakeCardFace(*Owner, Plane, Surface, TEXT("IdleCard"), CardPaper, LightingChannels);
		IdleCard->SetUsingAbsoluteLocation(true);
		IdleCard->SetUsingAbsoluteRotation(true);
		IdleCard->SetUsingAbsoluteScale(true);
		IdleCard->SetupAttachment(this);
		IdleCard->RegisterComponent();
		UStaticMeshComponent* CardBackFace = MakeCardFace(*Owner, Plane, Surface, TEXT("IdleCardBack"), CardBack, LightingChannels);
		CardBackFace->SetupAttachment(IdleCard);
		CardBackFace->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -CardBackGap), FRotator(0.0, 0.0, 180.0));
		CardBackFace->RegisterComponent();
	}

	// On the knuckle it's crossing, the long side along the fingers, turned over about it; standing up on its edge as it goes
	// over, as a card walked over the knuckles does.
	const float At = FMath::Clamp(Idle.Card, 0.f, static_cast<float>(Count - 1));
	const int32 From = FMath::Min(FMath::FloorToInt(At), Count - 1);
	const int32 To = FMath::Min(From + 1, Count - 1);
	const FVector Knuckle = FMath::Lerp(Points[From], Points[To], At - static_cast<float>(From));
	const float Scale = static_cast<float>(GetComponentScale().Z);
	const FVector Back = -Palm;
	const FVector Center = Knuckle + Back * ((CardLift + CardStand * FMath::Abs(FMath::Sin(Idle.CardSpin))) * Scale);
	const FVector Face = FQuat(Fingers, Idle.CardSpin).RotateVector(Back);
	IdleCard->SetWorldLocationAndRotation(Center, FRotationMatrix::MakeFromXZ(Fingers, Face).ToQuat());
	// The plane is 100 cm square; Z stays 1 so the back's gap is in cm.
	const float Size = 0.01f * Scale * FMath::Clamp(Alpha * 2.f, 0.f, 1.f);
	IdleCard->SetWorldScale3D(FVector(CardLong * Size, CardShort * Size, 1.f));
	// Drawn as this body is: hidden to its owner in first person.
	IdleCard->SetOwnerNoSee(bOwnerNoSee);
	for (USceneComponent* Child : IdleCard->GetAttachChildren())
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Child))
		{
			Primitive->SetOwnerNoSee(bOwnerNoSee);
		}
	}
	IdleCard->SetVisibility(true, /*bPropagateToChildren*/ true);
}

void UBodyFigureComponent::HideCard()
{
	if (IdleCard && IdleCard->IsVisible())
	{
		IdleCard->SetVisibility(false, /*bPropagateToChildren*/ true);
	}
}
