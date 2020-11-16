// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CustomAnimNode/Public/AnimNode_PR2IK.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAnimNode_PR2IK() {}
// Cross Module References
	CUSTOMANIMNODE_API UScriptStruct* Z_Construct_UScriptStruct_FAnimNode_PR2IK();
	UPackage* Z_Construct_UPackage__Script_CustomAnimNode();
	ANIMGRAPHRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FAnimNode_SkeletalControlBase();
	ENGINE_API UScriptStruct* Z_Construct_UScriptStruct_FBoneReference();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FTransform();
	ENGINE_API UEnum* Z_Construct_UEnum_Engine_EBoneControlSpace();
// End Cross Module References
class UScriptStruct* FAnimNode_PR2IK::StaticStruct()
{
	static class UScriptStruct* Singleton = NULL;
	if (!Singleton)
	{
		extern CUSTOMANIMNODE_API uint32 Get_Z_Construct_UScriptStruct_FAnimNode_PR2IK_Hash();
		Singleton = GetStaticStruct(Z_Construct_UScriptStruct_FAnimNode_PR2IK, Z_Construct_UPackage__Script_CustomAnimNode(), TEXT("AnimNode_PR2IK"), sizeof(FAnimNode_PR2IK), Get_Z_Construct_UScriptStruct_FAnimNode_PR2IK_Hash());
	}
	return Singleton;
}
template<> CUSTOMANIMNODE_API UScriptStruct* StaticStruct<FAnimNode_PR2IK>()
{
	return FAnimNode_PR2IK::StaticStruct();
}
static FCompiledInDeferStruct Z_CompiledInDeferStruct_UScriptStruct_FAnimNode_PR2IK(FAnimNode_PR2IK::StaticStruct, TEXT("/Script/CustomAnimNode"), TEXT("AnimNode_PR2IK"), false, nullptr, nullptr);
static struct FScriptStruct_CustomAnimNode_StaticRegisterNativesFAnimNode_PR2IK
{
	FScriptStruct_CustomAnimNode_StaticRegisterNativesFAnimNode_PR2IK()
	{
		UScriptStruct::DeferCppStructOps(FName(TEXT("AnimNode_PR2IK")),new UScriptStruct::TCppStructOps<FAnimNode_PR2IK>);
	}
} ScriptStruct_CustomAnimNode_StaticRegisterNativesFAnimNode_PR2IK;
	struct Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_RootBone_MetaData[];
#endif
		static const UE4CodeGen_Private::FStructPropertyParams NewProp_RootBone;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_TipBone_MetaData[];
#endif
		static const UE4CodeGen_Private::FStructPropertyParams NewProp_TipBone;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_EffectorGoalTransform_MetaData[];
#endif
		static const UE4CodeGen_Private::FStructPropertyParams NewProp_EffectorGoalTransform;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_TranslationSpace_MetaData[];
#endif
		static const UE4CodeGen_Private::FBytePropertyParams NewProp_TranslationSpace;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UE4CodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintInternalUseOnly", "true" },
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/AnimNode_PR2IK.h" },
		{ "ToolTip", "USTRUCT(BlueprintType)\nstruct FPelvisAdjustmentInterp\n{\n       GENERATED_BODY()\n\n               UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)\n               float Stiffness;\n\n       UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)\n               float Dampen;\n\n       FPelvisAdjustmentInterp() :\n               Stiffness(1.0f),\n               Dampen(1.0f)\n       {}\n\n};" },
	};
#endif
	void* Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FAnimNode_PR2IK>();
	}
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_RootBone_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/AnimNode_PR2IK.h" },
		{ "ToolTip", "RootBone of the chain. E.g. shoulder" },
	};
#endif
	const UE4CodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_RootBone = { "RootBone", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(FAnimNode_PR2IK, RootBone), Z_Construct_UScriptStruct_FBoneReference, METADATA_PARAMS(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_RootBone_MetaData, ARRAY_COUNT(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_RootBone_MetaData)) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TipBone_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/AnimNode_PR2IK.h" },
		{ "ToolTip", "TipBone of the chain/End Effector" },
	};
#endif
	const UE4CodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TipBone = { "TipBone", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(FAnimNode_PR2IK, TipBone), Z_Construct_UScriptStruct_FBoneReference, METADATA_PARAMS(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TipBone_MetaData, ARRAY_COUNT(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TipBone_MetaData)) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_EffectorGoalTransform_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/AnimNode_PR2IK.h" },
		{ "PinShownByDefault", "" },
		{ "ToolTip", "Transform for the Gripper-Goal" },
	};
#endif
	const UE4CodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_EffectorGoalTransform = { "EffectorGoalTransform", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(FAnimNode_PR2IK, EffectorGoalTransform), Z_Construct_UScriptStruct_FTransform, METADATA_PARAMS(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_EffectorGoalTransform_MetaData, ARRAY_COUNT(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_EffectorGoalTransform_MetaData)) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TranslationSpace_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/AnimNode_PR2IK.h" },
		{ "ToolTip", "Name of bone to control. This is the main bone chain to modify from. *" },
	};
#endif
	const UE4CodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TranslationSpace = { "TranslationSpace", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(FAnimNode_PR2IK, TranslationSpace), Z_Construct_UEnum_Engine_EBoneControlSpace, METADATA_PARAMS(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TranslationSpace_MetaData, ARRAY_COUNT(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TranslationSpace_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_RootBone,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TipBone,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_EffectorGoalTransform,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::NewProp_TranslationSpace,
	};
	const UE4CodeGen_Private::FStructParams Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_CustomAnimNode,
		Z_Construct_UScriptStruct_FAnimNode_SkeletalControlBase,
		&NewStructOps,
		"AnimNode_PR2IK",
		sizeof(FAnimNode_PR2IK),
		alignof(FAnimNode_PR2IK),
		Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::PropPointers,
		ARRAY_COUNT(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000201),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::Struct_MetaDataParams, ARRAY_COUNT(Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FAnimNode_PR2IK()
	{
#if WITH_HOT_RELOAD
		extern uint32 Get_Z_Construct_UScriptStruct_FAnimNode_PR2IK_Hash();
		UPackage* Outer = Z_Construct_UPackage__Script_CustomAnimNode();
		static UScriptStruct* ReturnStruct = FindExistingStructIfHotReloadOrDynamic(Outer, TEXT("AnimNode_PR2IK"), sizeof(FAnimNode_PR2IK), Get_Z_Construct_UScriptStruct_FAnimNode_PR2IK_Hash(), false);
#else
		static UScriptStruct* ReturnStruct = nullptr;
#endif
		if (!ReturnStruct)
		{
			UE4CodeGen_Private::ConstructUScriptStruct(ReturnStruct, Z_Construct_UScriptStruct_FAnimNode_PR2IK_Statics::ReturnStructParams);
		}
		return ReturnStruct;
	}
	uint32 Get_Z_Construct_UScriptStruct_FAnimNode_PR2IK_Hash() { return 290970766U; }
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
