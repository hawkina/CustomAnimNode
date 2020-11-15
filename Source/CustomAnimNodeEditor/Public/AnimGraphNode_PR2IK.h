// Copyright 2018 Sean Chen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNode_PR2IK.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimGraphNode_PR2IK.generated.h"

struct FAnimNode_PR2IK;
/**
*
*/
UCLASS()
class CUSTOMANIMNODEEDITOR_API UAnimGraphNode_PR2IK : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_PR2IK Node;

public:

	UAnimGraphNode_PR2IK();

	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_Base interface
	virtual void CopyPinDefaultsToNodeData(UEdGraphPin* InPin) override;
	// End of UAnimGraphNode_Base interface

	// UAnimGraphNode_SkeletalControlBase interface
	virtual const FAnimNode_SkeletalControlBase* GetNode() const { return &Node; }
	// End of UAnimGraphNode_SkeletalControlBase interface


};
