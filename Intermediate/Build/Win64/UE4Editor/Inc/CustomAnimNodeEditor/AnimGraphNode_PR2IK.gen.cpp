// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CustomAnimNodeEditor/Public/AnimGraphNode_PR2IK.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAnimGraphNode_PR2IK() {}
// Cross Module References
	CUSTOMANIMNODEEDITOR_API UClass* Z_Construct_UClass_UAnimGraphNode_PR2IK_NoRegister();
	CUSTOMANIMNODEEDITOR_API UClass* Z_Construct_UClass_UAnimGraphNode_PR2IK();
	ANIMGRAPH_API UClass* Z_Construct_UClass_UAnimGraphNode_SkeletalControlBase();
	UPackage* Z_Construct_UPackage__Script_CustomAnimNodeEditor();
	CUSTOMANIMNODE_API UScriptStruct* Z_Construct_UScriptStruct_FAnimNode_PR2IK();
// End Cross Module References
	void UAnimGraphNode_PR2IK::StaticRegisterNativesUAnimGraphNode_PR2IK()
	{
	}
	UClass* Z_Construct_UClass_UAnimGraphNode_PR2IK_NoRegister()
	{
		return UAnimGraphNode_PR2IK::StaticClass();
	}
	struct Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Node_MetaData[];
#endif
		static const UE4CodeGen_Private::FStructPropertyParams NewProp_Node;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UAnimGraphNode_SkeletalControlBase,
		(UObject* (*)())Z_Construct_UPackage__Script_CustomAnimNodeEditor,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "AnimGraphNode_PR2IK.h" },
		{ "ModuleRelativePath", "Public/AnimGraphNode_PR2IK.h" },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::NewProp_Node_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/AnimGraphNode_PR2IK.h" },
	};
#endif
	const UE4CodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::NewProp_Node = { "Node", nullptr, (EPropertyFlags)0x0040000000000001, UE4CodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(UAnimGraphNode_PR2IK, Node), Z_Construct_UScriptStruct_FAnimNode_PR2IK, METADATA_PARAMS(Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::NewProp_Node_MetaData, ARRAY_COUNT(Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::NewProp_Node_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::NewProp_Node,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAnimGraphNode_PR2IK>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::ClassParams = {
		&UAnimGraphNode_PR2IK::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::PropPointers,
		nullptr,
		ARRAY_COUNT(DependentSingletons),
		0,
		ARRAY_COUNT(Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UAnimGraphNode_PR2IK()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UAnimGraphNode_PR2IK_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UAnimGraphNode_PR2IK, 3962965231);
	template<> CUSTOMANIMNODEEDITOR_API UClass* StaticClass<UAnimGraphNode_PR2IK>()
	{
		return UAnimGraphNode_PR2IK::StaticClass();
	}
	static FCompiledInDefer Z_CompiledInDefer_UClass_UAnimGraphNode_PR2IK(Z_Construct_UClass_UAnimGraphNode_PR2IK, &UAnimGraphNode_PR2IK::StaticClass, TEXT("/Script/CustomAnimNodeEditor"), TEXT("UAnimGraphNode_PR2IK"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAnimGraphNode_PR2IK);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
