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

FTransform FlipTransform(FTransform const& input)
{
	return input;
	FTransform retq = input;
	double x = retq.GetLocation().X;
	double y = retq.GetLocation().Y;
	double z = retq.GetLocation().Z;
	retq.SetLocation(FVector(x, y, z));
	x = retq.GetRotation().GetRotationAxis().X;
	y = retq.GetRotation().GetRotationAxis().Y;
	z = retq.GetRotation().GetRotationAxis().Z;
	retq.SetRotation(FQuat(FVector(x,y,z), retq.GetRotation().GetAngle()));
	return retq;
}

FAnimNode_PR2IK::FAnimNode_PR2IK() :
	TranslationSpace(BCS_WorldSpace),
	EffectorGoalTransform(FTransform::Identity)	
{
}

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
	/*UE_LOG(LogTemp, Warning, TEXT("+++AnyThread evaluation+++"));*/


	// get UE World Object
	const UWorld * TheWorld = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();
	// Create or get BoneContainer
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	//get bone index and varify them exists.
	FCompactPoseBoneIndex TipBoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
	//FCompactPoseBoneIndex RootBoneCompactPoseIndex = RootBone.GetCompactPoseIndex(BoneContainer);
	
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
		//add last bone, because the condition will be false
		//NameToIndex.Add(TPair<int32, FName>(BoneIndex.GetInt(), BoneName));

		TArray<TPair<int32, FName> > NewNameToIndex;


		//reverse The Map, so that the order is from Root to Tip
		for (signed int i = 0; i < NameToIndex.Num(); i++) {
			unsigned int k = NameToIndex.Num() - i - 1;
			NewNameToIndex.Add(NameToIndex[k]);
		}

		//Init previousAngles with Default BoneAngle if we do not already have rpevious angles stored
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

		//Print the list to ensure it is correct:
		FString namePair;
		for (const TPair<int32, FName>& pair : NewNameToIndex){
			namePair = pair.Value.ToString();
			//UE_LOG(LogTemp, Warning, TEXT("Bone Name: %s"), *namePair);
			//UE_LOG(LogTemp, Warning, TEXT("Index: %d"), pair.Key);
		}
	
	

	//------------------------------------- Create a KDL Chain -------------------------------------------
	KDL::Chain ikchain;
	// get Component transform
	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	ComponentTransform = FlipTransform(ComponentTransform);
	ComponentTransform.SetLocation(ComponentTransform.GetLocation() / 100);
	ComponentTransform.SetScale3D(FVector(1.0, 1.0, 1.0));

	//convert effector goal transform into component space, given it is provied in world space
	FTransform ScaledEffectorGoalTransform = EffectorGoalTransform;
	ScaledEffectorGoalTransform = FlipTransform(ScaledEffectorGoalTransform);
	ScaledEffectorGoalTransform.SetLocation(ScaledEffectorGoalTransform.GetLocation() / 100);
	ScaledEffectorGoalTransform.SetScale3D(FVector(1.0, 1.0, 1.0));
	//UE_LOG(LogTemp, Warning, TEXT("Before: %s \n"), *ScaledEffectorGoalTransform.ToHumanReadableString());
	FQuat dq(FRotator(ScaledEffectorGoalTransform.GetRotation().Rotator().Pitch, ScaledEffectorGoalTransform.GetRotation().Rotator().Yaw + 0, ScaledEffectorGoalTransform.GetRotation().Rotator().Roll));
	ScaledEffectorGoalTransform.SetRotation(dq);
	//UE_LOG(LogTemp, Warning, TEXT("After: %s \n"), *ScaledEffectorGoalTransform.ToHumanReadableString());

	//UE_LOG(LogTemp, Warning, TEXT("------------------------------"));
	//UE_LOG(LogTemp, Warning, TEXT("Effector Goal Transform (Original): %s \n"), *EffectorGoalTransform.ToHumanReadableString());
	//UE_LOG(LogTemp, Warning, TEXT("Scaled Effector Goal Transform %s \n"), *ScaledEffectorGoalTransform.ToHumanReadableString());
	//UE_LOG(LogTemp, Warning, TEXT("Inverse Component Transform: %s \n"), *ComponentTransform.Inverse().ToHumanReadableString());
	//UE_LOG(LogTemp, Warning, TEXT("------------------------------"));

	KDL::JntArray temp_min_limits(NameToIndex.Num()); //in rad
	KDL::JntArray temp_max_limits(NameToIndex.Num()); //in rad

	//iterate over all elements, find matching limits and add create the joints
	int32 j = 0; //needed for the counter of the elements
	for (const TPair<int32, FName>& pair : NewNameToIndex) {
		//UE_LOG(LogTemp, Warning, TEXT("Creating Segment for: %s"), *pair.Value.ToString());

		//make sure a new one is created each time
		KDL::Joint joint;


		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);
		FCompactPoseBoneIndex ParentIndex = Output.Pose.GetPose().GetParentBoneIndex(index);
		
		//get name of joint/link
		std::string name(TCHAR_TO_UTF8(*pair.Value.ToString())); //convert ue4 string to normal string. 
		//need to access it this way since this might be outside of our usual chain
		std::string ParentName(TCHAR_TO_UTF8(*Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneName(ParentIndex.GetInt()).ToString()));
		// pair int index to unreal index
		
		//get Parent index for joint
		


		//create chain element and add it to the chain
		FTransform BoneLocalTransform = Output.Pose.GetLocalSpaceTransform(index);
		BoneLocalTransform = FlipTransform(BoneLocalTransform);
		FVector JointOriginUE = BoneLocalTransform.GetTranslation();
		
		//UE_LOG(LogTemp, Warning, TEXT("Setting Joint Origin to:  %s"), *BoneLocalTransform.GetTranslation().ToString());
		KDL::Vector JointOrigin = KDL::Vector(JointOriginUE.X, JointOriginUE.Y, JointOriginUE.Z);
		KDL::Vector VecX = KDL::Vector(1.0, 0.0, 0.0);
		KDL::Vector VecY = KDL::Vector(0.0, 1.0, 0.0);
		KDL::Vector VecZ = KDL::Vector(0.0, 0.0, 1.0);

		if (RangeLimits.Num() == 0) {
			//UE_LOG(LogTemp, Warning, TEXT("No Limits set for this IK chain. Please set some Limits"));
			return;
		}
		else {
			//Check if that Element exists
			for (int32 i = 0; i < RangeLimits.Num(); i++) {

				//UE_LOG(LogTemp, Warning, TEXT("found limits for bone name: %s"), *RangeLimits[i].BoneName.BoneName.ToString());
				std::string tempName(TCHAR_TO_UTF8(*RangeLimits[i].BoneName.BoneName.ToString()));

				//check if limit exist for current bone
				if ((i < RangeLimits.Num()) && (tempName == name)) {
					//if element exists and this is the one, create matching joint
					temp_min_limits.data(j) = RangeLimits[i].LimitMin;
					temp_max_limits.data(j) = RangeLimits[i].LimitMax;
					//UE_LOG(LogTemp, Warning, TEXT("LimitMin: %f"), RangeLimits[i].LimitMin);
					//UE_LOG(LogTemp, Warning, TEXT("LimitMax: %f"), RangeLimits[i].LimitMax);
					
					KDL::Vector VecT;


					switch(RangeLimits[i].Axis.GetValue()) {
					case RotX:
						joint = KDL::Joint(tempName, JointOrigin, VecX, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						VecT = VecX;
						//UE_LOG(LogTemp, Warning, TEXT("Axis: X"));
						break;

					case RotY:
						joint = KDL::Joint(tempName, JointOrigin, VecY, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						//UE_LOG(LogTemp, Warning, TEXT("Axis: Y"));
						VecT = VecY;
						break;

					case RotZ:
						joint = KDL::Joint(tempName, JointOrigin, VecZ, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						/*UE_LOG(LogTemp, Warning, TEXT("Axis: Z"));*/
						VecT = VecZ;
						break;

					default:
						joint = KDL::Joint(tempName, JointOrigin, VecZ, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						/*UE_LOG(LogTemp, Warning, TEXT("no axis set for this joint"));*/
						break;
					}

					switch (RangeLimits[i].JointType.GetValue()) {
					
					case Fixed:
						/*UE_LOG(LogTemp, Warning, TEXT("Fixed Joint"));*/
						joint = KDL::Joint(tempName, JointOrigin, VecT, KDL::Joint::RotAxis, 1, 0, 0, 0, 0);
						break;

					case Revolute:
						break;

					default:
						break;
					}

					//get transform of current joint aka bone origin
					

					FTransform BoneTransform = Output.Pose.GetLocalSpaceTransform(index);
					BoneTransform = FlipTransform(BoneTransform);
					//get vector of transform
					FVector locVector = BoneTransform.GetTranslation();

					KDL::Rotation offsetRotation;
					if (name == "r_shoulder_pan_link") {
						//offsetRotation = KDL::Rotation::RotZ(-3.14159 / 2);
						/*UE_LOG(LogTemp, Error, TEXT("Offset Set"));*/
					}

					ikchain.addSegment(KDL::Segment(name, joint, KDL::Frame(offsetRotation, KDL::Vector(locVector.X, locVector.Y, locVector.Z))));
					//UE_LOG(LogTemp, Warning, TEXT("LimitMin: for Segment: %f"), temp_min_limits(i));
					//UE_LOG(LogTemp, Warning, TEXT("LimitMax: for Segment %f"), temp_max_limits(i));
					
					
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
		
		//this is done for "seeding" aka give pose of where joint is right now instead of default. let's do it without first
		if (n < ikchain.getNrOfJoints()) {
			input.data(n) = oldAngle;
			
			//UE_LOG(LogTemp, Warning, TEXT("Angle:  %f"), angle);
			//UE_LOG(LogTemp, Warning, TEXT("Angle in input:  %f"), input.data(n));
			//UE_LOG(LogTemp, Warning, TEXT("counter:  %d"), n);
			//UE_LOG(LogTemp, Warning, TEXT("Nr of Joints:  %d"), ikchain.getNrOfJoints());
			
			if (!((oldAngle <= temp_max_limits.data(n)) && (oldAngle >= temp_min_limits.data(n)))) {
				//UE_LOG(LogTemp, Error, TEXT("ANGLE OUT OF BOUNDS!!!"));
				//UE_LOG(LogTemp, Error, TEXT("Joint: %s"), *pair.Value.ToString());
				//UE_LOG(LogTemp, Error, TEXT("temp Max Limit:  %f"), temp_max_limits.data(n));
				//UE_LOG(LogTemp, Error, TEXT("temp Min Limit:  %f"), temp_min_limits.data(n));
				//UE_LOG(LogTemp, Error, TEXT("Angle: %f \n"), oldAngle);
			}
		}
		n++;
	}

	


	
	//Now that we have a chain, let's apply some IK solver to it
	KDL::ChainFkSolverPos_recursive fksolver1(ikchain);
	KDL::ChainIkSolverVel_pinv iksolver1v(ikchain,1e-5,400);
	//KDL::ChainIkSolverPos_NR iksolver(ikchain, fksolver1, iksolver1v, 100, 1e-2);
	KDL::ChainIkSolverPos_NR_JL iksolver(ikchain, temp_min_limits, temp_max_limits, fksolver1, iksolver1v, 400, 1e-6);
	//iksolver.setJointLimits(min_limits, max_limits);
	
	//create destination Frame
	//transform goal pose from world frame into bone frame
	//recalculate the goal transform to be relative to our RootBone
	FTransform RootTransformComp = Output.Pose.GetComponentSpaceTransform(Output.Pose.GetPose().GetParentBoneIndex(RootIndex));
	RootTransformComp = FlipTransform(RootTransformComp);
	RootTransformComp.SetLocation(RootTransformComp.GetLocation() / 100);
	RootTransformComp.SetScale3D(FVector(1.0, 1.0, 1.0));
	FTransform adjustYaw;
	adjustYaw.SetRotation(FQuat(FVector(0, 0, 1), 3.14159 / 2));
	FTransform GoalInBoneSpace = adjustYaw*ScaledEffectorGoalTransform * ComponentTransform.Inverse() * RootTransformComp.Inverse();


	//Fix scale...
	GoalInBoneSpace.SetScale3D(FVector(1.0, 1.0, 1.0));
	//UE_LOG(LogTemp, Warning, TEXT("------------------------------"));
	//UE_LOG(LogTemp, Warning, TEXT("Component Transform: %s \n"), *ComponentTransform.ToHumanReadableString());
	//UE_LOG(LogTemp, Warning, TEXT("Root Transform in Component Inverse: %s \n"), *RootTransformComp.Inverse().ToHumanReadableString());
	//UE_LOG(LogTemp, Warning, TEXT("Goal Transform in Bone Space: %s \n"), *GoalInBoneSpace.ToHumanReadableString());
	//UE_LOG(LogTemp, Warning, TEXT("------------------------------"));
	//transform goal into component space
	
	//UE_LOG(LogTemp, Warning, TEXT("KDL Goal Translation: %f, %f, %f"), goal.p.x(), goal.p.y(), goal.p.z());
	FTransform TipTransform = Output.Pose.GetComponentSpaceTransform(TipBoneIndex);
	TipTransform = FlipTransform(TipTransform);
	TipTransform.SetLocation(TipTransform.GetLocation() / 100);
	TipTransform.SetScale3D(FVector(1.0, 1.0, 1.0));

	FTransform ParentBoneInComponent = Output.Pose.GetComponentSpaceTransform(Output.Pose.GetPose().GetParentBoneIndex(RootIndex));
	ParentBoneInComponent = FlipTransform(ParentBoneInComponent);
	ParentBoneInComponent.SetLocation(ParentBoneInComponent.GetLocation() / 100);
	ParentBoneInComponent.SetScale3D(FVector(1.0, 1.0, 1.0));
	FTransform TorsoInComponent = ParentBoneInComponent;
	FTransform CurrentGoal = GoalInBoneSpace;

	TipTransform = TipTransform * ParentBoneInComponent.Inverse();

	//KDL Calculation
	int maxDiv = 1;
	int found = 0;
	UE_LOG(LogTemp, Warning, TEXT("Goal for KDL Location: %s \n"), *CurrentGoal.ToHumanReadableString());
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
			UE_LOG(LogTemp, Error, TEXT("Max Iterrations exceeded."));
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
			result.data(k) = old;//(temp_min_limits.data(k) + temp_max_limits.data(k))*0.5;
			k++;
		}
	}

	
	//these are JointAngles, aka floats, aka rotation around specified axis in radians
	for (unsigned int i = 0; i < result.rows(); i++) {
		//UE_LOG(LogTemp, Warning, TEXT("input was %f"), input(i));
		//UE_LOG(LogTemp, Warning, TEXT("Ik solver result: %f"), result(i));
		//UE_LOG(LogTemp, Warning, TEXT("result counter: %d"), i);
	}
	//UE_LOG(LogTemp, Warning, TEXT("+++Start debugging+++"));


	//std::vector<float> test(3);
	//test[0] = EffectorGoalTransform.GetRotation().Rotator().Roll * 3.14 / 180;
	//test[1] = EffectorGoalTransform.GetRotation().Rotator().Pitch * 3.14 / 180;
	//test[2] = EffectorGoalTransform.GetRotation().Rotator().Yaw * 3.14 / 180;

	//UE_LOG(LogTemp, Warning, TEXT("--------- After KDL Calculation ---------------------"));

	
	

	int counter = 0;
	for (const TPair<int32, FName>& pair : NewNameToIndex) {
	
		//get name of joint/link
		std::string name(TCHAR_TO_UTF8(*pair.Value.ToString())); //convert ue4 string to normal string. 

		//UE_LOG(LogTemp, Warning, TEXT("Current Joint Name: %s"), *pair.Value.ToString());
		// pair int index to unreal index
		FCompactPoseBoneIndex index = FCompactPoseBoneIndex(pair.Key);

		FTransform BoneTransform = Output.Pose.GetLocalSpaceTransform(index);
		BoneTransform = FlipTransform(BoneTransform);
		//Set axis vector for which the angle should be applied
		Eigen::Vector3d vec;
		//UE_LOG(LogTemp, Warning, TEXT("Set Vector axis. "));
		switch (ikchain.getSegment(counter).getJoint().getType()) {
		case KDL::Joint::RotX:
			vec = Eigen::Vector3d(1.0, 0.0, 0.0);
			//UE_LOG(LogTemp, Warning, TEXT("Axis: X"));
			break;

		case KDL::Joint::RotY:
			vec = Eigen::Vector3d(0.0, 1.0, 0.0);
			//UE_LOG(LogTemp, Warning, TEXT("Axis: Y"));
			break;

		case KDL::Joint::RotZ:
			vec = Eigen::Vector3d(0.0, 0.0, 1.0);
			//UE_LOG(LogTemp, Warning, TEXT("Axis: Z"));
			break;

		default:
			KDL::Vector tempVec = ikchain.getSegment(counter).getJoint().JointAxis();
			vec = Eigen::Vector3d(tempVec.x(), tempVec.y(), tempVec.z());
			FVector printVector(tempVec.x(), tempVec.y(), tempVec.z());
			//UE_LOG(LogTemp, Warning, TEXT("Rotation Axis: %s"), *printVector.ToString());
			break;
		}

		//put KDL result into proper transform
		//Eigen::AngleAxis<double> angle(result(counter), vec);
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
	
		//UE_LOG(LogTemp, Warning, TEXT("After Eigen, the result quaternion is: %s"), *resultQuat.ToString());

		FTransform ResultTransform;
		ResultTransform.SetIdentity();
		ResultTransform.SetRotation(resultQuat);

		//FTransform ParentBoneInComponent = Output.Pose.GetComponentSpaceTransform(Output.Pose.GetPose().GetParentBoneIndex(index));
		FTransform BoneLocalTransform = Output.Pose.GetLocalSpaceTransform(index);
		BoneLocalTransform = FlipTransform(BoneLocalTransform);
		FVector JointOriginUE = BoneLocalTransform.GetTranslation();
		FTransform LocScale;
		LocScale.SetTranslation(JointOriginUE);
		//LocScale.SetScale3D(FVector(1.0, 1.0, 1.0));
		 

		FTransform LocalToComponentResult = ResultTransform * LocScale * ParentBoneInComponent;

		if (name == "r_gripper_palm_link") {
			LocalToComponentResult = GoalInBoneSpace * TorsoInComponent;
			UE_LOG(LogTemp, Warning, TEXT("setting goal for palm"));
		}

		//hotfix
		//LocalToComponentResult.SetLocation(Output.Pose.GetComponentSpaceTransform(index).GetLocation());
		// this fixes some scaling issues and unwanted translation
		FTransform OutputTransform = LocalToComponentResult;
		OutputTransform = FlipTransform(OutputTransform);
		OutputTransform.SetLocation(OutputTransform.GetLocation() * 100);
		OutputTransform.SetScale3D(FVector(100.0, 100.0, 100.0));


		//UE_LOG(LogTemp, Warning, TEXT("Bone Name: %s"), *pair.Value.ToString());
		//UE_LOG(LogTemp, Warning, TEXT("Bone Angle: %f \n"), OutputTransform.GetRotation().GetAngle());

		OutBoneTransforms.Add(FBoneTransform(index, OutputTransform));

		//ensure parent pose is the one we just computed
		ParentBoneInComponent = LocalToComponentResult;
		

		Output.Pose.LocalBlendCSBoneTransforms(OutBoneTransforms, 1.0);
		OutBoneTransforms.Reset();

		counter++;

	}
	//OutBoneTransforms.Reset();
	//UE_LOG(LogTemp, Warning, TEXT("Moving One Bone"));
	//PoseMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PosebleMesh"));
	//static USkeletalMesh* ActorMesh = Output.AnimInstanceProxy->GetSkelMeshComponent()->SkeletalMesh;
	//Output.AnimInstanceProxy->GetSkelMeshComponent()->SkeletalMesh;
	//PoseMesh->SetupAttachment(RootComponent);
	//PoseMesh->SetSkeletalMesh(ActorMesh.Object);
	//UPoseableMeshComponent* PosableMeshComponent = a->CreateDefaultSubobject(this, TEXT("MeshComponent"));
	//UPoseableMeshComponent* PosableMeshComponent = CreateDefaultSubobject(this, TEXT("MeshComponent"));
	


	//// This works
	//TArray<FBoneTransform> tempArray;

	//FCompactPoseBoneIndex ind2(79);
	//FTransform temp2;// = Output.Pose.GetLocalSpaceTransform(ind2);
	//temp2.SetIdentity();
	//float zComp = GoalTransformComponentSpace.GetLocation().Z / 100;
	//// w = cos(alpha/2), x=sin(alpha/2)*dx etc for y, z
	//FQuat tempQ2(0.0, 0.0, -sin(zComp / 2), cos(zComp / 2));
	////FQuat tempQ2(0, 0, -0.7071068, 0.7071068);


	//temp2.SetRotation(tempQ2);
	//FTransform FinalPose2 = temp2 * Output.Pose.GetComponentSpaceTransform(ind2);// 

	////Output.Pose.SetComponentSpaceTransform(ind2, FinalPose2);
	//tempArray.Add(FBoneTransform(ind2, FinalPose2));
	//Output.Pose.LocalBlendCSBoneTransforms(tempArray, 1.0);
	//tempArray.Reset();
	//
	//FCompactPoseBoneIndex ind(81);
	//FTransform temp;  //Output.Pose.GetLocalSpaceTransform(ind);
	//temp.SetIdentity();
	////FQuat tempQ(-0.707, 0.0, 0.0, 0.707);
	//float xComp = GoalTransformComponentSpace.GetLocation().X / 100;
	//FQuat tempQ(-sin(xComp / 2), 0.0, 0.0, cos(xComp / 2));
	//temp.SetRotation(tempQ);
	//FTransform FinalPose = temp * Output.Pose.GetComponentSpaceTransform(ind);// *temp;
	//
	////Output.Pose.SetComponentSpaceTransform(ind, FinalPose);

	//tempArray.Add(FBoneTransform(ind, FinalPose));
	//Output.Pose.LocalBlendCSBoneTransforms(tempArray, 1.0);
	//tempArray.Reset();
	////FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, temp, ind, TranslationSpace);
	////FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, temp2, ind2, TranslationSpace);
	////tempArray.Add(FBoneTransform(ind, temp));
	////tempArray.Add(FBoneTransform(ind2, temp2));
	
	
	

}

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


