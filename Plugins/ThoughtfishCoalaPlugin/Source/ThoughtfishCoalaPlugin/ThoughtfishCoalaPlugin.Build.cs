// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

namespace UnrealBuildTool.Rules
{
	public class ThoughtfishCoalaPlugin : ModuleRules
	{
		public ThoughtfishCoalaPlugin(ReadOnlyTargetRules Target) 
			: base(Target)
		{
			PrivatePCHHeaderFile = "ThoughtfishCoalaPlugin.h";

			PublicDependencyModuleNames.AddRange(
				new string[] { 
					"Core", "CoreUObject", "Engine", "InputCore",
					"ProceduralMeshComponent",
					"Slate", "SlateCore",
					"HTTP", "Json", "JsonUtilities",
					"LocationServicesBPLibrary"
				}
			);
			
			if (Target.bBuildEditor == true)
			{
				//@TODO: Needed for FPropertyEditorModule::NotifyCustomizationModuleChanged()
				//@TOOD: To move/implement in FStreetMapComponentDetails
				PrivateDependencyModuleNames.Add("EditorStyle");
			}
		}
	}
}