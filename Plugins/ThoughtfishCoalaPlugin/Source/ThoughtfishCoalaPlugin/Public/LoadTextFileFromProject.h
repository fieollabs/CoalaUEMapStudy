// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoadTextFileFromProject.generated.h"

/**
 * 
 */
UCLASS()
class THOUGHTFISHCOALAPLUGIN_API ULoadTextFileFromProject
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		UFUNCTION( BlueprintCallable, meta=(DisplayName="Load text file from project(also from Engine/Plugins/Marketplace/ThoughtfishCoalaPlugin/Content/ folder"),Category = "Coala|Utility" )
		static FString LoadTextFileFromProject(FString pathInContentFolder);
};
