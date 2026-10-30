#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Elements/ElementCallout.h"
#include "DamageNumberActor.generated.h"

class UWidgetComponent;

/** What a damage number actor is showing. */
enum class EDamageNumberKind : uint8
{
	/** One hit's number. */
	Hit,
	/** An element status's pieces on one creature, rolled into one number that counts up while they come (DamageNumberStream). */
	Stream,
	/** A status's word the first time it is set on a creature ("BURNING"), in its colour with its glyph. */
	Callout
};

/**
 * Short-lived actor that floats a damage number up from the hit point and fades it out. A normal hit's number ticks in
 * with a small pop; a critical hit's is thrown: it slams in at twice its size, white-hot, settles with a little bounce
 * into the crit colour and arcs off to one side (a lob on the screen: up, over and a little back down), tilted the way
 * it flies, and stays a little longer (DamageNumberMotion has its numbers).
 *
 * A status's pieces (a burn's, the salt's, the rot's: UElementStatusComponent::IsDealingNow) don't each get a number: the
 * first starts a stream over the creature (its owner, UHealthComponent::SpawnDamageNumber sets it) and each after it joins
 * that one (the new actor goes at once), which rolls its total up, pulses a little at each piece, follows the creature and
 * fades once the pieces stop (DamageNumberStream). One stream a creature an element; two at once stack up the screen.
 * The first time a status is set on a creature its word pops up over it (ShowCallout; UStatusCalloutSubsystem).
 * DamageNumberActorStream.cpp has the stream and the callout.
 */
UCLASS(Blueprintable)
class AI_LOOTER_SHOOTER_API ADamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	ADamageNumberActor();

	/**
	 * Look: how it shows the element it carried (ElementCallout::LookFor): its colour, a WEAK or RESIST tag, a bigger number.
	 * A status's piece on a creature (the owner) goes into its stream instead: it may destroy this actor at once.
	 */
	void Show(float Damage, bool bCritical, const FElementNumberLook& Look = FElementNumberLook());

	/** A status's word over a creature the first time it's set (ElementRules::StatusWord); Slot lifts it over another's. */
	void ShowCallout(ELooterElement Element, int32 Slot = 0);

	/** The live stream of Element's pieces over Creature, if one is showing. */
	static ADamageNumberActor* FindStream(const UWorld* World, const AActor* Creature, ELooterElement Element);

	/** How many streams are showing over Creature (where a new one stacks). */
	static int32 CountStreams(const UWorld* World, const AActor* Creature);

	EDamageNumberKind GetKind() const { return Kind; }
	/** A stream's element, its total so far and the number shown (rolling up to it). */
	ELooterElement GetStreamElement() const { return StreamElement; }
	float GetStreamTotal() const { return StreamTotal; }
	float GetStreamShown() const { return StreamShown; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float Lifetime = 0.9f;

	/** cm/s upward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float RiseSpeed = 120.f;

	/** Random sideways drift so rapid hits don't stack on top of each other. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float MaxDriftSpeed = 60.f;

	/** A critical hit's number stays this much longer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float CriticalExtraLife = 0.25f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	// DamageNumberActorStream.cpp
	/** Starts this actor as Creature's stream of Element's pieces, its first piece Damage. */
	void StartStream(float Damage, const FElementNumberLook& Look, AActor* Creature, ELooterElement Element);
	/** One more piece into this stream: its total grows, it pulses, its fade clock starts over. */
	void AddToStream(float Damage, const FElementNumberLook& Look);
	void TickStream(float DeltaSeconds);
	void TickCallout(float DeltaSeconds);

	EDamageNumberKind Kind = EDamageNumberKind::Hit;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	bool bCritical = false;
	/** A WEAK or RESIST tag is beside the number (it pops in over its first moments). */
	bool bTagged = false;
	/** Seconds since the tag showed (a stream's may show at a later piece). */
	float TagAge = 0.f;
	/** A crit's lob goes this way across the screen (-1 left, 1 right). */
	float Side = 1.f;

	/** A stream's creature, element, where it sits from the creature, its place up the screen among the creature's streams. */
	TWeakObjectPtr<AActor> StreamCreature;
	ELooterElement StreamElement = ELooterElement::None;
	FVector StreamAnchor = FVector::ZeroVector;
	int32 Slot = 0;
	/** Its total so far, the number shown rolling up to it, seconds since its last piece and since its last pulse. */
	float StreamTotal = 0.f;
	float StreamShown = 0.f;
	float SincePiece = 0.f;
	/** The whole number on show (the text changes only when it does). */
	int32 StreamShownWhole = -1;
};

/** A status's stream of pieces as plain math (the actor plays it; Looter.UI.StatusIcons checks it). */
namespace DamageNumberStream
{
	/**
	 * The element whose stream a number joins: a status's piece of damage over time (being dealt as a piece now, not a crit),
	 * None for anything else (a hit, a smite's or a jolt's burst: those keep numbers of their own).
	 */
	AI_LOOTER_SHOOTER_API ELooterElement StreamElementOf(bool bCritical, bool bDealingPiece, ELooterElement Element);

	/** The shown number DeltaSeconds on: rolling up toward Total (quickly when far behind, at least RollMinRate a second), never past it. */
	AI_LOOTER_SHOOTER_API float RollToward(float Shown, float Total, float DeltaSeconds);

	/** Its opacity SincePiece seconds after its last piece: whole for HoldSeconds, then fading out over FadeSeconds. */
	AI_LOOTER_SHOOTER_API float OpacityAt(float SincePiece);

	/** Its scale SincePiece seconds after a piece: a small pulse (PulseScale) settling by PulseSeconds. */
	AI_LOOTER_SHOOTER_API float PulseAt(float SincePiece);

	/** How far it has risen over the creature Age seconds in (cm): eased out to RiseHeight. */
	AI_LOOTER_SHOOTER_API float RiseAt(float Age);

	/** Longer than the statuses' half-second tick (Looter.Element.TickSeconds), so it holds whole between pieces. */
	inline constexpr float HoldSeconds = 0.8f;
	inline constexpr float FadeSeconds = 0.45f;
	inline constexpr float PulseScale = 1.18f;
	inline constexpr float PulseSeconds = 0.14f;
	inline constexpr float RiseHeight = 40.f;
	/** The roll: at least this many a second, and this share of the gap a second when further behind. */
	inline constexpr float RollMinRate = 40.f;
	inline constexpr float RollCatchUp = 10.f;
	/** Another stream on the same creature sits this far up the screen (px), up to MaxSlots. */
	inline constexpr float SlotStep = 24.f;
	inline constexpr int32 MaxSlots = 4;

	/** Gone: past its hold and its fade. */
	inline bool IsSpent(float SincePiece) { return SincePiece >= HoldSeconds + FadeSeconds; }
}

/** A status's first-time word over a creature, as plain math. */
namespace DamageNumberCallout
{
	/** Its scale Age seconds in: it slams in at PopScale and settles by PopSeconds. */
	AI_LOOTER_SHOOTER_API float ScaleAt(float Age);

	/** Its opacity Age seconds in: whole, then fading out over its last FadeShare of Seconds. */
	AI_LOOTER_SHOOTER_API float OpacityAt(float Age);

	inline constexpr float Seconds = 1.3f;
	inline constexpr float FadeShare = 0.35f;
	inline constexpr float PopScale = 1.7f;
	inline constexpr float PopSeconds = 0.16f;
	/** It rises this fast (cm/s), slower than a hit's number, and sits this far up the screen (px), each slot a step more. */
	inline constexpr float RiseSpeed = 55.f;
	inline constexpr float Lift = 18.f;
	inline constexpr float SlotStep = 22.f;
}

/** A damage number's motion on the screen over its life, as plain math (the actor plays it; the tests check it). */
namespace DamageNumberMotion
{
	struct FFrame
	{
		/** On-screen offset (px, y down), scale, tilt (degrees) and how white-hot it is (0-1). */
		FVector2D Offset = FVector2D::ZeroVector;
		float Scale = 1.f;
		float Angle = 0.f;
		float Heat = 0.f;
	};

	/** Its frame Age seconds in; Side: a crit's lob's way (-1 or 1). */
	AI_LOOTER_SHOOTER_API FFrame At(float Age, bool bCritical, float Side);

	/** The WEAK / RESIST tag's size Age seconds in: it pops in big and settles to 1 (TagPopScale, over TagPopSeconds). */
	AI_LOOTER_SHOOTER_API float TagScaleAt(float Age);

	inline constexpr float TagPopScale = 1.9f;
	inline constexpr float TagPopSeconds = 0.14f;

	/** A crit slams in at this size, and its lob rises this fast against this pull (px/s, px/s^2) while drifting this far sideways (px). */
	inline constexpr float CritSlamScale = 2.2f;
	inline constexpr float CritLobSpeed = 220.f;
	inline constexpr float CritLobPull = 420.f;
	inline constexpr float CritLobSide = 36.f;
}
