// ARespawnMarker's roost: the model (or its greybox stand-in) placed by its Wake socket, the beacon and its light on its top,
// its sockets, the light's flare as it's found, its sound near the player, and the crows' welcome when they bring someone.

#include "World/RespawnMarker.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "World/RoostBeacon.h"
#include "World/RoostCarrySubsystem.h"
#include "World/RoostCrows.h"
#include "World/RoostKit.h"
#include "World/RoostSettings.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A found roost's light spikes to this many times its rest as it's found, to the second as it's made the save point again. */
	constexpr float FoundPeak = 5.f;
	constexpr float AgainPeak = 2.2f;

	/** The roost's sound fades out over this long as the player walks away (seconds). */
	constexpr float AmbientFadeSeconds = 1.2f;

	/** The bell's swing on its loop, as the vigil stone's: how far at a full ring (degrees), how fast, how soon it settles (s). */
	constexpr float BellSwingDegrees = 24.f;
	constexpr float BellSwingHz = 1.2f;
	constexpr float BellSettleSeconds = 1.3f;

	/** How hard the bell is rung: a find, a homecoming after a death, a fast travel's landing, a mark again (0 to 1). */
	constexpr float RingFound = 1.f;
	constexpr float RingDeath = 0.7f;
	constexpr float RingTravel = 0.5f;
	constexpr float RingAgain = 0.3f;

	/** A socket of the roost's model, in the model's own frame; false when the model has none so named. */
	bool ModelSocket(const UStaticMesh* Model, FName Name, FTransform& Out)
	{
		const UStaticMeshSocket* Socket = Model ? Model->FindSocket(Name) : nullptr;
		if (!Socket)
		{
			return false;
		}
		Out = FTransform(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale);
		return true;
	}

	/** The stand-in's own socket by name (RoostKit's), in the roost's frame; false for a name it hasn't. */
	bool StandInSocket(FName Name, FTransform& Out)
	{
		if (Name == RoostKit::BeaconSocket())
		{
			Out = FTransform(RoostKit::FallbackBeacon());
			return true;
		}
		if (Name == RoostKit::BellSocket())
		{
			Out = FTransform(RoostKit::FallbackBell());
			return true;
		}
		if (Name == RoostKit::SpeakerSocket())
		{
			Out = FTransform(RoostKit::FallbackSpeaker());
			return true;
		}
		if (Name == RoostKit::WakeSocket())
		{
			Out = RoostKit::FallbackWake();
			return true;
		}
		for (int32 Index = 0; Index < RoostKit::PerchCount; ++Index)
		{
			if (Name == RoostKit::PerchSocket(Index))
			{
				Out = RoostKit::FallbackPerch(Index);
				return true;
			}
		}
		return false;
	}
}

void ARespawnMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FitRoost();
	ApplyLook();
}

void ARespawnMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceLook(DeltaSeconds);
}

void ARespawnMarker::UpdateTicking()
{
	// Only for the second of its light's flare and the few seconds of its bell's swing; the beacon ticks its own show.
	SetActorTickEnabled(FlareAge >= 0.f || BellStrength > 0.f);
}

// ---------------------------------------------------------------------------
// Where its pieces are
// ---------------------------------------------------------------------------

void ARespawnMarker::FitRoost()
{
	if (Roost->GetStaticMesh() != RoostMesh)
	{
		Roost->SetStaticMesh(RoostMesh);
	}
	// Where the player wakes, in the roost's own frame: its model's Wake socket, or the stand-in's. Only its place on the
	// ground and its turn count: the player stands level.
	FTransform Wake = RoostKit::FallbackWake();
	FTransform FromModel;
	if (ModelSocket(RoostMesh, RoostKit::WakeSocket(), FromModel))
	{
		Wake = FTransform(FRotator(0.0, FromModel.Rotator().Yaw, 0.0), FVector(FromModel.GetLocation().X, FromModel.GetLocation().Y, 0.0));
	}
	// The roost stands so that spot is this actor's: the player waking here stands on it facing the arrow, out over the land
	// with the mast at their back; lifted or sunk to the ground at the mast's foot (RoostLift, measured by the build script).
	FTransform Placement = Wake.Inverse();
	Placement.AddToTranslation(FVector(0.0, 0.0, RoostLift));
	Roost->SetRelativeTransform(Placement);
	BuildGreybox();

	// The bell on its socket, hung by the top of its loop; the stand-in's box hangs its height under the socket.
	if (Bell->GetStaticMesh() != BellMesh && BellMesh)
	{
		Bell->SetStaticMesh(BellMesh);
	}
	FTransform Hook;
	if (!ModelSocket(RoostMesh, RoostKit::BellSocket(), Hook))
	{
		StandInSocket(RoostKit::BellSocket(), Hook);
	}
	if (BellMesh)
	{
		Bell->SetRelativeTransform(FTransform(Hook.GetRotation(), Hook.GetLocation()));
	}
	else
	{
		const FVector Box = RoostKit::StandInBellSize();
		Bell->SetRelativeTransform(FTransform(Hook.GetRotation(), Hook.GetLocation() - FVector(0.0, 0.0, Box.Z * 0.5), Box / 100.0));
	}
	BellRest = Bell->GetRelativeRotation().Quaternion();
	BellStrength = 0.f;

	// The beacon and its light in the fire basket.
	FTransform Top;
	if (!ModelSocket(RoostMesh, RoostKit::BeaconSocket(), Top))
	{
		StandInSocket(RoostKit::BeaconSocket(), Top);
	}
	Beacon->SetRelativeLocationAndRotation(Top.GetLocation(), FRotator::ZeroRotator);
	BeaconLight->SetRelativeLocation(Top.GetLocation());
	Beacon->BuildCards();
	const URoostSettings& Settings = URoostSettings::Get();
	// The fire's own light on the mast and the ground round it: its hot core's colour.
	BeaconLight->SetLightColor(Settings.GetFlameCoreColor());
	BeaconLight->SetAttenuationRadius(Settings.LightRadius);
}

void ARespawnMarker::BuildGreybox()
{
	// The stand-in until the art's roost is imported: boxes of the engine's cube in their materials, the roost's own shape.
	const bool bStandIn = RoostMesh == nullptr;
	const TPair<UInstancedStaticMeshComponent*, TArray<RoostKit::FGreyboxPart>> Sets[] = {
		{ GreyboxWood, RoostKit::GreyboxWood() }, { GreyboxIron, RoostKit::GreyboxIron() }, { GreyboxStone, RoostKit::GreyboxStone() } };
	for (const TPair<UInstancedStaticMeshComponent*, TArray<RoostKit::FGreyboxPart>>& Set : Sets)
	{
		UInstancedStaticMeshComponent* Pieces = Set.Key;
		if (!Pieces)
		{
			continue;
		}
		Pieces->ClearInstances();
		Pieces->SetVisibility(bStandIn);
		Pieces->SetCollisionEnabled(bStandIn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (!bStandIn || !Pieces->GetStaticMesh())
		{
			continue;
		}
		for (const RoostKit::FGreyboxPart& Part : Set.Value)
		{
			// The engine's cube is 100 cm a side about its middle.
			Pieces->AddInstance(FTransform(FRotator::ZeroRotator, Part.Center, Part.Size / 100.0));
		}
	}
}

bool ARespawnMarker::GetRoostSocket(FName Socket, FTransform& OutWorld) const
{
	FTransform Local;
	if (Socket.IsNone())
	{
		// No name: the post's foot, the roost's own pivot.
		Local = FTransform::Identity;
	}
	else if (!ModelSocket(RoostMesh, Socket, Local) && !StandInSocket(Socket, Local))
	{
		return false;
	}
	OutWorld = Local * Roost->GetComponentTransform();
	return true;
}

FVector ARespawnMarker::GetBeaconLocation() const
{
	return Beacon->GetComponentLocation();
}

TArray<FTransform> ARespawnMarker::GetPerches() const
{
	TArray<FTransform> Perches;
	for (int32 Index = 0; Index < RoostKit::PerchCount; ++Index)
	{
		FTransform Perch;
		if (GetRoostSocket(RoostKit::PerchSocket(Index), Perch))
		{
			Perches.Add(Perch);
		}
	}
	return Perches;
}

ARoostCrows* ARespawnMarker::GetCrows() const
{
	return Crows.Get();
}

ARoostCrows* ARespawnMarker::GetCirclingCrows() const
{
	return CirclingCrows.Get();
}

void ARespawnMarker::UpdateCirclingCrows()
{
	ARoostCrows* Circling = CirclingCrows.Get();
	const int32 Count = URoostSettings::Get().CirclingCrows;
	UWorld* World = GetWorld();
	const bool bWanted = bLit && bInPlay && Count > 0 && World;
	if (!bWanted)
	{
		if (Circling)
		{
			Circling->Destroy();
		}
		CirclingCrows.Reset();
		return;
	}
	if (Circling)
	{
		return;
	}
	// A few of Hob's crows riding the fire's heat over the basket, as long as it burns: with the column, a roost's form far off.
	const FTransform At(FRotator::ZeroRotator, GetBeaconLocation());
	Circling = World->SpawnActorDeferred<ARoostCrows>(ARoostCrows::StaticClass(), At, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Circling)
	{
		return;
	}
	Circling->MakeCircling(GetBeaconLocation(), Count);
	Circling->Seed = static_cast<int32>(GetTypeHash(GetMarkerId()) * 31u) | 1;
	Circling->FinishSpawning(At);
	CirclingCrows = Circling;
}

// ---------------------------------------------------------------------------
// The bell
// ---------------------------------------------------------------------------

void ARespawnMarker::RingBell(float Strength)
{
	// Rung again while it swings, it swings on from the harder of the two.
	BellStrength = FMath::Clamp(FMath::Max(Strength, BellStrength * FMath::Exp(-BellTime / BellSettleSeconds)), 0.f, 1.f);
	BellTime = 0.f;
	UpdateTicking();
}

void ARespawnMarker::SwingBell(float DeltaSeconds)
{
	if (BellStrength <= 0.f)
	{
		return;
	}
	BellTime += DeltaSeconds;
	const float Fade = FMath::Exp(-BellTime / BellSettleSeconds);
	if (Fade < 0.02f)
	{
		BellStrength = 0.f;
		Bell->SetRelativeRotation(BellRest);
		return;
	}
	// Side to side on its loop (about the socket's +X, as the vigil stone's bell), dying away.
	Bell->SetRelativeRotation(BellRest * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(GetBellSwing())));
}

float ARespawnMarker::GetBellSwing() const
{
	if (BellStrength <= 0.f)
	{
		return 0.f;
	}
	return BellSwingDegrees * BellStrength * FMath::Exp(-BellTime / BellSettleSeconds) * FMath::Sin(BellTime * UE_TWO_PI * BellSwingHz);
}

// ---------------------------------------------------------------------------
// Dark, found and the light's flare
// ---------------------------------------------------------------------------

void ARespawnMarker::ApplyLook()
{
	Beacon->SetLit(bLit);
	ApplyLightFlare();
	UpdateCirclingCrows();
}

void ARespawnMarker::ApplyLightFlare()
{
	BeaconLight->SetVisibility(bLit);
	const URoostSettings& Settings = URoostSettings::Get();
	const bool bFound = FlareKind == EGraveMark::First;
	const float Pulse = GraveMarkRules::FlarePulse(FlareAge, FlareKind);
	// A find's light catches from nothing in the rise; every other moment, at rest or lifting, the light is already on.
	const float Catch = bFound && FlareAge >= 0.f ? FMath::Clamp(FlareAge / GraveMarkRules::FlareRiseSeconds, 0.f, 1.f) : 1.f;
	const float Peak = bFound ? FoundPeak : AgainPeak;
	BeaconLight->SetIntensity(bLit ? Settings.LightCandela * Catch * (1.f + (Peak - 1.f) * Pulse) : 0.f);
}

void ARespawnMarker::SetLit(bool bInLit)
{
	bLit = bInLit;
	FlareAge = -1.f;
	ApplyLook();
	UpdateTicking();
}

void ARespawnMarker::StartFlare(EGraveMark Kind)
{
	if (Kind == EGraveMark::None)
	{
		return;
	}
	bLit = true;
	FlareKind = Kind;
	FlareLength = GraveMarkRules::FlareSeconds(Kind);
	FlareAge = 0.f;
	// Found: the column shoots up out of the basket and the bell rings out; made the save point again: a pulse climbs it and
	// the bell is tapped.
	if (Kind == EGraveMark::First || !Beacon->IsLit())
	{
		Beacon->Ignite();
	}
	else
	{
		Beacon->Pulse();
	}
	RingBell(Kind == EGraveMark::First ? RingFound : RingAgain);
	ApplyLightFlare();
	UpdateCirclingCrows();
	// Only for the second the light's flare lasts and the bell's swing (the beacon ticks its own show).
	UpdateTicking();
}

void ARespawnMarker::AdvanceLook(float DeltaSeconds)
{
	if (FlareAge >= 0.f)
	{
		FlareAge += DeltaSeconds;
		if (FlareAge >= FlareLength)
		{
			FlareAge = -1.f;
		}
		ApplyLightFlare();
	}
	SwingBell(DeltaSeconds);
	UpdateTicking();
}

float ARespawnMarker::GetLightCandela() const
{
	return bLit && BeaconLight ? BeaconLight->Intensity : 0.f;
}

// ---------------------------------------------------------------------------
// Its life: the sound near it, the crows' welcome
// ---------------------------------------------------------------------------

void ARespawnMarker::UpdateAmbient(bool bNear)
{
	UAudioComponent* Playing = AmbientSound.Get();
	if (bNear && !Playing)
	{
		// From the bell, where the crows sit and the clapper ticks (the post's foot on the stand-in).
		const FName At = RoostMesh && Roost->DoesSocketExist(RoostKit::BellSocket()) ? RoostKit::BellSocket() : NAME_None;
		AmbientSound = LooterSound::Start(this, LooterSoundCue::RoostMark::Ambient, Roost, At);
	}
	else if (!bNear && Playing)
	{
		LooterSound::Stop(Playing, AmbientFadeSeconds);
		AmbientSound.Reset();
	}
}

bool ARespawnMarker::IsAmbientPlaying() const
{
	return AmbientSound.IsValid();
}

void ARespawnMarker::Welcome(ERoostArrival Arrival, APawn* Player) const
{
	// The crows that carried them burst off the roost round them, as the screen's crows burst away (URoostCarrySubsystem).
	if (ARoostCrows* Birds = Crows.Get())
	{
		Birds->Scatter(Player ? Player->GetActorLocation() : GetActorLocation());
	}
	LooterSound::PlayAt(this, Arrival == ERoostArrival::Travel ? LooterSoundCue::RoostMark::CarryIn : LooterSoundCue::RoostMark::DeathWake,
		GetActorLocation());
	if (bLit)
	{
		Beacon->Pulse();
	}
	// The bell jostled by the crows bursting off the mast (a homecoming from death harder than a ride's landing). The bell's
	// swing is the roost's look, not its state: rung from here as the vigil stone rings its own.
	const_cast<ARespawnMarker*>(this)->RingBell(Arrival == ERoostArrival::Death ? RingDeath : RingTravel);
	if (URoostCarrySubsystem* Carry = URoostCarrySubsystem::Get(this))
	{
		Carry->SayOnArrival(Arrival);
	}
	UE_LOG(LogLooter, Log, TEXT("Roost %s: the crows set the player down (%s)."), *GetMarkerId().ToString(),
		Arrival == ERoostArrival::Travel ? TEXT("fast travel") : TEXT("a death"));
}
