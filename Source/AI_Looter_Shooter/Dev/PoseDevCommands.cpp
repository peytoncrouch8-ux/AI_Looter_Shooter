// Developer console commands for the heroes' stances and third-person aim (not in shipping builds; the
// Looter.Pose.* console variables are in Player/Animation/HeroPoseTuning.cpp):
//   Looter.Pose.Set <Hero> <Idle|Aim>.<Field> <Value>   changes one field of a hero's FHeroIdle or FHeroAim in memory (the
//                        loaded DA_Hero_<Id>, not saved): every body wearing them shows it at once. Values as Unreal
//                        writes them: 8, (X=0,Y=14,Z=-6), ChainHand.
//   Looter.Pose.Dump [Hero]    prints a hero's (or all five's) stance and aim as create_ember_assets.py's HERO_IDLE and
//                        HERO_AIM lines, to bake what was tuned.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/HeroPoseTypes.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"

namespace
{
	/** HeadLift -> head_lift: the name Unreal's Python gives a field. */
	FString PythonName(const FString& Name)
	{
		FString Out;
		for (int32 Index = 0; Index < Name.Len(); ++Index)
		{
			const TCHAR Letter = Name[Index];
			if (FChar::IsUpper(Letter) && Index > 0 && !FChar::IsUpper(Name[Index - 1]))
			{
				Out.AppendChar(TEXT('_'));
			}
			Out.AppendChar(FChar::ToLower(Letter));
		}
		return Out;
	}

	/** One field's value as the script writes it: a number, a tuple, or an enum's name in quotes. */
	FString PythonValue(const FProperty& Property, const void* Struct)
	{
		const void* Value = Property.ContainerPtrToValuePtr<void>(Struct);
		if (const FStructProperty* AsStruct = CastField<FStructProperty>(&Property))
		{
			if (AsStruct->Struct == TBaseStructure<FVector>::Get())
			{
				const FVector& Vector = *static_cast<const FVector*>(Value);
				return FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), Vector.X, Vector.Y, Vector.Z);
			}
		}
		if (const FEnumProperty* AsEnum = CastField<FEnumProperty>(&Property))
		{
			const int64 Number = AsEnum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value);
			return FString::Printf(TEXT("'%s'"), *AsEnum->GetEnum()->GetNameStringByValue(Number));
		}
		if (const FFloatProperty* AsFloat = CastField<FFloatProperty>(&Property))
		{
			return FString::Printf(TEXT("%.2f"), AsFloat->GetPropertyValue(Value));
		}
		FString Text;
		Property.ExportTextItem_Direct(Text, Value, nullptr, nullptr, PPF_None);
		return Text;
	}

	/** Struct's fields that differ from its defaults, as `name=value, ...`. */
	FString Fields(const UScriptStruct& Type, const void* Struct, const void* Defaults)
	{
		TArray<FString> Parts;
		for (TFieldIterator<FProperty> It(&Type); It; ++It)
		{
			if (!It->Identical_InContainer(Struct, Defaults))
			{
				Parts.Add(FString::Printf(TEXT("%s=%s"), *PythonName(It->GetName()), *PythonValue(**It, Struct)));
			}
		}
		return FString::Join(Parts, TEXT(", "));
	}

	void Dump(const TArray<FString>& Args)
	{
		TArray<FName> Ids;
		if (Args.Num() > 0)
		{
			Ids.Add(FName(*Args[0]));
		}
		else
		{
			Ids = UHeroDefinition::RosterIds();
		}
		const FHeroIdle IdleDefaults;
		const FHeroAim AimDefaults;
		for (const FName Id : Ids)
		{
			const UHeroDefinition* Hero = UHeroDefinition::Load(Id);
			if (!Hero)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Pose.Dump: no hero %s."), *Id.ToString());
				continue;
			}
			UE_LOG(LogLooter, Display, TEXT("HERO_IDLE '%s': dict(%s),"), *Id.ToString(), *Fields(*FHeroIdle::StaticStruct(), &Hero->Idle, &IdleDefaults));
			UE_LOG(LogLooter, Display, TEXT("HERO_AIM '%s': dict(%s),"), *Id.ToString(), *Fields(*FHeroAim::StaticStruct(), &Hero->Aim, &AimDefaults));
		}
	}

	void Set(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Pose.Set <Hero> <Idle|Aim>.<Field> <Value>, e.g. Looter.Pose.Set Crane Idle.Lean 9"));
			return;
		}
		UHeroDefinition* Hero = UHeroDefinition::Load(FName(*Args[0]));
		FString Group;
		FString Field;
		if (!Hero || !Args[1].Split(TEXT("."), &Group, &Field))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Pose.Set: no hero %s, or no Idle./Aim. field in %s."), *Args[0], *Args[1]);
			return;
		}
		const bool bIdle = Group.Equals(TEXT("Idle"), ESearchCase::IgnoreCase);
		const UScriptStruct* Type = bIdle ? FHeroIdle::StaticStruct() : FHeroAim::StaticStruct();
		void* Struct = bIdle ? static_cast<void*>(&Hero->Idle) : static_cast<void*>(&Hero->Aim);
		const FProperty* Property = Type->FindPropertyByName(FName(*Field));
		// The value may have spaces in it ((X=0, Y=14, Z=-6)): everything after the field.
		TArray<FString> Rest(Args.GetData() + 2, Args.Num() - 2);
		const FString Value = FString::Join(Rest, TEXT(" "));
		if (!Property || !Property->ImportText_InContainer(*Value, Struct, Hero, PPF_None))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Pose.Set: %s has no field %s, or %s isn't a value for it."), *Group, *Field, *Value);
			return;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Pose.Set: %s's %s.%s = %s (in memory; Looter.Pose.Dump %s prints what to bake)."), *Args[0], *Group,
			*Field, *PythonValue(*Property, Struct), *Args[0]);
	}

	FAutoConsoleCommand CmdPoseSet(TEXT("Looter.Pose.Set"),
		TEXT("Looter.Pose.Set <Hero> <Idle|Aim>.<Field> <Value>: one field of a hero's idle stance or third-person aim, live (in memory)."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Set));
	FAutoConsoleCommand CmdPoseDump(TEXT("Looter.Pose.Dump"),
		TEXT("Looter.Pose.Dump [Hero]: prints the heroes' idle stances and aims as create_ember_assets.py's HERO_IDLE and HERO_AIM lines."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Dump));
}

#endif
