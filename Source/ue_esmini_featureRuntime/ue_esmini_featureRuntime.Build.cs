// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ue_esmini_featureRuntime : ModuleRules
{
	public ue_esmini_featureRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        string esminiPath = Path.Combine(ModuleDirectory, "ThirdParty", "esmini");
        string esminiLibPath = Path.Combine(esminiPath, "Lib");
        string esminiIncludePath = Path.Combine(esminiPath, "Include");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			string DLLName = "esminiLib.dll";
            string DLLNameRM = "esminiRMLib.dll";
            string DLLSourcePath = Path.Combine(esminiLibPath, DLLName);
            string DLLSourcePathRM = Path.Combine(esminiLibPath, DLLName);
            string DLLDestinationPath = Path.Combine("$(PluginDir)", "Binaries", "Win64", DLLName);
            string DLLDestinationPathRM = Path.Combine("$(PluginDir)", "Binaries", "Win64", DLLNameRM);
            
			RuntimeDependencies.Add(DLLDestinationPath, DLLSourcePath, StagedFileType.NonUFS);
            RuntimeDependencies.Add(DLLDestinationPathRM, DLLSourcePathRM, StagedFileType.NonUFS);
            System.Console.WriteLine($"Copying {DLLName} from {DLLSourcePath} to {DLLDestinationPath}");
            System.Console.WriteLine($"Copying {DLLNameRM} from {DLLSourcePathRM} to {DLLDestinationPathRM}");

            PublicAdditionalLibraries.Add(Path.Combine(esminiLibPath, "esminiLib.lib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(Path.Combine(esminiLibPath, "esminiLib.a"));
        }
        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				esminiIncludePath,
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "GEMSTAR_Interfaces"
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
				"Projects",
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
