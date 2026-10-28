#include "Player/FirstPersonHandsGrips.h"
#include "AI_Looter_Shooter.h"
#include "HAL/IConsoleManager.h"

// What the hands touch on each gun kind (FPGRIP, 2026-10-22; the user: "the fingers and hand need to grip the gun better
// and like an actual military officer"): each handle's solid as a rounded box, the trigger's face, the thumbs' swing and
// the trigger finger's indexed lie. Read off the default parts' models (Art/Models/Weapons/<Gun>.py: u, v, w are X, Z
// and -Y here; cm of the modeled gun; the game draws it bigger and scales these with it). The hands' rig closes the
// fingers on these (FirstPersonHandsContact.cpp). Live tuning: Looter.Hands.GripShape* / ForeShape* / TriggerAt* /
// *Thumb* (Looter.Hands.Kind picks the gun), Looter.Hands.ContactDump prints what to bake.

namespace
{
	FHandleSpec Handle(const FVector& Offset, const FVector& Axis, const FVector& Across, float HalfAcross, float HalfThrough, float Corner)
	{
		FHandleSpec Spec;
		Spec.Offset = Offset;
		Spec.Axis = Axis.GetSafeNormal();
		Spec.Across = Across.GetSafeNormal();
		Spec.HalfAcross = HalfAcross;
		Spec.HalfThrough = HalfThrough;
		Spec.Corner = Corner;
		Spec.bValid = true;
		return Spec;
	}

	/** A pistol grip raked as Axis: front to back along the gun (Across X), its width across it. */
	FHandleSpec PistolGrip(const FVector& Offset, const FVector& Axis, float HalfDepth, float HalfWidth, float Corner)
	{
		return Handle(Offset, Axis, FVector::ForwardVector, HalfDepth, HalfWidth, Corner);
	}

	/** A fore-end along the bore: its height (Across Z) and width, endless along the gun (the hand slides on it). */
	FHandleSpec ForeEnd(const FVector& Offset, float HalfHeight, float HalfWidth, float Corner)
	{
		return Handle(Offset, FVector::ForwardVector, FVector::UpVector, HalfHeight, HalfWidth, Corner);
	}

	// Shares and sizes for every gun (camera cm). The contact on (1), the palm seated (1), a glove's half thickness.
	constexpr float DefaultContact = 1.f;
	constexpr float DefaultSeat = 1.f;
	constexpr float DefaultSeatGap = 0.15f;
	constexpr float DefaultFingerRadius = 0.8f;
	constexpr float DefaultTriggerHold = 0.6f;

#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<float> CVarContact(TEXT("Looter.Hands.Contact"), DefaultContact,
		TEXT("0-1: how much the first-person fingers close till they touch the gun's handles (0: the grip tables' curls, as before FPGRIP)."));
	TAutoConsoleVariable<float> CVarSeat(TEXT("Looter.Hands.Seat"), DefaultSeat, TEXT("0-1: how much each palm slides along its facing onto its handle."));
	TAutoConsoleVariable<float> CVarSeatGap(TEXT("Looter.Hands.SeatGap"), DefaultSeatGap, TEXT("cm a seated palm stays off its handle."));
	TAutoConsoleVariable<float> CVarFingerRadius(TEXT("Looter.Hands.FingerRadius"), DefaultFingerRadius,
		TEXT("A gloved finger's half thickness (camera cm): fingers stop closing this far from a handle's surface."));
	TAutoConsoleVariable<int32> CVarDiscipline(TEXT("Looter.Hands.TriggerDiscipline"), 1,
		TEXT("1: the trigger finger lies straight along the frame unless firing, aiming or just fired. 0: always on the trigger."));
	TAutoConsoleVariable<float> CVarTriggerHold(TEXT("Looter.Hands.TriggerHoldSeconds"), DefaultTriggerHold,
		TEXT("Seconds the trigger finger stays on the trigger after the last pull."));

	struct FShapeNudge
	{
		TAutoConsoleVariable<float> Ahead;
		TAutoConsoleVariable<float> Right;
		TAutoConsoleVariable<float> Up;
		TAutoConsoleVariable<float> HalfAcross;
		TAutoConsoleVariable<float> HalfThrough;
		TAutoConsoleVariable<float> Corner;

		explicit FShapeNudge(const TCHAR* Which)
			: Ahead(*FString::Printf(TEXT("Looter.Hands.%sAhead"), Which), 0.f, TEXT("The handle's middle moved ahead (cm of the modeled gun)."))
			, Right(*FString::Printf(TEXT("Looter.Hands.%sRight"), Which), 0.f, TEXT("The handle's middle moved right (cm)."))
			, Up(*FString::Printf(TEXT("Looter.Hands.%sUp"), Which), 0.f, TEXT("The handle's middle moved up (cm)."))
			, HalfAcross(*FString::Printf(TEXT("Looter.Hands.%sHalfAcross"), Which), 0.f, TEXT("The handle's half depth (a grip front to back, a fore-end's height) grown (cm)."))
			, HalfThrough(*FString::Printf(TEXT("Looter.Hands.%sHalfThrough"), Which), 0.f, TEXT("The handle's half width grown (cm)."))
			, Corner(*FString::Printf(TEXT("Looter.Hands.%sCorner"), Which), 0.f, TEXT("The handle's edges rounded more (cm)."))
		{
		}

		void Apply(FHandleSpec& Spec) const
		{
			Spec.Offset += FVector(Ahead.GetValueOnGameThread(), Right.GetValueOnGameThread(), Up.GetValueOnGameThread());
			Spec.HalfAcross = FMath::Max(Spec.HalfAcross + HalfAcross.GetValueOnGameThread(), 0.2f);
			Spec.HalfThrough = FMath::Max(Spec.HalfThrough + HalfThrough.GetValueOnGameThread(), 0.2f);
			Spec.Corner = FMath::Max(Spec.Corner + Corner.GetValueOnGameThread(), 0.f);
		}
	};
	FShapeNudge GripShapeNudge(TEXT("GripShape"));
	FShapeNudge ForeShapeNudge(TEXT("ForeShape"));
	TAutoConsoleVariable<float> CVarTriggerAtAhead(TEXT("Looter.Hands.TriggerAtAhead"), 0.f, TEXT("The trigger finger's spot on the trigger moved ahead (cm)."));
	TAutoConsoleVariable<float> CVarTriggerAtUp(TEXT("Looter.Hands.TriggerAtUp"), 0.f, TEXT("The trigger finger's spot on the trigger moved up (cm)."));
	TAutoConsoleVariable<float> CVarTriggerThumbSwing(TEXT("Looter.Hands.TriggerThumbSwing"), 0.f, TEXT("Trigger hand's thumb swung further in across the palm (degrees)."));
	TAutoConsoleVariable<float> CVarTriggerThumbSpread(TEXT("Looter.Hands.TriggerThumbSpread"), 0.f, TEXT("Trigger hand's thumb spread toward the fingers' way (degrees)."));
	TAutoConsoleVariable<float> CVarSupportThumbSwing(TEXT("Looter.Hands.SupportThumbSwing"), 0.f, TEXT("Support hand's thumb swung further in across the palm (degrees)."));
	TAutoConsoleVariable<float> CVarSupportThumbSpread(TEXT("Looter.Hands.SupportThumbSpread"), 0.f, TEXT("Support hand's thumb spread toward the fingers' way (degrees)."));
	TAutoConsoleVariable<float> CVarIndexedKnuckle(TEXT("Looter.Hands.IndexedKnuckle"), 0.f, TEXT("The indexed trigger finger bent in at its knuckle (degrees), onto the frame's side."));
	TAutoConsoleVariable<float> CVarIndexedSpread(TEXT("Looter.Hands.IndexedSpread"), 0.f, TEXT("The indexed trigger finger lifted further toward the thumb (degrees)."));

	bool TuningThisKind(EWeaponKind Kind)
	{
		const IConsoleVariable* Wanted = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Hands.Kind"));
		const int32 Kinds = Wanted ? Wanted->GetInt() : 0;
		return Kinds == 0 || Kinds == static_cast<int32>(Kind);
	}

	FString DumpSpec(const TCHAR* Name, const FHandleSpec& Spec)
	{
		return FString::Printf(TEXT("%s = Handle(FVector(%.2ff, %.2ff, %.2ff), ..., %.2ff, %.2ff, %.2ff)"), Name, Spec.Offset.X, Spec.Offset.Y, Spec.Offset.Z,
			Spec.HalfAcross, Spec.HalfThrough, Spec.Corner);
	}

	/** Looter.Hands.ContactDump: every gun's handles, trigger and thumbs with the tuning on them, to bake into AddContact. */
	void DumpContact()
	{
		const TPair<EWeaponKind, const TCHAR*> Kinds[] = { { EWeaponKind::Rifle, TEXT("Rifle") }, { EWeaponKind::Shotgun, TEXT("Shotgun") },
			{ EWeaponKind::Revolver, TEXT("Revolver") }, { EWeaponKind::SMG, TEXT("SMG") }, { EWeaponKind::Sniper, TEXT("Sniper") },
			{ EWeaponKind::LMG, TEXT("LMG") }, { EWeaponKind::Launcher, TEXT("Launcher") } };
		for (const TPair<EWeaponKind, const TCHAR*>& Kind : Kinds)
		{
			const FGunHands Hands = FirstPersonHandsGrips::Tuned(Kind.Key);
			UE_LOG(LogLooter, Display, TEXT("Contact, %s: %s; %s; TriggerAt = FVector(%.2ff, 0.f, %.2ff); GripThumbSwing = %.1ff; GripThumbSpread = %.1ff; ForeThumbSwing = %.1ff; ForeThumbSpread = %.1ff; IndexedSpread = %.1ff;"),
				Kind.Value, *DumpSpec(TEXT("GripShape"), Hands.GripShape), *DumpSpec(TEXT("ForeShape"), Hands.ForeShape), Hands.TriggerAt.X, Hands.TriggerAt.Z,
				Hands.GripThumbSwing, Hands.GripThumbSpread, Hands.ForeThumbSwing, Hands.ForeThumbSpread, Hands.IndexedSpread);
		}
		UE_LOG(LogLooter, Display, TEXT("Contact shares: Contact %.2f, Seat %.2f, SeatGap %.2f, FingerRadius %.2f, TriggerDiscipline %d, TriggerHoldSeconds %.2f"),
			CVarContact.GetValueOnGameThread(), CVarSeat.GetValueOnGameThread(), CVarSeatGap.GetValueOnGameThread(), CVarFingerRadius.GetValueOnGameThread(),
			CVarDiscipline.GetValueOnGameThread(), CVarTriggerHold.GetValueOnGameThread());
	}

	FAutoConsoleCommand CmdContactDump(TEXT("Looter.Hands.ContactDump"),
		TEXT("Prints the first-person hands' handles, trigger spots and thumbs with the Looter.Hands.* touch tuning on them, ready to bake."),
		FConsoleCommandDelegate::CreateStatic(&DumpContact));
#endif
}

void FirstPersonHandsGrips::AddContact(EWeaponKind Kind, FGunHands& Hands)
{
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		// Ranchhand: the stock's pistol wrist on SOCKET_Grip (-4.5, -6; the AR-style grip's middle, 5 cm front to back, 3 wide,
		// leaning forward at the top); the trigger blade's face at u 3.4, v -5 (the trigger group moved 5 cm back, 2026-10-28,
		// so the finger reaches it: it was at 8.4); the pump a 4.2 cm round tube on SOCKET_Foregrip.
		Hands.GripShape = PistolGrip(FVector::ZeroVector, FVector(0.22f, 0.f, 0.97f), 2.5f, 1.6f, 1.1f);
		Hands.TriggerAt = FVector(3.9f, 0.f, -5.2f);
		Hands.ForeShape = ForeEnd(FVector::ZeroVector, 2.1f, 2.1f, 2.1f);
		// The thumb along the pump's near side toward the muzzle (tuned in PIE: straight up it stood off the pump).
		Hands.ForeThumbSwing = 0.f;
		Hands.ForeThumbSpread = 8.f;
		break;
	case EWeaponKind::Revolver:
		// Drover: the plow-handle grip (front edge u 1.2, back -3.7 at v -7.4, 3.3 wide, raked back about 19 degrees); the
		// trigger's face at u 4.6, v -4.8; the support hand closes round the shooting hand (the grip grown by its fingers).
		Hands.GripShape = PistolGrip(FVector(0.15f, 0.f, 0.f), FVector(0.33f, 0.f, 0.94f), 2.4f, 1.65f, 1.0f);
		Hands.TriggerAt = FVector(5.1f, 0.f, -4.8f);
		Hands.ForeShape = PistolGrip(FVector(0.6f, -0.4f, 0.f), FVector(0.33f, 0.f, 0.94f), 4.2f, 3.4f, 2.4f);
		Hands.ForeShape.bFromGrip = true;
		Hands.ForeContact = 0.7f;
		Hands.ForeThumbSwing = 18.f;
		break;
	case EWeaponKind::SMG:
		// Rustler: the grip's middle at u 5.6, v -8.6 (6 cm front to back over the finger grooves, 3.2 wide); the trigger's
		// face at u 12, v -5.2; the helix magazine a 5.5 cm round tube, its underside SOCKET_Foregrip.
		Hands.GripShape = PistolGrip(FVector(0.2f, 0.f, 0.f), FVector(0.3f, 0.f, 0.95f), 2.7f, 1.6f, 0.9f);
		Hands.TriggerAt = FVector(12.5f, 0.f, -5.2f);
		Hands.ForeShape = ForeEnd(FVector(0.f, 0.f, 2.75f), 2.75f, 2.75f, 2.75f);
		break;
	case EWeaponKind::Sniper:
		// Longhorn: the thumbhole's grip between the hole (u 16.6) and the chassis's front (23.0) at v -9, 3.8 wide; the trigger's
		// face at u 28.5, v -5.6; the fore-end 3.5 tall (v -1.3 to -4.8), 3.8 wide, SOCKET_Foregrip under it.
		Hands.GripShape = PistolGrip(FVector::ZeroVector, FVector(0.27f, 0.f, 0.96f), 3.0f, 1.9f, 1.0f);
		Hands.TriggerAt = FVector(29.1f, 0.f, -5.6f);
		Hands.ForeShape = ForeEnd(FVector(0.f, 0.f, 2.15f), 1.75f, 1.9f, 0.9f);
		// The thumb laid forward along the fore-end's near side, as on the rifle (tuned in PIE).
		Hands.ForeThumbSwing = -10.f;
		Hands.ForeThumbSpread = -45.f;
		break;
	case EWeaponKind::LMG:
		// Thresher: the grip's middle at u 5.65, v -9.6 (as the Rustler's, a little longer), 3.2 wide; the trigger's face at
		// u 12.3, v -5.6; the handguard 3 tall (v -1.85 to -4.8), SOCKET_Foregrip its underside.
		Hands.GripShape = PistolGrip(FVector(-0.15f, 0.f, 0.f), FVector(0.3f, 0.f, 0.95f), 2.7f, 1.6f, 0.8f);
		Hands.TriggerAt = FVector(12.8f, 0.f, -5.6f);
		Hands.ForeShape = ForeEnd(FVector(0.f, 0.f, 1.48f), 1.48f, 2.0f, 0.6f);
		// The thumb laid forward along the handguard's near side, as on the rifle (tuned in PIE).
		Hands.ForeThumbSwing = -10.f;
		Hands.ForeThumbSpread = -45.f;
		break;
	case EWeaponKind::Launcher:
		// Powderkeg: the pistol grip's middle at u 48.5, v -10 (5.6 front to back, 3.2 wide); the trigger's face at u 54.8,
		// v -6.2; the upright front grip 3.5 front to back, 3 wide, on SOCKET_Foregrip.
		Hands.GripShape = PistolGrip(FVector(-0.1f, 0.f, 0.f), FVector(0.19f, 0.f, 0.98f), 2.8f, 1.6f, 0.8f);
		Hands.TriggerAt = FVector(55.4f, 0.f, -6.2f);
		Hands.ForeShape = PistolGrip(FVector::ZeroVector, FVector(0.02f, 0.f, 1.f), 1.75f, 1.5f, 0.8f);
		Hands.ForeThumbSwing = 35.f;
		break;
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		// Bullpup: the thumbhole loop's rear post on SOCKET_Grip (22.5, -10: back edge u 20.15, the hole's 24.9; 3.6 wide,
		// leaning back at the top); the trigger blade's face at u 27.3, v -6.6, inside the loop; the handguard 6.1 tall
		// (v -3.3 to 2.8 at u 44), 5 wide, rounded 1, its underside 1.2 above SOCKET_Foregrip (44, -4.5).
		Hands.GripShape = PistolGrip(FVector::ZeroVector, FVector(-0.21f, 0.f, 0.98f), 2.4f, 1.8f, 1.0f);
		Hands.TriggerAt = FVector(28.0f, 0.f, -6.6f);
		Hands.ForeShape = ForeEnd(FVector(0.f, 0.f, 4.25f), 3.05f, 2.5f, 1.0f);
		// The thumb laid forward along the handguard's top-left edge toward the muzzle (the user's "Modern tactical",
		// 2026-10-22: tuned in PIE; swung over the top it stood up like a peg); the trigger hand's round the post's back.
		Hands.ForeThumbSwing = -10.f;
		Hands.ForeThumbSpread = -45.f;
		Hands.GripThumbSpread = 20.f;
		break;
	}
}

void FirstPersonHandsGrips::TuneContact(EWeaponKind Kind, FGunHands& Hands)
{
#if !UE_BUILD_SHIPPING
	if (!TuningThisKind(Kind))
	{
		return;
	}
	if (Hands.GripShape.bValid)
	{
		GripShapeNudge.Apply(Hands.GripShape);
	}
	if (Hands.ForeShape.bValid)
	{
		ForeShapeNudge.Apply(Hands.ForeShape);
	}
	if (!Hands.TriggerAt.IsZero())
	{
		Hands.TriggerAt += FVector(CVarTriggerAtAhead.GetValueOnGameThread(), 0.f, CVarTriggerAtUp.GetValueOnGameThread());
	}
	Hands.GripThumbSwing += CVarTriggerThumbSwing.GetValueOnGameThread();
	Hands.GripThumbSpread += CVarTriggerThumbSpread.GetValueOnGameThread();
	Hands.ForeThumbSwing += CVarSupportThumbSwing.GetValueOnGameThread();
	Hands.ForeThumbSpread += CVarSupportThumbSpread.GetValueOnGameThread();
	Hands.IndexedSpread += CVarIndexedSpread.GetValueOnGameThread();
	Hands.IndexedCurl[0] += CVarIndexedKnuckle.GetValueOnGameThread();
#endif
}

FHandContact FirstPersonHandsGrips::ContactOnGun(const FHandsGun& Gun, const FVector& Socket, const FHandleSpec& Spec, float Share,
	const FVector& TriggerAt, float TriggerShare, const FVector& Slide)
{
	FHandContact Contact;
	if (!Spec.bValid)
	{
		return Contact;
	}
#if !UE_BUILD_SHIPPING
	const float ContactShare = FMath::Clamp(CVarContact.GetValueOnGameThread(), 0.f, 1.f);
	const float SeatShare = FMath::Clamp(CVarSeat.GetValueOnGameThread(), 0.f, 1.f);
	Contact.SeatGap = CVarSeatGap.GetValueOnGameThread();
	Contact.FingerRadius = FMath::Max(CVarFingerRadius.GetValueOnGameThread(), 0.1f);
#else
	const float ContactShare = DefaultContact;
	const float SeatShare = DefaultSeat;
	Contact.SeatGap = DefaultSeatGap;
	Contact.FingerRadius = DefaultFingerRadius;
#endif
	// The handle in the gun's space, drawn to scale (Socket already is), then in the camera's.
	FHandleShape Shape;
	Shape.bValid = true;
	Shape.Center = Gun.ToCameraPoint(Socket + (Spec.Offset + Slide) * Gun.Scale);
	Shape.Axis = Gun.ToCameraDir(Spec.Axis);
	Shape.Across = Gun.ToCameraDir(Spec.Across);
	Shape.HalfAcross = Spec.HalfAcross * Gun.Scale;
	Shape.HalfThrough = Spec.HalfThrough * Gun.Scale;
	Shape.Corner = Spec.Corner * Gun.Scale;
	Shape.HalfLength = Spec.HalfLength * Gun.Scale;
	Contact.Handle = Shape;
	Contact.Share = FMath::Clamp(Share, 0.f, 1.f) * ContactShare;
	Contact.Seat = SeatShare * (ContactShare > 0.f ? 1.f : 0.f);
	if (!TriggerAt.IsZero())
	{
		Contact.bTrigger = true;
		Contact.TriggerPoint = Gun.ToCameraPoint(TriggerAt * Gun.Scale);
		Contact.TriggerShare = FMath::Clamp(TriggerShare, 0.f, 1.f) * ContactShare;
	}
	return Contact;
}

bool FirstPersonHandsGrips::UseTriggerDiscipline()
{
#if !UE_BUILD_SHIPPING
	return CVarDiscipline.GetValueOnGameThread() != 0;
#else
	return true;
#endif
}

float FirstPersonHandsGrips::TriggerHoldSeconds()
{
#if !UE_BUILD_SHIPPING
	return FMath::Max(CVarTriggerHold.GetValueOnGameThread(), 0.f);
#else
	return DefaultTriggerHold;
#endif
}
