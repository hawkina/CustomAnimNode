// Copyright 2018 Sean Chen. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class CustomAnimNode : ModuleRules
{
    private string ModulePath {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../ThirdParty/")); }
    }

    private string BinariesPath {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../Binaries/")); }
    }


    public CustomAnimNode(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add("CustomAnimNode/Private");
        PublicIncludePaths.Add("CustomAnimNode/Public");

        //     PublicIncludePaths.AddRange(
        //         new string[] {
        //             "CustomAnimNode/Public"
        //	// ... add public include paths required here ...
        //}
        //         );



        //PublicIncludePaths.Add(Path.Combine(ModulePath, "Eigen/eigen2"));

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
                "KDL"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
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
