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

        PublicIncludePaths.Add(Path.Combine(ModulePath, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModulePath, "Private"));

        PublicIncludePaths.Add(Path.Combine(ModulePath, "ThirdParty/Eigen"));
        PublicIncludePaths.Add(Path.Combine(ModulePath, "ThirdParty/KDL"));

        PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "ThirdParty/KDL/orocos-kdl.lib"));



        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "AnimationCore",
                "AnimGraphRuntime",
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
