#include "Player/PlayerMeleeComponent.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Player/PlayerViewComponent.h"
#include "Session/SessionSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// UPlayerMeleeComponent's tool: the played hero's (UHeroDefinition::GetMeleeTool; the user's picks, 2026-10-27) made on
// the first-person camera at the first swing, placed on MeleeMotion's arc each frame of a swing, hidden otherwise. Until a
// hero's mesh is imported the placeholder's is used, and until that one is, a stand-in of the engine's basic shapes.

namespace
{
	TAutoConsoleVariable<float> CVarToolScale(TEXT("Looter.Melee.ToolScale"), 1.f,
		TEXT("Every melee tool's size in first person against its hero's own (1: as each hero's data says)."));
	// MeleeTools.py: the piston fist's clamp ring is 6-7 cm behind the wrist, the fist's front about 19 cm ahead of it; the
	// hands hold a fist by its middle, about 8 cm ahead of the wrist.
	TAutoConsoleVariable<float> CVarWristBack(TEXT("Looter.Melee.WristBack"), 15.f,
		TEXT("A forearm tool's clamp behind the middle of the fist (cm): where its Forearm socket sits."));

	const FName GripSocket(TEXT("Grip"));
	const FName ForearmSocket(TEXT("Forearm"));
	const FName TipSocket(TEXT("Tip"));
	const TCHAR* const SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");

	/** The stand-in (no tool mesh in the game at all): a mallet, an ash handle and an iron head, in the tool's own space. */
	const FLinearColor StandInWood(0.32f, 0.19f, 0.09f);
	const FLinearColor StandInIron(0.2f, 0.2f, 0.22f);

	UStaticMeshComponent* MakePart(AActor& Owner, USceneComponent& Root, UStaticMesh* Mesh, const TCHAR* Name, const FTransform& Place,
		const FLinearColor* Colour)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(&Owner, Name, RF_Transient);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		// Drawn like the arms and the gun in hand: the view model's field of view and scale (never in a wall), owner only.
		Part->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		Part->SetOnlyOwnerSee(true);
		Part->SetCastShadow(false);
		Part->bReceivesDecals = false;
		if (Colour)
		{
			if (UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, SurfacePath, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Surface, Part);
				Look->SetVectorParameterValue(TEXT("Color"), *Colour);
				Part->SetMaterial(0, Look);
			}
		}
		Part->SetupAttachment(&Root);
		Part->SetRelativeTransform(Place);
		// Born hidden: a part made while the root is already hidden would otherwise show at rest (DEMO, round 4: Gauge's
		// piston fist left in view after Looter.Hero Ellis).
		Part->SetVisibility(false);
		Part->RegisterComponent();
		return Part;
	}

	/** A socket's place on its mesh (the identity without one: the mesh's origin and axes stand in). */
	FTransform SocketOn(const UStaticMesh& Mesh, FName Name)
	{
		const UStaticMeshSocket* Socket = Mesh.FindSocket(Name);
		return Socket ? FTransform(Socket->RelativeRotation, Socket->RelativeLocation) : FTransform::Identity;
	}
}

void UPlayerMeleeComponent::PrepareTool()
{
	// The hero played (the session's pick or Looter.Hero's); none in a test level: Ellis's.
	const UHeroDefinition* Hero = USessionSubsystem::FindPlayedHero(this);
	if (bToolPrepared && Hero == ToolHero.Get())
	{
		return;
	}
	bToolPrepared = true;
	ToolHero = Hero;
	Tool = Hero ? Hero->GetMeleeTool() : UHeroDefinition::MeleeToolFor(UHeroDefinition::DefaultId());
	UStaticMesh* Mesh = Hero ? Hero->LoadMeleeTool() : nullptr;
	// The placeholder and the stand-in are held: a forearm tool swings as held till its own mesh is in.
	const bool bOwnMesh = Mesh && Mesh->GetPathName() == Tool.Mesh.ToSoftObjectPath().ToString();
	bForearmMesh = bOwnMesh && Tool.Grip == EMeleeGripKind::Forearm;
	if (!bOwnMesh)
	{
		Tool.Grip = EMeleeGripKind::Held;
	}
	// How far the striking end sits from the hand (SOCKET_Tip from SOCKET_Grip, drawn): a long tool's arc is aimed so its
	// head, not its handle, crosses the middle of the view at the blow (DEMO, round 4: the spade's shaft did).
	ToolTipLength = 0.f;
	if (Mesh && Mesh->FindSocket(TipSocket) && !bForearmMesh)
	{
		const FVector Tip = SocketOn(*Mesh, TipSocket).GetLocation() - SocketOn(*Mesh, GripSocket).GetLocation();
		ToolTipLength = static_cast<float>(Tip.Size()) * FMath::Max(Tool.Scale * CVarToolScale.GetValueOnGameThread(), 0.1f);
	}
	BuildTool(Mesh);
	BuildThirdPersonTool(Mesh);
	UE_LOG(LogLooter, Log, TEXT("Melee: %s's tool %s (%s)"), Hero ? *Hero->Id.ToString() : TEXT("no hero"),
		Mesh ? *Mesh->GetName() : TEXT("stand-in"), bOwnMesh ? TEXT("their own") : TEXT("until theirs is imported"));
}

void UPlayerMeleeComponent::BuildTool(UStaticMesh* Mesh)
{
	for (UStaticMeshComponent* Part : ToolParts)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	ToolParts.Reset();
	AActor* Owner = GetOwner();
	UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	if (!Owner || !Eye)
	{
		return;
	}
	if (!ToolRoot)
	{
		ToolRoot = NewObject<USceneComponent>(Owner, TEXT("MeleeToolRoot"), RF_Transient);
		ToolRoot->SetupAttachment(Eye);
		ToolRoot->RegisterComponent();
	}
	if (Mesh)
	{
		// The mesh set so its socket (the palm's grip, or the forearm's clamp) is on the root, its axes on the root's.
		const FTransform Socket = SocketOn(*Mesh, bForearmMesh ? ForearmSocket : GripSocket);
		ToolParts.Add(MakePart(*Owner, *ToolRoot, Mesh, TEXT("MeleeTool"), Socket.Inverse(), nullptr));
	}
	else
	{
		// No tool in the game at all: a mallet the size of a hatchet, handle up the tool's X from just under the fist, the
		// head across it striking along Z (the engine's cylinder stands 100 cm along its Z, the cube 100 cm a side).
		UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (Cylinder && Cube)
		{
			ToolParts.Add(MakePart(*Owner, *ToolRoot, Cylinder, TEXT("MeleeToolHandle"),
				FTransform(FRotator(-90.f, 0.f, 0.f), FVector(10.f, 0.f, 0.f), FVector(0.032f, 0.032f, 0.4f)), &StandInWood));
			ToolParts.Add(MakePart(*Owner, *ToolRoot, Cube, TEXT("MeleeToolHead"),
				FTransform(FRotator::ZeroRotator, FVector(28.f, 0.f, 0.f), FVector(0.05f, 0.045f, 0.14f)), &StandInIron));
		}
	}
	HideTool();
}

void UPlayerMeleeComponent::PlaceTool()
{
	// Third person: the tool rides the body's right hand through the swing (the stance layer's jab moves it).
	const bool bFirstPerson = IsFirstPersonView();
	if (ThirdPersonTool)
	{
		const bool bShow = bSwinging && !bFirstPerson;
		if (ThirdPersonTool->IsVisible() != bShow)
		{
			ThirdPersonTool->SetVisibility(bShow);
		}
	}
	if (!bSwinging || !ToolRoot || ToolParts.IsEmpty() || !bFirstPerson)
	{
		HideTool();
		return;
	}
	ToolPose = MeleeMotion::ToolPose(Tool.Swing, Tool.Heft, SwingTime, bLanded, ToolTipLength);
	const float Scale = FMath::Max(Tool.Scale * CVarToolScale.GetValueOnGameThread(), 0.1f);
	FTransform Root = ToolPose.Grip;
	if (bForearmMesh)
	{
		// The clamp on the wrist behind the fist, and the piston's kick past it at the blow.
		Root.AddToTranslation(Root.GetRotation().GetAxisX() * (ToolPose.Push - FMath::Max(CVarWristBack.GetValueOnGameThread(), 0.f)));
	}
	Root.SetScale3D(FVector(Scale));
	ToolRoot->SetRelativeTransform(Root);
	ToolRoot->SetVisibility(ToolPose.bShown, true);
}

void UPlayerMeleeComponent::HideTool()
{
	ToolPose = FMeleeToolPose();
	// Every part, whatever the root says: a root's flag doesn't hide its children by itself.
	if (ToolRoot)
	{
		ToolRoot->SetVisibility(false, true);
	}
	for (UStaticMeshComponent* Part : ToolParts)
	{
		if (Part)
		{
			Part->SetVisibility(false);
		}
	}
	// Third person: hidden too, unless a swing still runs (a switch to third person mid-swing shows it there).
	if (ThirdPersonTool && ThirdPersonTool->IsVisible() && !(bSwinging && !IsFirstPersonView()))
	{
		ThirdPersonTool->SetVisibility(false);
	}
}

bool UPlayerMeleeComponent::IsFirstPersonView() const
{
	const UPlayerViewComponent* View = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	return View && View->IsFirstPerson();
}

bool UPlayerMeleeComponent::GetToolHold(FMeleeToolHold& OutHold) const
{
	if (!bSwinging || !ToolPose.bShown || !ToolRoot || !ToolRoot->IsVisible())
	{
		return false;
	}
	OutHold.Grip = ToolPose.Grip;
	OutHold.HandShare = ToolPose.HandShare;
	OutHold.HandleRadius = Tool.HandleRadius * FMath::Max(Tool.Scale * CVarToolScale.GetValueOnGameThread(), 0.1f);
	OutHold.Kind = bForearmMesh ? EMeleeGripKind::Forearm : EMeleeGripKind::Held;
	return true;
}
