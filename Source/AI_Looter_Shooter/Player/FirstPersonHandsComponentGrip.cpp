#include "Player/FirstPersonHandsComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/FirstPersonHandsGrips.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// UFirstPersonHandsComponent's grip (FPGRIP, 2026-10-22): the trigger finger's discipline (on the trigger only when it's
// wanted there, else laid along the frame, as a trained shooter holds a gun) and the grip's debug drawing.

namespace
{
	/** The trigger finger reaches the trigger this fast (a pull fires at once; the finger is there within a few frames) and leaves slower. */
	constexpr float TriggerOnSeconds = 0.06f;
	constexpr float TriggerOffSeconds = 0.2f;
	/** Sights this far up count as aiming (the finger goes to the trigger as they come up). */
	constexpr float TriggerAimShare = 0.25f;
}

float UFirstPersonHandsComponent::NextTriggerReady(const AWeaponBase& Weapon, float DeltaTime)
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Weapon.WantsToFire())
	{
		LastTriggerTime = Now;
	}
	const UPlayerViewComponent* PlayerView = View.Get();
	const bool bAiming = PlayerView && PlayerView->GetAimAlpha() > TriggerAimShare;
	const bool bWanted = !FirstPersonHandsGrips::UseTriggerDiscipline() || Weapon.WantsToFire() || bAiming
		|| Now - LastTriggerTime < FirstPersonHandsGrips::TriggerHoldSeconds();
	const float Wanted = bWanted ? 1.f : 0.f;
	const float Seconds = Wanted > TriggerReady ? TriggerOnSeconds : TriggerOffSeconds;
	TriggerReady = FMath::FInterpConstantTo(TriggerReady, Wanted, DeltaTime, 1.f / Seconds);
	return TriggerReady;
}

FHandsCupFit UFirstPersonHandsComponent::CupFitFor(const UWeaponModelComponent& Model, EWeaponKind Kind, bool bAtRest) const
{
	// The gun as assembled: this model, its kind and its parts' meshes (a swap at the bench or another gun changes it).
	uint32 Key = HashCombine(HashCombine(GetTypeHash(&Model), static_cast<uint32>(Kind) + 1u), FirstPersonHandsGrips::CupFitSettings());
	for (const UStaticMeshComponent* Part : Model.GetParts())
	{
		Key = HashCombine(Key, GetTypeHash(Part ? Part->GetStaticMesh() : nullptr));
	}
	if (Key == CupFitKey)
	{
		return CupFit;
	}
	if (!bAtRest)
	{
		return FHandsCupFit();
	}
	CupFitKey = Key;
	CupFit = FirstPersonHandsGrips::FitCup(Model, Kind);
	const FGunHands& Table = FirstPersonHandsGrips::For(Kind);
	if (CupFit.bFitted)
	{
		UE_LOG(LogLooter, Log, TEXT("Cup fit, %s: underside %s (the table's %.2f), half width %.2f (the table's %.2f; 0: not measured)."),
			*UEnum::GetValueAsString(Kind), *CupFit.Under.ToCompactString(), Table.CupShape.Offset.Z - Table.CupShape.HalfAcross, CupFit.HalfWidth,
			Table.CupShape.HalfThrough);
	}
	else if (Table.bCup)
	{
		UE_LOG(LogLooter, Log, TEXT("Cup fit, %s: no underside found on the parts; the table's cup."), *UEnum::GetValueAsString(Kind));
	}
	return CupFit;
}

void UFirstPersonHandsComponent::ReportGrip() const
{
	if (!Rig.IsBound() || PosedSpace.IsEmpty())
	{
		UE_LOG(LogLooter, Display, TEXT("Grip report: no hands posed."));
		return;
	}
	const FTransform MeshToCamera = GetRelativeTransform();
	const float Scale = FMath::Max(LastGun.Scale, 0.01f);
	auto InGun = [&](const FVector& Camera) { return LastGun.ToCamera.InverseTransformPosition(Camera) / Scale; };
	UE_LOG(LogLooter, Display, TEXT("Grip report, %s (gun cm, modeled; drawn x%.2f; arms x%.2f), trigger ready %.2f"),
		*UEnum::GetValueAsString(LastGun.Kind), Scale, MeshToCamera.GetScale3D().X, LastGun.TriggerReady);
	UE_LOG(LogLooter, Display, TEXT("  Cup: %s, underside %s from the grip, half width %.2f"), LastGun.Cup.bFitted ? TEXT("fitted to the parts") : TEXT("the table's"),
		*LastGun.Cup.Under.ToCompactString(), LastGun.Cup.HalfWidth);
	for (const EFirstPersonHand Hand : { EFirstPersonHand::Right, EFirstPersonHand::Left })
	{
		const int32 Side = FirstPersonHands::Index(Hand);
		const FHandPlacement& Target = Targets[Side];
		const FHandContact& Contact = Target.Contact;
		const FVector Posed = MeshToCamera.TransformPosition(Rig.HoldPointIn(Hand, PosedSpace));
		const FVector PalmDir = MeshToCamera.TransformVectorNoScale(Rig.PalmDirIn(Hand, PosedSpace));
		const FVector PalmSkin = Posed - PalmDir * (FirstPersonHands::GripOffPalmSkin * MeshToCamera.GetScale3D().X);
		UE_LOG(LogLooter, Display, TEXT("  %s hand: asked %s, got %s (%.1f cm apart), palm %s, palm gap %.2f, share %.2f seat %.2f, touched bits %d"),
			Hand == EFirstPersonHand::Right ? TEXT("Trigger") : TEXT("Support"), *InGun(Target.Hold).ToCompactString(), *InGun(Posed).ToCompactString(),
			FVector::Dist(Target.Hold, Posed), *LastGun.ToCamera.InverseTransformVectorNoScale(PalmDir).ToCompactString(),
			Contact.Handle.Distance(PalmSkin), Contact.Share, Contact.Seat, Rig.GetLastTouched(Hand));
		const FHandFingers& Solved = Rig.GetLastSolved(Hand);
		const TCHAR* const Names[] = { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
		for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
		{
			FVector Chain[4];
			bool bAll = true;
			for (int32 Joint = 0; Joint < 3; ++Joint)
			{
				const int32 Bone = Rig.GetFingerBone(Hand, Finger, Joint);
				bAll = bAll && Bone != INDEX_NONE;
				Chain[Joint] = Bone != INDEX_NONE ? MeshToCamera.TransformPosition(PosedSpace[Bone].GetLocation()) : FVector::ZeroVector;
			}
			if (!bAll)
			{
				continue;
			}
			Chain[3] = MeshToCamera.TransformPosition(Rig.FingerTipIn(Hand, Finger, PosedSpace));
			float Nearest = TNumericLimits<float>::Max();
			for (int32 Segment = 0; Segment < 3; ++Segment)
			{
				for (const float T : { 0.34f, 0.67f, 1.f })
				{
					Nearest = FMath::Min(Nearest, Contact.Handle.Distance(FMath::Lerp(Chain[Segment], Chain[Segment + 1], T)) - Contact.FingerRadius);
				}
			}
			UE_LOG(LogLooter, Display, TEXT("    %s: solved %.0f/%.0f/%.0f spread %.0f, gap %.2f, knuckle %s tip %s"), Names[Finger], Solved.Curl[Finger][0],
				Solved.Curl[Finger][1], Solved.Curl[Finger][2], Solved.Spread[Finger], Nearest, *InGun(Chain[0]).ToCompactString(), *InGun(Chain[3]).ToCompactString());
		}
		if (Contact.bTrigger)
		{
			const int32 End = Rig.GetFingerBone(Hand, FHandFingers::IndexFinger, 2);
			if (End != INDEX_NONE)
			{
				const FVector Pad = MeshToCamera.TransformPosition(FMath::Lerp(PosedSpace[End].GetLocation(), Rig.FingerTipIn(Hand, FHandFingers::IndexFinger, PosedSpace), 0.55f));
				UE_LOG(LogLooter, Display, TEXT("    trigger: spot %s, pad %s, %.2f cm off"), *InGun(Contact.TriggerPoint).ToCompactString(), *InGun(Pad).ToCompactString(),
					FVector::Dist(Pad, Contact.TriggerPoint));
			}
		}
	}
}

namespace
{
	void ReportPlayerGrip(UWorld* World)
	{
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (const UFirstPersonHandsComponent* Hands = Pawn ? Pawn->FindComponentByClass<UFirstPersonHandsComponent>() : nullptr)
		{
			Hands->ReportGrip();
		}
	}

	FAutoConsoleCommandWithWorld CmdGripReport(TEXT("Looter.Hands.Grip.Report"),
		TEXT("Logs each first-person hand on its gun in numbers: asked and got, palm gap, fingers touched, curls and gaps, the trigger finger's miss."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ReportPlayerGrip));
}

void UFirstPersonHandsComponent::DrawGripDebug() const
{
#if ENABLE_DRAW_DEBUG
	const UCameraComponent* Eye = Camera.Get();
	const UWorld* World = GetWorld();
	if (!Eye || !World)
	{
		return;
	}
	// The targets are in the camera's space: each handle as a box (a long one drawn 8 cm), the trigger spot, then the
	// posed finger joints (green: the trigger hand, cyan: the support hand).
	const FTransform CameraWorld = Eye->GetComponentTransform();
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FHandContact Contact = Targets[Side].Contact.Moved(CameraWorld);
		const FHandleShape& Handle = Contact.Handle;
		const FColor Color = Side == FirstPersonHands::Index(EFirstPersonHand::Right) ? FColor::Green : FColor::Cyan;
		if (Handle.bValid)
		{
			const FVector Axis = Handle.Axis.GetSafeNormal();
			const FVector Across = (Handle.Across - Axis * FVector::DotProduct(Handle.Across, Axis)).GetSafeNormal();
			const FQuat Turn = FRotationMatrix::MakeFromXY(Axis, Across).ToQuat();
			const FVector Extent(Handle.HalfLength > 0.f ? Handle.HalfLength : 4.f, Handle.HalfAcross, Handle.HalfThrough);
			DrawDebugBox(World, Handle.Center, Extent, Turn, Color, false, -1.f, SDPG_Foreground, 0.1f);
		}
		if (Contact.bTrigger)
		{
			DrawDebugPoint(World, Contact.TriggerPoint, 6.f, FColor::Red, false, -1.f, SDPG_Foreground);
		}
		const EFirstPersonHand Hand = Side == 0 ? EFirstPersonHand::Left : EFirstPersonHand::Right;
		for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
		{
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				const int32 Bone = Rig.GetFingerBone(Hand, Finger, Joint);
				if (Bone != INDEX_NONE)
				{
					DrawDebugPoint(World, GetBoneTransform(Bone).GetLocation(), 4.f, Color, false, -1.f, SDPG_Foreground);
				}
			}
		}
	}
#endif
}
