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
#include "Components/PoseableMeshComponent.h"
#include "UObject/UObjectGlobals.h"
#include "ThirdParty/Eigen/Geometry.h"
THIRD_PARTY_INCLUDES_END


FAnimNode_PR2IK::FAnimNode_PR2IK() :
	TranslationSpace(BCS_WorldSpace),
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
//contains all added code
void FAnimNode_PR2IK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext & Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	// get UE World Object
	const UWorld * TheWorld = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();
	// Create or get BoneContainer
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	//get bone index and varify them exists.
	FCompactPoseBoneIndex TipBoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
	
	//ensure that the order gets preserved
	TArray<TPair<int32, FName> > NameToIndex;
	FCompactPoseBoneIndex RootIndex = RootBone.GetCompactPoseIndex(BoneContainer);
	FCompactPoseBoneIndex BoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
		//abbort mission if index does not exist
		if (BoneIndex == INDEX_NONE || RootIndex == INDEX_NONE)
		{
			return;
		}
		//get first bone
		FName BoneName = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneName(BoneIndex.GetInt());
		NameToIndex.Add(TPair<int32, FName>(BoneIndex.GetInt(), BoneName));
		//collect all the other bones
		do
		{
			BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
			BoneName = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneName(BoneIndex.GetInt());
			NameToIndex.Add(TPair<int32, FName>(BoneIndex.GetInt(), BoneName));
		} while (BoneIndex != RootIndex);


		//reverse The Map, so that the order is from Root to Tip
		TArray<TPair<int32, FName> > NewNameToIndex;
		for (signed int i = 0; i < NameToIndex.Num(); i++) {
			unsigned int k = NameToIndex.Num() - i - 1;
			NewNameToIndex.Add(NameToIndex[k]);
		}

		//init previousAngles with Default BoneAngle if we do not already have previous angles stored
		for (const TPair<int32, FName>& pair : NewNameToIndex) {
			for (int32 i = 0; i < RangeLimits.Num(); i++) {
				if (RangeLimits[i].BoneName.BoneName == pair.Value) {

					int32 mindex = FCompactPoseBoneIndex(pair.Key).GetInt();
					if (!this->previousAngles.Contains(mindex)) {
						previousAngles.Add(TPair<int32, float>(mindex, RangeLimits[i].DefaultBoneAngle));
					}
				}
			}
		}
		

	//------------------------------------- Create a KDL Chain -------------------------------------------
	KDL::Chain ikchain;
	// get Component transform
	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	ComponentTransform.SetLocation(ComponentTransform.GetLocation() / 100); //fix scale between UE and KDL
	ComponentTransform.SetScale3D(FVector(1.0, 1.0, 1.0));

	//convert effector goal transform into component space, given it is provied in world space
	FTransform ScaledEffectorGoalTransform = EffectorGoalTransform;
	ScaledEffectorGoalTransform.SetLocation(ScaledEffectorGoalTransform.GetLocation() / 100); //fix scale
	ScaledEffectorGoalTransform.SetScale3D(FVector(1.0, 1.0, 1.0));
	FQuat dq(FRotator(ScaledEffectorGoalTransform.GetRotation().Rotator().Pitch, ScaledEffectorGoalTransform.GetRotation().Rotator().Yaw + 0, ScaledEffectorGoalTransform.GetRotation().Rotator().Roll));
	ScaledEffectorGoalTransform.SetRotation(dq);

	KDL::JntArray temp_min_limits(NameToIndex.Num()); //in rad
	KDL::JntArray temp_max_limits(NameToIndex.Num()); //in rad

	//iterate over all elements, find matching limits and add create the joints
	int32 j = 0; //needed for the counter of the elements
	for (const TPair<int32, FName>& pair : NewNameToIndex) {
		//make sure a new one is created each time
		KDL::Joint joint;

		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);
		FCompactPoseBoneIndex ParentIndex = Output.Pose.GetPose().GetParentBoneIndex(index);
		
		//get name of joint/link
		std::string name(TCHAR_TO_UTF8(*pair.Value.ToString())); //convert ue4 string to normal string. 
		//need to access it this way since this might be outside of our usual chain
		std::string ParentName(TCHAR_TO_UTF8(*Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneName(ParentIndex.GetInt()).ToString()));

		//create chain element and add it to the chain
		FTransform BoneLocalTransform = Output.Pose.GetLocalSpaceTransform(index);
		FVector JointOriginUE = BoneLocalTransform.GetTranslation();
		
		KDL::Vector JointOrigin = KDL::Vector(JointOriginUE.X, JointOriginUE.Y, JointOriginUE.Z);
		KDL::Vector VecX = KDL::Vector(1.0, 0.0, 0.0);
		KDL::Vector VecY = KDL::Vector(0.0, 1.0, 0.0);
		KDL::Vector VecZ = KDL::Vector(0.0, 0.0, 1.0);

		if (RangeLimits.Num() == 0) {
			UE_LOG(LogTemp, Warning, TEXT("No Limits set for this IK chain. Please set some Limits"));
			return;
		}
		else {
			//Check if that Element exists
			for (int32 i = 0; i < RangeLimits.Num(); i++) {
				std::string tempName(TCHAR_TO_UTF8(*RangeLimits[i].BoneName.BoneName.ToString()));

				//check if limit exist for current bone
				if ((i < RangeLimits.Num()) && (tempName == name)) {
					//if element exists and this is the one, create matching joint
					temp_min_limits.data(j) = RangeLimits[i].LimitMin;
					temp_max_limits.data(j) = RangeLimits[i].LimitMax;
					
					KDL::Vector VecT;
					switch(RangeLimits[i].Axis.GetValue()) {
					case RotX:
						joint = KDL::Joint(tempName, JointOrigin, VecX, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						VecT = VecX;
						break;

					case RotY:
						joint = KDL::Joint(tempName, JointOrigin, VecY, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						VecT = VecY;
						break;

					case RotZ:
						joint = KDL::Joint(tempName, JointOrigin, VecZ, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						VecT = VecZ;
						break;

					default:
						joint = KDL::Joint(tempName, JointOrigin, VecZ, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						break;
					}

					switch (RangeLimits[i].JointType.GetValue()) {
					
					case Fixed:
						joint = KDL::Joint(tempName, JointOrigin, VecT, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						break;

					case Revolute:
						break;

					default:
						break;
					}

					//get transform of current joint aka bone origin
					FTransform BoneTransform = Output.Pose.GetLocalSpaceTransform(index);
					FVector locVector = BoneTransform.GetTranslation();

					ikchain.addSegment(KDL::Segment(name, joint, KDL::Frame(KDL::Vector(locVector.X, locVector.Y, locVector.Z))));	
				}
			}
		}
		j++;
	}

	KDL::JntArray result(ikchain.getNrOfJoints());
	KDL::JntArray input(ikchain.getNrOfJoints());

	//get seed state
	unsigned int n = 0;
	for (const TPair<int32, FName>& pair : NewNameToIndex) {
		// pair int index to unreal index
		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);

		double oldAngle = previousAngles[index.GetInt()];
		
		//this is done for "seeding" aka give pose of where joint is right now instead of joint default position. let's do it without first
		if (n < ikchain.getNrOfJoints()) {
			input.data(n) = oldAngle;
			
			if (!((oldAngle <= temp_max_limits.data(n)) && (oldAngle >= temp_min_limits.data(n)))) {
			}
		}
		n++;
	}

	//Now that we have a chain, let's apply some IK solver to it
	KDL::ChainFkSolverPos_recursive fksolver1(ikchain);
	KDL::ChainIkSolverVel_pinv iksolver1v(ikchain,1e-5,400);
	KDL::ChainIkSolverPos_NR_JL iksolver(ikchain, temp_min_limits, temp_max_limits, fksolver1, iksolver1v, 400, 1e-6);
	
	//create destination Frame
	//transform goal pose from world frame into bone frame
	//recalculate the goal transform to be relative to our RootBone
	FTransform RootTransformComp = Output.Pose.GetComponentSpaceTransform(Output.Pose.GetPose().GetParentBoneIndex(RootIndex));
	RootTransformComp.SetLocation(RootTransformComp.GetLocation() / 100);
	RootTransformComp.SetScale3D(FVector(1.0, 1.0, 1.0));
	FTransform adjustYaw;
	adjustYaw.SetRotation(FQuat(FVector(0, 0, 1), 3.14159 / 2)); //arm offset rotation
	FTransform GoalInBoneSpace = adjustYaw*ScaledEffectorGoalTransform * ComponentTransform.Inverse() * RootTransformComp.Inverse();


	//Fix scale...
	GoalInBoneSpace.SetScale3D(FVector(1.0, 1.0, 1.0));
	
	FTransform TipTransform = Output.Pose.GetComponentSpaceTransform(TipBoneIndex);
	TipTransform.SetLocation(TipTransform.GetLocation() / 100);
	TipTransform.SetScale3D(FVector(1.0, 1.0, 1.0));

	FTransform ParentBoneInComponent = Output.Pose.GetComponentSpaceTransform(Output.Pose.GetPose().GetParentBoneIndex(RootIndex));
	ParentBoneInComponent.SetLocation(ParentBoneInComponent.GetLocation() / 100);
	ParentBoneInComponent.SetScale3D(FVector(1.0, 1.0, 1.0));
	FTransform TorsoInComponent = ParentBoneInComponent;
	FTransform CurrentGoal = GoalInBoneSpace;

	TipTransform = TipTransform * ParentBoneInComponent.Inverse();

	//KDL Calculation
	int maxDiv = 1;
	int found = 0;
	while (maxDiv)
	{
		maxDiv--;
		FTransform CurrentDiff = TipTransform.Inverse()*CurrentGoal;
		KDL::Vector goalvec = KDL::Vector::Vector(CurrentGoal.GetLocation().X, CurrentGoal.GetLocation().Y, CurrentGoal.GetLocation().Z);
		KDL::Rotation goalrot = KDL::Rotation::Quaternion(CurrentGoal.GetRotation().X, CurrentGoal.GetRotation().Y, CurrentGoal.GetRotation().Z, CurrentGoal.GetRotation().W); 
		//finalize goal frame for KDL
		KDL::Frame goal = KDL::Frame::Frame(goalrot, goalvec);

		int resultInt = iksolver.CartToJnt(input, goal, result);
		int Jumpy = 0;
		int Smooth = 1;
		int counter = 0;
		for (const TPair<int32, FName>& pair : NewNameToIndex) {
			float old = previousAngles[FCompactPoseBoneIndex(pair.Key).GetInt()];
			float diff = old-result.data(counter);
			if (0.25 < diff)
			{
				Jumpy = 1;
				break;
			}
			counter++;
		}
		if ((KDL::ChainIkSolverPos_NR_JL::E_MAX_ITERATIONS_EXCEEDED == resultInt) || (Smooth && Jumpy)) {
			CurrentDiff.SetLocation(CurrentDiff.GetLocation()*0.9);
			CurrentDiff.SetRotation(FQuat(CurrentDiff.GetRotation().GetRotationAxis(), CurrentDiff.GetRotation().GetAngle()*0.95));
			CurrentGoal = CurrentDiff * TipTransform;
		}
		else {
			found = 1;
			maxDiv = 0;
		}
	}
	if (!found)
	{
		unsigned int k = 0;
		for (const TPair<int32, FName>& pair : NewNameToIndex) {
			float old = previousAngles[FCompactPoseBoneIndex(pair.Key).GetInt()];
			result.data(k) = old;
			k++;
		}
	}

	//Apply KDL calculated poses to skeleton
	int counter = 0;
	for (const TPair<int32, FName>& pair : NewNameToIndex) {
	
		//get name of joint/link
		std::string name(TCHAR_TO_UTF8(*pair.Value.ToString())); //convert ue4 string to normal string. 
		// pair int index to unreal index
		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);

		FTransform BoneTransform = Output.Pose.GetLocalSpaceTransform(index);
		//Set axis vector for which the angle should be applied
		Eigen::Vector3d vec;
		switch (ikchain.getSegment(counter).getJoint().getType()) {
		case KDL::Joint::RotX:
			vec = Eigen::Vector3d(1.0, 0.0, 0.0);
			break;

		case KDL::Joint::RotY:
			vec = Eigen::Vector3d(0.0, 1.0, 0.0);
			break;

		case KDL::Joint::RotZ:
			vec = Eigen::Vector3d(0.0, 0.0, 1.0);
			break;

		default:
			KDL::Vector tempVec = ikchain.getSegment(counter).getJoint().JointAxis();
			vec = Eigen::Vector3d(tempVec.x(), tempVec.y(), tempVec.z());
			FVector printVector(tempVec.x(), tempVec.y(), tempVec.z());
			break;
		}

		//put KDL result into proper transform
		std::vector<Eigen::Vector3d> angleTest(3);
		angleTest[0] = Eigen::Vector3d(0.0, 0.0, 1.0);
		angleTest[1] = Eigen::Vector3d(1.0, 0.0, 0.0);
		angleTest[2] = Eigen::Vector3d(0.0, 1.0, 0.0);


		Eigen::AngleAxis<double> angle(result(counter), vec);
	 	Eigen::Matrix3d anglemat = angle.toRotationMatrix();
		Eigen::Quaterniond eigenQuat(anglemat);
		Eigen::Quaterniond normQuat = eigenQuat.normalized();

		this->previousAngles.Add(TPair<int32, float>(FCompactPoseBoneIndex(pair.Key).GetInt(), result(counter)));
		FQuat resultQuat(normQuat.x(), normQuat.y(), normQuat.z(), normQuat.w());

		FTransform ResultTransform;
		ResultTransform.SetIdentity();
		ResultTransform.SetRotation(resultQuat);

		FTransform BoneLocalTransform = Output.Pose.GetLocalSpaceTransform(index);
		FVector JointOriginUE = BoneLocalTransform.GetTranslation();
		FTransform LocScale;
		LocScale.SetTranslation(JointOriginUE);
		 

		FTransform LocalToComponentResult = ResultTransform * LocScale * ParentBoneInComponent;

		//detach gripper if goal is outside of the computational space
		// TODO
		if ((name == "r_gripper_palm_link") || (name == "l_gripper_palm_link")) {
			LocalToComponentResult = GoalInBoneSpace * TorsoInComponent;
		}

		// this fixes some scaling issues and unwanted translation
		FTransform OutputTransform = LocalToComponentResult;
		OutputTransform.SetLocation(OutputTransform.GetLocation() * 100);
		OutputTransform.SetScale3D(FVector(100.0, 100.0, 100.0));

		OutBoneTransforms.Add(FBoneTransform(index, OutputTransform));

		//ensure parent pose is the one we just computed
		ParentBoneInComponent = LocalToComponentResult;
		

		Output.Pose.LocalBlendCSBoneTransforms(OutBoneTransforms, 1.0);
		OutBoneTransforms.Reset();

		counter++;

	}
}

bool FAnimNode_PR2IK::IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones)
{
	return (TipBone.IsValidToEvaluate(RequiredBones));
}


	void FAnimNode_PR2IK::InitializeBoneReferences(const FBoneContainer & RequiredBones)
{
	TipBone.Initialize(RequiredBones);
	RootBone.Initialize(RequiredBones);
}

void FAnimNode_PR2IK::PreUpdate(const UAnimInstance * InAnimInstance)
{
	const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
	const UWorld* World = SkelComp->GetWorld();
}


