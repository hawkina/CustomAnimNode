// Copyright 2018 Sean Chen. All Rights Reserved.

#include "AnimNode_PR2IK.h"
#include "GameFramework/WorldSettings.h"
#include "Animation/AnimInstanceProxy.h"

THIRD_PARTY_INCLUDES_START
#include <cmath> //needed for Eigen, otherwise Syntax errors
//#include <algorithm> //needed for Eigen
//#include "ThirdParty/Eigen/Eigen"
//#include "ThirdParty/KDL/chain.hpp"
//#include "ThirdParty/Eigen/src/Array/Norms.h"
//#include <UKDL/KDL/>
#include "ThirdParty/KDL/chain.hpp"
#include "ThirdParty/KDL/chainiksolverpos_nr.hpp"
#include "ThirdParty/KDL/chainfksolver.hpp"
#include "ThirdParty/KDL/chainfksolverpos_recursive.hpp"
#include "ThirdParty/KDL/chainiksolvervel_pinv.hpp"
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
		ikchain.addSegment(KDL::Segment(name, KDL::Joint(KDL::Joint::RotZ), KDL::Frame(KDL::Vector(locVector.X, locVector.Y, locVector.Z))));

	}

	//Now that we have a chain, let's apply some IK solver to it
	KDL::ChainFkSolverPos_recursive fksolver1(ikchain);
	KDL::ChainIkSolverVel_pinv iksolver1v(ikchain);
	KDL::ChainIkSolverPos_NR iksolver(ikchain, fksolver1, iksolver1v, 100, 1e-2);
	
	KDL::JntArray result(ikchain.getNrOfJoints());
	KDL::JntArray input(ikchain.getNrOfJoints());

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


	
	// Translate Bone to target location
	//FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, TipBoneTransform, TipBoneCompactPoseIndex, TranslationSpace);
	
	// set translation
	/*TipBoneTransform.SetTranslation(EffectorGoalTransform.GetTranslation());*/
	//back in 
	//FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, TipBoneTransform, TipBoneCompactPoseIndex, TranslationSpace);


	//Output the result of the calculation
	/*OutBoneTransforms.Add(FBoneTransform(TipBone.GetCompactPoseIndex(BoneContainer), TipBoneTransform));*/
	


	//// Every leg apply a spring force to pelvis.
	//TArray<FVector> TSpringForce;
	//TArray<FIKFootLocationT> TIKFootLocationTs;

	//if (!FeetDefinitions.Num() == 0)
	//{
	//	//Calculate each foot's real position.
	//	for (FIKBonesT & Each : FeetDefinitions)
	//	{
	//		bool bInvalidLimb = false;
	//		FCompactPoseBoneIndex IKFootBoneCompactPoseIndex = Each.IKFootBone.GetCompactPoseIndex(BoneContainer);
	//		FCompactPoseBoneIndex FKFootBoneCompactPoseIndex = Each.FKFootBone.GetCompactPoseIndex(BoneContainer);
	//		FCompactPoseBoneIndex UpperLimbIndex(INDEX_NONE);
	//		const FCompactPoseBoneIndex LowerLimbIndex = BoneContainer.GetParentBoneIndex(FKFootBoneCompactPoseIndex);

	//		// if any leg is invalid, cancel the whole process.
	//		if (LowerLimbIndex == INDEX_NONE)
	//		{
	//			bInvalidLimb = true;
	//		}
	//		else
	//		{
	//			UpperLimbIndex = BoneContainer.GetParentBoneIndex(LowerLimbIndex);
	//			if (UpperLimbIndex == INDEX_NONE)
	//			{
	//				bInvalidLimb = true;
	//			}
	//		}
	//		if (bInvalidLimb)
	//		{
	//			return;
	//		}

	//		// Get Local Space transforms for our bones.
	//		const FTransform IKBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKFootBoneCompactPoseIndex);
	//		const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(FKFootBoneCompactPoseIndex);
	//		const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(LowerLimbIndex);
	//		const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(UpperLimbIndex);

	//		// Get Component Space transforms for our bones.
	//		FTransform IKBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKFootBoneCompactPoseIndex);
	//		FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(FKFootBoneCompactPoseIndex);
	//		FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	//		FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(UpperLimbIndex);

	//		// Get current position of root of limb. 
	//		// All position are in Component space.
	//		const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	//		const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	//		const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();
	//		const FVector OriginIKPos = IKBoneCSTransform.GetTranslation();

	//		// Length of limbs.
	//		float LowerLimbLength = (EndBoneCSTransform.GetLocation() - LowerLimbCSTransform.GetLocation()).Size();
	//		float UpperLimbLength = (LowerLimbCSTransform.GetLocation() - UpperLimbCSTransform.GetLocation()).Size();

	//		// Get scale pivot
	//		const FVector ScalePivot = FVector(RootPos.X, RootPos.Y, OriginIKPos.Z);

	//		// Calculate new IKPos
	//		FVector NewIKPos = OriginIKPos;
	//		switch (PR2IKAxisMode)
	//		{
	//		case EIKFootRootLocalAxis::NONE:
	//			break;
	//		case EIKFootRootLocalAxis::X:
	//			NewIKPos.X = ((OriginIKPos - ScalePivot).X * SpeedScaling) + ScalePivot.X;
	//		case EIKFootRootLocalAxis::Y:
	//			NewIKPos.Y = ((OriginIKPos - ScalePivot).Y * SpeedScaling) + ScalePivot.Y;
	//		case EIKFootRootLocalAxis::Z:
	//			NewIKPos.Z = ((OriginIKPos - ScalePivot).Z * SpeedScaling) + ScalePivot.Z;
	//		default:
	//			break;
	//		}

	//		// Clamp Ik foot position use total leg length.
	//		FVector ActualIKPos = NewIKPos;
	//		if (ClampIKUsingFKLeg)
	//		{
	//			float OriginIKLength = (OriginIKPos - RootPos).Size();
	//			float NewIkLength = (NewIKPos - RootPos).Size();
	//			float CutoffPercent = 1 - (1 / SpeedScaling);
	//			float TotalLimbLength = LowerLimbLength + UpperLimbLength;
	//			float MaxScaledLimbLength = OriginIKLength + (TotalLimbLength - OriginIKLength) * CutoffPercent;
	//			FVector IKDirection = (NewIKPos - RootPos).GetSafeNormal();
	//			if (NewIkLength > OriginIKLength)
	//			{
	//				FVector AdjustedIKPos = NewIKPos + IKDirection * (MaxScaledLimbLength - NewIkLength);
	//				ActualIKPos = (AdjustedIKPos - RootPos).Size() > (ActualIKPos - RootPos).Size() ? ActualIKPos : AdjustedIKPos;
	//			}
	//		}

	//		// Set Ik foot's actual position.
	//		IKBoneCSTransform.SetTranslation(ActualIKPos);
	//		OutBoneTransforms.Add(FBoneTransform(IKFootBoneCompactPoseIndex, IKBoneCSTransform));

	//		// Record origin and actual IK foot position.
	//		FIKFootLocationT IKFootLocationT;
	//		IKFootLocationT.LimbRootLocation = RootPos;
	//		IKFootLocationT.OriginLocation = OriginIKPos;
	//		IKFootLocationT.ActualLocation = ActualIKPos;
	//		IKFootLocationT.NewLocation = NewIKPos;
	//		TIKFootLocationTs.Add(IKFootLocationT);
	//	}
	//}

	//// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//// Calculate influence upon pelvis by this leg, according to actual Ik foot position.
	////++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//for (FIKFootLocationT EachIK : TIKFootLocationTs)
	//{
	//	// Init values first time
	//	RemainingTime = DeltaTime;
	//	BoneLocation = EachIK.OriginLocation;
	//	BoneVelocity = FVector::ZeroVector;
	//	//		GEngine->AddOnScreenDebugMessage(17, 10, FColor::Green, FString::Printf(TEXT("OriginIKSize: %0.5f"), EachIK.OriginLocation.Size()));

	//	while (RemainingTime > TimeStep)
	//	{
	//		// Calculate error vector.
	//		FVector const IKDirection = (EachIK.ActualLocation - EachIK.LimbRootLocation).GetSafeNormal();
	//		float IKStretch = ((EachIK.ActualLocation - EachIK.LimbRootLocation).Size() - (EachIK.OriginLocation - EachIK.LimbRootLocation).Size());
	//		FVector const Error = IKDirection * IKStretch;
	//		FVector const SpringForce = PelvisAdjustmentInterp.Stiffness * Error;
	//		FVector const DampingForce = PelvisAdjustmentInterp.Dampen * BoneVelocity;

	//		// Calculate force based on error and velocity
	//		FVector const Acceleration = SpringForce - DampingForce;

	//		// Integrate velocity
	//		// Make sure damping with variable frame rate actually dampens velocity. Otherwise Spring will go nuts.
	//		float const CutOffDampingValue = 1.f / TimeStep;
	//		if (PelvisAdjustmentInterp.Dampen > CutOffDampingValue)
	//		{
	//			float const SafetyScale = CutOffDampingValue / PelvisAdjustmentInterp.Dampen;
	//			BoneVelocity += SafetyScale * (Acceleration * TimeStep);
	//		}
	//		else
	//		{
	//			BoneVelocity += (Acceleration * TimeStep);
	//		}

	//		// Integrate position
	//		FVector const OldBoneLocation = BoneLocation;
	//		FVector const DeltaMove = (BoneVelocity * TimeStep);
	//		BoneLocation += DeltaMove;

	//		// Update velocity to reflect post processing done to bone location.
	//		BoneVelocity = (BoneLocation - OldBoneLocation) / TimeStep;

	//		check(!BoneLocation.ContainsNaN());
	//		check(!BoneVelocity.ContainsNaN());

	//		// End of calculation.
	//		RemainingTime -= TimeStep;
	//	}
	//	TSpringForce.Add(BoneVelocity);
	//}
	//// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	//// Calculate resultant force.
	//FVector ResultantForce = FVector(0.0f, 0.0f, 0.0f);
	//for (FVector const EachVector : TSpringForce)
	//{
	//	ResultantForce += EachVector;
	//}

	////Calculate actual adjusted pelvis posiiton.
	//FTransform PelvisBoneCSTransform = Output.Pose.GetComponentSpaceTransform(PelvisBoneCompactPoseIndex);
	//FVector OriginPelvisLocation = PelvisBoneCSTransform.GetLocation();
	//FVector NewPelvisLocation = OriginPelvisLocation + ResultantForce;
	//FVector ActralPelvisLocation = OriginPelvisLocation + (ResultantForce * PelvisAdjustmentAlpha);

	//// Set new pelvis transform.
	//PelvisBoneCSTransform.SetTranslation(ActralPelvisLocation);
	//OutBoneTransforms.Insert(FBoneTransform(PelvisBoneCompactPoseIndex, PelvisBoneCSTransform), 0);

}
//
bool FAnimNode_PR2IK::IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones)
{
	return (TipBone.IsValidToEvaluate(RequiredBones));
}
//
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


