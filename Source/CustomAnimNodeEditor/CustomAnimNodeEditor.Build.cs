// Copyright 2018 Sean Chen. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class CustomAnimNodeEditor : ModuleRules
{

    private string ModulePath {
        get { return ModuleDirectory; }
    }


    public CustomAnimNodeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(Path.Combine(ModulePath, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModulePath, "Private"));
        //     PublicIncludePaths.AddRange(
        //         new string[] {
        //             "CustomAnimNodeEditor/Public"
        //	// ... add public include paths required here ...
        //}
        //         );


        //     PrivateIncludePaths.AddRange(
        //         new string[] {
        //             "CustomAnimNodeEditor/Private",
        //	// ... add other private include paths required here ...
        //}
        //         );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CustomAnimNode"

				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "AnimGraph",
                "BlueprintGraph",
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
