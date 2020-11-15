// Copyright 2018 Sean Chen. All Rights Reserved.

#include "AnimGraphNode_PR2IK.h"
#include "UnrealWidget.h"
#include "Kismet2/CompilerResultsLog.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_PR2IK

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_PR2IK::UAnimGraphNode_PR2IK()
{

}


FText UAnimGraphNode_PR2IK::GetTooltipText() const
{
	return LOCTEXT("PR2IK", "PR2 IK");
}

FLinearColor UAnimGraphNode_PR2IK::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.75f, 0.1f);
}

FText UAnimGraphNode_PR2IK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PR2IK", "PR2 IK");
}

void UAnimGraphNode_PR2IK::CopyPinDefaultsToNodeData(UEdGraphPin * InPin)
{

}

#undef LOCTEXT_NAMESPACE