// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "CoalaDevelopmentConfigurations.h"
/**
 * 
 */
class CoalaConverter
{
	public:
		static class UCoalaArea* JsonToCoalaArea( FString jsonString, int defaultBuildingLevel = 1, bool clampToDefaultBuildingLevel = false, int limitMaxBuildingLevelTo = 0 );
		static class UCoalaBounds* calculateBounds( const TArray<class UCoalaGPSCoordinates*>& area );

		static FVector ToScenePosition( double lon, double lat );
		static void PixelToGps( float x, float y, float& lon, float& lat );
};
