#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/WindowLifeRules.h"
#include "WindowLifeCard.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The card a lit window's moment plays on (UWindowLifeSubsystem spawns one per world and moves it to each window): the
 * engine's plane standing just in front of the pane's glass, inside the wall's depth, in M_WindowLife (one unlit
 * translucent material, Tools/Unreal/build_window_life_material.py: its shapes are drawn by the material, no texture). A
 * shadow is a soft dark head and shoulders, never a face; a curtain is linen pulled in from the pane's side. The wall
 * round the window hides whatever of the card lies past the pane, so the shadow walks out of the wall and back into it.
 *
 * Hidden and still between events; it ticks only while one plays (a test world, which never ticks, calls Advance).
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AWindowLifeCard : public AActor
{
	GENERATED_BODY()

public:
	AWindowLifeCard();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Card;

	/** Plays Event at Window (a lit window's SOCKET_Window_<n> in the world: X out through the glass, Z up). False when it can't show. */
	bool Play(const FWindowLifeEvent& Event, const FTransform& Window);

	/** Moves the event on by DeltaSeconds (the tick does); hides the card once it's over. */
	void Advance(float DeltaSeconds);

	/** Hides it at once. */
	void Stop();

	bool IsPlaying() const { return bPlaying; }

	/**
	 * Finds its plane and material (both made outside the code: the engine's plane, M_WindowLife) and gives the card its
	 * instance, the first time; one not made yet is looked for again ten seconds on. False while it can't show.
	 */
	bool Prepare();

	/** Whether it can show (Prepare found what it needs). */
	bool CanShow() const { return Look != nullptr; }

	const FWindowLifeEvent& GetEvent() const { return Event; }
	float GetAge() const { return Age; }

	/** What the card shows now (as its material was last given). */
	const FWindowLifePose& GetPose() const { return Pose; }

	/** The card's size (cm): wider and taller than a pane, the wall hides the rest; and how far it stands off the glass. */
	static constexpr float Width = 130.f;
	static constexpr float Height = 170.f;
	static constexpr float OffGlass = 0.5f;

	/** The material's path, and the names of its parameters the card sets. */
	static const TCHAR* MaterialPath;
	static const FName KindParameter;
	static const FName SlideParameter;
	static const FName RiseParameter;
	static const FName SwingParameter;
	static const FName SideParameter;
	static const FName StrengthParameter;
	static const FName ColorParameter;
	static const FName WidthParameter;
	static const FName HeightParameter;

private:
	void ApplyPose();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Look;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CardMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> CardMaterial;

	FWindowLifeEvent Event;
	FWindowLifePose Pose;
	float Age = 0.f;
	bool bPlaying = false;
	/** When Prepare last looked for a material it didn't find (real seconds), or below 0; and whether it said so once. */
	double LastTry = -1.0;
	bool bWarned = false;
};
