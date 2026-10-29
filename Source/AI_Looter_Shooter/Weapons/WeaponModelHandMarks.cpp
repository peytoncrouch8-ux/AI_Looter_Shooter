#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponHandMarks.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

// The hands' grip spots on the gun's parts (2026-10-29, grip spots: Weapons/WeaponHandMarks.h): each part's sockets read as
// markers, in the gun's own space, as the gun is assembled now (a part swapped at the bench brings its own).

namespace
{
	/** The name as Blender's empty has it (SOCKET_Hold_L_Cup) or as the export leaves it on the mesh (Hold_L_Cup): the latter. */
	FString WithoutPrefix(FName Socket)
	{
		static const FString Prefix(TEXT("SOCKET_"));
		const FString Name = Socket.ToString();
		return Name.StartsWith(Prefix, ESearchCase::IgnoreCase) ? Name.RightChop(Prefix.Len()) : Name;
	}

	/** One word of letters and digits, starting with a letter (a pose's name). */
	bool IsPoseWord(const FString& Word)
	{
		if (Word.IsEmpty() || !FChar::IsAlpha(Word[0]))
		{
			return false;
		}
		for (const TCHAR Char : Word)
		{
			if (!FChar::IsAlnum(Char))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * A socket on Part in Gun's space (Gun's own scale divided out, as UWeaponModelComponent::FindSocket's points are), its
	 * turn kept, no scale. bAtRest: where it is with Part back at its rest place on its parent (the reload part's travel, a
	 * cylinder's swing and turn, taken out), so the hands' own travel isn't put on twice.
	 */
	FTransform SocketInGun(const USceneComponent& Gun, const UStaticMeshComponent& Part, const UStaticMeshSocket& Socket, bool bAtRest, bool bCylinder)
	{
		const FTransform OnPart(Socket.RelativeRotation, Socket.RelativeLocation, Socket.RelativeScale);
		FTransform World = OnPart * Part.GetComponentTransform();
		if (bAtRest)
		{
			if (const USceneComponent* Parent = Part.GetAttachParent())
			{
				// A magazine or pump rests on its socket (SetReloadTravel only moves it off along its way); a cylinder rests
				// shut and unturned (its swing and turn are its whole relative turn).
				const FQuat RestTurn = bCylinder ? FQuat::Identity : Part.GetRelativeRotation().Quaternion();
				const FTransform Rest(RestTurn, FVector::ZeroVector, Part.GetRelativeScale3D());
				World = OnPart * Rest * Parent->GetSocketTransform(Part.GetAttachSocketName(), RTS_World);
			}
		}
		const FTransform& Space = Gun.GetComponentTransform();
		return FTransform((Space.GetRotation().Inverse() * World.GetRotation()).GetNormalized(), Space.InverseTransformPosition(World.GetLocation()));
	}
}

bool WeaponHandMarks::ParseSocket(FName Socket, EHandMarkRole& OutRole, int32& OutSide, FName& OutPose)
{
	TArray<FString> Words;
	// Empty words are kept: "Hold__Cup" has no side and "Hold_L_" no pose, so neither is a marker.
	WithoutPrefix(Socket).ParseIntoArray(Words, TEXT("_"), false);
	if (Words.Num() != 3)
	{
		return false;
	}
	EHandMarkRole Role;
	if (Words[0].Equals(TEXT("Hold"), ESearchCase::IgnoreCase))
	{
		Role = EHandMarkRole::Hold;
	}
	else if (Words[0].Equals(TEXT("Rack"), ESearchCase::IgnoreCase))
	{
		Role = EHandMarkRole::Rack;
	}
	else if (Words[0].Equals(TEXT("Reload"), ESearchCase::IgnoreCase))
	{
		Role = EHandMarkRole::Reload;
	}
	else
	{
		return false;
	}
	int32 Side;
	if (Words[1].Equals(TEXT("L"), ESearchCase::IgnoreCase))
	{
		Side = 0;
	}
	else if (Words[1].Equals(TEXT("R"), ESearchCase::IgnoreCase))
	{
		Side = 1;
	}
	else
	{
		return false;
	}
	if (!IsPoseWord(Words[2]))
	{
		return false;
	}
	OutRole = Role;
	OutSide = Side;
	OutPose = FName(*Words[2]);
	return true;
}

bool WeaponHandMarks::IsTriggerSocket(FName Socket)
{
	return WithoutPrefix(Socket).Equals(TEXT("Trigger"), ESearchCase::IgnoreCase);
}

bool UWeaponModelComponent::GetHandMarks(FHandMarks& Out) const
{
	Out = FHandMarks();
	// In assembly order, so the latest part that marks a role and side wins (as FindSocket's latest part does): a part
	// hung later (a magazine, a fore-end) over the body's own spot for the same hand.
	for (const UStaticMeshComponent* Part : Parts)
	{
		const UStaticMesh* Mesh = Part ? Part->GetStaticMesh() : nullptr;
		if (!Mesh)
		{
			continue;
		}
		const bool bAtRest = Part == ReloadPartMesh.Get() && ReloadPart != EWeaponReloadPart::None;
		const bool bCylinder = bAtRest && ReloadPart == EWeaponReloadPart::Cylinder;
		for (const UStaticMeshSocket* Socket : Mesh->Sockets)
		{
			if (!Socket)
			{
				continue;
			}
			EHandMarkRole Role = EHandMarkRole::Hold;
			int32 Side = 0;
			FName Pose;
			if (WeaponHandMarks::ParseSocket(Socket->SocketName, Role, Side, Pose))
			{
				FHandMark& Mark = Out.Get(Role, Side);
				Mark.bValid = true;
				Mark.Local = SocketInGun(*this, *Part, *Socket, bAtRest, bCylinder);
				Mark.Pose = Pose;
			}
			else if (WeaponHandMarks::IsTriggerSocket(Socket->SocketName))
			{
				Out.bTrigger = true;
				Out.Trigger = SocketInGun(*this, *Part, *Socket, bAtRest, bCylinder).GetLocation();
			}
		}
	}
	return Out.HasAny();
}
