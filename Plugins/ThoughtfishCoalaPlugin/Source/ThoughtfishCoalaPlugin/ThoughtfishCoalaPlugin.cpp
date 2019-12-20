// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "ThoughtfishCoalaPlugin.h"
#include "Modules/ModuleManager.h"

class FCoalaPluginModule : public IModuleInterface
{

public:

	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};


IMPLEMENT_MODULE( FCoalaPluginModule, ThoughtfishCoalaPlugin )



void FCoalaPluginModule::StartupModule()
{
}


void FCoalaPluginModule::ShutdownModule()
{
}

