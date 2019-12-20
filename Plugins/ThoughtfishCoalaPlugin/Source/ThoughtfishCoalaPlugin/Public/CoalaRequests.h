// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaRequests.generated.h"

/**
 * 
 */
UCLASS()
class UCoalaRequestBase : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
};

UCLASS()
class UCoalaRequest : public UCoalaRequestBase
{
	GENERATED_BODY()
	
};

UCLASS()
class UGolfcourseRequest : public UCoalaRequestBase
{
	GENERATED_BODY()
	
};

UCLASS()
class UVectorTileRequest : public UCoalaRequestBase
{
	GENERATED_BODY()

};

UCLASS()
class UWeatherRequest : public UCoalaRequestBase
{
	GENERATED_BODY()

};

UCLASS()
class UGeofencingRequest : public UCoalaRequestBase
{
	GENERATED_BODY()

};
