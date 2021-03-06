// Copyright 2018 Sean Chen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ThirdParty/KDL/joint.hpp"
#include "Components/PoseableMeshComponent.h"
#include "AnimNode_PR2IK.generated.h"


class USkeletalMeshComponent;

UENUM(BlueprintType)
enum ELimitAxis{

	RotX UMETA(DisplayName = "RotX"),
	RotY UMETA(DisplayName = "RotY"),
	RotZ UMETA(DisplayName = "RotZ"),

};

UENUM(BlueprintType)
enum EJointType {

	Fixed UMETA(DisplayName = "fixed"),
	Revolute UMETA(DisplayName = "revolute")

};

USTRUCT()
struct FJointLimits{
	GENERATED_USTRUCT_BODY()

	/** Name of Bone for which the limit is to be set **/
	UPROPERTY(EditAnywhere, Category = Angular)
		FBoneReference BoneName;

	/** Which Axis should the limit apply to. Other axis will be locked.**/
	UPROPERTY(EditAnywhere, Category = Angular)
		TEnumAsByte<ELimitAxis> Axis;

	/** Is it a movable revolute joint or a fixed joint?**/
	UPROPERTY(EditAnywhere, Category = Angular)
		TEnumAsByte<EJointType> JointType;

	/** Minimal limit for given joint in radians**/
	UPROPERTY(EditAnywhere, Category = Angular)
		float LimitMin;

	/** Maximal limit for given joint in radians**/
	UPROPERTY(EditAnywhere, Category = Angular)
		float LimitMax;

	/** Offset for the bone**/
	UPROPERTY(EditAnywhere, Category = Angular)
		float DefaultBoneAngle;

};


USTRUCT(BlueprintInternalUseOnly)
struct CUSTOMANIMNODE_API FAnimNode_PR2IK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	/** Name of bone to control. This is the main bone chain to modify from. **/

	UPROPERTY(EditAnywhere, Category = Settings)
		TEnumAsByte<enum EBoneControlSpace> TranslationSpace;

	// Transform for the Gripper-Goal
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault))
		FTransform EffectorGoalTransform;


	//TipBone of the chain/End Effector
	UPROPERTY(EditAnywhere, Category = Settings)
		FBoneReference TipBone;

	//RootBone of the chain. E.g. shoulder
	UPROPERTY(EditAnywhere, Category = Settings)
		FBoneReference RootBone;

	/* Add Limits to some of the bones. Note: Every bone needs to have a limit set. Starting from Tip back to root. */
	UPROPERTY(EditAnywhere, Category = Settings)
		TArray<FJointLimits> RangeLimits;


	/** Delta time */
	float DeltaTime;

	/** Internal use - Amount of time we need to simulate. */
	float RemainingTime;

	/** Internal use - Current timestep */
	float TimeStep;

	/** Internal use - Current time dilation */
	float TimeDilation;

	/** Did we have a non-zero ControlStrength last frame. */
	bool bHadValidStrength;

	/** World-space location of the bone. */
	FVector BoneLocation;

	/** World-space velocity of the bone. */
	FVector BoneVelocity;

	/** Velocity of the owning actor */
	FVector OwnerVelocity;

	UPoseableMeshComponent* PosableMeshComponent;
	UObject *a;

	TMap<int32, float> previousAngles;

public:
	FAnimNode_PR2IK();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	// Resused bone transform array to avoid reallocating in skeletal controls
	TArray<FBoneTransform> BoneTransforms;

public:
#if WITH_EDITOR
	void ResizeRotationLimitPerJoints(int32 NewSize);
#endif
};
