using UnrealBuildTool;

public class FlyingCabNarrativeEditor : ModuleRules
{
	public FlyingCabNarrativeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] {
			"Core", "CoreUObject", "Engine", "UnrealEd", "FlyingCabFlightLab",
			"Slate", "SlateCore", "PropertyEditor", "AssetTools", "AssetRegistry",
			"ToolMenus", "InputCore", "EditorSubsystem", "Projects"
		});
	}
}
