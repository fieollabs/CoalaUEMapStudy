// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "LoadTextFileFromProject.h"

#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"

FString
ULoadTextFileFromProject::LoadTextFileFromProject( FString pathInContentFolder )
{
	//Read file ini [project]/Content/Data/ 
	//you can change with other location
	
	FString result;
	{
		FString path_in_project = FPaths::ProjectContentDir() + pathInContentFolder;
		FFileHelper::LoadFileToString( result, *path_in_project );
	}

	if( !result.IsEmpty() )
		return result;

	{
		FString path_in_plugin = FPaths::EnginePluginsDir() + "Marketplace/ThoughtfishCoalaPlugin/Content/" + pathInContentFolder;
		FFileHelper::LoadFileToString( result, *path_in_plugin );
	}


	return result;
}
