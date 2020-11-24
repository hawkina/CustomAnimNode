// Copyright 2018 Sean Chen. All Rights Reserved.

#include "AnimNode_PR2IK.h"
#include "GameFramework/WorldSettings.h"
#include "Animation/AnimInstanceProxy.h"

THIRD_PARTY_INCLUDES_START
#include <cmath> //needed for Eigen, otherwise Syntax errors
#include "ThirdParty/KDL/chain.hpp"
#include "ThirdParty/KDL/chainiksolverpos_nr.hpp"
#include "ThirdParty/KDL/chainfksolver.hpp"
#include "ThirdParty/KDL/chainfksolverpos_recursive.hpp"
#include "ThirdParty/KDL/chainiksolvervel_pinv.hpp"
#include "ThirdParty/KDL/chainiksolverpos_nr_jl.hpp"
#include "ThirdParty/Eigen/Geometry.h"
THIRD_PARTY_INCLUDES_END

FAnimNode_PR2IK::FAnimNode_PR2IK() :
	TranslationSpace(BCS_BoneSpace),
	EffectorGoalTransform(FTransform::Identity)
	
{}

//interface. Must be implemented
void FAnimNode_PR2IK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);

	RemainingTime = 0.0f;
}

//interface. Must be implemented
void FAnimNode_PR2IK::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_SkeletalControlBase::CacheBones_AnyThread(Context);
}

//interface. Must be implemented
void FAnimNode_PR2IK::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);
	DeltaTime += Context.GetDeltaTime();
	// simulation step
	TimeStep = Context.GetDeltaTime() * TimeDilation;
}

//interface. Must be implemented
void FAnimNode_PR2IK::GatherDebugData(FNodeDebugData & DebugData)
{
	ComponentPose.GatherDebugData(DebugData);
}

//interface. Must be implemented
//this is also where all the code goes, apperently
void FAnimNode_PR2IK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext & Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	//DEBUGGING Print to LOG
	UE_LOG(LogTemp, Warning, TEXT("+++AnyThread evaluation+++"));

	check(OutBoneTransforms.Num() == 0);
	// get UE World Object
	const UWorld * TheWorld = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();
	// Create or get BoneContainer
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	////get bone index and varify them exists.
	FCompactPoseBoneIndex TipBoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
	//FCompactPoseBoneIndex RootBoneCompactPoseIndex = RootBone.GetCompactPoseIndex(BoneContainer);
	
	//get Bone Chain by getting all indices from root to tip
	TArray<FCompactPoseBoneIndex> BoneIndices;
	// essentially Int and String...
	TMap<int32, FName> NameToIndex;

	//for applying transforms
	TArray<FBoneTransform> TempTransform;

	{
		FCompactPoseBoneIndex RootIndex = RootBone.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex BoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
		//abbort mission if index does not exist
		if (BoneIndex == INDEX_NONE || RootIndex == INDEX_NONE)
		{
			return;
		}

		FName BoneName = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneName(BoneIndex.GetInt());
		NameToIndex.Add(BoneIndex.GetInt(), BoneName);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
			BoneName = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneName(BoneIndex.GetInt());
			NameToIndex.Add(BoneIndex.GetInt(), BoneName);

		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
		NameToIndex.Add(BoneIndex.GetInt(), BoneName);

		//Print the list to ensure it is correct:
		FString name;
		for (const TPair<int32, FName>& pair : NameToIndex){
			name = pair.Value.ToString();
			UE_LOG(LogTemp, Warning, TEXT("Bone Name: %s"), *name);
			UE_LOG(LogTemp, Warning, TEXT("Index: %d"), pair.Key);
		
		}
	}
	

	//Create a KDL Chain
	KDL::Chain ikchain;
	// get Component transform
	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	//Add Segments to the chain, with the corresponding names and other data

	KDL::JntArray min_limits(NameToIndex.Num()); //in rad
	KDL::JntArray max_limits(NameToIndex.Num()); //in rad

	int32 j = 0; //needed for the counter of the elements

	KDL::Joint joint;
	for (const TPair<int32, FName>& pair : NameToIndex) {
		//get name of joint/link
		std::string name(TCHAR_TO_UTF8(*pair.Value.ToString())); //convert ue4 string to normal string. 

		
		// pair int index to unreal index
		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);
		//get transform of joint
		FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(index);

		//transform transform to bone space, aka. relative from one bone to another (hopefully)
		//TODO: if this doesn't work, check if another TranslationSpace will work
		//TODO: map Joint Rotation Axis to User Input of Node
		FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, BoneTransform, index, TranslationSpace);
		//get vector of transform
		FVector locVector = BoneTransform.GetTranslation();

		//create chain element and add it to the chain
		if (RangeLimits.Num() == 0) {
			UE_LOG(LogTemp, Warning, TEXT("No Limits set for this IK chain. Please set some Limits"));
			return;
		}
		else {
			//Check if that Element exists
			for (int32 i = 0; i < RangeLimits.Num(); i++) {
				//UE_LOG(LogTemp, Warning, TEXT("i: %d"), i);
				std::string tempName(TCHAR_TO_UTF8(*RangeLimits[i].BoneName.BoneName.ToString()));

				//UE_LOG(LogTemp, Warning, TEXT("temp name (from limit list): %s"), *RangeLimits[i].BoneName.BoneName.ToString());
				//UE_LOG(LogTemp, Warning, TEXT("name: %s"), *pair.Value.ToString());

				if (i < RangeLimits.Num() && tempName == name) {
					//if element exists and this is the one, create matching joint
					//UE_LOG(LogTemp, Warning, TEXT("Creating limits for bone: %s"), *RangeLimits[i].BoneName.BoneName.ToString());
					min_limits.data(j) = RangeLimits[i].LimitMin;
					max_limits.data(j) = RangeLimits[i].LimitMax;
					//UE_LOG(LogTemp, Warning, TEXT("LimitMin: %f"), RangeLimits[i].LimitMin);
					//UE_LOG(LogTemp, Warning, TEXT("LimitMax: %f"), RangeLimits[i].LimitMax);
					
					switch(RangeLimits[i].Axis.GetValue()) {
					case RotX:
						joint = KDL::Joint(tempName, KDL::Joint::RotX, 1, 0, 0, 0, 0);
						UE_LOG(LogTemp, Warning, TEXT("Axis: X"));
						break;

					case RotY:
						joint = KDL::Joint(tempName, KDL::Joint::RotY, 1, 0, 0, 0, 0);
						UE_LOG(LogTemp, Warning, TEXT("Axis: Y"));
						break;

					case RotZ:
						joint = KDL::Joint(tempName, KDL::Joint::RotZ, 1, 0, 0, 0, 0);
						UE_LOG(LogTemp, Warning, TEXT("Axis: Z"));
						break;

					default:
						joint = KDL::Joint(tempName, KDL::Joint::RotX, 1, 0, 0, 0, 0);
						UE_LOG(LogTemp, Warning, TEXT("no axis set for this joint"));
						break;
					}

					switch (RangeLimits[i].JointType.GetValue()) {
					
					case Fixed:
						UE_LOG(LogTemp, Warning, TEXT("Fixed Joint"));
						joint = KDL::Joint(tempName, KDL::Joint::Fixed, 1, 0, 0, 0, 0);
						break;

					case Revolute:
						break;

					default:
						break;
					}

					ikchain.addSegment(KDL::Segment(name, joint, KDL::Frame(KDL::Vector(locVector.X, locVector.Y, locVector.Z))));
				}
			}
			//when there is no limit given for that particular element.
			//min_limits(j) = -3.12159;
			//max_limits(j) = 3.12159;
			//ikchain.addSegment(KDL::Segment(name, KDL::Joint(KDL::Joint::Fixed), KDL::Frame(KDL::Vector(locVector.X, locVector.Y, locVector.Z))));
		}

	
		//check if array contains limit for our element
		
		j++;
	}

	KDL::JntArray result(ikchain.getNrOfJoints());
	KDL::JntArray input(ikchain.getNrOfJoints());

	//get seed state
	int n = 0;
	for (const TPair<int32, FName>& pair : NameToIndex) {
		// pair int index to unreal index
		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);
		//get transform of joint
		FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(index);
		float angle = BoneTransform.GetRotation().GetAngle();
		
		if ((unsigned) n < ikchain.getNrOfJoints()) {
			input.data(n) = angle;
		}
		//UE_LOG(LogTemp, Warning, TEXT("------------------------"));
		//UE_LOG(LogTemp, Warning, TEXT("input: %s"), *pair.Value.ToString());
		//UE_LOG(LogTemp, Warning, TEXT("input: %f"), input(n));
		//UE_LOG(LogTemp, Warning, TEXT("limit max: %f"), max_limits(n));
		//UE_LOG(LogTemp, Warning, TEXT("limit min: %f"), min_limits(n));
		//std::string temp1 = ikchain.getSegment(n).getName();
		//FString temp2(temp1.c_str());
		//UE_LOG(LogTemp, Warning, TEXT("Chain Segment name: %s"), *temp2);
		n++;
	}

	//UE_LOG(LogTemp, Warning, TEXT("Chain length segments: %d"), ikchain.getNrOfSegments());
	//UE_LOG(LogTemp, Warning, TEXT("Chain length joints: %d"), ikchain.getNrOfJoints());
	//UE_LOG(LogTemp, Warning, TEXT("Max limits: %d"), max_limits.rows());
	//UE_LOG(LogTemp, Warning, TEXT("Min limits: %d"), min_limits.rows());
	
	//Now that we have a chain, let's apply some IK solver to it
	KDL::ChainFkSolverPos_recursive fksolver1(ikchain);
	KDL::ChainIkSolverVel_pinv iksolver1v(ikchain);
	//KDL::ChainIkSolverPos_NR iksolver(ikchain, fksolver1, iksolver1v, 100, 1e-2);
	KDL::ChainIkSolverPos_NR_JL iksolver(ikchain, min_limits, max_limits, fksolver1, iksolver1v, 100, 1e-2);
	iksolver.setJointLimits(min_limits, max_limits);

	
	//create destination Frame
	// this is just a lot shorter to write... TODO: remove
	FTransform t = EffectorGoalTransform;
	KDL::Vector goalvec = KDL::Vector::Vector(t.GetLocation().X, t.GetLocation().Y, t.GetLocation().Z);
	FQuat rot = t.GetRotation();
	
	//convert quaternion into rotation matrix
	Eigen::Quaterniond quat;
	
	quat.x() = rot.X;
	quat.y() = rot.Y;
	quat.z() = rot.Z;
	quat.w() = rot.W;

	Eigen::Matrix3d mat;
	mat = quat.normalized().toRotationMatrix();
	
	// if not mistaken, Xx, Xy, Xz, Yx, Yy, Yz, Zx, Zy, Zz
	KDL::Rotation goalrot = KDL::Rotation::Rotation(mat(0), mat(1), mat(2), mat(3), mat(4), mat(5), mat(6), mat(7), mat(8));

	KDL::Frame goal = KDL::Frame::Frame(goalrot, goalvec);

	int resultInt = iksolver.CartToJnt(input, goal, result);
	

	UE_LOG(LogTemp, Warning, TEXT("Debugging soon"));
	//these are JointAngles, aka floats, aka rotation around specified axis in radians
	for (unsigned int i = 0; i < result.rows(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("input was %f"), input(i));
		UE_LOG(LogTemp, Warning, TEXT("Ik solver result: %f"), result(i));
		UE_LOG(LogTemp, Warning, TEXT("result counter: %d"), i);
	}
	UE_LOG(LogTemp, Warning, TEXT("+++Start debugging+++"));

	//set rotations for all the bones. 
	//iterate over all the bones
	int counter = 0;
	for (const TPair<int32, FName>& pair : NameToIndex) {
		//get name of joint/link
		std::string name(TCHAR_TO_UTF8(*pair.Value.ToString())); //convert ue4 string to normal string. 

		UE_LOG(LogTemp, Warning, TEXT("Debug name: %s"), *pair.Value.ToString());
		// pair int index to unreal index
		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);

		FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(index);

		
		//transform transform to bone space, aka. relative from one bone to another (hopefully)
		//TODO: if this doesn't work, check if another TranslationSpace will work
		//TODO: map Joint Rotation Axis to User Input of Node
		FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, BoneTransform, index, TranslationSpace);
		UE_LOG(LogTemp, Warning, TEXT("After some BoneSpace thingy conversion, before Eigen "));
		//FQuat BoneRotation

		//TODO match Axis to the User set Axis
		//convert radians to matrix to quaternion to unreal quaternion
		Eigen::AngleAxis<double> angle(result(counter), Eigen::Vector3d(0, 0, 1));
	 	Eigen::Matrix3d anglemat = angle.toRotationMatrix();
		Eigen::Quaterniond eigenQuat(anglemat);
		Eigen::Quaterniond normQuat = eigenQuat.normalized();
	
		
		FQuat resultQuat(normQuat.x(), normQuat.y(), normQuat.z(), normQuat.w());
	
		UE_LOG(LogTemp, Warning, TEXT("After Eigen, quaternion: %s"), *resultQuat.ToString());

		//maybe ConcatenateRotation?
		//BoneTransform.AccumulateWithShortestRotation();
		
		BoneTransform.SetRotation(resultQuat * BoneTransform.GetRotation());
		TipBone.GetCompactPoseIndex(BoneContainer);
		//Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneIndex(key);

		FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, BoneTransform, index, TranslationSpace);
		//OutBoneTransforms.Add(FBoneTransform(index, BoneTransform));
	
		TempTransform.Add(FBoneTransform(index, BoneTransform));
		Output.Pose.LocalBlendCSBoneTransforms(TempTransform, 1.0);
		TempTransform.Reset();

		counter++;

	}
	UE_LOG(LogTemp, Warning, TEXT("After loop"));

}
//
bool FAnimNode_PR2IK::IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones)
{
	return (TipBone.IsValidToEvaluate(RequiredBones));
}


	void FAnimNode_PR2IK::InitializeBoneReferences(const FBoneContainer & RequiredBones)
{
	TipBone.Initialize(RequiredBones);
	RootBone.Initialize(RequiredBones);
	
//	if (!FeetDefinitions.Num() == 0)
//	{
//		for (FIKBonesT & Each : FeetDefinitions)
//		{
//			Each.IKFootBone.Initialize(RequiredBones);
//			Each.FKFootBone.Initialize(RequiredBones);
//		}
//	}
}

void FAnimNode_PR2IK::PreUpdate(const UAnimInstance * InAnimInstance)
{
	const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
	const UWorld* World = SkelComp->GetWorld();
//	check(World->GetWorldSettings());
//	TimeDilation = World->GetWorldSettings()->GetEffectiveTimeDilation();
//
//	AActor* SkelOwner = SkelComp->GetOwner();
//	if (SkelComp->GetAttachParent() != NULL && (SkelOwner == NULL))
//	{
//		SkelOwner = SkelComp->GetAttachParent()->GetOwner();
//		OwnerVelocity = SkelOwner->GetVelocity();
//	}
//	else
//	{
//		OwnerVelocity = FVector::ZeroVector;
//	}
}


