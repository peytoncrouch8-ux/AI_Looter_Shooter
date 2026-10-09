#pragma once

#include "CoreMinimal.h"
#include "FaunaTypes.generated.h"

/** What a bird stands on: how it faces there and whether it may walk about. */
UENUM(BlueprintType)
enum class EFaunaPerchKind : uint8
{
	/** A fence rail, a wall's top, a headboard: the bird faces across it. */
	Rail,
	/** A post's or a cairn's top: any way it likes. */
	Post,
	/** A roof's ridge or a false front's top. */
	Roof,
	/** A dead tree's limb. */
	Branch,
	/** Open ground: it walks about and pecks. */
	Ground,
};

/** One place a bird of a flock can be: where its feet grip (world, cm) and which way it faces (degrees). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FFaunaPerch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	float Yaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	EFaunaPerchKind Kind = EFaunaPerchKind::Rail;

	/** The ground's up there (Ground only), so a bird walking about it stays on the slope. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FVector Normal = FVector::UpVector;

	/** How far (cm) a bird on the ground may walk from it: open ground the build script found clear and gentle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (ClampMin = "0"))
	float Wander = 0.f;
};

/** A place insects keep to: round Center (world; the ground or the water's surface), between two heights over it. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FFaunaZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (Units = "cm"))
	float MinHeight = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (Units = "cm"))
	float MaxHeight = 180.f;

	/** How many it holds while the player is near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (ClampMin = "0"))
	int32 Count = 4;
};

/** A stretch of open ground a tumbleweed rolls down, upwind end first (world, cm on the ground). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FFaunaLane
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FVector End = FVector::ZeroVector;
};

/** A piece of washing hung on a line: which of the cloth meshes, where it's pegged (its top middle) and the line's yaw. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FFaunaClothPiece
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (ClampMin = "0"))
	int32 Mesh = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FVector Location = FVector::ZeroVector;

	/** The cloth's facing: its front (+X) across the line, the line running along its Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	float Yaw = 0.f;
};

/** What made a noise the animals hear. */
enum class EFaunaNoise : uint8
{
	/** A gun fired: heard far off. */
	Gunshot,
	/** A bullet striking near them. */
	Impact,
	/** Anything else loud (a console scare, an explosion one day). */
	Other,
};

/** A noise in the world: where, how far it carries (cm) and when (world seconds). */
struct FFaunaNoise
{
	FVector Location = FVector::ZeroVector;
	float Radius = 0.f;
	double Time = 0.0;
	EFaunaNoise Kind = EFaunaNoise::Other;
};

/** Something animals keep away from: a player (by how boldly they move) or a hostile creature. */
struct FFaunaThreat
{
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	/** Scales the flee radius: a creeping player gets closer than a running one (FaunaRules::FearOf). */
	float Fear = 1.f;
	bool bPlayer = true;
};

/** The view fauna is drawn for: the local player's camera. */
struct FFaunaView
{
	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float FieldOfView = 90.f;
	bool bValid = false;
};

/** What every fauna actor reads in an update, gathered once a frame by UFaunaSubsystem (or made up by a test). */
struct FFaunaContext
{
	/** World seconds. */
	double Now = 0.0;
	FFaunaView View;
	TArray<FFaunaThreat> Threats;
	/** The last few seconds' noises (each actor takes those since its own last update). */
	TArray<FFaunaNoise> Noises;
	/** The level's lighting state (ULightingStateSubsystem; None in a level without states). */
	FName Lighting;
};

/** One actor's update: the time since its last, and how the view sees it now. */
struct FFaunaTick
{
	float DeltaSeconds = 0.f;
	/** From the view to the nearest of what it draws (cm). */
	float Distance = 0.f;
	bool bOnScreen = true;
	/** World seconds of its last update: noises after it are new to it. */
	double Since = 0.0;
};
