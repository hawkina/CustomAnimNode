// Copyright 2018 Sean Chen. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class CustomAnimNode : ModuleRules
{
    private string ModulePath {
        get { return ModuleDirectory; }
    }

    private string BinariesPath {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../Binaries/")); }
    }


    public CustomAnimNode(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        //PublicIncludePaths.Add("ThirdParty");

        //     PublicIncludePaths.AddRange(
        //         new string[] {
        //             "CustomAnimNode/Public"
        //	// ... add public include paths required here ...
        //}
        //         );



        PublicIncludePaths.Add(Path.Combine(ModulePath, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModulePath, "Private"));

        PublicIncludePaths.Add(Path.Combine(ModulePath, "ThirdParty/Eigen"));
        PublicIncludePaths.Add(Path.Combine(ModulePath, "ThirdParty/KDL"));

        PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "ThirdParty/KDL/orocos-kdl.lib"));

        //     PrivateIncludePaths.AddRange(
        //         new string[] {
        //             "CustomAnimNode/Private",
        //	// ... add other private include paths required here ...
        //}
        //         );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "AnimationCore",
                "AnimGraphRuntime",
                //"KDL"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
			
                //"KDL"
                // ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
