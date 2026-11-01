#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPath.h"

class AActor;
class UBodyFigureComponent;
class USkeletalMeshComponent;

/**
 * How a story person looks until their own model comes (the user's Tripo people, TRIPO_MODELS.md): a hero's body worn as a
 * stand-in, tinted toward who they are; and the one path their model will be at. A model at Model (rigged to the
 * mannequin's skeleton, A-pose, real size) is worn as the animated body itself, and the stand-in goes.
 */
struct FWardenLook
{
	/** Their own model (a skeletal mesh on the mannequin's skeleton); nothing there yet: the stand-in. */
	FSoftObjectPath Model;
	/** The size their own model is worn at: real heights, in the player's 0.85 world. */
	float ModelScale = 0.85f;
	/** The hero whose body stands in (UHeroDefinition id), and the size it's worn at to stand their height. */
	FName StandInHero = TEXT("Odessa");
	float StandInScale = 0.85f;
	/** The stand-in's colours, multiplied over its own: clothes and gear, skin, hair. Glowing parts (eyes) dimmed to Glow. */
	FLinearColor Cloth = FLinearColor::White;
	FLinearColor Skin = FLinearColor::White;
	FLinearColor Hair = FLinearColor::White;
	float Glow = 0.15f;
};

/** How much of a story person's body is worked out each frame, by how far the player is (FollowPlayer). */
enum class EWardenDetail : uint8
{
	/** Near: every frame. */
	Full,
	/** A way off: a few times a second (the pose still reads at that distance). */
	Reduced,
	/** Far off: held still (nobody can tell), and no shadow. */
	Held,
};

/** The shared body of the story people (AWardenCrewMember, ADeliaRansom): the mannequin, the stand-in, the detail by distance. */
namespace WardenCrewBody
{
	/** The player within this (cm): full detail; within the next, reduced; beyond, held. */
	inline constexpr float FullDetailRange = 3000.f;
	inline constexpr float ReducedDetailRange = 8000.f;
	/** Reduced detail's update rate (s between). */
	inline constexpr float ReducedInterval = 1.f / 15.f;
	/** The stand-in isn't drawn past this (cm): a person-sized thing that far is a few pixels. */
	inline constexpr float DrawDistance = 15000.f;

	/** The mannequin and the skeleton every story person's pose is on (a model must share it to be worn). */
	extern const TCHAR* const MannequinPath;

	/**
	 * In a constructor: Body becomes the hidden, animated mannequin (WardenCrewAnimInstance; facing the actor's +X, its feet
	 * at Body's origin), and the stand-in figure (returned) is made on it.
	 */
	UBodyFigureComponent* MakeBody(AActor& Owner, USkeletalMeshComponent& Body);

	/**
	 * Before play begins (the actor's BeginPlay, before its components'): wears Look's own model on Body when it's in the
	 * game and on the mannequin's skeleton (true; the figure then goes, and Figure is nulled), else sets the figure to stand
	 * in as Look's hero. Either way Body is sized for it.
	 */
	bool WearLook(USkeletalMeshComponent& Body, TObjectPtr<UBodyFigureComponent>& Figure, const FWardenLook& Look);

	/** Once the figure has put on its hero (after the components' BeginPlay): tinted, its holstered gun put away. */
	void DressStandIn(UBodyFigureComponent* Figure, const FWardenLook& Look);

	/** The detail for the player Distance away (cm; negative: no player, full). */
	EWardenDetail DetailFor(float Distance);

	/** Puts Body and Figure at Detail (their tick rate and shadow); a no-op when it's the one they're at. */
	void ApplyDetail(USkeletalMeshComponent& Body, UBodyFigureComponent* Figure, EWardenDetail Detail, EWardenDetail& Current);

	/** How far the local player's pawn is from Where (cm), or -1 with none. */
	float PlayerDistance(const AActor& Context, const FVector& Where);

	/** The local player's eyes (their view point), false with no player pawn. */
	bool PlayerEyes(const AActor& Context, FVector& OutEyes);

	/** Someone's eyes: a player's view point, anyone else's head height (their spot raised 60 cm). */
	FVector EyesOf(const AActor& Someone);

	/** How far Where is to From's right of the way From faces, flat (degrees, -180 to 180). */
	float YawTo(const AActor& From, const FVector& Where);
}
